// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;

public final class InlineImageSpan extends AbsInlineImageSpan {
  LynxImageManager mLynxImageManager;

  public InlineImageSpan(int width, int height, int[] margins, LynxImageManager lynxImageManager) {
    super(width, height, margins);
    mLynxImageManager = lynxImageManager;
    mLynxImageManager.onLayoutUpdated(getWidth(), getHeight(), 0, 0, 0, 0);
    mLynxImageManager.setDisableDefaultResize(true);
  }

  public InlineImageSpan(int width, int height, int[] margins) {
    super(width, height, margins);
  }

  public void setImageManager(LynxImageManager lynxImageManager) {
    mLynxImageManager = lynxImageManager;
    mLynxImageManager.onLayoutUpdated(getWidth(), getHeight(), 0, 0, 0, 0);
    mLynxImageManager.setDisableDefaultResize(true);
  }

  @Override
  public void setCallback(Drawable.Callback callback) {
    super.setCallback(callback);
    if (getWidth() > 0 && getHeight() > 0) {
      mLynxImageManager.updateNodeProps();
    }
  }

  @Override
  public void handleRequestResult() {
    super.handleRequestResult();
    if (mLynxImageManager == null) {
      return;
    }
    mLynxImageManager.tryHandleResult();
  }

  @Nullable
  @Override
  public Drawable getDrawable() {
    return mLynxImageManager.getSrcImageDrawable();
  }

  @Override
  public void onDetachedFromWindow() {}

  @Override
  public void onStartTemporaryDetach() {}

  @Override
  public void onAttachedToWindow() {}

  @Override
  public void onFinishTemporaryDetach() {}

  @Override
  public void draw(Canvas canvas, CharSequence text, int start, int end, float x, int top, int y,
      int bottom, Paint paint) {
    if (getCallback() == null) {
      return;
    }
    super.draw(canvas, text, start, end, x, top, y, bottom, paint);
  }
}
