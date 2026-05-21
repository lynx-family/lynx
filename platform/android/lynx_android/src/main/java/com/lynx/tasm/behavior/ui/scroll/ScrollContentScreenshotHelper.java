// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.Base64;
import android.view.View;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.service.ILynxSystemInvokeService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.BitmapUtils;
import java.util.ArrayList;
import java.util.List;

final class ScrollContentScreenshotHelper {
  private static final long MAX_SCREENSHOT_MEMORY_BYTES = 256L * 1024L * 1024L;

  private ScrollContentScreenshotHelper() {}

  static void takeContentScreenshot(View view, View contentView, int contentWidth,
      int contentHeight, LynxContext context, ReadableMap params, Callback callback,
      ScrollContentScreenshotController controller) {
    if (contentView == null) {
      takeContentScreenshot(
          view, contentWidth, contentHeight, context, params, callback, controller);
      return;
    }

    if (view == null || view.getWidth() <= 0 || view.getHeight() <= 0) {
      callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE, new JavaOnlyMap());
      return;
    }

    final String format = params == null ? "jpeg" : params.getString("format", "jpeg");
    Bitmap.Config config;
    final Bitmap.CompressFormat compressFormat;
    final String header;
    if ("png".equals(format)) {
      config = Bitmap.Config.ARGB_8888;
      compressFormat = Bitmap.CompressFormat.PNG;
      header = "data:image/png;base64,";
    } else {
      config = Bitmap.Config.RGB_565;
      compressFormat = Bitmap.CompressFormat.JPEG;
      header = "data:image/jpeg;base64,";
    }

