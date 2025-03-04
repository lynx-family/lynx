// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fontface;

import android.util.Pair;
import java.util.ArrayList;
import java.util.List;

public class FontFace {
  public enum TYPE {
    URL,
    LOCAL;
  }

  private String fontFamily;
  private List<Pair<TYPE, String>> src = new ArrayList<>();
  private StyledTypeface mStyledTypeface;

  void setStyledTypeface(StyledTypeface styledTypeface) {
    mStyledTypeface = styledTypeface;
  }

  StyledTypeface getTypeface() {
    return mStyledTypeface;
  }

  List<Pair<TYPE, String>> getSrc() {
    return src;
  }

  public void setFontFamily(String fontFamily) {
    this.fontFamily = fontFamily;
  }

  public void addUrl(String url) {
    src.add(new Pair<>(TYPE.URL, url));
  }

  public void addLocal(String local) {
    src.add(new Pair<>(TYPE.LOCAL, local));
  }

  boolean isSameFontFace(FontFace fontFace) {
    if (this == fontFace) {
      return true;
    }
    for (Pair<TYPE, String> myself : src) {
      for (Pair<TYPE, String> that : fontFace.src) {
        if (myself.equals(that)) {
          return true;
        }
      }
    }
    return false;
  }
}
