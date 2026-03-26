// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.content.Context;
import android.view.Choreographer;
import com.lynx.tasm.behavior.ui.view.AndroidView;
public class LynxMarkdownView extends AndroidView {
  private LynxServalViewWrapper mMarkdownView;
  private Choreographer.FrameCallback mFrameCallback = null;
  public LynxMarkdownView(Context context) {
    super(context);
    setWillNotDraw(true);
  }

  public void setBundle(LynxMarkdownBundle bundle) {
    if (mFrameCallback == null) {
      mFrameCallback = this::onVSync;
      Choreographer.getInstance().postFrameCallback(mFrameCallback);
    }
    if (bundle != null && bundle.mMarkdownView != mMarkdownView) {
      if (mMarkdownView != null) {
        removeView(mMarkdownView);
      }
      mMarkdownView = bundle.mMarkdownView;
      addView(bundle.mMarkdownView);
    }
  }

  private void onVSync(long time) {
    if (mFrameCallback == null) {
      return;
    }
    if (mMarkdownView != null) {
      mMarkdownView.onRendererFrame(time);
    }
    Choreographer.getInstance().postFrameCallback(mFrameCallback);
  }

  public void destroy() {
    mFrameCallback = null;
    mMarkdownView.destroy();
  }

  @Override
  public void invalidate() {
    super.invalidate();
    if (mMarkdownView != null) {
      mMarkdownView.invalidate();
    }
  }
}
