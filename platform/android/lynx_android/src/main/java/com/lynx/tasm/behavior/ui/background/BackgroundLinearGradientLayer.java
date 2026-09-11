// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.LinearGradient;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.Shader;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;

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
        mShader = new LinearGradient(
            start.x, start.y, end.x, end.y, mColors, mPositions, Shader.TileMode.CLAMP);
      } catch (Exception e) {
        mShader = null;
        mPaint.setColor(mColors[0]);
        e.printStackTrace();
        LLog.w("BackgroundLinearGradientLayer", "exception:\n" + e);
      }
    }
    super.setBounds(bounds);
  }
}
