// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Typeface;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Objects;

public class FontFamilySpan extends MetricAffectingSpan {
  private final @Nullable Typeface mTypeface;

  public FontFamilySpan(@Nullable Typeface typeface) {
    mTypeface = typeface;
  }

  @Override
  public void updateMeasureState(@NonNull TextPaint tp) {
    if (mTypeface != null) {
      tp.setTypeface(mTypeface);
    }
  }

  @Override
  public void updateDrawState(TextPaint tp) {
    if (mTypeface != null) {
      tp.setTypeface(mTypeface);
    }
  }

  @Override
  public boolean equals(Object o) {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    return Objects.equals(mTypeface, ((FontFamilySpan) o).mTypeface);
  }

  @Override
  public int hashCode() {
    return mTypeface == null ? 0 : mTypeface.hashCode();
  }
}
