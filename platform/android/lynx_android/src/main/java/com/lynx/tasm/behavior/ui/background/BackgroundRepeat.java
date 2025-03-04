// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

/**
 * enum value is defined in native enum class BackgroundRepeatType at Lynx/style/css_type.h
 */
public enum BackgroundRepeat {
  REPEAT, // 0
  NO_REPEAT, // 1
  REPEAT_X, // 2
  REPEAT_Y, // 3
  ROUND, // 4
  SPACE; // 5

  /**
   * Create Repeat from int value
   * @param value int value defined in css_type.h
   * @return BackgroundRepeat
   */
  public static BackgroundRepeat valueOf(int value) {
    switch (value) {
      case 0:
        return REPEAT;
      case 1:
        return NO_REPEAT;
      case 2:
        return REPEAT_X;
      case 3:
        return REPEAT_Y;
      case 4:
        return ROUND;
      case 5:
        return SPACE;
      default:
        return REPEAT;
    }
  }
}
