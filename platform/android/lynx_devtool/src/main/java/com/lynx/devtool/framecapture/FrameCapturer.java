// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.framecapture;

import android.graphics.Bitmap;
import android.os.Build;
import android.view.View;
import com.lynx.devtool.helper.ScreenCapturer;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public abstract class FrameCapturer {
  public interface FrameChangeListener {
    void onFrameChanged();
  }

  // compress quality of a picture, default is 70
  protected int mScreenshotQuality = 70;
  // Maximum number of pixels of a picture, default is 256000 pixels
  protected long mMaxScreenshotAreaSize = 256000;
  // Maximum screenshot interval(TimeUnit microsecond), default is 32000 us
  protected int mScreenshotInterval = 32000;
  protected WeakReference<View> mView;
  protected long mLastScreenshotTime;
  protected volatile String mScreenshotBitmapDataCache;
  protected FrameChangeListener mFrameChangeListener;
  private final Runnable mCallback;

  private static final int CALLBACK_LAST;

  static {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
      CALLBACK_LAST = 2;
    } else if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
      CALLBACK_LAST = 3;
    } else {
      CALLBACK_LAST = 4;
    }
  }

  public FrameCapturer() {
    mLastScreenshotTime = 0;
    mView = new WeakReference<>(null);
    mCallback = new Runnable() {
      @Override
      public void run() {
        if (mFrameChangeListener == null) {
          screenshot();
        } else {
          mFrameChangeListener.onFrameChanged();
        }

        if (isEnabled()) {
          addFrameViewCallback();
        }
      }
    };
  }

  /**
   * Indicate whether we should start frame capture or not
   * @return
   */
  protected abstract boolean isEnabled();

  /**
   * Notify caller new screenshot bitmap data is ready
   * @param data
   */
  protected abstract void onNewScreenshotBitmapData(String data);

  public void attachView(View view) {
    mView = new WeakReference<>(view);
  }

  public void startFrameViewTrace() {
    clearScreenshotBitmapDataCache();
    if (isEnabled()) {
      addFrameViewCallback();
    }
  }

  public void stopFrameViewTrace() {}

  public void clearScreenshotBitmapDataCache() {
    mScreenshotBitmapDataCache = null;
  }

  protected void addFrameViewCallback() {
    if (isEnabled()) {
      FrameTraceUtil.addFrameCallback(mCallback);
    }
  }

  protected void screenshot() {
    final long currentTime = FrameTraceUtil.getSystemBootTimeNs() / 1000;
    if (currentTime - mLastScreenshotTime < mScreenshotInterval) {
      return;
    }

    mLastScreenshotTime = currentTime;

    // post a message to main looper
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        View view = mView.get();
        if (view == null) {
          return;
        }
        onScreenshotActionStart();
        try {
          screenshot(view, new ScreenshotBitmapHandler() {
            @Override
            public void sendBitmap(Bitmap bitmap) {
              ScreenCapturer.getInstance().onScreenshotBitmapReady(bitmap);
            }
          });
        } catch (Throwable e) {
          e.printStackTrace();
        }
      }
    });
  }

  protected void onScreenshotActionEnd() {}

  protected void onScreenshotActionStart() {}

  protected abstract void screenshot(final View view, ScreenshotBitmapHandler handler);
}
