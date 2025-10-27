// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class AttributedTextBundle {
  final private CharSequence mSpan;
  final private TextAttributes mTextAttributes;
  private HashMap<Integer, NativeLayoutNodeSpan> mInlineViewMap;

  public AttributedTextBundle(CharSequence span, TextAttributes textAttributes) {
    mSpan = span;
    mTextAttributes = textAttributes;
  }

  public CharSequence getSpan() {
    return mSpan;
  }

  public TextAttributes getTextAttributes() {
    return mTextAttributes;
  }

  public void setInlineViewMap(HashMap<Integer, NativeLayoutNodeSpan> map) {
    mInlineViewMap = map;
  }

  public NativeLayoutNodeSpan getNativeLayoutNodeSpan(int sign) {
    if (mInlineViewMap == null) {
      return null;
    }
    return mInlineViewMap.get(sign);
  }

  public Set<Map.Entry<Integer, NativeLayoutNodeSpan>> getNativeLayoutNodeSpans() {
    if (mInlineViewMap == null || mInlineViewMap.isEmpty()) {
      return null;
    }

    return mInlineViewMap.entrySet();
  }
}
