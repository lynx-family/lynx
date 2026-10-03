// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list.container;

import android.view.View;
import androidx.annotation.NonNull;

/**
 * Applies parent-owned visual transformations to attached list-item views.
 *
 * <p>The list container owns scrolling, paging, layout, and item attachment. Implementations own
 * only visual properties such as scale and translation, and must not add or remove list children
 * from these callbacks.
 */
public interface ListItemTransformer {
  /**
   * Applies a visual transformation to one attached list item.
   *
   * @param listContainerView list container that is presenting the item
   * @param itemView attached list-item view to transform
   * @param isVertical whether the list's main scroll axis is vertical
   * @param isRTL whether the list uses right-to-left layout
   * @param mainAxisOffset signed distance from the viewport's logical start to the item's logical
   *     start, in pixels. Uses top edges for vertical lists, left edges for horizontal LTR lists,
   *     and right edges for horizontal RTL lists. Positive values follow the layout direction.
   *     Horizontal translations calculated along this logical axis must be negated for RTL before
   *     applying them to the view's physical X axis.
   */
  void transformItem(@NonNull ListContainerView listContainerView, @NonNull View itemView,
      boolean isVertical, boolean isRTL, int mainAxisOffset);

  /**
   * Clears visual properties previously written by this transformer before an item is detached or
   * the transformer is replaced.
   */
  void resetItem(@NonNull View itemView);
}
