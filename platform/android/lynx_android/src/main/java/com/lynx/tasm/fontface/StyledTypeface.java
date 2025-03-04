// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fontface;

import android.graphics.Typeface;
import com.lynx.tasm.base.LLog;

class StyledTypeface {
  private Typeface mOriginTypeface;
  private Typeface[] mStyledFontFace = new Typeface[4];
  private static final String TAG = StyledTypeface.class.getSimpleName();

  StyledTypeface(Typeface originTypeface) {
    mOriginTypeface = originTypeface;
    mStyledFontFace[0] = originTypeface;
  }

  Typeface getStyledTypeFace(int style) {
    if (style < 0 || style > 3) {
      style = 0;
    }
    if (mStyledFontFace[style] == null) {
      try {
        mStyledFontFace[style] = Typeface.create(mOriginTypeface, style);
      } catch (ArrayIndexOutOfBoundsException e) {
        LLog.e(TAG,
            "create typeface failed, style: " + style + ", origin typeface: " + mOriginTypeface
                + ", Typeface.create function is not thread safe lower than Android 9, exception: "
                + e);
      }
    }
    return mStyledFontFace[style];
  }

  boolean checkTypefaceHasCreated(int style) {
    return mStyledFontFace[style] != null;
  }
}
