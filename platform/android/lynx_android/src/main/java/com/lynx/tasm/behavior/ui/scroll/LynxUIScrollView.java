// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;

@LynxGeneratorName(packageName = "com.lynx.tasm.behavior.ui.scroll")
@LynxBehavior(tagName = "scroll-view-new-arch")

public class LynxUIScrollView extends LynxUIScrollViewInternal {
  public LynxUIScrollView(LynxContext context) {
    super(context);
  }

  @LynxProp(name = "scroll-orientation")
  public void setScrollOrientation(String value) {
    super.setScrollOrientation(value);
  }

  @LynxProp(name = "enable-scroll")
  public void setEnableScroll(boolean value) {
    super.setEnableScroll(value);
  }
  @LynxProp(name = "bounces")
  public void setBounces(boolean value) {
    super.setBounces(value);
  }
  @LynxProp(name = "forwards-nested-scroll")
  public void setForwardsNestedScroll(int value) {
    super.setForwardsNestedScroll(value);
  }
  @LynxProp(name = "backwards-nested-scroll")
  public void setBackwardsNestedScroll(int value) {
    super.setBackwardsNestedScroll(value);
  }
  @LynxProp(name = "initial-scroll-index")
  public void setInitialScrollIndex(int value) {
    super.setInitialScrollIndex(value);
  }
  @LynxProp(name = "initial-scroll-offset")
  public void setInitialScrollOffset(String value) {
    super.setInitialScrollOffset(value);
  }
  @LynxProp(name = "lower-threshold")
  public void setLowerThreshold(String value) {
    super.setLowerThreshold(value);
  }
  @LynxProp(name = "upper-threshold")
  public void setUpperThreshold(String value) {
    super.setUpperThreshold(value);
  }
  @LynxProp(name = "scroll-event-throttle")
  public void setScrollEventThrottle(float value) {
    super.setScrollEventThrottle(value);
  }
  @LynxUIMethod
  public void scrollTo(ReadableMap params, Callback callback) {
    super.scrollTo(params, callback);
  }
  @LynxUIMethod
  public void scrollBy(ReadableMap params, Callback callback) {
    super.scrollBy(params, callback);
  }
  @LynxUIMethod
  public void autoScroll(ReadableMap params, Callback callback) {
    super.autoScroll(params, callback);
  }
  @LynxUIMethod
  public void getScrollInfo(ReadableMap params, Callback callback) {
    super.getScrollInfo(params, callback);
  }
}
