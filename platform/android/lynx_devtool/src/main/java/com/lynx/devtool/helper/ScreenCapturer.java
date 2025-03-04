// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.os.HandlerThread;
import android.view.Choreographer;
import android.view.View;
import androidx.annotation.NonNull;
import com.lynx.devtool.framecapture.FrameCapturer;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.BitmapUtils;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicBoolean;

public class ScreenCapturer extends FrameCapturer {
  private static class ScreenRequest {
    private int mMaxWidth;
    private int mMaxHeight;
    private int mQuality;
    private String mScreenshotMode;
  }

  public static class ScreenMetadata {
    public float mOffsetTop;
    public float mPageScaleFactor;
    public float mDeviceWidth;
    public float mDeviceHeight;
    public float mScrollOffsetX;
    public float mScrollOffsetY;
    public float mTimestamp;
  }

  public interface ScreenshotListener {
    void onNewScreenshotBitmapData(String screenData, long timeCost);
  }

  private static final String TAG = "ScreenCapturer";
  private static final ScreenCapturer mInstance = new ScreenCapturer();

  private static final int CARD_PREVIEW_QUALITY = 80;
  private static final int CARD_PREVIEW_MAX_WIDTH = 150;
  private static final int CARD_PREVIEW_MAX_HEIGHT = 300;

  private IDevToolDelegate mDevToolDelegate = null;

  // Delay for 1500ms to allow time for rendering remote resources
  public static final int SCREEN_SHOT_PREVIEW_DELAY_TIME = 1500;

  private final ScreenRequest mScreenRequest;
  private final ScreenMetadata mScreenMetadata;
  private boolean mIsEnabled;
  private ScreenshotListener mScreenshotListener;
  // mAckReceived is true that represents we can capture screen now.
  // when we send screen data to remote, we need reset its value(i.e. false)
  private final AtomicBoolean mAckReceived;
  // mIsDirty represents current frame whether need capture or not
  // when devtool has captured screen, we need reset its value(i.e. false)
  protected AtomicBoolean mIsDirty;
  private Choreographer.FrameCallback mResetDirtyCallBack;

  private volatile long mScreenshotOnMainThreadTime = 0;
  private long mScreenshotStartTime = 0;

  // use this executorService to encode and send screen data
  private final ExecutorService executorService =
      Executors.newCachedThreadPool(new ThreadFactory() {
        @Override
        public Thread newThread(@NonNull Runnable runnable) {
          return new Thread(runnable, TAG);
        }
      });

  public static ScreenCapturer getInstance() {
    return mInstance;
  }

