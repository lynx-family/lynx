// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.text.Spannable;
import android.text.style.ReplacementSpan;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class CustomBaselineShiftSpan extends ReplacementSpan {
  final private int mTextVerticalAlign;
  final int mStart;
  final int mEnd;
  final float mShift;

  public CustomBaselineShiftSpan(int start, int end, int textVerticalAlign, float shift) {
    super();
    this.mStart = start;
    this.mEnd = end;
    this.mTextVerticalAlign = textVerticalAlign;
    this.mShift = shift;
  }

  @Override
  public int getSize(@NonNull Paint paint, CharSequence text, int start, int end,
      @Nullable Paint.FontMetricsInt fm) {
    return Math.round(paint.measureText(text, start, end));
  }

  @Override
  public void draw(@NonNull Canvas canvas, CharSequence text, int start, int end, float x, int top,
      int y, int bottom, @NonNull Paint paint) {
    Paint.FontMetrics fm = paint.getFontMetrics();

    if (text instanceof Spannable) {
      drawBackgroundIfNeed(canvas, (Spannable) text, start, end, x, top, y, bottom, paint);
    }

    if (mTextVerticalAlign == TextAttributes.TEXT_VERTICAL_ALIGN_TOP) {
      y = -Math.round(fm.top);
    } else if (mTextVerticalAlign == TextAttributes.TEXT_VERTICAL_ALIGN_BOTTOM) {
      y = bottom - Math.round(fm.leading);
    } else if (mTextVerticalAlign == TextAttributes.TEXT_VERTICAL_ALIGN_CENTER) {
      // always calculate normal baseline position
      // in case some old android framework may not update it
      y = top + Math.round(Math.abs(fm.ascent));
      float offset = (bottom - top - (fm.descent - fm.ascent)) / 2.f;
      y += offset;
      if (fm.leading == 0.f) {
        float strokeWidth = paint.getStrokeMiter();
        y += (fm.descent) / 2.f - strokeWidth;
      }
    } else if (mTextVerticalAlign == TextAttributes.TEXT_VERTICAL_ALIGN_CUSTOM) {
      y += mShift;
    }

    canvas.drawText(text, start, end, x, y, paint);
  }

  private void drawBackgroundIfNeed(Canvas canvas, Spannable text, int start, int end, float x,
      int top, int y, int bottom, Paint paint) {
    BackgroundColorSpan[] spans = text.getSpans(start, end, BackgroundColorSpan.class);
    if (spans == null || spans.length == 0) {
      return;
    }

    Paint bg = new Paint();
    bg.setStyle(Paint.Style.FILL);
    bg.setColor(spans[0].getBackgroundColor());

    float width = paint.measureText(text, start, end);
    Rect rect = new Rect(Math.round(x), top, Math.round(x + width), bottom);
    rect.offset(0, Math.round(mShift));

    canvas.drawRect(rect, bg);
  }
}
