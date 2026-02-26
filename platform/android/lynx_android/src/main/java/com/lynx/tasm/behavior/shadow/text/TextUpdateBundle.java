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
  private PointF mTextTranslateOffset;

  private boolean mNeedDrawStroke;
  private CharSequence mOriginText;

  // Layout event related fields for new text pipeline
  private int mLayoutEventLineCount;
  private int mLayoutEventEllipsisCount;
  private int mLayoutEventTextOverflow;
  private int mLayoutEventSpannableStringLength;
  private float mLayoutEventTextLayoutWidth;
  private boolean mLayoutEventContainTextSize;
  private boolean mLayoutEventDispatched;

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

  public void setTextTranslateOffset(PointF offset) {
    mTextTranslateOffset = offset;
  }

  public void setNeedDrawStroke(boolean need) {
    mNeedDrawStroke = need;
  }

  public boolean getNeedDrawStroke() {
    return mNeedDrawStroke;
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

  public void setViewTruncatedSet(Set viewTruncatedSet) {
    mViewTruncatedSet = viewTruncatedSet;
  }

  // -------- Layout event helpers (new text pipeline) --------

  public void setLayoutEventParams(int textOverflow, int lineCount, int ellipsisCount,
      int spannableStringLength, float textLayoutWidth, boolean containTextSize) {
    mLayoutEventTextOverflow = textOverflow;
    mLayoutEventLineCount = lineCount;
    mLayoutEventEllipsisCount = ellipsisCount;
    mLayoutEventSpannableStringLength = spannableStringLength;
    mLayoutEventTextLayoutWidth = textLayoutWidth;
    mLayoutEventContainTextSize = containTextSize;
  }

  public int getLayoutEventLineCount() {
    return mLayoutEventLineCount;
  }

  public int getLayoutEventEllipsisCount() {
    return mLayoutEventEllipsisCount;
  }

  public int getLayoutEventTextOverflow() {
    return mLayoutEventTextOverflow;
  }

  public int getLayoutEventSpannableStringLength() {
    return mLayoutEventSpannableStringLength;
  }

  public float getLayoutEventTextLayoutWidth() {
    return mLayoutEventTextLayoutWidth;
  }

  public boolean isLayoutEventContainTextSize() {
    return mLayoutEventContainTextSize;
  }

  public boolean hasLayoutEventParams() {
    return mLayoutEventLineCount > 0 && mTextLayout != null;
  }

  public boolean hasDispatchedLayoutEvent() {
    return mLayoutEventDispatched;
  }

  public void markLayoutEventDispatched() {
    mLayoutEventDispatched = true;
  }
}
