// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.utils.PixelUtils;

// code moved from BaseRoundedCornerPostprocessor
public class NinePatchHelper {
  private static float getCapInsetsScale(String insetScaleStr) {
    float res = 1;
    if (insetScaleStr == null || insetScaleStr.equalsIgnoreCase("")) {
      return res;
    }
    try {
      res = Float.parseFloat(insetScaleStr);
    } catch (Throwable e) {
      res = 1;
      LLog.w("BaseRoundedCornerPostprocessor", "initCapInsetsScale error " + e.getMessage());
    }
    return res;
  }

  private static String[] getCapInsetsArr(String insetStr) {
    String[] res = null;
    if (insetStr == null || insetStr.equalsIgnoreCase("")) {
      return res;
    }
    String[] arr = insetStr.split(" ");
    if (arr.length <= 4) {
      boolean legal = true;
      for (int i = 0; i < Math.min(arr.length, 4); ++i) {
        if (!arr[i].endsWith("px") && !arr[i].endsWith("%")) {
          legal = false;
        } else if (arr[i].endsWith("px")) {
          if (arr[i].length() <= 2) {
            legal = false;
          } else {
            legal = arr[i].substring(0, arr[i].length() - 2).matches("[+]?[0-9]*\\.?[0-9]+");
          }
        } else if (arr[i].endsWith("%")) {
          if (arr[i].length() <= 1) {
            legal = false;
          } else {
            legal = arr[i].substring(0, arr[i].length() - 1).matches("[+]?[0-9]*\\.?[0-9]+");
          }
        }
      }
      if (legal) {
        res = arr;
      } else {
        res = null;
      }
    }
    return res;
  }

  private static float[] getFloatCapInsetsArr(String[] strs, int sourceWidth, int sourceHeight) {
    if (strs == null || strs.length > 4) {
      return null;
    }
    float[] insets = new float[] {0, 0, 0, 0};
    for (int i = 0; i < strs.length; ++i) {
      if (strs[i].endsWith("px")) {
        insets[i] = Float.parseFloat(strs[i].substring(0, strs[i].length() - 2));
      } else if (strs[i].endsWith("%")) {
        if (i == 0 || i == 2) {
          insets[i] = Float.parseFloat(strs[i].substring(0, strs[i].length() - 2)) * sourceHeight;
        } else {
          insets[i] = Float.parseFloat(strs[i].substring(0, strs[i].length() - 2)) * sourceWidth;
        }
      }
    }
    return insets;
  }

  public static Matrix getMatrix(int availableWidth, int availableHeight, int sourceWidth,
      int sourceHeight, ScalingUtils.ScaleType scaleType) {
    TraceEvent.beginSection("image.NinePatchHelper.getMatrix");
    Matrix m = new Matrix();
    float w = 1f * availableWidth / sourceWidth;
    float h = 1f * availableHeight / sourceHeight;
    if (scaleType == ScalingUtils.ScaleType.FIT_XY) {
      m.preScale(w, h);
    } else if (scaleType == ScalingUtils.ScaleType.FIT_CENTER) {
      if (w > h) {
        float finalWidth = sourceWidth * h;
        float startW = (availableWidth - finalWidth) / 2;
        float startH = 0;
        m.setScale(h, h);
        m.postTranslate(startW, startH);
      } else {
        float finalHeight = sourceHeight * w;
        float startW = 0;
        float startH = (availableHeight - finalHeight) / 2;
        m.setScale(w, w);
        m.postTranslate(startW, startH);
      }
    } else if (scaleType == ScalingUtils.ScaleType.CENTER_CROP) {
      if (w > h) {
        float finalHeight = sourceHeight * w;
        float translateX = 0;
        float translateY = (availableHeight - finalHeight) / 2;
        m.setScale(w, w);
        m.postTranslate(translateX, translateY);
      } else {
        float finalWidth = sourceWidth * h;
        float translateX = (availableWidth - finalWidth) / 2;
        float translateY = 0;
        m.setScale(h, h);
        m.postTranslate(translateX, translateY);
      }
    } else {
      final float sourceSizeScale = PixelUtils.dipToPx(1.0f);
      float finalSourceWidth = sourceSizeScale * sourceWidth;
      float finalSourceHeight = sourceSizeScale * sourceHeight;
      float coorX = Math.round((availableWidth - finalSourceWidth) * 0.5f);
      float coorY = Math.round((availableHeight - finalSourceHeight) * 0.5f);
      m.setScale(sourceSizeScale, sourceSizeScale);
      m.postTranslate(coorX, coorY);
    }
    TraceEvent.endSection("image.NinePatchHelper.getMatrix");
    return m;
  }

