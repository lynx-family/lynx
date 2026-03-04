// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fontface;

import android.util.Pair;
import com.lynx.tasm.behavior.shadow.text.TypefaceCache;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

class FontFaceGroup {
  private final ConcurrentLinkedQueue<Pair<TypefaceCache.TypefaceListener, Integer>> mListeners =
      new ConcurrentLinkedQueue<>();
  private final Set<FontFace> mFontFaces =
      Collections.newSetFromMap(new ConcurrentHashMap<FontFace, Boolean>());

  void addListener(Pair<TypefaceCache.TypefaceListener, Integer> listener) {
    if (listener == null) {
      return;
    }
    mListeners.offer(listener);
  }

  Set<FontFace> getFontFaces() {
    return mFontFaces;
  }

  List<Pair<TypefaceCache.TypefaceListener, Integer>> getListeners() {
    return new ArrayList<>(mListeners);
  }

  List<Pair<TypefaceCache.TypefaceListener, Integer>> drainListeners() {
    List<Pair<TypefaceCache.TypefaceListener, Integer>> drained = new ArrayList<>();
    Pair<TypefaceCache.TypefaceListener, Integer> item;
    while ((item = mListeners.poll()) != null) {
      drained.add(item);
    }
    return drained;
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
