// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Paint;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;

public class InlineTextBaselineShiftSpan extends MetricAffectingSpan {
  protected int mValign;
  protected float mValignLength;
  private int mBaselineShift = 0;
  private BaselineShiftCalculator mBaselineShiftCalculator;
  private float mLineHeight = MeasureUtils.UNDEFINED;

  public void setVerticalAlign(int valign, float length) {
    mValign = valign;
    mValignLength = length;
  }

  public int getVerticalAlign() {
    return mValign;
  }

  public void setBaselineShiftCalculator(BaselineShiftCalculator baselineShiftCalculator) {
    mBaselineShiftCalculator = baselineShiftCalculator;
  }

  public void setLineHeight(float lineHeight) {
    mLineHeight = lineHeight;
  }

  @Override
  public void updateMeasureState(@NonNull TextPaint textPaint) {
    Paint.FontMetricsInt fm = textPaint.getFontMetricsInt();
    if (fm != null && mBaselineShiftCalculator != null) {
      int mCalcAscent = (int) mBaselineShiftCalculator.calcBaselineShiftAscender(
          mValign, mValignLength, fm.ascent, fm.descent);
      mBaselineShift = mCalcAscent - fm.ascent;
      textPaint.baselineShift = mBaselineShift;
    }
  }

  @Override
  public void updateDrawState(TextPaint tp) {
    Paint.FontMetricsInt fm = tp.getFontMetricsInt();
    if (mLineHeight != MeasureUtils.UNDEFINED && fm != null && mBaselineShiftCalculator != null) {
      float maxFontHeight = mBaselineShiftCalculator.getMaxFontDescent()
          - mBaselineShiftCalculator.getMaxFontAscent();
      if (mValign == StyleConstants.VERTICAL_ALIGN_TOP) {
        // move to top of line, equal half of gap plus distance of ascent
        mBaselineShift = -(int) ((Math.ceil(mLineHeight) - maxFontHeight) / 2 + fm.ascent
            - mBaselineShiftCalculator.getMaxFontAscent());
      } else if (mValign == StyleConstants.VERTICAL_ALIGN_BOTTOM) {
        // move to bottom of line, equal half of gap plus distance of descent
        mBaselineShift = (int) ((Math.ceil(mLineHeight) - maxFontHeight) / 2
            + (-fm.descent + mBaselineShiftCalculator.getMaxFontDescent()));
      }
    }
    tp.baselineShift = mBaselineShift;
  }
}
