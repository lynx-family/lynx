// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

public interface LynxBaseScrollViewAuto {
  int[] getScrollOffset();

  void setScrollContentSize(int[] contentSize);

  void scrollByUnlimited(int[] delta);

  void scrollBy(int[] delta);

  void scrollToUnlimited(int[] offset);

  void scrollTo(int[] offset);

  void animatedScrollTo(int[] offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback);

  void animatedScrollToUnlimited(
      int[] offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback);

  int[] getScrollRange();

  boolean canScrollForwards();

  boolean canScrollBackwards();
}
