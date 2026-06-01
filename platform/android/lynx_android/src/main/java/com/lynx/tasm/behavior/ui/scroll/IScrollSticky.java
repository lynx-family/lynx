// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

public interface IScrollSticky {
  /**
   * Enables sticky handling on the scroll container.
   */
  void setEnableSticky();

  /**
   * Registers a sticky child by its Lynx UI sign so the scroll container can update its sticky
   * position during scrolling or layout refreshes.
   *
   * @param sign the Lynx UI sign of the sticky child
   */
  default void addStickyChildSign(int sign) {}

  /**
   * Unregisters a sticky child that no longer needs sticky updates from this scroll container.
   *
   * @param sign the Lynx UI sign of the sticky child
   */
  default void removeStickyChildSign(int sign) {}

  /**
   * Refreshes sticky children immediately when layout or sticky metadata changes.
   */
  default void refreshStickyChildren() {}
}
