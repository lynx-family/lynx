// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import android.text.TextPaint;
import android.text.style.CharacterStyle;

public class TextDecorationSpan extends CharacterStyle {
  public boolean mUnderline;

  public boolean mLineThrough;

  public int mTextDecorationStyle;

  public int mTextDecorationColor;

  public TextDecorationSpan(
      boolean underline, boolean lineThrough, int textDecorationStyle, int textDecorationColor) {
    this.mUnderline = underline;
    this.mLineThrough = lineThrough;
    this.mTextDecorationStyle = textDecorationStyle;
    this.mTextDecorationColor = textDecorationColor;
  }

  @Override
  public void updateDrawState(TextPaint textPaint) {}
}
