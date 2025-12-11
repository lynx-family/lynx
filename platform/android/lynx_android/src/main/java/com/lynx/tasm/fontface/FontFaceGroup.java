// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fontface;

import android.util.Pair;
import com.lynx.tasm.behavior.shadow.text.TypefaceCache;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CopyOnWriteArraySet;

class FontFaceGroup {
  private final CopyOnWriteArrayList<Pair<TypefaceCache.TypefaceListener, Integer>> mListeners =
      new CopyOnWriteArrayList<>();
  private final CopyOnWriteArraySet<FontFace> mFontFaces = new CopyOnWriteArraySet<>();

  void addListener(Pair<TypefaceCache.TypefaceListener, Integer> listener) {
    if (listener == null) {
      return;
    }
    mListeners.add(listener);
  }

  Set<FontFace> getFontFaces() {
    return mFontFaces;
  }

  List<Pair<TypefaceCache.TypefaceListener, Integer>> getListeners() {
    return mListeners;
  }

  List<Pair<TypefaceCache.TypefaceListener, Integer>> drainListeners() {
    List<Pair<TypefaceCache.TypefaceListener, Integer>> snapshot = new ArrayList<>(mListeners);
    mListeners.clear();
    return snapshot;
  }

  void addFontFace(FontFace fontFace) {
    mFontFaces.add(fontFace);
  }

  boolean isSameFontFace(FontFace fontFace) {
    if (mFontFaces.contains(fontFace)) {
      return true;
    }
    for (FontFace face : mFontFaces) {
      if (face.isSameFontFace(fontFace)) {
        return true;
      }
    }
    return false;
  }
}
