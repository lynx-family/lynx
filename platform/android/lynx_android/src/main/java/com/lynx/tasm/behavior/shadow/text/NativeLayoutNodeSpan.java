// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Canvas;
import android.graphics.Paint;
import com.lynx.tasm.behavior.StyleConstants;

public class NativeLayoutNodeSpan extends AbsBaselineShiftCalculatorSpan {
  private int mHeight;
  private int mWidth;
  private int mBaseline;

  public NativeLayoutNodeSpan() {
    if (mEnableTextRefactor) {
      mValign = StyleConstants.VERTICAL_ALIGN_BASELINE;
    } else {
      mValign = StyleConstants.VERTICAL_ALIGN_DEFAULT;
    }
    mValignLength = 0.0f;
  }

  public int getWidth() {
    return mWidth;
  }

  public void updateLayoutNodeSize(int width, int height, int baseline) {
    mWidth = width;
    mHeight = height;
    mBaseline = baseline;
  }

  @Override
  public int getSize(Paint paint, CharSequence text, int start, int end, Paint.FontMetricsInt fm) {
    if (fm != null) {
      if (fm.descent == fm.ascent) {
        fm.ascent = paint.getFontMetricsInt().ascent;
        fm.descent = paint.getFontMetricsInt().descent;
      }
      if (mEnableTextRefactor) {
        if (getVerticalAlign() == StyleConstants.VERTICAL_ALIGN_BASELINE && mBaseline != 0) {
          mCalcAscent = -mBaseline;
        } else {
          mCalcAscent = (int) calcBaselineShiftAscender(-mHeight, 0);
        }
      } else {
        mCalcAscent = caYOffset(fm);
      }
      if (fm.ascent > mCalcAscent) {
        fm.ascent = mCalcAscent;
      }
      if (fm.descent < mCalcAscent + mHeight) {
        fm.descent = mCalcAscent + mHeight;
      }
      if (fm.top > fm.ascent) {
        fm.top = fm.ascent;
      }
      if (fm.bottom < fm.descent) {
        fm.bottom = fm.descent;
      }
    }
    return mWidth;
  }

  @Override
  public void draw(Canvas canvas, CharSequence text, int start, int end, float x, int top, int y,
      int bottom, Paint paint) {}

  private int caYOffset(Paint.FontMetricsInt fm) {
    final int lineHeight = fm.descent - fm.ascent;
    final int height = mHeight;
    int yOffset = 0;
    switch (mValign) {
      case StyleConstants.VERTICAL_ALIGN_BASELINE:
        yOffset = -height;
        break;
      case StyleConstants.VERTICAL_ALIGN_SUPER:
        yOffset = fm.ascent + (int) (lineHeight * 0.1f);
        break;
      case StyleConstants.VERTICAL_ALIGN_TOP:
      case StyleConstants.VERTICAL_ALIGN_TEXT_TOP:
        yOffset = fm.ascent;
        break;
      case StyleConstants.VERTICAL_ALIGN_SUB:
        yOffset = fm.descent - height - (int) (lineHeight * 0.1f);
        break;
      case StyleConstants.VERTICAL_ALIGN_BOTTOM:
      case StyleConstants.VERTICAL_ALIGN_TEXT_BOTTOM:
        yOffset = fm.descent - height;
        break;
      case StyleConstants.VERTICAL_ALIGN_LENGTH:
        yOffset = -height - (int) mValignLength;
        break;
      case StyleConstants.VERTICAL_ALIGN_MIDDLE:
      default:
        yOffset = fm.ascent + (lineHeight - height) / 2;
        break;
    }
    return yOffset;
  }

  // For text align.Return inline-view top position
  // line-height = bottom - top = descent - ascent
  public int getYOffset(int top, int bottom, int ascent, int descent) {
    if (!mEnableTextRefactor) {
      return bottom - descent - mHeight;
    }

    int yPos = 0;
    switch (mValign) {
      case StyleConstants.VERTICAL_ALIGN_TOP:
        yPos = top;
        break;
      case StyleConstants.VERTICAL_ALIGN_BOTTOM:
        yPos = bottom - mHeight;
        break;
      case StyleConstants.VERTICAL_ALIGN_CENTER:
        yPos = (bottom + top - mHeight) / 2;
        break;
      default:
        yPos = top - ascent + mCalcAscent;
        break;
    }
    return yPos;
  }

  @Override
  protected int getIncludeMarginHeight() {
    return mHeight;
  }
}
