// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import androidx.annotation.Nullable;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.AbsInlineImageShadowNode;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;

public class InlineImageShadowNode extends AbsInlineImageShadowNode {
  LynxImageManager mLynxImageManager;

  public InlineImageShadowNode() {}

  @Override
  public void setContext(LynxContext context) {
    super.setContext(context);
    mLynxImageManager = new LynxImageManager(mContext) {
      @Override
      public void invalidate() {
        markDirty();
      }

      @Override
      protected void onImageLoadSuccess(int width, int height) {
        notifyLoadSuccessIfNeeded(width, height);
      }

      @Override
      protected void onImageLoadError(LynxError error, int categorizedCode, int imageErrorCode) {
        notifyErrorIfNeeded(error.getSummaryMessage());
      }
    };
  }

  @Override
  public void setSource(@Nullable String source) {
    mLynxImageManager.setSrc(source);
  }

  @Override
  public void setMode(String mode) {
    mLynxImageManager.setMode(mode);
  }

  @Override
  public AbsInlineImageSpan generateInlineImageSpan() {
    float width = getStyle().getWidth();
    float height = getStyle().getHeight();
    int[] margins = getStyle().getMargins();

    AbsInlineImageSpan imageSpan = new InlineImageSpan(
        (int) Math.ceil(width), (int) Math.ceil(height), margins, mLynxImageManager);
    return imageSpan;
  }
  @Override
  public void onAfterUpdateTransaction() {
    mLynxImageManager.updateRedirectCheckResult();
    markDirty();
  }
}
