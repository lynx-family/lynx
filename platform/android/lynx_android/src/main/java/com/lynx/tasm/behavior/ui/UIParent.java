// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import android.view.ViewGroup;

public interface UIParent {
  /**
   * Generating a suitable LayoutParams for child.
   *
   * @param childParams childParams to check if child ui need
   *                    a new LayoutParams
   * @return LayoutParams, will be set into Android View if it
   * is not the same as childParams
   */
  ViewGroup.LayoutParams generateLayoutParams(ViewGroup.LayoutParams childParams);

  boolean needCustomLayout();

  void requestLayout();

  void invalidate();
}
