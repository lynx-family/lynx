// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.loader;

import android.content.Context;
import android.graphics.Typeface;
import android.text.TextUtils;
import android.util.Base64;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.fontface.FontFace;
import com.lynx.tasm.utils.TypefaceUtils;

public class LynxFontFaceLoader {
  public static abstract class Loader {
    public final Typeface loadFontFace(LynxContext context, FontFace.TYPE type, String src) {
      return onLoadFontFace(context, type, src);
    }

    /**
     * Recommend asynchronous processing
     *
     * @param context
     * @param src
     */
    protected Typeface onLoadFontFace(
        final LynxContext context, FontFace.TYPE type, final String src) {
      if (TextUtils.isEmpty(src) || type == FontFace.TYPE.LOCAL) {
        return null;
      }
      final String identify = "base64,";
      final int index = src.indexOf(identify);
      if (!src.startsWith("data:") || index == -1) {
        return null;
      }
      final String encoded = src.substring(index + identify.length());
      try {
        byte[] bytes = Base64.decode(encoded, Base64.DEFAULT);
        return TypefaceUtils.createFromBytes(context, bytes);
      } catch (Exception e) {
        context.reportResourceError(
            LynxSubErrorCode.E_RESOURCE_FONT_BASE64_PARSING_ERROR, src, "font", e.getMessage());
        return null;
      }
    }

    protected void reportException(LynxContext lynxContext, String errMsg) {
      lynxContext.reportResourceError(errMsg);
    }
  }

  private static Loader sLoader = new Loader() {};

  public static void setLoader(Loader loader) {
    if (loader == null) {
      sLoader = new Loader() {};
    } else {
      sLoader = loader;
    }
  }

  public static Loader getLoader(LynxContext lynxContext) {
    if (lynxContext.getFontLoader() != null) {
      return lynxContext.getFontLoader();
    }
    return sLoader;
  }
}
