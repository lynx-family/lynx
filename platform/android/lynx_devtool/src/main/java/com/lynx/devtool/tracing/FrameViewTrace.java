// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.tracing;

import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.view.View;
import com.lynx.devtool.framecapture.FrameCapturer;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.utils.BitmapUtils;

public class FrameViewTrace extends FrameCapturer {
  private long mNativeFrameViewTrace = 0;

  public FrameViewTrace() {
    mNativeFrameViewTrace = nativeCreateFrameViewTrace();
  }

  private static class FrameViewTraceLoader {
    private static final FrameViewTrace INSTANCE = new FrameViewTrace();
  }

  public static FrameViewTrace getInstance() {
    return FrameViewTraceLoader.INSTANCE;
  }

  @Override
  protected boolean isEnabled() {
    return TraceEvent.categoryEnabled(TraceEvent.CATEGORY_SCREENSHOTS);
  }

  private String takeScreenshot(View view) {
    String data = null;
    try {
      view.setDrawingCacheEnabled(true);
      Bitmap bitmap = Bitmap.createBitmap(view.getDrawingCache());
      view.setDrawingCacheEnabled(false);
      if (bitmap != null) {
        int originWidth = bitmap.getWidth();
        int originHeight = bitmap.getHeight();
        float area = originHeight * originWidth;
        if (area > mMaxScreenshotAreaSize) {
          float scale = (float) Math.sqrt(mMaxScreenshotAreaSize / area);
          Matrix matrix = new Matrix();
          matrix.setScale(scale, scale);
          bitmap = Bitmap.createBitmap(bitmap, 0, 0, originWidth, originHeight, matrix, false);
        }
        data = BitmapUtils.bitmapToBase64WithQuality(bitmap, mScreenshotQuality);
      }
    } catch (Throwable e) {
      e.printStackTrace();
    }
    return data;
  }

  @Override
  protected void onNewScreenshotBitmapData(String data) {
    if (isEnabled()) {
      FrameTraceService.getInstance().screenshot(data);
    }
  }

  @CalledByNative
  public void startFrameViewTrace() {
    super.startFrameViewTrace();
  }

  @CalledByNative
  public void stopFrameViewTrace() {
    super.stopFrameViewTrace();
  }

  public long getNativeFrameViewTrace() {
    return mNativeFrameViewTrace;
  }

  private native long nativeCreateFrameViewTrace();

  protected void screenshot(final View view, ScreenshotBitmapHandler handler) {
    String data = takeScreenshot(view);
    onScreenshotActionEnd();
    if (data == null || data.equals(mScreenshotBitmapDataCache)) {
      return;
    }
    mScreenshotBitmapDataCache = data;
    onNewScreenshotBitmapData(data);
  }
}
