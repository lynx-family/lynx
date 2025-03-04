// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import android.view.View;
import com.lynx.tasm.utils.UIThreadUtils;

/**
 * Uses in ui running mode so that the layout can be consistent with
 * Android View's layout system.
 *
 * Only trigger layout runnable in a VSync cycle.
 */
public class ViewLayoutTick implements LayoutTick {
  private View mView;
  private Runnable mRunnable;

  public ViewLayoutTick(View root) {
    mView = root;
  }

  @Override
  public void request(Runnable runnable) {
    // Ensure that the runable runs on the UI thread.
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (mView != null) {
          mView.requestLayout();
        }
        mRunnable = runnable;
      }
    });
  }

  public void attach(View view) {
    mView = view;
  }

  public void triggerLayout() {
    if (mRunnable != null) {
      mRunnable.run();
    }
    mRunnable = null;
  }
}