  private ScreenCapturer() {
    super();
    mScreenRequest = new ScreenRequest();
    mScreenMetadata = new ScreenMetadata();
    mIsEnabled = false;
    mScreenshotListener = null;
    mAckReceived = new AtomicBoolean(false);
    mIsDirty = new AtomicBoolean(false);
    mFrameChangeListener = new FrameChangeListener() {
      @Override
      public void onFrameChanged() {
        triggerNextCapture();
      }
    };
    mResetDirtyCallBack = new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        mIsDirty.set(true);
        Choreographer.getInstance().postFrameCallback(mResetDirtyCallBack);
      }
    };
  }

  public void setDevToolDelegate(IDevToolDelegate devToolDelegate) {
    mDevToolDelegate = devToolDelegate;
  }

  public void startCapture(int maxWidth, int maxHeight, int quality, String screenShotMode,
      ScreenshotListener listener) {
    mScreenRequest.mMaxWidth = maxWidth;
    mScreenRequest.mMaxHeight = maxHeight;
    mScreenRequest.mQuality = quality;
    mScreenRequest.mScreenshotMode = screenShotMode;
    mScreenshotListener = listener;
    mIsEnabled = true;
    addResetDirtyStatusCallBack();
    startFrameViewTrace();
    // manually trigger first screenshot
    triggerNextCapture();
  }

  private void addResetDirtyStatusCallBack() {
    Choreographer.getInstance().postFrameCallback(mResetDirtyCallBack);
  }

  public void stopCapture(View targetView) {
    if (targetView != mView.get()) {
      return;
    }
    stopFrameViewTrace();
    mIsEnabled = false;
    mView.clear();
  }

  public void triggerNextCapture() {
    if (!mIsEnabled) {
      return;
    }

    if (mScreenshotBitmapDataCache == null || (mAckReceived.get() && mIsDirty.get())) {
      screenshot();
    }
  }

  public void onAckReceived() {
    mAckReceived.set(true);
    triggerNextCapture();
  }

  public ScreenMetadata getScreenMetadata() {
    return mScreenMetadata;
  }

  @Override
  protected boolean isEnabled() {
    return mIsEnabled;
  }

  protected String getScreenshotDataFromBitmap(Bitmap bitmap, boolean isPreview) {
    int quality = isPreview ? CARD_PREVIEW_QUALITY : mScreenRequest.mQuality;
    int maxWidth = isPreview ? CARD_PREVIEW_MAX_WIDTH : mScreenRequest.mMaxWidth;
    int maxHeight = isPreview ? CARD_PREVIEW_MAX_HEIGHT : mScreenRequest.mMaxHeight;
    return BitmapUtils.bitmapToBase64WithQuality(scaleImage(bitmap, maxWidth, maxHeight), quality);
  }

  public void onScreenshotBitmapReady(Bitmap bitmap) {
    executorService.submit(new Runnable() {
      @Override
      public void run() {
        if (bitmap != null) {
          String screenshotData = getScreenshotDataFromBitmap(bitmap, false);
          if (screenshotData == null || screenshotData.equals(mScreenshotBitmapDataCache)) {
            return;
          }
          mScreenshotBitmapDataCache = screenshotData;
          onNewScreenshotBitmapData(mScreenshotBitmapDataCache);
        }
      }
    });
  }

  @Override
  protected void onNewScreenshotBitmapData(String data) {
    mAckReceived.set(false);
    if (mScreenshotListener != null) {
      if (mScreenshotOnMainThreadTime == 0) {
        LLog.e(TAG, "onNewScreenshotBitmapData: screenshotMainTime == 0");
      }
      mScreenshotListener.onNewScreenshotBitmapData(data, mScreenshotOnMainThreadTime);
    }
  }

  @Override
  protected void onScreenshotActionStart() {
    mIsDirty.set(false);
    mScreenshotStartTime = System.nanoTime();
    mScreenshotOnMainThreadTime = 0;
  }

  @Override
  protected void onScreenshotActionEnd() {
    if (mScreenshotStartTime == 0) {
      LLog.e(TAG, "onScreenshotActionEnd: lastTime == 0");
      return;
    }
    mScreenshotOnMainThreadTime = System.nanoTime() - mScreenshotStartTime;
  }

  @Override
  protected void screenshot(final View view, ScreenshotBitmapHandler handler) {
    if (mDevToolDelegate != null) {
      mDevToolDelegate.takeScreenshot(handler, mScreenRequest.mScreenshotMode);
    }
    onScreenshotActionEnd();
  }

  public void submit(Runnable task) {
    executorService.submit(task);
  }

  private float getScale(int originalWidth, int originalHeight, int maxWidth, int maxHeight) {
    float scale = 0;
    float scalingWidth = 1;
    float scalingHeight = 1;
    try {
      if (maxWidth != 0 && maxHeight != 0
          && ((originalWidth) > maxWidth || (originalHeight) > maxHeight)) {
        scalingWidth = maxWidth / (float) (originalWidth);
        scalingHeight = maxHeight / (float) (originalHeight);
      }
      scale = Math.min(scalingWidth, scalingHeight);
    } catch (Throwable e) {
      e.printStackTrace();
    }
    return scale;
  }

  private Bitmap scaleImage(Bitmap bitmap, int maxWidth, int maxHeight) {
    if (bitmap == null) {
      LLog.e(TAG, "scaleImage: bitmap is null");
      return null;
    }
    int originalWidth = bitmap.getWidth();
    int originalHeight = bitmap.getHeight();
    float scale = getScale(originalWidth, originalHeight, maxWidth, maxHeight);
    mScreenMetadata.mDeviceWidth = originalWidth;
    mScreenMetadata.mDeviceHeight = originalHeight;
    mScreenMetadata.mPageScaleFactor = 1;
    try {
      Matrix matrix = new Matrix();
      matrix.postScale(scale, scale);
      bitmap = Bitmap.createBitmap(bitmap, 0, 0, originalWidth, originalHeight, matrix, false);
    } catch (Throwable e) {
      e.printStackTrace();
    }
    return bitmap;
  }
}
