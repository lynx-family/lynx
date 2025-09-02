// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

public interface LynxBaseScrollViewVertical {
  int getScrollOffsetVertically();

  void setScrollContentSizeVertically(int contentSize);

  void scrollByUnlimitedVertically(int delta);

  void scrollByVertically(int delta);

  void scrollToUnlimitedVertically(int offset);

  void scrollToVertically(int offset);

  void animatedScrollToVertically(
      int offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback);

  void animatedScrollToUnlimitedVertically(
      int offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback);

  int[] getScrollRangeVertically();

  boolean canScrollForwardsVertically();

  boolean canScrollBackwardsVertically();
}
