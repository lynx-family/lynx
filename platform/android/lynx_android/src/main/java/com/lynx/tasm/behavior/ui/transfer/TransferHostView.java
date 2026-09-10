// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.transfer;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import com.lynx.tasm.behavior.ui.view.AndroidView;

@SuppressLint({"", "ViewConstructor"})
public final class TransferHostView extends AndroidView {
  private final TransferWrapperView mWrapperView;

  TransferHostView(Context context, TransferWrapperView wrapperView) {
    super(context);
    mWrapperView = wrapperView;
  }

  @Override
  public void addView(View child, int index, ViewGroup.LayoutParams params) {
    mWrapperView.addView(child, index, params);
  }

  @Override
  public void removeView(View view) {
    mWrapperView.removeView(view);
  }

  @Override
  public void removeViewAt(int index) {
    mWrapperView.removeViewAt(index);
  }

  @Override
  public void removeAllViews() {
    mWrapperView.removeAllViews();
    super.removeAllViews();
  }
}
