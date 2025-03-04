// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Color;
import android.graphics.Paint;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import androidx.annotation.NonNull;

public class ForegroundColorSpan extends MetricAffectingSpan {
  final private int mColor;
  private int mStrokeColor;
  private float mStrokeWidth;
  private boolean mDrawStroke = false;
  public ForegroundColorSpan(int color) {
    super();
    mColor = color;
  }

  @Override
  public boolean equals(Object o) {
    if (o instanceof ForegroundColorSpan) {
      ForegroundColorSpan span = (ForegroundColorSpan) o;
      return getForegroundColor() == span.getForegroundColor();
    }
    return false;
  }

  public int getForegroundColor() {
    return mColor;
  }

  public void setStrokeColor(int color) {
    mStrokeColor = color;
  }

  public void setStrokeWidth(float width) {
    mStrokeWidth = width;
  }

  public void setDrawStroke(boolean enable) {
    mDrawStroke = enable;
  }

  @Override
  public int hashCode() {
    return 31 + getForegroundColor();
  }

  @Override
  public void updateMeasureState(@NonNull TextPaint textPaint) {}

  @Override
  public void updateDrawState(TextPaint tp) {
    if (!mDrawStroke) {
      tp.setStyle(Paint.Style.FILL);
      tp.setColor(mColor);
    } else {
      tp.setStyle(Paint.Style.STROKE);
      tp.setStrokeWidth(mStrokeWidth);
      tp.setColor(mStrokeColor);
      tp.bgColor = Color.TRANSPARENT;
    }
  }
}
