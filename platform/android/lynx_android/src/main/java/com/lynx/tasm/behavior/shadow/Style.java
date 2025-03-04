// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

public class Style {
  public static final int EDGE_LEFT = 0;
  public static final int EDGE_TOP = 1;
  public static final int EDGE_RIGHT = 2;
  public static final int EDGE_BOTTOM = 3;

  // Ensure value consistent with the value on layout_node.cc
  public static final int FLEX_DIRECTION_COLUMN = 0;
  public static final int FLEX_DIRECTION_COLUMN_REVERSE = 1;
  public static final int FLEX_DIRECTION_ROW = 2;
  public static final int FLEX_DIRECTION_ROW_REVERSE = 3;

  private LayoutNode mLayoutNode;

  public Style(LayoutNode node) {
    mLayoutNode = node;
  }

  public int getFlexDirection() {
    return mLayoutNode.getFlexDirection();
  }

  public float getWidth() {
    return mLayoutNode.getWidth();
  }

  public float getHeight() {
    return mLayoutNode.getHeight();
  }

  public int[] getPaddings() {
    return mLayoutNode.getPadding();
  }

  public int[] getMargins() {
    return mLayoutNode.getMargins();
  }
}
