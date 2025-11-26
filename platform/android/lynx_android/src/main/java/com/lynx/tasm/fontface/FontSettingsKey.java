// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fontface;

import android.text.TextUtils;

public final class FontSettingsKey {
  private final String variation;
  private final float textSize;
  private final String fontFamily;

  public FontSettingsKey(String variation, float textSize, String fontFamily) {
    this.variation = variation != null ? variation : "";
    this.textSize = textSize;
    this.fontFamily = fontFamily;
  }

  @Override
  public boolean equals(Object o) {
    if (o == this)
      return true;
    if (!(o instanceof FontSettingsKey))
      return false;
    FontSettingsKey k = (FontSettingsKey) o;
    return TextUtils.equals(fontFamily, k.fontFamily) && Float.compare(textSize, k.textSize) == 0
        && TextUtils.equals(variation, k.variation);
  }

  @Override
  public int hashCode() {
    int h = variation.hashCode();
    h = 31 * h + Float.floatToIntBits(textSize);
    h = 31 * h + (fontFamily != null ? fontFamily.hashCode() : 0);
    return h;
  }
}
