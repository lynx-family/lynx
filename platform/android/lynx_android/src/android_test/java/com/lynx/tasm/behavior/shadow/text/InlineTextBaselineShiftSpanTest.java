// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;

import android.graphics.Paint;
import android.text.TextPaint;
import com.lynx.tasm.behavior.StyleConstants;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

public class InlineTextBaselineShiftSpanTest {
  @org.junit.Rule public MockitoRule mockitoRule = MockitoJUnit.rule();

  @Mock private BaselineShiftCalculator baselineShiftCalculator;

  @Test
  public void testVerticalAlign() {
    InlineTextBaselineShiftSpan span = new InlineTextBaselineShiftSpan();
    int valign = StyleConstants.VERTICAL_ALIGN_TOP;
    float length = 5.0f;

    span.setVerticalAlign(valign, length);

    assertEquals(valign, span.getVerticalAlign());
  }

  @Test
  public void testUpdateMeasureState() {
    InlineTextBaselineShiftSpan span = new InlineTextBaselineShiftSpan();
    span.setBaselineShiftCalculator(baselineShiftCalculator);

    int valign = StyleConstants.VERTICAL_ALIGN_TOP;
    float length = 5.0f;
    span.setVerticalAlign(valign, length);

    TextPaint textPaint = new TextPaint(TextPaint.ANTI_ALIAS_FLAG);
    textPaint.setTextSize(20.0f);

    Paint.FontMetricsInt fm = textPaint.getFontMetricsInt();
    int expectedBaselineShift = (int) (baselineShiftCalculator.calcBaselineShiftAscender(
                                    valign, length, fm.ascent, fm.descent))
        - fm.ascent;

    span.updateMeasureState(textPaint);

    assertEquals(expectedBaselineShift, textPaint.baselineShift);
  }

  @Test
  public void testUpdateDrawState() {
    InlineTextBaselineShiftSpan span = new InlineTextBaselineShiftSpan();
    span.setBaselineShiftCalculator(baselineShiftCalculator);

    int valign = StyleConstants.VERTICAL_ALIGN_TOP;
    float length = 5.0f;
    span.setVerticalAlign(valign, length);

    TextPaint textPaint = new TextPaint(TextPaint.ANTI_ALIAS_FLAG);
    textPaint.setTextSize(20.0f);

    span.setLineHeight((float) Math.ceil(50.0));

    span.updateDrawState(textPaint);
    int expectedBaselineShift = -6;
    assertEquals(expectedBaselineShift, textPaint.baselineShift);

    valign = StyleConstants.VERTICAL_ALIGN_BOTTOM;
    span.setVerticalAlign(valign, length);
    span.updateDrawState(textPaint);
    expectedBaselineShift = 20;
    assertEquals(expectedBaselineShift, textPaint.baselineShift);
  }
}
