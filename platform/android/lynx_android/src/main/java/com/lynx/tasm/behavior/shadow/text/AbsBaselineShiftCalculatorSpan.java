// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;
import android.graphics.Paint;
import android.text.style.ReplacementSpan;
import com.lynx.tasm.behavior.StyleConstants;
import java.util.List;

public abstract class AbsBaselineShiftCalculatorSpan extends ReplacementSpan {
  protected int mValign;
  protected float mValignLength;
  protected int mCalcAscent = 0;
  private BaselineShiftCalculator mBaselineShiftCalculator;
  protected boolean mEnableTextRefactor = false;

  public void setVerticalAlign(int valign, float length) {
    mValign = valign;
    mValignLength = length;
  }

  public int getVerticalAlign() {
    return mValign;
  }

  public void setEnableTextRefactor(boolean enableTextRefactor) {
    mEnableTextRefactor = enableTextRefactor;
  }

  public void setBaselineShiftCalculator(BaselineShiftCalculator baselineShiftCalculator) {
    mBaselineShiftCalculator = baselineShiftCalculator;
  }

  public float calcBaselineShiftAscender(float ascender, float descender) {
    if (mBaselineShiftCalculator != null) {
      return mBaselineShiftCalculator.calcBaselineShiftAscender(
          mValign, mValignLength, ascender, descender);
    }
    return ascender;
  }

  abstract protected int getIncludeMarginHeight();

  // For custom lineHeight.
  public void AdjustFontMetrics(Paint.FontMetricsInt fm) {
    if (fm.ascent > mCalcAscent) {
      fm.ascent = mCalcAscent;
    }
    if (fm.top > mCalcAscent) {
      fm.top = mCalcAscent;
    }

    int inlineElementHeightUnderBaseline = mCalcAscent + getIncludeMarginHeight();
    if (fm.descent < inlineElementHeightUnderBaseline) {
      fm.descent = inlineElementHeightUnderBaseline;
    }
    if (fm.bottom < inlineElementHeightUnderBaseline) {
      fm.bottom = inlineElementHeightUnderBaseline;
    }
  }
}
