// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.StaticLayout;
import android.text.TextPaint;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import org.junit.Assert;
import org.junit.Test;

public class CustomLineHeightSpanTest {
  @Test
  public void textChooseHeight() {
    Layout normalLayout = buildTextLayout(StyleConstants.VERTICAL_ALIGN_DEFAULT);
    Layout topLayout = buildTextLayout(StyleConstants.VERTICAL_ALIGN_TOP);
    Layout bottomLayout = buildTextLayout(StyleConstants.VERTICAL_ALIGN_BOTTOM);

    Assert.assertTrue("ascent will be smaller after set text-single-line-vertical-align as top",
        Math.abs(topLayout.getLineAscent(0)) < Math.abs(normalLayout.getLineAscent(0))
            && Math.abs(topLayout.getLineDescent(0)) > Math.abs(normalLayout.getLineDescent(0)));
    Assert.assertTrue("ascent will be longer after set text-single-line-vertical-align as bottom",
        Math.abs(bottomLayout.getLineAscent(0)) > Math.abs(normalLayout.getLineAscent(0))
            && Math.abs(bottomLayout.getLineDescent(0)) < Math.abs(normalLayout.getLineDescent(0)));
  }

  private Layout buildTextLayout(int textSingleLineVerticalAlign) {
    SpannableStringBuilder span = new SpannableStringBuilder("test text.");
    span.setSpan(
        new CustomLineHeightSpan(MeasureUtils.UNDEFINED, true, textSingleLineVerticalAlign, false),
        0, span.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

    return new StaticLayout(span, new TextPaint(), 200, Layout.Alignment.ALIGN_NORMAL, 1, 0, false);
  }
}
