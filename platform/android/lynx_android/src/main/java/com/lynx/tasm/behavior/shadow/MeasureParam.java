// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

public class MeasureParam {
  public float mWidth = 0;
  public MeasureMode mWidthMode = MeasureMode.UNDEFINED;
  public float mHeight = 0;
  public MeasureMode mHeightMode = MeasureMode.UNDEFINED;

  public MeasureParam(){};
  public void updateConstraints(
      float width, MeasureMode widthMode, float height, MeasureMode heightMode) {
    mWidth = width;
    mWidthMode = widthMode;
    mHeight = height;
    mHeightMode = heightMode;
  }
}
