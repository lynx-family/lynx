// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

// Whether to share threads
public enum ThreadStrategyForRendering {
  /**
   * ALL_ON_UI means that the template assembler will work on ui thread
   * completely including layout operation. Otherwise template
   * assembler will work on layout thread.
   *
   * If you want LynxView behavior similar to native view such as layout children
   * when onMeasure & onLayout happens, it's better to return true to get better
   * visual effect.
   *
   * It's better to return false if the template is too heavy so that getting
   * better performance.
   *
   * @return whether template assembler run on ui thread
   */
  ALL_ON_UI(0),

  MOST_ON_TASM(1),
  PART_ON_LAYOUT(2),
  MULTI_THREADS(3);

  private int mId;

  ThreadStrategyForRendering(int id) {
    mId = id;
  }

  public int id() {
    return mId;
  }
}
