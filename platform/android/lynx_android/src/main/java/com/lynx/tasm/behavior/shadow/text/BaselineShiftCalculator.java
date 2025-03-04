// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import com.lynx.tasm.behavior.StyleConstants;
import java.util.List;

public class BaselineShiftCalculator {
  // ascender,descender,x-height,line-height
  private List<Float> mTextFontMetricForVerticalAlign;

  public BaselineShiftCalculator(List<Float> textFontMetricForVerticalAlign) {
    mTextFontMetricForVerticalAlign = textFontMetricForVerticalAlign;
  }

  public float calcBaselineShiftAscender(
      int verticalAlign, float verticalAlignValue, float ascender, float descender) {
    float height = descender - ascender;
    float baselineShift = 0.f;
    switch (verticalAlign) {
      case StyleConstants.VERTICAL_ALIGN_LENGTH:
        baselineShift = verticalAlignValue;
        break;
      case StyleConstants.VERTICAL_ALIGN_PERCENT:
        // if set vertical-align:50%, baselineShift = 50 * lineHeight /100.f, the lineHeight is 0 if
        // lineHeight not set.
        baselineShift = verticalAlignValue * mTextFontMetricForVerticalAlign.get(3) / 100.f;
        break;
      case StyleConstants.VERTICAL_ALIGN_MIDDLE:
        // x-height is positive
        // the middle of element will be align to the middle of max x-height
        baselineShift = (descender + ascender + mTextFontMetricForVerticalAlign.get(2)) * 0.5f;
        break;
      case StyleConstants.VERTICAL_ALIGN_TEXT_TOP:
      case StyleConstants.VERTICAL_ALIGN_TOP:
        // the ascender of element will be align to text max ascender
        baselineShift = ascender - mTextFontMetricForVerticalAlign.get(0);
        break;
      case StyleConstants.VERTICAL_ALIGN_TEXT_BOTTOM:
      case StyleConstants.VERTICAL_ALIGN_BOTTOM:
        // the descender of element will be align to text max descender
        baselineShift = descender - mTextFontMetricForVerticalAlign.get(1);
        break;
      case StyleConstants.VERTICAL_ALIGN_SUB:
        baselineShift = -height * 0.1f;
        break;
      case StyleConstants.VERTICAL_ALIGN_SUPER:
        baselineShift = height * 0.1f;
        break;
      case StyleConstants.VERTICAL_ALIGN_CENTER:
        baselineShift = (-mTextFontMetricForVerticalAlign.get(0)
                            - mTextFontMetricForVerticalAlign.get(1) + ascender + descender)
            * 0.5f;
        break;
      default:
        baselineShift = 0;
        break;
    }
    return -baselineShift + ascender;
  }

  public float getMaxFontDescent() {
    return mTextFontMetricForVerticalAlign.get(1);
  }

  public float getMaxFontAscent() {
    return mTextFontMetricForVerticalAlign.get(0);
  }
}
