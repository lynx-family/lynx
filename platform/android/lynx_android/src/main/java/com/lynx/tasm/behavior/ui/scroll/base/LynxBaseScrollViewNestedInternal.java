// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

import androidx.core.view.NestedScrollingChild2;
import androidx.core.view.NestedScrollingParent2;

public interface LynxBaseScrollViewNestedInternal
    extends LynxBaseScrollViewInternal, NestedScrollingParent2, NestedScrollingChild2 {
  int getForwardNestedScrollMode();

  int getBackwardNestedScrollMode();

  public LynxBaseScrollViewNestedInternal getNestedScrollingParentForType(int type);
}
