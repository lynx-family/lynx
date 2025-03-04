// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow;

public class MeasureUtils {
  public static final float UNDEFINED = (float) (10E20);

  public static boolean isUndefined(float value) {
    return (Float.compare(value, (float) 10E8) >= 0 || Float.compare(value, (float) -10E8) <= 0);
  }
}
