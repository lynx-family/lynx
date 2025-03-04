// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static com.lynx.tasm.behavior.StyleConstants.FONTSTYLE_ITALIC;
import static com.lynx.tasm.behavior.StyleConstants.FONTSTYLE_NORMAL;
import static com.lynx.tasm.behavior.StyleConstants.FONTSTYLE_OBLIQUE;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_BOLD;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_NORMAL;
import static com.lynx.tasm.behavior.StyleConstants.TEXT_DECORATION_LINETHROUGH;
import static com.lynx.tasm.behavior.StyleConstants.TEXT_DECORATION_NONE;
import static com.lynx.tasm.behavior.StyleConstants.TEXT_DECORATION_UNDERLINE;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Style compat utils.
 * No need to adapt for new css value.This only use for old components.And old components will not
 * use new css feature.
 */
public final class StyleCompatUtils {
  /**
   * decide style should compat old version or not.
   * @param subClazz instance' class.
   * @param pClazz no need compat class.
   * @param methodName method name.
   * @param parameterTypes parameter types.
   * @return true means should compat.false means not.
   */
  public static boolean shouldCompat(@NonNull Class subClazz, @NonNull Class pClazz,
      String methodName, Class<?>... parameterTypes) {
    if (subClazz == pClazz) {
      return false;
    }
    try {
      if (subClazz.getDeclaredMethod(methodName, parameterTypes) != null) {
        return true;
      }
    } catch (Exception e) {
      // just ignore this error.no need care about this.
    }
    return false;
  }

  private static int parseNumericFontWeight(String fontWeightString) {
    // This should be much faster than using regex to verify input and
    // Integer.parseInt
    return fontWeightString.length() == 3 && fontWeightString.endsWith("00")
            && fontWeightString.charAt(0) <= '9' && fontWeightString.charAt(0) >= '1'
        ? 100 * (fontWeightString.charAt(0) - '0')
        : -1;
  }

  // for fangao
  public static int toFontWeight(String fontWeightString) {
    int fontWeightNumeric =
        fontWeightString != null ? parseNumericFontWeight(fontWeightString) : -1;
    int fontWeight = FONTWEIGHT_NORMAL;
    if (fontWeightNumeric >= 500 || "bold".equals(fontWeightString)) {
      fontWeight = FONTWEIGHT_BOLD;
    } else if ("normal".equals(fontWeightString)
        || (fontWeightNumeric != -1 && fontWeightNumeric < 500)) {
      fontWeight = FONTWEIGHT_NORMAL;
    } else if (fontWeightString == null) {
      fontWeight = FONTWEIGHT_NORMAL;
    }
    return fontWeight;
  }

  // for fangao
  public static int toFontStyle(@Nullable String style) {
    if (TextUtils.isEmpty(style)) {
      return FONTSTYLE_NORMAL;
    }
    switch (style) {
      case "italic":
        return FONTSTYLE_ITALIC;
      case "oblique":
        return FONTSTYLE_OBLIQUE;
      default:
        return FONTSTYLE_NORMAL;
    }
  }

  // for fangao
  public static int toTextDecoration(@Nullable String decoration) {
    if (TextUtils.isEmpty(decoration)) {
      return TEXT_DECORATION_NONE;
    }
    int result = TEXT_DECORATION_NONE;
    for (String item : decoration.split(" ")) {
      if ("underline".equals(item)) {
        result |= TEXT_DECORATION_UNDERLINE;
      } else if ("line-through".equals(item)) {
        result |= TEXT_DECORATION_LINETHROUGH;
      }
    }
    return result;
  }
}
