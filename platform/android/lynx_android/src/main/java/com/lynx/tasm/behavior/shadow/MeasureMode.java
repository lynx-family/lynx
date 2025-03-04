// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow;

import android.view.View;

public enum MeasureMode {
  UNDEFINED(0),
  EXACTLY(1),
  AT_MOST(2);

  private final int mIntValue;

  MeasureMode(int intValue) {
    mIntValue = intValue;
  }

  public int intValue() {
    return mIntValue;
  }

  public static MeasureMode fromInt(int value) {
    switch (value) {
      case 0:
        return UNDEFINED;
      case 1:
        return EXACTLY;
      case 2:
        return AT_MOST;
      default:
        throw new IllegalArgumentException("Unknown measureMode");
    }
  }

  public static int fromMeasureSpec(int measureSpec) {
    switch (View.MeasureSpec.getMode(measureSpec)) {
      case View.MeasureSpec.UNSPECIFIED:
        return UNDEFINED.intValue();
      case View.MeasureSpec.AT_MOST:
        return AT_MOST.intValue();
      case View.MeasureSpec.EXACTLY:
        return EXACTLY.intValue();
      default:
        throw new IllegalArgumentException("Unknown measureSpec");
    }
  }
}
