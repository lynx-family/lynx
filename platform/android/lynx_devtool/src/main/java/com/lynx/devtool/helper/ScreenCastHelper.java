// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.graphics.Bitmap;
import com.lynx.devtool.DevToolPlatformAndroidDelegate;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.tasm.LynxView;
import java.lang.ref.WeakReference;

public class ScreenCastHelper {
  private WeakReference<DevToolPlatformAndroidDelegate> mPlatformDelegate;
  private WeakReference<LynxView> mLynxView;
  private volatile boolean mPaused;
  private volatile boolean mScreencastEnabled;
  private ScreencastSession mSession;
  private volatile int mLegacyGeneration;

  public ScreenCastHelper(DevToolPlatformAndroidDelegate platformDelegate, LynxView lynxView) {
    mPlatformDelegate = new WeakReference<>(platformDelegate);
    mLynxView = new WeakReference<>(lynxView);
    mPaused = false;
    mScreencastEnabled = false;
  }

  public void attach(LynxView lynxView) {
    if (mSession != null) {
      stopCasting();
    }
    mLynxView = new WeakReference<>(lynxView);
    ScreenCapturer.getInstance().attachView(lynxView);
  }

  public void startCasting(int quality, int maxWidth, int maxHeight, String screenShotMode,
      String format, IDevToolDelegate devToolDelegate, boolean enableBetterScreencast,
      boolean enableJpegDelta) {
    stopCasting();
    mPaused = false;
    mScreencastEnabled = true;
    DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
    if (platformDelegate != null) {
      platformDelegate.dispatchScreencastVisibilityChanged(true);
    }
    if (enableBetterScreencast && !"png".equals(format) && devToolDelegate != null
        && !devToolDelegate.isClayRenderer()) {
      mSession = new ScreencastSession(mLynxView.get(), platformDelegate, screenShotMode, maxWidth,
          maxHeight, quality, enableJpegDelta);
      return;
    }
    ScreenCapturer.getInstance().attachView(mLynxView.get());
    ScreenCapturer.getInstance().setDevToolDelegate(devToolDelegate);
    final int generation = mLegacyGeneration;
    ScreenCapturer.getInstance().startCapture(maxWidth, maxHeight, quality, screenShotMode, format,
        new ScreenCapturer.ScreenshotListener() {
          @Override
          public void onNewScreenshotBitmapData(String screenData, long timeCost) {
            if (generation != mLegacyGeneration || !mScreencastEnabled || mPaused) {
              return;
            }
            ScreenCapturer.ScreenMetadata metadata =
                ScreenCapturer.getInstance().getScreenMetadata();
            // nanosecond to millisecond
            float time = (float) (((double) timeCost) / 1E6);
            DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
            if (platformDelegate != null) {
              platformDelegate.sendScreenCast(
                  screenData, metadata, metadata.mPageScaleFactor, time);
            }
          }
        });
  }

  public void stopCasting() {
    mLegacyGeneration++;
    if (mSession != null) {
      mSession.stop();
      mSession = null;
    }
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
        if (mSession != null) {
          mSession.setPaused(false);
        }
        DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
        if (platformDelegate != null) {
          platformDelegate.dispatchScreencastVisibilityChanged(true);
        }
        if (mSession == null) {
          ScreenCapturer.getInstance().triggerNextCapture();
        }
      }
    }
  }

  public void pauseCasting() {
    if (mScreencastEnabled) {
      if (!mPaused) {
        mPaused = true;
        if (mSession != null) {
          mSession.setPaused(true);
        }
        DevToolPlatformAndroidDelegate platformDelegate = mPlatformDelegate.get();
        if (platformDelegate != null) {
          platformDelegate.dispatchScreencastVisibilityChanged(false);
        }
      }
    }
  }

  public void onAckReceived() {
    if (mSession == null) {
      ScreenCapturer.getInstance().onAckReceived();
    }
  }

  public void onScreencastFrameAck(int sessionId, boolean frameApplied) {
    if (mSession != null) {
      mSession.acknowledge(sessionId, frameApplied);
    } else {
      onAckReceived();
    }
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
