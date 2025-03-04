// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.util.TypedValue;

/**
 * Android dp to pixel manipulation
 */
public class PixelUtils {
  /**
   * Convert from DIP to PX
   */

  public static float dipToPx(float value, float density) {
    if (density <= 0) {
      density = DisplayMetricsHolder.getScreenDisplayMetrics().density;
    }
    float res = value * density;
    return res;
  }

  public static float dipToPx(float value) {
    return dipToPx(value, 0);
  }

  public static float dipToPx(double value, float density) {
    return dipToPx((float) value, density);
  }

  public static float dipToPx(double value) {
    return dipToPx(value, 0);
  }

  /**
   * Convert from PX to DP
   */
  public static float pxToDip(float value) {
    return value / DisplayMetricsHolder.getScreenDisplayMetrics().density;
  }
}
