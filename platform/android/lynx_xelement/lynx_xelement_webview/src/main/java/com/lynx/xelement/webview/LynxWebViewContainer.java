// Copyright 2024 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.webview;

import android.content.Context;
import android.view.View;
import com.lynx.tasm.behavior.ui.view.AndroidView;

public class LynxWebViewContainer extends AndroidView {
  public LynxWebViewContainer(Context context) {
    super(context);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    if (getChildCount() > 0) {
      View view = getChildAt(0);
      view.measure(widthMeasureSpec, heightMeasureSpec);
    }
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    super.onLayout(changed, left, top, right, bottom);
    if (getChildCount() > 0) {
      View view = getChildAt(0);
      view.layout(0, 0, getWidth(), getHeight());
    }
  }
}
