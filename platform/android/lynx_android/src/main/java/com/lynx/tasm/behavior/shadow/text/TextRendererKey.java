// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.text.Spanned;
import com.lynx.tasm.behavior.shadow.MeasureMode;

public class TextRendererKey {
  public static class BaseKey {
    CharSequence mText;
    final TextAttributes mAttributes;

    BaseKey(CharSequence text, TextAttributes attributes) {
      this.mText = text;
      this.mAttributes = attributes;
    }

    @Override
    public boolean equals(Object o) {
      if (!(o instanceof BaseKey))
        return false;
      BaseKey key = (BaseKey) o;
      if (mText == null && key.mText != null)
        return false;
      if (mText != null && !mText.equals(key.mText))
        return false;
      if (mAttributes == null && key.mAttributes != null)
        return false;
      return mAttributes == null || mAttributes.equals(key.mAttributes);
    }

    @Override
    public int hashCode() {
      final int prime = 31;
      int result = mText == null ? 0 : mText.hashCode();
      result = result * prime + (mAttributes == null ? 0 : mAttributes.hashCode());
      return result;
    }
  }

  final BaseKey mBaseKey;

  final MeasureMode widthMode;
  final MeasureMode heightMode;
  public final float width;
  public final float height;
  final int wordBreakStrategy;
  final boolean enableTailColorConvert;
  final boolean mEnabledTextRefactor;
  final boolean mEnableTextBoringLayout;

  public TextRendererKey(CharSequence text, TextAttributes attributes, MeasureMode widthMode,
      MeasureMode heightMode, float width, float height, int wordBreakStrategy,
      boolean enableTailColorConvert, boolean enabledTextRefactor, boolean enableTextBoringLayout) {
    mBaseKey = new BaseKey(text, attributes);
    this.width = width;
    this.height = height;
    this.widthMode = widthMode;
    this.heightMode = heightMode;
    this.wordBreakStrategy = wordBreakStrategy;
    this.enableTailColorConvert = enableTailColorConvert;
    this.mEnabledTextRefactor = enabledTextRefactor;
    this.mEnableTextBoringLayout = enableTextBoringLayout;
  }

  public TextAttributes getAttributes() {
    return mBaseKey.mAttributes;
  }

  public CharSequence getSpan() {
    return mBaseKey.mText;
  }

  void setSpan(CharSequence span) {
    mBaseKey.mText = span;
  }

  boolean isValid() {
    boolean invalid = widthMode != MeasureMode.UNDEFINED && heightMode != MeasureMode.UNDEFINED
        && width == 0 && height == 0;
    return !invalid;
  }

  @Override
  public String toString() {
    return mBaseKey.mText + " " + width + " " + height;
  }

  @Override
  public boolean equals(Object o) {
    if (!(o instanceof TextRendererKey))
      return false;
    TextRendererKey key = (TextRendererKey) o;
    return mBaseKey.equals(key.mBaseKey) && widthMode == key.widthMode
        && heightMode == key.heightMode && width == key.width && height == key.height
        && wordBreakStrategy == key.wordBreakStrategy
        && enableTailColorConvert == key.enableTailColorConvert
        && mEnabledTextRefactor == key.mEnabledTextRefactor
        && mEnableTextBoringLayout == key.mEnableTextBoringLayout;
  }

  @Override
  public int hashCode() {
    final int prime = 31;
    int result = mBaseKey.hashCode();
    result = result * prime + widthMode.hashCode();
    result = result * prime + heightMode.hashCode();
    result = result * prime + Float.floatToIntBits(width);
    result = result * prime + Float.floatToIntBits(height);
    result = result * prime + wordBreakStrategy;
    result = result * prime + (enableTailColorConvert ? 1 : 0);
    result = result * prime + (mEnabledTextRefactor ? 1 : 0);
    result = result * prime + (mEnableTextBoringLayout ? 1 : 0);
    return result;
  }
}
