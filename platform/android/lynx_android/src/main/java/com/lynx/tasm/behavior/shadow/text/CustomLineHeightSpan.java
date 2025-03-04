/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Build;
import android.text.SpannableStringBuilder;
import android.text.TextPaint;
import android.text.style.LineHeightSpan;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;

/**
 * We use a custom {@link LineHeightSpan}, because `lineSpacingExtra` is broken. Details here:
 * https://github.com/facebook/react-native/issues/7546
 */
public class CustomLineHeightSpan implements LineHeightSpan.WithDensity {
  private final int mHeight;
  private final boolean mEnableTextRefactor;
  private final int mTextSingleLineVerticalAlign;
  private final boolean mIsSingLineAndOverflowClip;

  public CustomLineHeightSpan(float height, boolean enableTextRefactor,
      int textSingleLineVerticalAlign, boolean isSingLineAndOverflowClip) {
    if (height == MeasureUtils.UNDEFINED) {
      mHeight = 0;
    } else {
      mHeight = (int) Math.ceil(height);
    }
    mEnableTextRefactor = enableTextRefactor;
    mTextSingleLineVerticalAlign = textSingleLineVerticalAlign;
    mIsSingLineAndOverflowClip = isSingLineAndOverflowClip;
  }

  @Override
  public boolean equals(Object o) {
    if (o instanceof CustomLineHeightSpan) {
      CustomLineHeightSpan that = (CustomLineHeightSpan) o;
      return mHeight == that.mHeight;
    }
    return false;
  }

  @Override
  public int hashCode() {
    return 31 + mHeight;
  }

  @Override
  public void chooseHeight(CharSequence text, int start, int end, int spanstartv, int lineHeight,
      Paint.FontMetricsInt fm, TextPaint paint) {
    if (mEnableTextRefactor) {
      Rect bounds = null;
      // The textLayout may not be a single line when text-overflow is clip and restricting a single
      // line
      if (mTextSingleLineVerticalAlign != StyleConstants.VERTICAL_ALIGN_DEFAULT
          && (text.length() == end - start || mIsSingLineAndOverflowClip)) {
        bounds = new Rect();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
          paint.getTextBounds(text, 0, text.toString().length(), bounds);
        } else {
          paint.getTextBounds(text.toString(), 0, text.toString().length(), bounds);
        }
      }

      if (mHeight != 0 && bounds == null) {
        TextHelper.calcTextTranslateTopOffsetAndAdjustFontMetric(mHeight, fm, false);

        SpannableStringBuilder builder = (SpannableStringBuilder) text;
        AbsBaselineShiftCalculatorSpan[] baselineShiftCalculatorSpans =
            builder.getSpans(start, end, AbsBaselineShiftCalculatorSpan.class);
        for (AbsBaselineShiftCalculatorSpan span : baselineShiftCalculatorSpans) {
          span.AdjustFontMetrics(fm);
        }
      } else if (bounds != null) {
        int actualLineHeight = mHeight == 0 ? fm.bottom - fm.top : mHeight;
        if (mTextSingleLineVerticalAlign == StyleConstants.VERTICAL_ALIGN_TOP) {
          fm.top = fm.ascent = bounds.top;
          fm.bottom = fm.descent = actualLineHeight + fm.top;
        } else if (mTextSingleLineVerticalAlign == StyleConstants.VERTICAL_ALIGN_BOTTOM) {
          fm.bottom = fm.descent = bounds.bottom;
          fm.top = fm.ascent = fm.bottom - actualLineHeight;
        } else if (mTextSingleLineVerticalAlign == StyleConstants.VERTICAL_ALIGN_CENTER) {
          int halfPadding = (actualLineHeight - bounds.height()) / 2;
          fm.ascent = fm.top = bounds.top - halfPadding;
          fm.descent = fm.bottom = fm.top + actualLineHeight;
        }
      }
    } else if (mHeight != 0) {
      // This is more complicated that I wanted it to be. You can find a good explanation of what
      // the FontMetrics mean here: http://stackoverflow.com/questions/27631736. The general
      // solution is that if there's not enough height to show the full line height, we will
      // prioritize in this order: descent, ascent, bottom, top
      if (fm.descent > mHeight) {
        // Show as much descent as possible
        fm.bottom = fm.descent = Math.min(mHeight, fm.descent);
        fm.top = fm.ascent = 0;
      } else if (-fm.ascent + fm.descent > mHeight) {
        // Show all descent, and as much ascent as possible
        fm.bottom = fm.descent;
        fm.top = fm.ascent = -mHeight + fm.descent;
      } else if (-fm.ascent + fm.bottom > mHeight) {
        // Show all ascent, descent, as much bottom as possible
        fm.top = fm.ascent;
        fm.bottom = fm.ascent + mHeight;
      } else if (-fm.top + fm.bottom > mHeight) {
        // Show all ascent, descent, bottom, as much top as possible
        fm.top = fm.bottom - mHeight;
      } else {
        // Show proportionally additional ascent / top & descent / bottom
        final int additional = mHeight - (-fm.top + fm.bottom);
        final int delta = Math.round(additional / 2.0f);
        final int baseLineHeight = fm.descent - fm.ascent;
        final int baseLineDelta = Math.round((mHeight - baseLineHeight) / 2.0f);

        // if custom line height is taller than the metrics height
        // we need to translate the metrics not scale it
        fm.top -= delta;
        fm.bottom += delta;
        fm.ascent -= baseLineDelta;
        fm.descent += baseLineDelta;
      }
    }
  }

  @Override
  public void chooseHeight(CharSequence text, int start, int end, int spanstartv, int lineHeight,
      Paint.FontMetricsInt fm) {}
}
