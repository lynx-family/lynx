// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

public interface LynxBaseScrollViewPublic
    extends LynxBaseScrollViewAuto, LynxBaseScrollViewHorizontal, LynxBaseScrollViewVertical {
  void enableScroll(boolean enable);

  boolean scrollEnabled();

  void stopScrolling();

  int currentScrollState();

  public void enableBounces(boolean enableBounces);
  public boolean bounces();
}
