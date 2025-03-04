// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import com.lynx.tasm.behavior.StyleConstants;

public class ShadowStyle {
  public int verticalAlign;
  public float verticalAlignLength;

  public ShadowStyle() {
    verticalAlign = StyleConstants.VERTICAL_ALIGN_DEFAULT;
    verticalAlignLength = 0.0f;
  }
}
