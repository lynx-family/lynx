// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

public class AlignParam {
  private float mLeftOffset = 0.f;
  private float mTopOffset = 0.f;

  public void setLeftOffset(float leftOffset) {
    mLeftOffset = leftOffset;
  }
  public void setTopOffset(float topOffset) {
    mTopOffset = topOffset;
  }

  public float getLeftOffset() {
    return mLeftOffset;
  }
  public float getTopOffset() {
    return mTopOffset;
  }
}
