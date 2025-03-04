// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import com.lynx.react.bridge.Dynamic;
import com.lynx.tasm.behavior.ui.utils.PlatformLength;

public class BackgroundPosition {
  private final PlatformLength mLength;

  public BackgroundPosition(Dynamic value, int type) {
    mLength = new PlatformLength(value, type);
  }

  public float apply(float parentValue) {
    return mLength.getValue(parentValue);
  }
}
