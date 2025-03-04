// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import com.lynx.react.bridge.Dynamic;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.utils.PlatformLength;

public class BackgroundSize {
  private final PlatformLength mLength;

  public boolean isCover() {
    return mLength.asNumber() == StyleConstants.BACKGROUND_SIZE_COVER;
  }

  public boolean isContain() {
    return mLength.asNumber() == StyleConstants.BACKGROUND_SIZE_CONTAIN;
  }

  public boolean isAuto() {
    return mLength.asNumber() == StyleConstants.BACKGROUND_SIZE_AUTO;
  }

  public BackgroundSize(Dynamic value, int type) {
    mLength = new PlatformLength(value, type);
  }

  public float apply(float parentValue, float currentValue) {
    if (isAuto()) {
      return currentValue;
    } else {
      return mLength.getValue(parentValue);
    }
  }
}
