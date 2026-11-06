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
import com.lynx.tasm.base.TraceEvent;
import java.io.ByteArrayOutputStream;
import java.lang.ref.WeakReference;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** State lives on the main thread; only pixel comparison and encoding run on the worker. */
final class ScreencastSession {
  private static final Handler MAIN = new Handler(Looper.getMainLooper());
  private static final int INTERVAL_MS = 125;
  private final WeakReference<View> mView;
  private final WeakReference<DevToolPlatformAndroidDelegate> mDelegate;
  private final String mMode;
  private final int mMaxWidth;
  private final int mMaxHeight;
  private final int mQuality;
  private final boolean mJpegDelta;
  private final ScreencastAck mAck = new ScreencastAck();
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
  private int[] mPrevious;
  private int[] mCurrent;
  private int mReferenceWidth;
  private int mReferenceHeight;
  private float mReferenceDeviceWidth;
  private float mReferenceDeviceHeight;
  private float mPendingDeviceWidth;
  private float mPendingDeviceHeight;

  ScreencastSession(View view, DevToolPlatformAndroidDelegate delegate, String mode, int maxWidth,
      int maxHeight, int quality, boolean jpegDelta) {
    mView = new WeakReference<>(view);
    mDelegate = new WeakReference<>(delegate);
    mMode = mode;
    // Bound allocations even when the client omits limits or sends unreasonable dimensions.
    mMaxWidth = maxWidth > 0 ? Math.min(maxWidth, 1920) : 720;
    mMaxHeight = maxHeight > 0 ? Math.min(maxHeight, 1920) : 1280;
    mQuality = Math.max(0, Math.min(quality, 100));
    mJpegDelta = jpegDelta;
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
    if (mAck.expire(SystemClock.uptimeMillis())) {
      mPrevious = null;
    }
    if (mAck.isPending()) {
      MAIN.postDelayed(mTick, INTERVAL_MS);
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
          boolean full = mAck.needsFullFrame() || mReferenceWidth != bitmap.getWidth()
              || mReferenceHeight != bitmap.getHeight() || mReferenceDeviceWidth != deviceWidth
              || mReferenceDeviceHeight != deviceHeight;
          int baseId = mAck.reference();
          double timestamp = System.currentTimeMillis() / 1000.0;
          mWorker.execute(
              () -> encode(bitmap, deviceWidth, deviceHeight, timestamp, full, baseId, generation));
        });
  }

  private void encode(Bitmap bitmap, float deviceWidth, float deviceHeight, double timestamp,
      boolean full, int baseId, int generation) {
    String data = null;
    ScreencastDiff.Region region = null;
    boolean delta = false;
    try {
      int width = bitmap.getWidth();
      int height = bitmap.getHeight();
      TraceEvent.beginSection("ScreenCast.Diff");
      try {
        if (mCurrent == null || mCurrent.length != width * height) {
          mCurrent = new int[width * height];
        }
        bitmap.getPixels(mCurrent, 0, width, 0, 0, width, height);
        region = ScreencastDiff.compare(full ? null : mPrevious, mCurrent, width, height);
      } finally {
        TraceEvent.endSection("ScreenCast.Diff");
      }
      if (region != null) {
        delta = mJpegDelta && !full && region.isSmall(width, height);
        if (!delta) {
          region = new ScreencastDiff.Region(0, 0, width, height);
        }
        Bitmap image = delta
            ? Bitmap.createBitmap(bitmap, region.x, region.y, region.width, region.height)
            : bitmap;
        TraceEvent.beginSection("ScreenCast.Encode");
        try {
          mEncoded.reset();
          if (!image.compress(Bitmap.CompressFormat.JPEG, mQuality, mEncoded)) {
            throw new IllegalStateException("JPEG encoding failed");
          }
        } finally {
          if (image != bitmap) {
            image.recycle();
          }
          TraceEvent.endSection("ScreenCast.Encode");
        }
        TraceEvent.beginSection("ScreenCast.Base64");
        try {
          data = Base64.encodeToString(mEncoded.toByteArray(), Base64.NO_WRAP);
        } finally {
          TraceEvent.endSection("ScreenCast.Base64");
        }
      }
    } catch (RuntimeException error) {
      LLog.w("Screencast", "Cannot encode snapshot: " + error);
    }
    final String frameData = data;
    final ScreencastDiff.Region frameRegion = region;
    final boolean isDelta = delta;
    MAIN.post(() -> {
      if (mStopped || mPaused || generation != mGeneration) {
        finishDiscarded(bitmap);
        return;
      }
      mBusy = false;
      mBitmap = bitmap;
      if (frameData != null) {
        DevToolPlatformAndroidDelegate delegate = mDelegate.get();
        if (delegate == null) {
          stop();
          return;
        }
        int id = mAck.send(SystemClock.uptimeMillis(), isDelta);
        mPendingDeviceWidth = deviceWidth;
        mPendingDeviceHeight = deviceHeight;
        TraceEvent.beginSection("ScreenCast.Send");
        try {
          delegate.sendOptimizedScreenCast(frameData, deviceWidth, deviceHeight, timestamp,
              new int[] {id, isDelta ? baseId : 0, bitmap.getWidth(), bitmap.getHeight(),
                  frameRegion.x, frameRegion.y, frameRegion.width, frameRegion.height});
        } finally {
          TraceEvent.endSection("ScreenCast.Send");
        }
      }
      schedule();
    });
  }

  void acknowledge(int id, boolean applied) {
    MAIN.post(() -> {
      if (mStopped || mPaused || !mAck.acknowledge(id, applied)) {
        return;
      }
      if (applied) {
        int[] reusable = mPrevious;
        mPrevious = mCurrent;
        mCurrent = reusable;
        mReferenceWidth = mBitmap.getWidth();
        mReferenceHeight = mBitmap.getHeight();
        mReferenceDeviceWidth = mPendingDeviceWidth;
        mReferenceDeviceHeight = mPendingDeviceHeight;
      } else {
        mPrevious = null;
      }
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
      mAck.reset();
      // The worker may still be reading the reference. Defer its release until completion.
      MAIN.removeCallbacks(mTick);
      if (!mBusy) {
        mPrevious = null;
        schedule();
      }
    });
  }

  void stop() {
    runOnMain(() -> {
      mStopped = true;
      mGeneration++;
      MAIN.removeCallbacks(mTick);
      mAck.reset();
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
    mPrevious = null;
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
    mPrevious = null;
    mCurrent = null;
    mView.clear();
    mDelegate.clear();
  }

  private static void runOnMain(Runnable runnable) {
    if (Looper.myLooper() == Looper.getMainLooper()) {
      runnable.run();
    } else {
      MAIN.post(runnable);
    }
  }
}
