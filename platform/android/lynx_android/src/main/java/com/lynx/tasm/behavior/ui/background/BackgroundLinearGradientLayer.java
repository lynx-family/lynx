// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.Shader;
import android.os.Build;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;

public class BackgroundLinearGradientLayer extends BackgroundGradientLayer {
  private double mAngle;
  private static final int TOP = 1;
  private static final int BOTTOM = 2;
  private static final int LEFT = 3;
  private static final int RIGHT = 4;
  private static final int TOP_RIGHT = 5;
  private static final int TOP_LEFT = 6;
  private static final int BOTTOM_RIGHT = 7;
  private static final int BOTTOM_LEFT = 8;
  private static final int ANGLE = 9;
  private int mDirectionType;
  // Use bitmap shader to draw linear gradient
  private boolean mEnableBitmapGradient = false;

  public BackgroundLinearGradientLayer(ReadableArray array) {
    if (array == null) {
      LLog.e("LinearGradient", "native parse error array is null");
      return;
    }

    // [angle, colors, stops, directionType]
    if (array.size() < 3) {
      LLog.e("LinearGradient", "native parse error, array.size must be 4  ");
      return;
    }

    mAngle = array.getDouble(0);
    setColorAndStop(array.getArray(1), array.getArray(2));
    // value from old parsed binary dosen't have the last field. default to ANGLE.
    mDirectionType = array.size() == 4 ? array.getInt(3) : ANGLE;
  }