  public static boolean drawNinePatch(int availableWidth, int availableHeight, int sourceWidth,
      int sourceHeight, ScalingUtils.ScaleType scaleType, String capInsets, String capInsetsScale,
      Canvas canvas, Bitmap bitmap) {
    TraceEvent.beginSection("image.NinePatchHelper.drawNinePatch");
    boolean drawn = false;
    try {
      boolean useCapInsets = true;
      String[] capInsetsArr = getCapInsetsArr(capInsets);
      float[] floatCapInsetsArr = getFloatCapInsetsArr(capInsetsArr, sourceWidth, sourceHeight);
      float floatCapInsetsScale = getCapInsetsScale(capInsetsScale);
      if (floatCapInsetsArr == null || floatCapInsetsArr.length != 4
          || (floatCapInsetsArr[0] == 0 && floatCapInsetsArr[1] == 0 && floatCapInsetsArr[2] == 0
              && floatCapInsetsArr[3] == 0)) {
        useCapInsets = false;
      }
      if (!useCapInsets) {
        Matrix m = getMatrix(availableWidth, availableHeight, sourceWidth, sourceHeight, scaleType);
        canvas.drawBitmap(bitmap, m, null);
      } else {
        drawWithCapInsets(availableWidth, availableHeight, sourceWidth, sourceHeight, scaleType,
            floatCapInsetsArr, floatCapInsetsScale, canvas, bitmap);
      }
      drawn = true;
    } catch (Throwable e) {
      LLog.w("BaseRoundedCornerPostprocessor", "process customDraw warn " + e.getMessage());
    }
    TraceEvent.endSection("image.NinePatchHelper.drawNinePatch");
    return drawn;
  }

