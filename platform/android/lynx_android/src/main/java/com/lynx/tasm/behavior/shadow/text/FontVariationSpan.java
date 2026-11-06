// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.os.Build;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

@RequiresApi(Build.VERSION_CODES.O)
final class FontVariationSpan extends MetricAffectingSpan {
  private final String mFontVariationSettings;

  FontVariationSpan(@NonNull String fontVariationSettings) {
    mFontVariationSettings = fontVariationSettings;
  }

  @Override
  public void updateMeasureState(@NonNull TextPaint textPaint) {
    if (TextHelper.enableSetFontVariation()) {
      textPaint.setFontVariationSettings(mFontVariationSettings);
    }
  }

  @Override
  public void updateDrawState(TextPaint textPaint) {
    updateMeasureState(textPaint);
  }

  @Override
  public boolean equals(Object o) {
    return o instanceof FontVariationSpan
        && mFontVariationSettings.equals(((FontVariationSpan) o).mFontVariationSettings);
  }

  @Override
  public int hashCode() {
    return mFontVariationSettings.hashCode();
  }
}
