// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.content.Context;
import com.lynx.markdown.ServalMarkdownView;
import com.lynx.tasm.behavior.shadow.ShadowNode;

public class LynxServalViewWrapper extends ServalMarkdownView {
  protected final ShadowNode mShadowNode;
  public LynxServalViewWrapper(Context context, ShadowNode shadowNode) {
    super(context);
    mShadowNode = shadowNode;
  }

  @Override
  public void requestMeasure() {
    if (mShadowNode == null) {
      return;
    }
    mShadowNode.markDirty();
  }

  @Override
  public void requestAlign() {
    if (mShadowNode == null) {
      return;
    }
    mShadowNode.markDirty();
  }
}
