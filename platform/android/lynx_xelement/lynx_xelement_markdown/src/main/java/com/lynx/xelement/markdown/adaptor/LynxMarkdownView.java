// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.content.Context;
import com.lynx.tasm.behavior.ui.view.AndroidView;
public class LynxMarkdownView extends AndroidView {
  private LynxServalViewWrapper mMarkdownView;
  public LynxMarkdownView(Context context) {
    super(context);
  }

  public void setBundle(LynxMarkdownBundle bundle) {
    if (bundle != null && bundle.mMarkdownView != mMarkdownView) {
      if (mMarkdownView != null) {
        removeView(mMarkdownView);
      }
      mMarkdownView = bundle.mMarkdownView;
      addView(bundle.mMarkdownView);
    }
  }
}
