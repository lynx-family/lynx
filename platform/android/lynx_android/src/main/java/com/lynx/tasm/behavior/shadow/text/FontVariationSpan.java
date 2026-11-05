// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Paint;
import android.graphics.Typeface;
import android.os.Build;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import java.util.Objects;

@RequiresApi(Build.VERSION_CODES.O)
final class FontVariationSpan extends MetricAffectingSpan {
  private final String mFontVariationSettings;
  private String mInheritedFontVariationSettings;
  private volatile TypefaceEntry mTypefaceEntry;

  private static final class TypefaceEntry {
    final Typeface input;
    final String inheritedSettings;
    final Typeface result;

    TypefaceEntry(Typeface input, String inheritedSettings, Typeface result) {
      this.input = input;
      this.inheritedSettings = inheritedSettings;
      this.result = result;
    }
  }

  FontVariationSpan(@NonNull String fontVariationSettings) {
    mFontVariationSettings = fontVariationSettings;
  }

  String getFontVariationSettingsForReset() {
    if (mInheritedFontVariationSettings == null || mInheritedFontVariationSettings.isEmpty()) {
      return mFontVariationSettings;
    }
    return mFontVariationSettings.isEmpty()
        ? mInheritedFontVariationSettings
        : mInheritedFontVariationSettings + "," + mFontVariationSettings;
  }

  // Assigned before installation, while the enclosing inline ranges are available.
  void setInheritedFontVariationSettings(String settings) {
    mInheritedFontVariationSettings = settings;
  }

  @Override
  public void updateMeasureState(@NonNull TextPaint textPaint) {
    if (!TextHelper.enableSetFontVariation()) {
      return;
    }
    Typeface input = textPaint.getTypeface();
    String inheritedSettings = mFontVariationSettings.isEmpty()
        ? (mInheritedFontVariationSettings != null ? mInheritedFontVariationSettings
                                                   : textPaint.getFontVariationSettings())
        : null;
    TypefaceEntry entry = mTypefaceEntry;
    if (entry == null || entry.input != input
        || !Objects.equals(entry.inheritedSettings, inheritedSettings)) {
      Paint fontPaint = new Paint();
      fontPaint.setTypeface(input);
      // A cached typeface does not carry Paint's variation string. Seed it for reset,
      // otherwise setFontVariationSettings("") would incorrectly take the no-op path.
      // Keep ancestor axes too: an unsupported child axis does not clear a parent's axes.
      if (inheritedSettings != null && !inheritedSettings.isEmpty()) {
        fontPaint.setFontVariationSettings(inheritedSettings);
      }
      fontPaint.setFontVariationSettings(mFontVariationSettings);
      entry = new TypefaceEntry(input, inheritedSettings, fontPaint.getTypeface());
      mTypefaceEntry = entry;
    }
    if (textPaint.getTypeface() != entry.result) {
      textPaint.setTypeface(entry.result);
    }
  }

  @Override
  public void updateDrawState(TextPaint textPaint) {
    updateMeasureState(textPaint);
  }

  @Override
  public boolean equals(Object o) {
    return o instanceof FontVariationSpan
        && mFontVariationSettings.equals(((FontVariationSpan) o).mFontVariationSettings)
        && Objects.equals(mInheritedFontVariationSettings,
            ((FontVariationSpan) o).mInheritedFontVariationSettings);
  }

  @Override
  public int hashCode() {
    return 31 * mFontVariationSettings.hashCode()
        + Objects.hashCode(mInheritedFontVariationSettings);
  }
}
