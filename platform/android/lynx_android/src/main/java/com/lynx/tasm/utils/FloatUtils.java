// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;

public class FloatUtils {
  private static final float EPSILON = .00001f;

  public static boolean floatsEqual(float f1, float f2) {
    if (Float.isNaN(f1) || Float.isNaN(f2)) {
      return Float.isNaN(f1) && Float.isNaN(f2);
    }
    return Math.abs(f2 - f1) < EPSILON;
  }

  public static boolean isContainOnlyZero(@Nullable float[] array) {
    if (array != null) {
      for (float x : array) {
        if (x != 0f) {
          return false;
        }
      }
    }
    return true;
  }

  public static float sanitizeFloatPropertyValue(float value) {
    if (value >= -Float.MAX_VALUE && value <= Float.MAX_VALUE) {
      return value;
    }
    if (value < -Float.MAX_VALUE || value == Float.NEGATIVE_INFINITY) {
      return -Float.MAX_VALUE;
    }
    if (value > Float.MAX_VALUE || value == Float.POSITIVE_INFINITY) {
      return Float.MAX_VALUE;
    }
    if (Float.isNaN(value)) {
      return 0;
    }

    LLog.w("lynx", "Invalid float property value: " + value);
    return 0;
  }

  public static float getValue(String value) {
    if (value == null) {
      return Float.NaN;
    }
    value = value.trim();
    if (value.endsWith("%")) {
      if (value.length() < 2) {
        return Float.NaN;
      }
      try {
        return Float.parseFloat(value.substring(0, value.length() - 1)) / 100;
      } catch (Exception e) {
        return Float.NaN;
      }
    } else {
      try {
        return Float.parseFloat(value);
      } catch (Exception e) {
        return Float.NaN;
      }
    }
  }
}
