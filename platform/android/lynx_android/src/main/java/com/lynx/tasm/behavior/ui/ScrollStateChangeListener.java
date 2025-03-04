// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

public interface ScrollStateChangeListener {
  int SCROLL_STATE_IDLE = 0;
  int SCROLL_STATE_DRAGGING = 1;
  int SCROLL_STATE_SETTLING = 2;

  void onScrollStateChanged(int state);
}
