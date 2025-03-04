/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.tasm.behavior.shadow.text;

import android.os.Build;
import android.text.TextPaint;
import android.text.style.CharacterStyle;
import com.lynx.tasm.behavior.ui.ShadowData;

public class ShadowStyleSpan extends CharacterStyle {
  private final ShadowData mTextShadow;

  public ShadowStyleSpan(ShadowData textShadow) {
    mTextShadow = textShadow;
  }

  @Override
  public void updateDrawState(TextPaint textPaint) {
    float blurRadius = mTextShadow.blurRadius;

    // make sure blurRadius in range (0, 25]
    // solve this https://t.wtturl.cn/NxU4vLm/
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
      blurRadius = Math.min(Math.max(0, blurRadius), 25.f);
    }

    textPaint.setShadowLayer(
        blurRadius, mTextShadow.offsetX, mTextShadow.offsetY, mTextShadow.color);
  }
}
