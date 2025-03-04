// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Rect;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.ui.background.BackgroundGradientLayer;

public class LynxTextGradientSpan extends MetricAffectingSpan {
  private Rect mBounds = new Rect();

  private final BackgroundGradientLayer mGradient;

  public LynxTextGradientSpan(BackgroundGradientLayer gradient) {
    this.mGradient = gradient;
  }

  public void updateBounds(Rect bounds) {
    mBounds.set(bounds);
  }

  @Override
  public void updateMeasureState(@NonNull TextPaint textPaint) {
    // gradient not effect text measure, this function do no thing
  }

  @Override
  public void updateDrawState(TextPaint tp) {
    if (mBounds.width() == 0 || mBounds.height() == 0) {
      return;
    }
    mGradient.setBounds(mBounds);
    tp.setShader(mGradient.getShader());
  }
}
