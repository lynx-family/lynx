// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.event;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.PixelUtils;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class LynxListEvent extends LynxDetailEvent {
  public static final String EVENT_SCROLL = "scroll";
  public static final String EVENT_SCROLL_TOUPPER = "scrolltoupper";
  public static final String EVENT_SCROLL_TOLOWER = "scrolltolower";
  public static final String EVENT_NODE_APPEAR = "nodeappear";
  public static final String EVENT_NODE_DISAPPEAR = "nodedisappear";
  public static final String EVENT_SCROLL_STATE_CHANGE = "scrollstatechange";
  public static final String EVENT_LAYOUT_COMPLETE = "layoutcomplete";

  public static final int SCROLL_STATE_IDLE = 1;
  public static final int SCROLL_STATE_DRAGGING = 2;
  public static final int SCROLL_STATE_SETTLING = 3;

  public LynxListEvent(int tag, String type) {
    super(tag, type);
  }

  public static LynxListEvent createListEvent(int tag, String type) {
    return new LynxListEvent(tag, type);
  }

  public void setCellParams(int position) {
    addDetail("position", position);
  }

  public void setScrollParams(
      int left, int top, int dx, int dy, @Nullable JavaOnlyArray attachedCells) {
    addDetail("scrollLeft", PixelUtils.pxToDip(left));
    addDetail("scrollTop", PixelUtils.pxToDip(top));
    addDetail("deltaX", PixelUtils.pxToDip(dx));
    addDetail("deltaY", PixelUtils.pxToDip(dy));
    addAttachCells(attachedCells);
  }

  private void addAttachCells(@Nullable JavaOnlyArray attachedCells) {
    if (attachedCells == null) {
      return;
    }
    List<Map<String, Object>> attachedCellsMsg = new ArrayList<>();
    if (attachedCells != null) {
      for (int i = 0; i < attachedCells.size(); ++i) {
        JavaOnlyMap entry = attachedCells.getMap(i);
        if (entry == null || entry.isEmpty()) {
          continue;
        }
        attachedCellsMsg.add(entry);
      }
    }
    addDetail("attachedCells", attachedCellsMsg);
  }

  public void setListScrollStateChangeParams(int state, @Nullable JavaOnlyArray attachedCells) {
    addDetail("state", state);
    if (attachedCells != null) {
      addAttachCells(attachedCells);
    }
  }
}
