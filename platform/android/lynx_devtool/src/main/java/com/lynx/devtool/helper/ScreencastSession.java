// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.os.SystemClock;
import android.util.Base64;
import android.view.View;
import com.lynx.devtool.DevToolPlatformAndroidDelegate;
import com.lynx.tasm.base.LLog;
import java.io.ByteArrayOutputStream;
import java.lang.ref.WeakReference;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** State lives on the main thread; JPEG encoding runs on the worker. */
final class ScreencastSession {
  private static final Handler MAIN = new Handler(Looper.getMainLooper());
  private static final int INTERVAL_MS = 1000;
  private final WeakReference<View> mView;
  private final WeakReference<DevToolPlatformAndroidDelegate> mDelegate;
  private final String mMode;
  private final int mMaxWidth;
  private final int mMaxHeight;
  private final int mQuality;
  private boolean mAwaitingAck;
  private String mLastData;
  private final ScreencastSnapshot.Buffers mCaptureBuffers = new ScreencastSnapshot.Buffers();
  private final ExecutorService mWorker = Executors.newSingleThreadExecutor(runnable -> {
    Thread thread = new Thread(() -> {
      Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
      runnable.run();
    }, "ScreencastEncoder");
    return thread;
  });
  private final ByteArrayOutputStream mEncoded = new ByteArrayOutputStream(64 * 1024);
  private final Runnable mTick = this::tick;
  private boolean mStopped;
  private boolean mPaused;
  private boolean mBusy;
  private int mGeneration;
  private long mLastCapture;
  private Bitmap mBitmap;

  ScreencastSession(View view, DevToolPlatformAndroidDelegate delegate, String mode, int maxWidth,
      int maxHeight, int quality) {
    mView = new WeakReference<>(view);
    mDelegate = new WeakReference<>(delegate);
    mMode = mode;
    mMaxWidth = maxWidth;
    mMaxHeight = maxHeight;
    mQuality = quality;
    MAIN.post(mTick);
  }

  private void schedule() {
    MAIN.removeCallbacks(mTick);
    if (!mStopped && !mPaused) {
      MAIN.postDelayed(
          mTick, Math.max(1, INTERVAL_MS - (SystemClock.uptimeMillis() - mLastCapture)));
    }
  }

  private void tick() {
    if (mStopped || mPaused || mBusy) {
      return;
    }
    if (mAwaitingAck) {
      return;
    }
    View view = mView.get();
    if (view == null || mDelegate.get() == null) {
      stop();
      return;
    }
    mBusy = true;
    mLastCapture = SystemClock.uptimeMillis();
    int generation = mGeneration;
    float density = view.getResources().getDisplayMetrics().density;
    Bitmap reusable = mBitmap;
    mBitmap = null;
    ScreencastSnapshot.capture(view, mMode, mMaxWidth, mMaxHeight, reusable, mCaptureBuffers,
        (bitmap, sourceWidth, sourceHeight) -> {
          if (bitmap == null || mStopped || mPaused || generation != mGeneration) {
            finishDiscarded(bitmap);
            return;
          }
          float deviceWidth = sourceWidth / density;
          float deviceHeight = sourceHeight / density;
          mWorker.execute(() -> encode(bitmap, deviceWidth, deviceHeight, generation));
        });
  }

  private void encode(Bitmap bitmap, float deviceWidth, float deviceHeight, int generation) {
    String data = null;
    try {
      mEncoded.reset();
      if (bitmap.compress(Bitmap.CompressFormat.JPEG, mQuality, mEncoded)) {
        data = Base64.encodeToString(mEncoded.toByteArray(), Base64.NO_WRAP);
      }
    } catch (RuntimeException error) {
      LLog.w("Screencast", "Cannot encode snapshot: " + error);
    }
    final String frameData = data;
    MAIN.post(() -> {
      if (mStopped || mPaused || generation != mGeneration) {
        finishDiscarded(bitmap);
        return;
      }
      mBusy = false;
      mBitmap = bitmap;
      if (frameData != null && !frameData.equals(mLastData)) {
        DevToolPlatformAndroidDelegate delegate = mDelegate.get();
        if (delegate == null) {
          stop();
          return;
        }
        mAwaitingAck = true;
        mLastData = frameData;
        sendLegacyFrame(delegate, frameData, deviceWidth, deviceHeight);
      }
      schedule();
    });
  }

  void acknowledge() {
    MAIN.post(() -> {
      if (mStopped || mPaused || !mAwaitingAck) {
        return;
      }
      mAwaitingAck = false;
      schedule();
    });
  }

  void setPaused(boolean paused) {
    runOnMain(() -> {
      if (mStopped || mPaused == paused) {
        return;
      }
      mPaused = paused;
      mGeneration++;
      mAwaitingAck = false;
      mLastData = null;
      MAIN.removeCallbacks(mTick);
      if (!mBusy) {
        schedule();
      }
    });
  }

  void stop() {
    runOnMain(() -> {
      mStopped = true;
      mGeneration++;
      MAIN.removeCallbacks(mTick);
      mAwaitingAck = false;
      mLastData = null;
      mWorker.shutdown();
      if (!mBusy) {
        release();
      }
    });
  }

  private void finishDiscarded(Bitmap bitmap) {
    if (bitmap != null) {
      bitmap.recycle();
    }
    mBusy = false;
    if (mStopped) {
      release();
    } else {
      schedule();
    }
  }

  private void release() {
    mCaptureBuffers.release();
    if (mBitmap != null) {
      mBitmap.recycle();
      mBitmap = null;
    }
    mView.clear();
    mDelegate.clear();
  }

  private static void sendLegacyFrame(
      DevToolPlatformAndroidDelegate delegate, String data, float deviceWidth, float deviceHeight) {
    ScreenCapturer.ScreenMetadata metadata = new ScreenCapturer.ScreenMetadata();
    metadata.mDeviceWidth = deviceWidth;
    metadata.mDeviceHeight = deviceHeight;
    metadata.mPageScaleFactor = 1;
    delegate.sendScreenCast(data, metadata, metadata.mPageScaleFactor, 0);
  }

  private static void runOnMain(Runnable runnable) {
    if (Looper.myLooper() == Looper.getMainLooper()) {
      runnable.run();
    } else {
      MAIN.post(runnable);
    }
  }
}