  private static void drawWithCapInsets(int availableWidth, int availableHeight, int sourceWidth,
      int sourceHeight, ScalingUtils.ScaleType scaleType, float[] capInsets, float capInsetsScale,
      Canvas canvas, Bitmap bitmap) {
    TraceEvent.beginSection("image.NinePatchHelper.drawWithCapInsets");
    Rect src0 = new Rect(
        0, 0, (int) (capInsets[3] * capInsetsScale), (int) (capInsets[0] * capInsetsScale));
    Rect src1 = new Rect(
        src0.right, src0.top, (int) (sourceWidth - capInsets[1] * capInsetsScale), src0.bottom);
    Rect src2 = new Rect(src1.right, src0.top, sourceWidth, src0.bottom);
    Rect src3 = new Rect(
        src0.left, src0.bottom, src0.right, (int) (sourceHeight - capInsets[2] * capInsetsScale));
    Rect src4 = new Rect(src3.right, src3.top, src1.right, src3.bottom);
    Rect src5 = new Rect(src4.right, src3.top, src2.right, src3.bottom);
    Rect src6 = new Rect(src3.left, src3.bottom, src3.right, (int) sourceHeight);
    Rect src7 = new Rect(src6.right, src6.top, src4.right, src6.bottom);
    Rect src8 = new Rect(src7.right, src6.top, src5.right, src6.bottom);

    Rect dest0 = new Rect();
    Rect dest1 = new Rect();
    Rect dest2 = new Rect();
    Rect dest3 = new Rect();
    Rect dest4 = new Rect();
    Rect dest5 = new Rect();
    Rect dest6 = new Rect();
    Rect dest7 = new Rect();
    Rect dest8 = new Rect();

    float finalHeight = availableHeight;
    float finalWidth = availableWidth;
    float offsetStartX = 0;
    float offsetStartY = 0;
    float startW = 0;
    float startH = 0;
    float w_rate = availableWidth / sourceWidth;
    float h_rate = availableHeight / sourceHeight;

    if (scaleType == ScalingUtils.ScaleType.FIT_CENTER) {
      // According to the width and height factor to shrink the image displaying
      // center
      if (w_rate > h_rate) {
        finalWidth = sourceWidth * h_rate;
        startW = (availableWidth - finalWidth) / 2 + offsetStartX;
        startH = offsetStartY;
      } else {
        finalHeight = sourceHeight * w_rate;
        startW = offsetStartX;
        startH = (availableHeight - finalHeight) / 2 + offsetStartY;
      }
    } else if (scaleType == ScalingUtils.ScaleType.CENTER_CROP) {
      // According to the width and height factor to enlarge the image displaying
      // center
      if (w_rate > h_rate) {
        finalHeight = sourceHeight * w_rate;
        startW = offsetStartX;
        startH = (availableHeight - finalHeight) / 2 + offsetStartY;
      } else {
        finalWidth = sourceWidth * h_rate;
        startW = (availableWidth - finalWidth) / 2 + offsetStartX;
        startH = offsetStartY;
      }
    } else if (scaleType == ScalingUtils.ScaleType.CENTER) {
      // same to browser and ios, use source image w/h as dip
      final float sourceSizeScale = PixelUtils.dipToPx(1.0f);
      sourceWidth *= sourceSizeScale;
      sourceHeight *= sourceSizeScale;
      startW = Math.round((availableWidth - sourceWidth) * 0.5f);
      startH = Math.round((availableHeight - sourceHeight) * 0.5f);
      finalWidth = sourceWidth;
      finalHeight = sourceHeight;
    }

    dest0.left = (int) startW;
    dest0.top = (int) startH;
    dest0.right = (int) (startW + PixelUtils.dipToPx(capInsets[3]));
    dest0.bottom = (int) (startH + PixelUtils.dipToPx(capInsets[0]));

    dest1.left = dest0.right;
    dest1.top = dest0.top;
    dest1.right = (int) (startW + finalWidth - PixelUtils.dipToPx(capInsets[1]));
    dest1.bottom = dest0.bottom;

    dest2.left = dest1.right;
    dest2.top = dest0.top;
    dest2.right = (int) startW + (int) finalWidth;
    dest2.bottom = dest0.bottom;

    dest3.left = dest0.left;
    dest3.top = dest0.bottom;
    dest3.right = dest0.right;
    dest3.bottom = (int) (startH + finalHeight - PixelUtils.dipToPx(capInsets[2]));

    dest4.left = dest3.right;
    dest4.top = dest3.top;
    dest4.right = dest1.right;
    dest4.bottom = dest3.bottom;

    dest5.left = dest4.right;
    dest5.top = dest2.bottom;
    dest5.right = dest2.right;
    dest5.bottom = dest3.bottom;

    dest6.left = dest0.left;
    dest6.top = dest3.bottom;
    dest6.right = dest3.right;
    dest6.bottom = (int) startH + (int) finalHeight;

    dest7.left = dest6.right;
    dest7.top = dest4.bottom;
    dest7.right = dest4.right;
    dest7.bottom = dest6.bottom;

    dest8.left = dest7.right;
    dest8.top = dest5.bottom;
    dest8.right = dest5.right;
    dest8.bottom = dest7.bottom;

    Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    canvas.drawBitmap(bitmap, src0, dest0, paint);
    canvas.drawBitmap(bitmap, src1, dest1, paint);
    canvas.drawBitmap(bitmap, src2, dest2, paint);
    canvas.drawBitmap(bitmap, src3, dest3, paint);
    canvas.drawBitmap(bitmap, src4, dest4, paint);
    canvas.drawBitmap(bitmap, src5, dest5, paint);
    canvas.drawBitmap(bitmap, src6, dest6, paint);
    canvas.drawBitmap(bitmap, src7, dest7, paint);
    canvas.drawBitmap(bitmap, src8, dest8, paint);

    TraceEvent.endSection("image.NinePatchHelper.drawWithCapInsets");
  }
}
