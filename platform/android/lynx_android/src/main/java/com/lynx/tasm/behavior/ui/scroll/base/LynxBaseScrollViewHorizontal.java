// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

public interface LynxBaseScrollViewHorizontal {
  int getScrollOffsetHorizontally();

  void setScrollContentSizeHorizontally(int contentSize);

  void scrollByUnlimitedHorizontally(int delta);

  void scrollByHorizontally(int delta);

  void scrollToUnlimitedHorizontally(int offset);

  void scrollToHorizontally(int offset);

  void animatedScrollToHorizontally(
      int offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback);

  void animatedScrollToUnlimitedHorizontally(
      int offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback);

  int[] getScrollRangeHorizontally();

  boolean canScrollForwardsHorizontally();

  boolean canScrollBackwardsHorizontally();
}
