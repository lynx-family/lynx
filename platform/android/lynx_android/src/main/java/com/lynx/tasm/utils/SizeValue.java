// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import androidx.annotation.Nullable;

/**
 * This class aims to extends the ability of 'com.lynx.tasm.util.Value', you can use it to compute
 * location of point dynamically
 */
public class SizeValue {
  public enum Type {
    UNKNOWN,
    PERCENTAGE,
    DEVICE_PX, // device px, equal to CSS px multiples density on Android
  }

  public float value;
  public SizeValue.Type type;

  public SizeValue(SizeValue.Type type, float value) {
    this.type = type;
    this.value = value;
  }
  public SizeValue() {
    this.type = Type.UNKNOWN;
    this.value = 0.f;
  }

  @Nullable
  static public SizeValue fromCSSString(String valueStr) {
    int length = valueStr == null ? 0 : valueStr.length();
    if (length > 1 && valueStr.endsWith("%")) {
      float value = UnitUtils.toPx(valueStr, 0, 0, 0, 0, 0);
      return new SizeValue(Type.PERCENTAGE, value);
    } else if (length > 2 && valueStr.endsWith("px")) {
      float value = UnitUtils.toPx(valueStr, 0, 0, 0, 0, 0);
      return new SizeValue(Type.DEVICE_PX, value);
    }
    return null;
  }

  public float convertToDevicePx(float fullSize) {
    if (type == Type.PERCENTAGE) {
      return value * fullSize;
    } else if (type == Type.DEVICE_PX) {
      return value;
    }
    return 0.f;
  }
}
