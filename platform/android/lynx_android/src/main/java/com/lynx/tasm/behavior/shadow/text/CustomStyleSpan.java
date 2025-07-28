// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import android.os.Build;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import java.util.Objects;

@RequiresApi(Build.VERSION_CODES.P)
public class CustomStyleSpan extends MetricAffectingSpan {
  private final int mStyle;
  private final int mWeight;
  private final String mFontFamily;
  private final String mFontVariationSettings;
  private final String mFontFeatureSettings;
  private final boolean mHasValidTypeface;

  public CustomStyleSpan(int style, int weight, String fontFamily, String fontVariationSettings,
      String fontFeatureSettings, boolean hasValidTypeface) {
    this.mStyle = style;
    this.mWeight = weight;
    this.mFontFamily = fontFamily;
    this.mFontVariationSettings = fontVariationSettings;
    this.mFontFeatureSettings = fontFeatureSettings;
    this.mHasValidTypeface = hasValidTypeface;
  }

  @Override
  public void updateMeasureState(@NonNull TextPaint textPaint) {
    apply(textPaint);
  }

  @Override
  public void updateDrawState(TextPaint tp) {
    apply(tp);
  }

  private void apply(TextPaint textPaint) {
    TextHelper.updateTextPaintTypeFace(textPaint, mFontFamily, mStyle, mWeight,
        mFontVariationSettings, mFontFeatureSettings, mHasValidTypeface);
  }

  @Override
  public boolean equals(Object o) {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    CustomStyleSpan that = (CustomStyleSpan) o;
    return mStyle == that.mStyle && mWeight == that.mWeight;
  }

  @Override
  public int hashCode() {
    return Objects.hash(mStyle, mWeight);
  }

  public int getStyle() {
    return mStyle;
  }
}
