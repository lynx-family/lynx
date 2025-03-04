// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.event;

import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.PixelUtils;

public class LynxScrollEvent extends LynxDetailEvent {
  public static final String EVENT_SCROLL = "scroll";
  public static final String EVENT_SCROLL_TOUPPER = "scrolltoupper";
  public static final String EVENT_SCROLL_TO_UPPER_EDGE = "scrolltoupperedge";
  public static final String EVENT_SCROLL_TOLOWER = "scrolltolower";
  public static final String EVENT_SCROLL_TO_LOWER_EDGE = "scrolltoloweredge";
  public static final String EVENT_SCROLL_TO_NORMAL_STATE = "scrolltonormalstate";
  public static final String EVENT_SCROLL_START = "scrollstart";
  public static final String EVENT_SCROLL_END = "scrollend";
  public static final String EVENT_SCROLL_STATE_CHANGE = "scrollstatechange";

  public LynxScrollEvent(int tag, String type) {
    super(tag, type);
  }

  public static LynxScrollEvent createScrollEvent(int tag, String type) {
    return new LynxScrollEvent(tag, type);
  }

  public void setScrollParams(
      int left, int top, int contentHeight, int contentWidth, int x, int y) {
    addDetail("scrollLeft", PixelUtils.pxToDip(left));
    addDetail("scrollTop", PixelUtils.pxToDip(top));
    addDetail("scrollHeight", PixelUtils.pxToDip(contentHeight));
    addDetail("scrollWidth", PixelUtils.pxToDip(contentWidth));
    addDetail("deltaX", PixelUtils.pxToDip(x));
    addDetail("deltaY", PixelUtils.pxToDip(y));
  }
}
