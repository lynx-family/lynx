// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.attr;

import com.lynx.tasm.behavior.ui.background.BackgroundGradientLayer;

public class MaskImage implements Cloneable {
  private final BackgroundGradientLayer mGradientLayer;

  public MaskImage(BackgroundGradientLayer gradientLayer) {
    this.mGradientLayer = gradientLayer;
  }

  public BackgroundGradientLayer getGradientLayer() {
    return mGradientLayer;
  }
}
