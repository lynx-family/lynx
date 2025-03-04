// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import android.graphics.PointF;
import android.text.Layout;
import androidx.annotation.ColorInt;
import java.util.Set;

public class TextUpdateBundle {
  private final boolean mHasImages;
  private final boolean mIsJustify;

  private Layout mTextLayout;
  private Set mViewTruncatedSet;
  @ColorInt private int mSelectionColor = 0;
  private PointF mTextTranslateOffset;

  private boolean mNeedDrawStroke;
  private CharSequence mOriginText;

  public TextUpdateBundle(
      Layout layout, boolean containsImages, Set viewTruncatedSet, boolean isJustify) {
    mTextLayout = layout;
    mHasImages = containsImages;
    mViewTruncatedSet = viewTruncatedSet;
    mIsJustify = isJustify;
  }

  public Layout getTextLayout() {
    return mTextLayout;
  }

  public boolean hasImages() {
    return mHasImages;
  }

  public Set getViewTruncatedSet() {
    return mViewTruncatedSet;
  }

  public void setSelectionColor(@ColorInt int color) {
    mSelectionColor = color;
  }

  public void setTextTranslateOffset(PointF offset) {
    mTextTranslateOffset = offset;
  }

  public void setNeedDrawStroke(boolean need) {
    mNeedDrawStroke = need;
  }

  public boolean getNeedDrawStroke() {
    return mNeedDrawStroke;
  }

  public @ColorInt int getSelectionColor() {
    return mSelectionColor;
  }

  public PointF getTextTranslateOffset() {
    return mTextTranslateOffset;
  }

  public boolean isJustify() {
    return mIsJustify;
  }

  public void setOriginText(CharSequence string) {
    mOriginText = string;
  }

  public CharSequence getOriginText() {
    return mOriginText;
  }
}
