// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import static com.lynx.tasm.behavior.StyleConstants.*;

import android.graphics.Path;
import android.graphics.Typeface;
import android.os.Build;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.style.MetricAffectingSpan;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import java.util.Objects;
import java.util.Optional;

@RequiresApi(Build.VERSION_CODES.P)
public class CustomStyleSpan extends MetricAffectingSpan {
  private final int style;
  private final int weight;

  private String mFontFamily;

  private Optional<Float> mRawTextSkewX = Optional.empty();

  public CustomStyleSpan(int style, int weight, String fontFamily) {
    this.style = style;
    this.weight = weight;
    this.mFontFamily = fontFamily;
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
    Typeface originTypeface = textPaint.getTypeface();
    Typeface newTypeface =
        Typeface.create(originTypeface, getStyleWeight(), style == Typeface.ITALIC);
    textPaint.setTypeface(newTypeface);

    if (style > 0 && TextUtils.isEmpty(mFontFamily)) {
      // no font-family set, do fake bold&italic when needed
      int typefaceStyle = newTypeface != null ? newTypeface.getStyle() : 0;
      int need = style & ~typefaceStyle;
      textPaint.setFakeBoldText(((need & Typeface.BOLD) != 0) && (weight == FONTWEIGHT_BOLD));
      if ((need & Typeface.ITALIC) != 0) {
        if (!mRawTextSkewX.isPresent()) {
          mRawTextSkewX = Optional.of(textPaint.getTextSkewX());
        }
        textPaint.setTextSkewX(-0.25f);
      }
    } else {
      textPaint.setFakeBoldText(false);
      if (mRawTextSkewX.isPresent()) {
        textPaint.setTextSkewX(mRawTextSkewX.get());
      }
    }
  }

  private int getStyleWeight() {
    if (weight == FONTWEIGHT_BOLD) {
      return 700;
    } else if (weight == FONTWEIGHT_NORMAL) {
      return 400;
    } else {
      return (weight - 1) * 100;
    }
  }

  @Override
  public boolean equals(Object o) {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    CustomStyleSpan that = (CustomStyleSpan) o;
    return style == that.style && weight == that.weight;
  }

  @Override
  public int hashCode() {
    return Objects.hash(style, weight);
  }

  public int getStyle() {
    return style;
  }
}
