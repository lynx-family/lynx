// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.text.SpannableStringBuilder;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class AttributedTextBundle {
  final private CharSequence mSpan;
  final private TextAttributes mTextAttributes;
  private HashMap<Integer, NativeLayoutNodeSpan> mInlineViewMap;
  private final CharSequence mSource;
  private final CharSequence mTruncationSpan;

  public AttributedTextBundle(CharSequence span, TextAttributes textAttributes) {
    this(span, textAttributes, -1);
  }

  AttributedTextBundle(CharSequence span, TextAttributes textAttributes, int bodyLength) {
    boolean hasToken = bodyLength >= 0 && bodyLength < span.length();
    mSpan = hasToken ? span.subSequence(0, bodyLength) : span;
    mSource = span;
    mTruncationSpan = hasToken ? span.subSequence(bodyLength, span.length()) : null;
    mTextAttributes = textAttributes;
  }

  public CharSequence getSpan() {
    return mSpan;
  }

  public TextAttributes getTextAttributes() {
    return mTextAttributes;
  }

  CharSequence getTruncationSpan() {
    return mTruncationSpan;
  }

  SpannableStringBuilder buildTruncatedSpan(int end) {
    return new SpannableStringBuilder(mSource).delete(end, mSpan.length());
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