    final float scale = params == null ? 1.0f : (float) params.getDouble("scale", 1.0);
    if (Float.isNaN(scale) || Float.isInfinite(scale) || scale <= 0.f) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "scale must be greater than 0");
      return;
    }

    final int captureWidth =
        Math.max(Math.max(view.getWidth(), contentView.getWidth()), contentWidth);
    final int captureHeight =
        Math.max(Math.max(view.getHeight(), contentView.getHeight()), contentHeight);
    int[] bitmapSize = resolveBitmapSize(captureWidth, captureHeight, scale, config, callback);
    if (bitmapSize == null) {
      return;
    }
    final int bitmapWidth = bitmapSize[0];
    final int bitmapHeight = bitmapSize[1];

    final int originX = controller.getScrollX();
    final int originY = controller.getScrollY();
    Bitmap bitmap = null;
    try {
      controller.scrollTo(0, 0);
      bitmap = Bitmap.createBitmap(bitmapWidth, bitmapHeight, config);
      Canvas canvas = new Canvas(bitmap);
      canvas.scale(scale, scale);
      drawBackground(view, canvas, captureWidth, captureHeight);

      int saveCount = canvas.save();
      canvas.translate(-contentView.getLeft(), -contentView.getTop());
      boolean dirty = contentView.isDirty();
      contentView.draw(canvas);
      if (dirty) {
        contentView.postInvalidate();
      }
      canvas.restoreToCount(saveCount);
    } catch (Throwable e) {
      if (bitmap != null) {
        bitmap.recycle();
      }
      controller.scrollTo(originX, originY);
      callback.invoke(LynxUIMethodConstants.UNKNOWN, new JavaOnlyMap());
      return;
    }

    controller.scrollTo(originX, originY);
    encodeBitmapAsync(bitmap, compressFormat, header, callback);
  }

  static void takeContentScreenshot(View view, int contentWidth, int contentHeight,
      LynxContext context, ReadableMap params, Callback callback,
      ScrollContentScreenshotController controller) {
    if (view == null || view.getWidth() <= 0 || view.getHeight() <= 0) {
      callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE, new JavaOnlyMap());
      return;
    }

    final String format = params == null ? "jpeg" : params.getString("format", "jpeg");
    Bitmap.Config config;
    final Bitmap.CompressFormat compressFormat;
    final String header;
    if ("png".equals(format)) {
      config = Bitmap.Config.ARGB_8888;
      compressFormat = Bitmap.CompressFormat.PNG;
      header = "data:image/png;base64,";
    } else {
      config = Bitmap.Config.RGB_565;
      compressFormat = Bitmap.CompressFormat.JPEG;
      header = "data:image/jpeg;base64,";
    }

    final float scale = params == null ? 1.0f : (float) params.getDouble("scale", 1.0);
    if (Float.isNaN(scale) || Float.isInfinite(scale) || scale <= 0.f) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "scale must be greater than 0");
      return;
    }

    final int viewportWidth = view.getWidth();
    final int viewportHeight = view.getHeight();
    final int captureWidth = Math.max(viewportWidth, contentWidth);
    final int captureHeight = Math.max(viewportHeight, contentHeight);
    int[] bitmapSize = resolveBitmapSize(captureWidth, captureHeight, scale, config, callback);
    if (bitmapSize == null) {
      return;
    }
    final int bitmapWidth = bitmapSize[0];
    final int bitmapHeight = bitmapSize[1];

    final int originX = controller.getScrollX();
    final int originY = controller.getScrollY();
    final boolean pixelCopy = params != null && params.getBoolean("androidEnablePixelCopy", false);
    final ILynxSystemInvokeService systemInvokeService =
        LynxServiceCenter.inst().getService(ILynxSystemInvokeService.class);
    Bitmap bitmap = null;
    try {
      bitmap = Bitmap.createBitmap(bitmapWidth, bitmapHeight, config);
      Canvas canvas = new Canvas(bitmap);
      canvas.scale(scale, scale);

      for (int y : buildOffsets(captureHeight - viewportHeight, viewportHeight)) {
        for (int x : buildOffsets(captureWidth - viewportWidth, viewportWidth)) {
          controller.scrollTo(x, y);
          drawViewportAt(view, canvas, config, context, pixelCopy, systemInvokeService, x, y);
        }
      }
    } catch (Throwable e) {
      if (bitmap != null) {
        bitmap.recycle();
      }
      controller.scrollTo(originX, originY);
      callback.invoke(LynxUIMethodConstants.UNKNOWN, new JavaOnlyMap());
      return;
    }

    controller.scrollTo(originX, originY);
    encodeBitmapAsync(bitmap, compressFormat, header, callback);
  }

  private static int[] resolveBitmapSize(
      int captureWidth, int captureHeight, float scale, Bitmap.Config config, Callback callback) {
    final long bitmapWidth = Math.max(1L, Math.round((double) captureWidth * scale));
    final long bitmapHeight = Math.max(1L, Math.round((double) captureHeight * scale));
    if (bitmapWidth > Integer.MAX_VALUE || bitmapHeight > Integer.MAX_VALUE) {
      callback.invoke(LynxUIMethodConstants.OPERATION_ERROR, "screenshot size exceeds limit");
      return null;
    }
    if (bitmapWidth > Long.MAX_VALUE / bitmapHeight) {
      callback.invoke(LynxUIMethodConstants.OPERATION_ERROR, "screenshot memory size overflow");
      return null;
    }
    final long pixels = bitmapWidth * bitmapHeight;
    final int pixelSize = bytesPerPixel(config);
    if (pixels > Long.MAX_VALUE / pixelSize) {
      callback.invoke(LynxUIMethodConstants.OPERATION_ERROR, "screenshot memory size overflow");
      return null;
    }
    final long byteCount = pixels * pixelSize;
    if (byteCount > MAX_SCREENSHOT_MEMORY_BYTES) {
      callback.invoke(LynxUIMethodConstants.OPERATION_ERROR, "screenshot memory exceeds limit");
      return null;
    }
    return new int[] {(int) bitmapWidth, (int) bitmapHeight};
  }

  private static int bytesPerPixel(Bitmap.Config config) {
    if (config == Bitmap.Config.RGB_565) {
      return 2;
    }
    return 4;
  }

  private static void drawBackground(View view, Canvas canvas, int width, int height) {
    Drawable background = view.getBackground();
    if (background == null) {
      return;
    }
    Rect originalBounds = new Rect(background.getBounds());
    background.setBounds(0, 0, width, height);
    background.draw(canvas);
    background.setBounds(originalBounds);
  }

  private static List<Integer> buildOffsets(int maxOffset, int viewportSize) {
    List<Integer> offsets = new ArrayList<>();
    offsets.add(0);
    if (maxOffset <= 0 || viewportSize <= 0) {
      return offsets;
    }

    int offset = 0;
    while (offset < maxOffset) {
      offset = Math.min(offset + viewportSize, maxOffset);
      if (offsets.get(offsets.size() - 1) != offset) {
        offsets.add(offset);
      }
    }
    return offsets;
  }

  private static void drawViewportAt(View view, Canvas canvas, Bitmap.Config config,
      LynxContext context, boolean pixelCopy, ILynxSystemInvokeService systemInvokeService, int x,
      int y) {
    if (systemInvokeService != null) {
      Bitmap viewportBitmap = systemInvokeService.takeScreenshot(view, config);
      if (viewportBitmap != null) {
        canvas.drawBitmap(viewportBitmap, x, y, null);
        viewportBitmap.recycle();
        return;
      }
    }
    if (pixelCopy && context != null && context.getUIBodyView() != null) {
      Bitmap viewportBitmap = Bitmap.createBitmap(view.getWidth(), view.getHeight(), config);
      Canvas viewportCanvas = new Canvas(viewportBitmap);
      context.getUIBodyView().getLynxUIRendererInternal().drawViewToBitmap(
          view, viewportBitmap, viewportCanvas);
      canvas.drawBitmap(viewportBitmap, x, y, null);
      viewportBitmap.recycle();
      return;
    }
    int saveCount = canvas.save();
    canvas.translate(x, y);
    boolean dirty = view.isDirty();
    view.draw(canvas);
    if (dirty) {
      view.postInvalidate();
    }
    canvas.restoreToCount(saveCount);
  }

  private static void encodeBitmapAsync(final Bitmap bitmap,
      final Bitmap.CompressFormat compressFormat, final String header, final Callback callback) {
    LynxThreadPool.getBriefIOExecutor().execute(new Runnable() {
      @Override
      public void run() {
        try {
          String data = BitmapUtils.bitmapToBase64(bitmap, compressFormat, 100, Base64.NO_WRAP);
          JavaOnlyMap result = new JavaOnlyMap();
          result.putInt("width", bitmap.getWidth());
          result.putInt("height", bitmap.getHeight());
          result.putString("data", header + data);
          callback.invoke(LynxUIMethodConstants.SUCCESS, result);
        } catch (Throwable e) {
          callback.invoke(LynxUIMethodConstants.UNKNOWN, new JavaOnlyMap());
        } finally {
          bitmap.recycle();
        }
      }
    });
  }
}
