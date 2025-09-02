// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class LynxBaseScrollViewScrolling extends FrameLayout implements LynxBaseScrollViewPublic {
  protected LynxBaseScrollViewScroller mScrollHelper;
  private boolean mEnableBounces = true;

  public interface ScrollDelegate {
    void onScrollStateChanged(int from, int to);
    void scrollViewDidScroll(LynxBaseScrollViewScrolling scrollView);
  }

  public static final int SCROLL_STATE_IDLE = 0;
  public static final int SCROLL_STATE_DRAGGING = 1;
  public static final int SCROLL_STATE_ANIMATING = 2;
  public static final int SCROLL_STATE_FLING = 3;

  public boolean mIsVertical = true;

  public int mWidth = 0;

  public int mHeight = 0;

  private boolean mEnableScroll = true;

  protected final int[] mContentSize = new int[2];

  protected int mScrollState = SCROLL_STATE_IDLE;

  protected ScrollDelegate mScrollDelegate;

  private LynxBaseScrollViewScroller.ScrollFinishedCallback mProgrammaticallyScrollFinishedCallback;

  public LynxBaseScrollViewScrolling(
      @NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
    mScrollHelper = new LynxBaseScrollViewScroller(context);
  }

  @Override
  public void enableScroll(boolean enable) {
    mEnableScroll = enable;
  }

  @Override
  public boolean scrollEnabled() {
    return mEnableScroll;
  }

  @Override
  public void stopScrolling() {
    mScrollHelper.stopAnimating(false);
    mScrollHelper.stopAutoScroll();
    tryToUpdateScrollState(SCROLL_STATE_IDLE);
  }

  @Override
  public int[] getScrollOffset() {
    return new int[] {getScrollX(), getScrollY()};
  }

  @Override
  public void setScrollContentSize(int[] contentSize) {
    if (mIsVertical) {
      setScrollContentSizeVertically(contentSize[1]);
    } else {
      setScrollContentSizeHorizontally(contentSize[0]);
    }
  }

  @Override
  public void scrollByUnlimited(int[] delta) {
    if (mIsVertical) {
      scrollByUnlimitedVertically(delta[1]);
    } else {
      scrollByUnlimitedHorizontally(delta[0]);
    }
  }

  @Override
  public void scrollBy(int[] delta) {
    if (mIsVertical) {
      scrollByVertically(delta[1]);
    } else {
      scrollByHorizontally(delta[0]);
    }
  }

  @Override
  public void scrollToUnlimited(int[] offset) {
    if (mIsVertical) {
      scrollToUnlimitedVertically(offset[1]);
    } else {
      scrollToUnlimitedHorizontally(offset[0]);
    }
  }

  @Override
  public void scrollTo(int[] offset) {
    if (mIsVertical) {
      scrollToVertically(offset[1]);
    } else {
      scrollToHorizontally(offset[0]);
    }
  }

  @Override
  public void animatedScrollTo(
      int[] offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback) {
    if (mIsVertical) {
      animatedScrollToVertically(offset[1], callback);
    } else {
      animatedScrollToHorizontally(offset[0], callback);
    }
  }

  @Override
  public void animatedScrollToUnlimited(
      int[] offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback) {
    if (mIsVertical) {
      animatedScrollToUnlimitedVertically(offset[1], callback);
    } else {
      animatedScrollToUnlimitedHorizontally(offset[0], callback);
    }
  }

  @Override
  public int[] getScrollRange() {
    int[] scrollRangeHorizontal = getScrollRangeHorizontally();
    int[] scrollRangeVertical = getScrollRangeVertically();

    return new int[] {
        scrollRangeHorizontal[0],
        scrollRangeHorizontal[1],
        scrollRangeVertical[0],
        scrollRangeVertical[1],
    };
  }

  @Override
  public boolean canScrollForwards() {
    return mIsVertical ? canScrollForwardsVertically() : canScrollForwardsHorizontally();
  }

  @Override
  public boolean canScrollBackwards() {
    return mIsVertical ? canScrollBackwardsVertically() : canScrollBackwardsHorizontally();
  }

  @Override
  public int getScrollOffsetHorizontally() {
    return getScrollX();
  }

  @Override
  public void setScrollContentSizeHorizontally(int contentSize) {
    mContentSize[0] = contentSize;
    mContentSize[1] = mHeight;
  }

  @Override
  public void scrollByUnlimitedHorizontally(int delta) {
    scrollToUnlimitedHorizontally(getScrollX() + delta);
  }

  @Override
  public void scrollByHorizontally(int delta) {
    scrollToHorizontally(getScrollX() + delta);
  }

  @Override
  public void scrollToUnlimitedHorizontally(int offset) {
    scrollTo(offset, getScrollY());
  }

  @Override
  public void scrollToHorizontally(int offset) {
    int[] scrollRange = getScrollRangeHorizontally();
    offset = Math.min(Math.max(offset, scrollRange[0]), scrollRange[1]);
    scrollToUnlimitedHorizontally(offset);
  }

  @Override
  public void animatedScrollToHorizontally(
      int offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback) {
    int[] scrollRange = getScrollRangeHorizontally();
    offset = Math.min(Math.max(offset, scrollRange[0]), scrollRange[1]);
    animatedScrollToUnlimitedHorizontally(offset, callback);
  }

  @Override
  public void animatedScrollToUnlimitedHorizontally(
      int offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback) {
    mScrollHelper.animatedScrollTo(
        offset, getScrollY(), new LynxBaseScrollViewScroller.ScrollFinishedCallback() {
          @Override
          public void finished(boolean completed) {
            if (completed) {
              tryToUpdateScrollState(SCROLL_STATE_IDLE);
              updateProgrammaticallyScrollFinishedCallback(null);
            }
          }
        });
    tryToUpdateScrollState(SCROLL_STATE_ANIMATING);
    updateProgrammaticallyScrollFinishedCallback(callback);
  }

  @Override
  public int[] getScrollRangeHorizontally() {
    return new int[] {0, Math.max(0, mContentSize[0] - mWidth)};
  }

  @Override
  public boolean canScrollForwardsHorizontally() {
    int offset = getScrollOffsetHorizontally();
    int[] scrollRange = getScrollRangeHorizontally();
    if (offset >= scrollRange[1]) {
      return false;
    } else {
      return true;
    }
  }

  @Override
  public boolean canScrollBackwardsHorizontally() {
    int offset = getScrollOffsetHorizontally();
    int[] scrollRange = getScrollRangeHorizontally();
    if (offset <= scrollRange[0]) {
      return false;
    } else {
      return true;
    }
  }

  @Override
  public int getScrollOffsetVertically() {
    return getScrollY();
  }

  @Override
  public void setScrollContentSizeVertically(int contentSize) {
    mContentSize[0] = mWidth;
    mContentSize[1] = contentSize;
  }

  @Override
  public void scrollByUnlimitedVertically(int delta) {
    scrollToUnlimitedVertically(getScrollY() + delta);
  }

  @Override
  public void scrollByVertically(int delta) {
    scrollToVertically(getScrollY() + delta);
  }

  @Override
  public void scrollToUnlimitedVertically(int offset) {
    scrollTo(getScrollX(), offset);
  }

  @Override
  public void scrollToVertically(int offset) {
    int[] scrollRange = getScrollRangeVertically();
    offset = Math.min(Math.max(offset, scrollRange[0]), scrollRange[1]);
    scrollToUnlimitedVertically(offset);
  }

  @Override
  public void animatedScrollToVertically(
      int offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback) {
    int[] scrollRange = getScrollRangeVertically();
    offset = Math.min(Math.max(offset, scrollRange[0]), scrollRange[1]);
    animatedScrollToUnlimitedVertically(offset, callback);
  }

  @Override
  public void animatedScrollToUnlimitedVertically(
      int offset, LynxBaseScrollViewScroller.ScrollFinishedCallback callback) {
    mScrollHelper.animatedScrollTo(
        getScrollX(), offset, new LynxBaseScrollViewScroller.ScrollFinishedCallback() {
          @Override
          public void finished(boolean completed) {
            if (completed) {
              tryToUpdateScrollState(SCROLL_STATE_IDLE);
              updateProgrammaticallyScrollFinishedCallback(null);
            }
          }
        });
    tryToUpdateScrollState(SCROLL_STATE_ANIMATING);
    updateProgrammaticallyScrollFinishedCallback(callback);
  }

  @Override
  public int[] getScrollRangeVertically() {
    return new int[] {0, Math.max(0, mContentSize[1] - mHeight)};
  }

  @Override
  public boolean canScrollForwardsVertically() {
    int offset = getScrollOffsetVertically();
    int[] scrollRange = getScrollRangeVertically();
    if (offset >= scrollRange[1]) {
      return false;
    } else {
      return true;
    }
  }

  @Override
  public boolean canScrollBackwardsVertically() {
    int offset = getScrollOffsetVertically();
    int[] scrollRange = getScrollRangeVertically();
    if (offset <= scrollRange[0]) {
      return false;
    } else {
      return true;
    }
  }

  public int currentScrollState() {
    return mScrollState;
  }

  @Override
  public void enableBounces(boolean enableBounces) {
    mEnableBounces = enableBounces;
  }

  @Override
  public boolean bounces() {
    return mEnableBounces;
  }

  protected void tryToUpdateScrollState(int newState) {
    int oldState = mScrollState;
    if (oldState != newState) {
      mScrollState = newState;
      if (mScrollDelegate != null) {
        mScrollDelegate.onScrollStateChanged(oldState, newState);
      }
    }
    if (newState != SCROLL_STATE_ANIMATING) {
      updateProgrammaticallyScrollFinishedCallback(null);
    }
  }

  protected void setScrollDelegate(ScrollDelegate delegate) {
    mScrollDelegate = delegate;
  }

  protected void updateProgrammaticallyScrollFinishedCallback(
      LynxBaseScrollViewScroller.ScrollFinishedCallback callback) {
    if (mProgrammaticallyScrollFinishedCallback != callback) {
      if (mProgrammaticallyScrollFinishedCallback != null) {
        mProgrammaticallyScrollFinishedCallback.finished(callback == null);
      }
      mProgrammaticallyScrollFinishedCallback = callback;
    }
  }
}
