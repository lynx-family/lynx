// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation.layout;

import android.graphics.Rect;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.Transformation;
import com.lynx.tasm.behavior.ui.LynxUI;

public class PositionAndSizeAnimation extends Animation implements LayoutHandlingAnimation {
  private final LynxUI mUI;
  private final View mView;
  private final int mPaddingLeft, mPaddingTop, mPaddingRight, mPaddingBottom;
  private final int mMarginLeft, mMarginTop, mMarginRight, mMarginBottom;
  private final int mBorderLeftWidth, mBorderTopWidth, mBorderRightWidth, mBorderBottomWidth;
  private final Rect mBound;
  private float mStartX, mStartY, mDeltaX, mDeltaY;
  private int mStartWidth, mStartHeight, mDeltaWidth, mDeltaHeight;

  public PositionAndSizeAnimation(LynxUI ui, int x, int y, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, final Rect bound) {
    mUI = ui;
    mView = ui.getView();
    mPaddingLeft = paddingLeft;
    mPaddingTop = paddingTop;
    mPaddingRight = paddingRight;
    mPaddingBottom = paddingBottom;
    mMarginLeft = marginLeft;
    mMarginTop = marginTop;
    mMarginRight = marginRight;
    mMarginBottom = marginBottom;
    mBorderLeftWidth = borderLeftWidth;
    mBorderTopWidth = borderTopWidth;
    mBorderRightWidth = borderRightWidth;
    mBorderBottomWidth = borderBottomWidth;
    mBound = bound;

    calculateAnimation(x, y, width, height);
  }

  @Override
  protected void applyTransformation(float interpolatedTime, Transformation t) {
    float newX = mStartX + mDeltaX * interpolatedTime;
    float newY = mStartY + mDeltaY * interpolatedTime;
    float newWidth = mStartWidth + mDeltaWidth * interpolatedTime;
    float newHeight = mStartHeight + mDeltaHeight * interpolatedTime;
    mUI.updateLayout(Math.round(newX), Math.round(newY), Math.round(newWidth),
        Math.round(newHeight), mPaddingLeft, mPaddingTop, mPaddingRight, mPaddingBottom,
        mMarginLeft, mMarginTop, mMarginRight, mMarginBottom, mBorderLeftWidth, mBorderTopWidth,
        mBorderRightWidth, mBorderBottomWidth, mBound);
  }

  @Override
  public void onLayoutUpdate(int x, int y, int width, int height) {
    calculateAnimation(x, y, width, height);
  }

  @Override
  public boolean willChangeBounds() {
    return true;
  }

  private void calculateAnimation(int x, int y, int width, int height) {
    mStartX = mUI.getOriginLeft() - mUI.getTranslationX();
    mStartY = mUI.getOriginTop() - mUI.getTranslationY();
    mStartWidth = mUI.getWidth();
    mStartHeight = mUI.getHeight();

    mDeltaX = x - mStartX;
    mDeltaY = y - mStartY;
    mDeltaWidth = width - mStartWidth;
    mDeltaHeight = height - mStartHeight;
  }
}
