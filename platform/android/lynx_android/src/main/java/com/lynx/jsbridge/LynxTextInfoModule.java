// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.text.TextUtils;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextHelper;

public class LynxTextInfoModule extends LynxContextModule {
  public static final String NAME = "LynxTextInfoModule";

  public LynxTextInfoModule(LynxContext context) {
    super(context);
  }

  public LynxTextInfoModule(LynxContext context, Object param) {
    super(context, param);
  }

  @LynxMethod
  WritableMap getTextInfo(final String text, final ReadableMap params) {
    String fontSize = params.getString("fontSize");
    if (TextUtils.isEmpty(fontSize)) {
      JavaOnlyMap ret = new JavaOnlyMap();
      ret.putInt("width", 0);
      return ret;
    }
    String fontFamily = params.getString("fontFamily");
    String maxWidth = params.getString("maxWidth");
    int maxLine = 1;
    if (params.hasKey("maxLine")) {
      maxLine = params.getInt("maxLine");
    }
    return TextHelper.getTextInfo(text, fontSize, fontFamily, maxWidth, maxLine);
  }
}
