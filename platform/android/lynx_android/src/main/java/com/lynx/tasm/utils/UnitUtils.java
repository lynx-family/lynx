// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import android.text.TextUtils;
import android.util.DisplayMetrics;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;

/**
 * A utils to parse value with rpx/px/number to pixel
 */
public class UnitUtils {
  public static boolean isPercentage(String v) {
    return v.endsWith("%");
  }

  /**
   * Does not support rem em vw vh
   * @param valueWithUnit
   * @return
   */
  @Deprecated
  public static float toPx(String valueWithUnit) {
    return toPx(valueWithUnit, 0);
  }

  /**
   * Does not support rem em vw vh
   * @param valueWithUnit
   * @return
   */
  @Deprecated
  public static float toPx(String valueWithUnit, float defaultValue) {
    return toPx(valueWithUnit, 0, 0, 0, 0, defaultValue);
  }

  public static float toPx(String valueWithUnit, float rootFontSize, float curFontSize,
      float rootWidth, float rootHeight) {
    return toPxWithDisplayMetrics(valueWithUnit, rootFontSize, curFontSize, rootWidth, rootHeight,
        0, DisplayMetricsHolder.getScreenDisplayMetrics());
  }

  public static float toPxWithDisplayMetrics(String valueWithUnit, float rootFontSize,
      float curFontSize, float rootWidth, float rootHeight, DisplayMetrics displayMetrics) {
    return toPxWithDisplayMetrics(
        valueWithUnit, rootFontSize, curFontSize, rootWidth, rootHeight, 0, displayMetrics);
  }

  public static float toPx(String valueWithUnit, float rootFontSize, float curFontSize,
      float rootWidth, float rootHeight, float defaultValue) {
    return toPxWithDisplayMetrics(valueWithUnit, rootFontSize, curFontSize, rootWidth, rootHeight,
        defaultValue, DisplayMetricsHolder.getScreenDisplayMetrics());
  }

  public static float toPxWithDisplayMetrics(String valueWithUnit, float rootFontSize,
      float curFontSize, float rootWidth, float rootHeight, float defaultValue,
      DisplayMetrics displayMetrics) {
    int length = TextUtils.isEmpty(valueWithUnit) ? 0 : valueWithUnit.length();
    float result = defaultValue;
    try {
      if (length > 3 && valueWithUnit.endsWith("rpx")) {
        result = Float.parseFloat(valueWithUnit.substring(0, length - 3));
        result = displayMetrics.widthPixels * result / 750;
      } else if (length > 3 && valueWithUnit.endsWith("ppx")) {
        // On Android, default unit is physical pixel.
        result = Float.parseFloat(valueWithUnit.substring(0, length - 3));
      } else if (length > 2 && valueWithUnit.endsWith("px")) {
        result = PixelUtils.dipToPx(Float.parseFloat(valueWithUnit.substring(0, length - 2)));
      } else if (length > 1 && valueWithUnit.endsWith("%")) {
        result = Float.parseFloat(valueWithUnit.substring(0, length - 1)) / 100;
      } else if (length > 3 && valueWithUnit.endsWith("rem")) {
        // rootFontSize unit:px
        result = rootFontSize * Float.parseFloat(valueWithUnit.substring(0, length - 3));
      } else if (length > 2 && valueWithUnit.endsWith("em")) {
        // curFontSize unit:px
        result = curFontSize * Float.parseFloat(valueWithUnit.substring(0, length - 2));
      } else if (length > 2 && valueWithUnit.endsWith("vw")) {
        result = rootWidth * Float.parseFloat(valueWithUnit.substring(0, length - 2)) / 100;
      } else if (length > 2 && valueWithUnit.endsWith("vh")) {
        result = rootHeight * Float.parseFloat(valueWithUnit.substring(0, length - 2)) / 100;
      } else if (length > 0) {
        result = Float.parseFloat(valueWithUnit);
      }
    } catch (Throwable e) {
      LLog.w("lynx", "Number parse error from value = " + valueWithUnit + " to px!");
    }

    return result;
  }

  public static float toPx(String valueWithUnit, float rootFontSize, float curFontSize,
      float rootWidth, float rootHeight, float viewSize, float defaultValue) {
    return toPxWithDisplayMetrics(valueWithUnit, rootFontSize, curFontSize, rootWidth, rootHeight,
        viewSize, defaultValue, DisplayMetricsHolder.getScreenDisplayMetrics());
  }

  public static float toPxWithDisplayMetrics(String valueWithUnit, float rootFontSize,
      float curFontSize, float rootWidth, float rootHeight, float viewSize, float defaultValue,
      DisplayMetrics displayMetrics) {
    int length = TextUtils.isEmpty(valueWithUnit) ? 0 : valueWithUnit.length();
    float result = defaultValue;
    try {
      if (length > 1 && valueWithUnit.endsWith("%")) {
        result = Float.parseFloat(valueWithUnit.substring(0, length - 1)) * viewSize / 100;
      } else {
        result = toPxWithDisplayMetrics(valueWithUnit, rootFontSize, curFontSize, rootWidth,
            rootHeight, defaultValue, displayMetrics);
      }
    } catch (Throwable e) {
      LLog.w("lynx", "Number parse error from value = " + valueWithUnit + " to px!");
    }

    return result;
  }

  public static float toPx(String valueWithUnit, float viewSize, float defaultValue) {
    return toPxWithDisplayMetrics(
        valueWithUnit, viewSize, defaultValue, DisplayMetricsHolder.getScreenDisplayMetrics());
  }

  public static float toPxWithDisplayMetrics(
      String valueWithUnit, float viewSize, float defaultValue, DisplayMetrics displayMetrics) {
    int length = TextUtils.isEmpty(valueWithUnit) ? 0 : valueWithUnit.length();
    float result = defaultValue;
    try {
      if (length > 1 && valueWithUnit.endsWith("%")) {
        result = Float.parseFloat(valueWithUnit.substring(0, length - 1)) * viewSize / 100;
      } else {
        result = toPxWithDisplayMetrics(valueWithUnit, 0, 0, 0, 0, defaultValue, displayMetrics);
      }
    } catch (Throwable e) {
      LLog.w("lynx", "Number parse error from value = " + valueWithUnit + " to px!");
    }

    return result;
  }

  public static Value parserValue(String s, LynxBaseUI ui) {
    if (TextUtils.isEmpty(s) || ui == null) {
      return null;
    }
    UIBody uiBody = ui.getLynxContext().getUIBody();
    final float result = UnitUtils.toPxWithDisplayMetrics(s, uiBody.getFontSize(), ui.getFontSize(),
        uiBody.getWidth(), uiBody.getHeight(), MeasureUtils.UNDEFINED,
        ui.getLynxContext().getScreenMetrics());
    if (MeasureUtils.isUndefined(result)) {
      return null;
    }
    if (s.endsWith("%")) {
      return new Value(result, Value.Unit.PERCENTAGE);
    }
    return new Value(result, Value.Unit.PX);
  }

  public static <T extends Comparable<T>> T clamp(T val, T min, T max) {
    return val.compareTo(min) < 0 ? min : ((val.compareTo(max) > 0) ? max : val);
  }
}