  @Override
  public void setBounds(Rect bounds) {
    mWidth = Math.max(bounds.width(), 1);
    mHeight = Math.max(bounds.height(), 1);
    int left = bounds.left;
    int top = bounds.top;

    if (mColors == null || mColors.length < 2) {
      mShader = null;
    } else if (mPositions != null && mPositions.length != mColors.length) {
      mShader = null;
    } else {
      PointF start = new PointF();
      PointF end = new PointF();

      try {
        final float mul = 2.0f * mWidth * mHeight
            / (mWidth * mWidth
                + mHeight * mHeight); // The diagonals are not directly connected, but instead, a
                                      // perpendicular line is drawn to the other diagonal.
        if (mDirectionType == TOP) { // to top
          start.x = left;
          start.y = top + mHeight;
          end.x = left;
          end.y = top;
        } else if (mDirectionType == BOTTOM) { // to bottom
          start.x = left;
          start.y = top;
          end.x = left;
          end.y = top + mHeight;
        } else if (mDirectionType == LEFT) { // to left
          start.x = left + mWidth;
          start.y = top;
          end.x = left;
          end.y = top;
        } else if (mDirectionType == RIGHT) { // to right
          start.x = left;
          start.y = top;
          end.x = left + mWidth;
          end.y = top;
        } else if (mDirectionType == TOP_RIGHT) {
          start.x = left + mWidth - mHeight * mul;
          start.y = top + mWidth * mul;
          end.x = left + mWidth;
          end.y = top;
        } else if (mDirectionType == TOP_LEFT) {
          start.x = left + mHeight * mul;
          start.y = top + mWidth * mul;
          end.x = left;
          end.y = top;
        } else if (mDirectionType == BOTTOM_RIGHT) {
          start.x = left;
          start.y = top;
          end.x = left + mHeight * mul;
          end.y = top + mWidth * mul;
        } else if (mDirectionType == BOTTOM_LEFT) {
          start.x = left + mWidth;
          start.y = top;
          end.x = left + mWidth - mHeight * mul;
          end.y = top + mWidth * mul;
        } else {
          PointF center = new PointF(mWidth / 2.f, mHeight / 2.f), m;
          final double radial = Math.toRadians(mAngle);
          float sin = (float) Math.sin(radial), cos = (float) Math.cos(radial),
                tan = (float) Math.tan(radial);
          if (sin >= 0 && cos >= 0) { // Bottom left to top right
            m = new PointF(mWidth, 0);
          } else if (sin >= 0 && cos < 0) { // Top left to bottom right
            m = new PointF(mWidth, mHeight);
          } else if (sin < 0 && cos < 0) { // Top right to bottom left
            m = new PointF(0, mHeight);
          } else { // Bottom right to top left
            m = new PointF(0, 0);
          }
          start.offset(left, top);
          end.offset(left, top);
          center.offset(left, top);
          m.offset(left, top);
          // reference: https://developer.mozilla.org/zh-CN/docs/Web/CSS/linear-gradient
          // It can be solved using pen and paper.
          float tmp = (center.y - m.y - tan * center.x + tan * m.x);
          end.x = center.x + sin * tmp / (sin * tan + cos);
          end.y = center.y - tmp / (tan * tan + 1);
          start.x = 2 * center.x - end.x;
          start.y = 2 * center.y - end.y;
        }
        // The version number should lower than 9.0
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P && mEnableBitmapGradient) {
          final String eventName = "createBitmapShader";
          TraceEvent.beginSection(eventName);
          createBitmapShader(start, end, mColors, mPositions, (float) mAngle);
          TraceEvent.endSection(eventName);
        } else {
          mShader = new LinearGradient(
              start.x, start.y, end.x, end.y, mColors, mPositions, Shader.TileMode.CLAMP);
        }
      } catch (Exception e) {
        mShader = null;
        mPaint.setColor(mColors[0]);
        e.printStackTrace();
        LLog.w("BackgroundLinearGradientLayer", "exception:\n" + e);
      }
    }
    super.setBounds(bounds);
  }

  public void setEnableBitmapGradient(boolean enable) {
    mEnableBitmapGradient = enable;
  }

  private static class FloatColor {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;
    void set(int color) {
      a = ((color >> 24) & 0xff) / 255.0f;
      r = ((color >> 16) & 0xff) / 255.0f;
      g = ((color >> 8) & 0xff) / 255.0f;
      b = ((color) & 0xff) / 255.0f;
    }
    void set(FloatColor color) {
      a = color.a;
      r = color.r;
      g = color.g;
      b = color.b;
    }
  }

  private static void mix(FloatColor start, FloatColor end, float amount, int index, int[] dst) {
    float oppAmount = 1.0f - amount;
    int r = (int) ((start.r * oppAmount + end.r * amount) * 255.0f);
    int g = (int) ((start.g * oppAmount + end.g * amount) * 255.0f);
    int b = (int) ((start.b * oppAmount + end.b * amount) * 255.0f);
    int a = (int) ((start.a * oppAmount + end.a * amount) * 255.0f);
    dst[index] = (a << 24 | r << 16 | g << 8 | b);
  }

  private static void fillPixels(int[] colors, float[] positions, int width, int[] output) {
    FloatColor start = new FloatColor();
    start.set(colors[0]);

    FloatColor end = new FloatColor();
    end.set(colors[1]);

    int currentPos = 1;
    float startPos = positions[0];
    float distance = positions[1] - startPos;
    for (int x = 0; x < width; x++) {
      float pos = x / ((float) width - 1);
      if (pos > positions[currentPos]) {
        start.set(end);
        startPos = positions[currentPos];
        currentPos++;
        end.set(colors[currentPos]);
        distance = positions[currentPos] - startPos;
      }

      float amount = (pos - startPos) / distance;
      mix(start, end, amount, x, output);
    }
  }

  private void createBitmapShader(
      PointF start, PointF end, int[] colors, float[] pos, float angle) {
    int length = (int) PointF.length(end.x - start.x, end.y - start.y);
    if (length <= 0) {
      mShader = null;
      return;
    }
    int[] buffer = new int[length];
    if (pos == null) {
      pos = new float[colors.length];
      if (colors.length == 2) {
        pos[0] = 0;
        pos[1] = 1;
      } else if (colors.length > 2) {
        for (int i = 0; i < colors.length; i++) {
          pos[i] = i / (float) (colors.length - 1);
        }
      }
    }
    // Check if need to add in dummy start and/or end position/colors
    boolean dummyFirst = pos[0] != 0;
    boolean dummyLast = pos[pos.length - 1] != 1;
    int count = pos.length + (dummyFirst ? 1 : 0) + (dummyLast ? 1 : 0);
    if (count != pos.length) {
      int[] colorList = new int[count];
      float[] posList = new float[count];
      if (dummyFirst) {
        colorList[0] = colors[0];
        posList[0] = 0.f;
      }
      // Copy the original position/colors
      System.arraycopy(colors, 0, colorList, dummyFirst ? 1 : 0, colors.length);
      System.arraycopy(pos, 0, posList, dummyFirst ? 1 : 0, pos.length);
      if (dummyLast) {
        colorList[count - 1] = colors[colors.length - 1];
        posList[count - 1] = 1.f;
      }
      fillPixels(colorList, posList, length, buffer);
    } else {
      fillPixels(colors, pos, length, buffer);
    }
    Bitmap bitmap = Bitmap.createBitmap(buffer, length, 1, Bitmap.Config.ARGB_8888);
    mShader = new BitmapShader(bitmap, Shader.TileMode.CLAMP, Shader.TileMode.REPEAT);
    Matrix matrix = new Matrix();
    matrix.postRotate(angle + 270);
    matrix.postTranslate(start.x, start.y);
    mShader.setLocalMatrix(matrix);
  }
}
