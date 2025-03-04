// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.graphics.Bitmap;
import android.util.DisplayMetrics;
import com.lynx.devtool.DevToolPlatformAndroidDelegate;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.lang.ref.WeakReference;

public class ScreenCastHelper {
  private WeakReference<DevToolPlatformAndroidDelegate> mPlatformDelegate;
  private WeakReference<LynxView> mLynxView;
  private volatile boolean mPaused;
  private volatile boolean mScreencastEnabled;

  public ScreenCastHelper(DevToolPlatformAndroidDelegate platformDelegate, LynxView lynxView) {
    mPlatformDelegate = new WeakReference<>(platformDelegate);
    mLynxView = new WeakReference<>(lynxView);
    mPaused = false;
    mScreencastEnabled = false;
  }

  public void attach(LynxView lynxView) {
    mLynxView = new WeakReference<>(lynxView);
    ScreenCapturer.getInstance().attachView(lynxView);
  }

  public void startCasting(int quality, int maxWidth, int maxHeight, String screenShotMode,
      IDevToolDelegate devToolDelegate) {
    mScreencastEnabled = true;
    DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
    if (platformDelegate != null) {
      platformDelegate.dispatchScreencastVisibilityChanged(true);
    }
    ScreenCapturer.getInstance().attachView(mLynxView.get());
    ScreenCapturer.getInstance().setDevToolDelegate(devToolDelegate);
    ScreenCapturer.getInstance().startCapture(
        maxWidth, maxHeight, quality, screenShotMode, new ScreenCapturer.ScreenshotListener() {
          @Override
          public void onNewScreenshotBitmapData(String screenData, long timeCost) {
            ScreenCapturer.ScreenMetadata metadata =
                ScreenCapturer.getInstance().getScreenMetadata();
            DisplayMetrics dm = DisplayMetricsHolder.getScreenDisplayMetrics();
            // nanosecond to millisecond
            float time = (float) (((double) timeCost) / 1E6);
            DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
            if (platformDelegate != null) {
              platformDelegate.sendScreenCast(
                  screenData, metadata, metadata.mPageScaleFactor * dm.density, time);
            }
          }
        });
  }

  public void stopCasting() {
    mScreencastEnabled = false;
    LynxView lynxView = mLynxView.get();
    if (lynxView != null) {
      ScreenCapturer.getInstance().stopCapture(lynxView);
      DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
      if (platformDelegate != null) {
        platformDelegate.dispatchScreencastVisibilityChanged(false);
      }
    }
  }

  public void continueCasting() {
    if (mScreencastEnabled) {
      if (mPaused) {
        ScreenCapturer.getInstance().clearScreenshotBitmapDataCache();
        mPaused = false;
        DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
        if (platformDelegate != null) {
          platformDelegate.dispatchScreencastVisibilityChanged(true);
        }
        ScreenCapturer.getInstance().triggerNextCapture();
      }
    }
  }

  public void pauseCasting() {
    if (mScreencastEnabled) {
      if (!mPaused) {
        mPaused = true;
        DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
        if (platformDelegate != null) {
          platformDelegate.dispatchScreencastVisibilityChanged(false);
        }
      }
    }
  }

  public void onAckReceived() {
    ScreenCapturer.getInstance().onAckReceived();
  }

  public void sendCardPreview(IDevToolDelegate devToolDelegate) {
    LynxView lynxView = mLynxView.get();
    if (lynxView != null && devToolDelegate != null) {
      lynxView.postDelayed(new Runnable() {
        @Override
        public void run() {
          Bitmap bitmap = devToolDelegate.getBitmapOfView();
          // Delay for 1500ms to allow time for rendering remote resources
          ScreenCapturer.getInstance().submit(new Runnable() {
            @Override
            public void run() {
              String cardPreviewData =
                  ScreenCapturer.getInstance().getScreenshotDataFromBitmap(bitmap, true);
              DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
              if (platformDelegate != null) {
                platformDelegate.sendCardPreviewData(cardPreviewData);
              }
            }
          });
        }
      }, ScreenCapturer.SCREEN_SHOT_PREVIEW_DELAY_TIME);
    }
  }
}
