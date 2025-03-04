// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import android.view.ViewGroup;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.view.UISimpleView;

public abstract class AbsLynxUIScroll<T extends ViewGroup> extends UISimpleView<T> {
  public static final int SCROLL_UP = 0;
  public static final int SCROLL_DOWN = 1;
  public static final int SCROLL_LEFT = 2;
  public static final int SCROLL_RIGHT = 3;

  public AbsLynxUIScroll(LynxContext context) {
    super(context);
  }

  @LynxProp(name = "scroll-y")
  public void setScrollY(Dynamic enable) {
    if (enable == null) {
      setScrollY(true);
    } else {
      switch (enable.getType()) {
        case Boolean:
          setScrollY(enable.asBoolean());
          break;
        case String:
          setScrollY("true".equals(enable.asString()));
          break;
      }
    }
  }

  public abstract void setScrollY(boolean enable);

  @LynxProp(name = "scroll-x")
  public void setScrollX(Dynamic enable) {
    if (enable == null) {
      setScrollX(false);
    } else {
      switch (enable.getType()) {
        case Boolean:
          setScrollX(enable.asBoolean());
          break;
        case String:
          setScrollX("true".equals(enable.asString()));
          break;
      }
    }
  }

  @LynxProp(name = "enable-scroll", defaultBoolean = true)
  public void setEnableScroll(boolean value) {}

  public abstract void setScrollX(boolean enable);

  @LynxProp(name = "scroll-bar-enable", defaultBoolean = false)
  public abstract void setScrollBarEnable(boolean value);

  @LynxProp(name = "upper-threshold", defaultInt = 0)
  public abstract void setUpperThreshole(int value);

  @LynxProp(name = "lower-threshold", defaultInt = 0)
  public abstract void setLowerThreshole(int value);

  @LynxProp(name = "scroll-top", defaultInt = 0) public abstract void setScrollTop(int value);

  @LynxProp(name = "scroll-left", defaultInt = 0) public abstract void setScrollLeft(int value);

  @LynxProp(name = "scroll-tap", defaultBoolean = false)
  public abstract void setScrollTap(boolean value);

  @LynxProp(name = "scroll-to-index", defaultInt = 0) public abstract void scrollToIndex(int index);

  @LynxProp(name = "forbid-fling-focus-change", defaultBoolean = false)
  public void setForbidFlingFocusChange(boolean value) {}

  @LynxProp(name = "block-descendant-focusability", defaultBoolean = false)
  public void setBlockDescendantFocusability(boolean value) {}

  // event api
  public abstract void sendCustomEvent(int l, int t, int oldl, int oldt, String type);

  public void scrollInto(LynxBaseUI node, boolean isSmooth, String block, String inline) {}
  public void scrollInto(
      LynxBaseUI node, boolean isSmooth, String block, String inline, int bottomInset) {}

  public boolean canScroll(int direction) {
    return false;
  }

  public void scrollByX(double delta) {}

  public void scrollByY(double delta) {}

  public void flingX(double velocityX) {}

  public void flingY(double velocityY) {}

  @Override
  public boolean isScrollable() {
    return true;
  }

  /**
   * @name: enable-new-nested
   * @description: Used for two feature: (1) scroll-view supports nested scroll in fling (2)
   * horizontal scroll-view supports nested scroll.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.8
   **/
  @LynxProp(name = "enable-new-nested", defaultBoolean = false)
  public void setEnableNewNested(boolean value) {}
}
