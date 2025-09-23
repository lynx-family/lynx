// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import android.text.TextUtils;

public class ColorUtils {
  public static int parse(String color) {
    if (TextUtils.isEmpty(color)) {
      return 0;
    }
    CSSColor cssColor = CSSColor.parse(color);
    if (cssColor == null) {
      return 0;
    }
    return cssColor.cast();
  }

  public static boolean isValid(String color) {
    if (color == null || color.isEmpty()) {
      return false;
    }
    return CSSColor.parse(color, new CSSColor[1]);
  }

  private static native int nativeParse(String color);

  private static native boolean nativeValidate(String color);
}
