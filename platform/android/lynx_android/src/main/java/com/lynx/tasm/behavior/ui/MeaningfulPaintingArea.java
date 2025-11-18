// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import android.view.View;

public class MeaningfulPaintingArea {
  public interface IMeaningfulPaintingAreaInvalidateHook {
    void invalidateMeaningfulPaintingArea();
    IDrawChildHook getDrawChildHook();
  }

  int mOffsetX;
  int mOffsetY;
  int mWidth;
  int mHeight;
  boolean mIsValid = true;

  int mVisibleStatus = View.VISIBLE;
  float mAlpha = 1;
  float mScaleX = 1;
  float mScaleY = 1;
  ILynxUIMeaningfulContent.MeaningfulContentStatus mMeaningfulContentStatus =
      ILynxUIMeaningfulContent.MeaningfulContentStatus.IRRELEVANT;
  long mFirstMeaningfulContentPresentedTimestampMicros = -1;

  public void setMeaningfulContentStatus(ILynxUIMeaningfulContent.MeaningfulContentStatus status) {
    mMeaningfulContentStatus = status;
  }

  public ILynxUIMeaningfulContent.MeaningfulContentStatus getMeaningfulContentStatus() {
    return mMeaningfulContentStatus;
  }

  public long getFirstMeaningfulContentPresentedTimestampMicros() {
    return mFirstMeaningfulContentPresentedTimestampMicros;
  }

  public void setFirstMeaningfulContentPresentedTimestampMicros(long timestampMicros) {
    mFirstMeaningfulContentPresentedTimestampMicros = timestampMicros;
  }

  public MeaningfulPaintingArea(int offsetX, int offsetY, int width, int height, boolean isValid) {
    mOffsetX = offsetX;
    mOffsetY = offsetY;
    mWidth = width;
    mHeight = height;
    mIsValid = isValid;
  }

  public int getOffsetX() {
    return mOffsetX;
  }

  public int getOffsetY() {
    return mOffsetY;
  }

  public int getWidth() {
    return mWidth;
  }

  public int getHeight() {
    return mHeight;
  }

  // Only image may return false.
  public boolean isValid() {
    return mIsValid;
  }

  public float getAlpha() {
    return mAlpha;
  }

  public void setAlpha(float alpha) {
    mAlpha = alpha;
  }

  public float getScaleX() {
    return mScaleX;
  }

  public void setScaleX(float scaleX) {
    mScaleX = scaleX;
  }

  public float getScaleY() {
    return mScaleY;
  }

  public void setScaleY(float scaleY) {
    mScaleY = scaleY;
  }

  public int getVisibleStatus() {
    return mVisibleStatus;
  }

  public void setVisibleStatus(int status) {
    mVisibleStatus = status;
  }
}
