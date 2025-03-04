// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.behavior.shadow.text.TextHelper;
import com.lynx.tasm.common.LepusBuffer;
import java.nio.ByteBuffer;

class TextUtils {
  @CalledByNative
  static private ByteBuffer getTextInfo(
      String text, String fontSize, String fontFamily, String maxWidth, int maxLine) {
    ReadableMap result = TextHelper.getTextInfo(text, fontSize, fontFamily, maxWidth, maxLine);
    // use the ByteBuffer is better for the lepus API invoke
    return LepusBuffer.INSTANCE.encodeMessage(result);
  }

  @CalledByNative
  static private double getTextWidth(String text, String fontSize, String fontFamily) {
    // fontSize's unit is px.
    if (text.isEmpty() || fontSize.isEmpty()) {
      return 0;
    }
    return TextHelper.getTextWidth(text, fontSize, fontFamily);
  }

  @CalledByNative
  static private String getFirstLineText(
      String text, String fontSize, String fontFamily, String maxWidth) {
    // fontSize's unit is px.
    if (text.isEmpty() || fontSize.isEmpty()) {
      return "";
    }
    return TextHelper.getFirstLineText(text, fontSize, fontFamily, maxWidth);
  }
}
