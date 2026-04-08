// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator;

import android.content.Context;
import android.util.AttributeSet;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import com.google.android.material.appbar.AppBarLayout;

public abstract class BaseScrollCoordinatorAppBarLayout
    extends AppBarLayout implements CoordinatorLayout.AttachedBehavior {
  private boolean enableToolbarDrag = true;
  private boolean scrollEnabled = true;

  public BaseScrollCoordinatorAppBarLayout(Context context) {
    super(context);
  }

  public BaseScrollCoordinatorAppBarLayout(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  public void setScrollEnabled(boolean enable) {
    scrollEnabled = enable;
  }

  public boolean isScrollEnabled() {
    return scrollEnabled;
  }

  public void setToolbarDragEnabled(boolean enable) {
    enableToolbarDrag = enable;
  }

  public boolean isToolbarDragEnabled() {
    return enableToolbarDrag;
  }
}
