// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

import static androidx.core.view.ViewCompat.TYPE_NON_TOUCH;

import android.content.Context;
import android.view.Choreographer;
import android.view.animation.Interpolator;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.behavior.ui.list.container.ListCustomScroller;

public class LynxBaseScrollViewScroller implements Runnable {
  public interface ScrollFinishedCallback {
    void finished(boolean completed);
  }

  public static final int NESTED_SCROLL_MODE_SELF_ONLY = 0;
  public static final int NESTED_SCROLL_MODE_SELF_FIRST = 1;
  public static final int NESTED_SCROLL_MODE_PARENT_FIRST = 2;
  public static final int NESTED_SCROLL_MODE_PARALLEL = 3;
  protected static final int TYPE_EXTEND_FLING = 0x7FFFFFF0;
  protected static final int TYPE_EXTEND_BOUNCING_BACKWARDS = 0x7FFFFFF2;
  protected final ListCustomScroller mProgrammaticScroller;
  private ScrollFinishedCallback mScrollFinishedCallback;
  private int mScrollType = TYPE_NON_TOUCH;
  private int mAutoScrollRate = 0;
  private LynxAutoScrollFrameCallback mAutoScrollCallback = null;

  protected LynxBaseScrollViewNestedInternal scrollView;

  class LynxAutoScrollFrameCallback implements Choreographer.FrameCallback {
    @Override
    public void doFrame(long frameTimeNanos) {
      boolean canScroll =
          mAutoScrollRate >= 0 ? scrollView.canScrollForwards() : scrollView.canScrollBackwards();
      if (canScroll) {
        int[] delta = {mAutoScrollRate, mAutoScrollRate};
        scrollView.scrollBy(delta);
        if (mAutoScrollCallback != null) {
          Choreographer.getInstance().postFrameCallback(mAutoScrollCallback);
        }
      } else {
        stopAutoScroll();
      }
    }
  }

  public LynxBaseScrollViewScroller(Context context) {
    mProgrammaticScroller = new ListCustomScroller(context, new Interpolator() {
      @Override
      public float getInterpolation(float t) {
        t -= 1.0f;
        return t * t * t * t * t + 1.0f;
      }
    });
  }

  private void startAutoScrollInternal(int rate) {
    stopAutoScroll();
    mAutoScrollRate = rate;
    mAutoScrollCallback = new LynxAutoScrollFrameCallback();
    Choreographer.getInstance().postFrameCallback(mAutoScrollCallback);
  }

  public void stopAutoScroll() {
    if (mAutoScrollCallback != null) {
      Choreographer.getInstance().removeFrameCallback(mAutoScrollCallback);
      mAutoScrollCallback = null;
    }
  }

  public boolean tryBouncesBack(LynxBaseScrollViewNestedInternal view, boolean isVertical,
      @Nullable ScrollFinishedCallback callback) {
    if (view.bounces()) {
      int scrollOffset =
          isVertical ? view.getScrollOffsetVertically() : view.getScrollOffsetHorizontally();
      int[] scrollRange =
          isVertical ? view.getScrollRangeVertically() : view.getScrollRangeHorizontally();
      if (view.isBouncingForwards(scrollOffset, scrollRange)) {
        bouncesBack(view, -10000, scrollOffset, scrollRange, isVertical, callback);
        return true;
      } else if (view.isBouncingBackwards(scrollOffset, scrollRange)) {
        bouncesBack(view, 10000, scrollOffset, scrollRange, isVertical, callback);
        return true;
      }
    } else {
      LynxBaseScrollViewNestedInternal parent =
          view.getNestedScrollingParentForType(TYPE_NON_TOUCH);
      if (parent != null) {
        return tryBouncesBack(parent, isVertical, callback);
      }
    }
    return false;
  }

  private void bouncesBack(LynxBaseScrollViewInternal view, int velocity, int scrollOffset,
      int[] scrollRange, boolean isVertical, @Nullable ScrollFinishedCallback callback) {
    stopAnimating(false);
    view.startNestedScroll(getNestedScrollAxis(), TYPE_NON_TOUCH);

    scrollOffset =
        isVertical ? view.getScrollOffsetVertically() : view.getScrollOffsetHorizontally();
    scrollRange = isVertical ? view.getScrollRangeVertically() : view.getScrollRangeHorizontally();

    mScrollFinishedCallback = callback;

    if (velocity >= 0) {
      mProgrammaticScroller.fling(view.getScrollX(), view.getScrollY(), isVertical ? 0 : velocity,
          isVertical ? velocity : 0, Integer.MIN_VALUE, Integer.MAX_VALUE, Integer.MIN_VALUE,
          Integer.MAX_VALUE, 0, 0, -scrollOffset - scrollRange[0], 0);
    } else {
      mProgrammaticScroller.fling(view.getScrollX(), view.getScrollY(), isVertical ? 0 : velocity,
          isVertical ? velocity : 0, Integer.MIN_VALUE, Integer.MAX_VALUE, Integer.MIN_VALUE,
          Integer.MAX_VALUE, 0, 0, 0, scrollOffset - scrollRange[1]);
    }
    mScrollType = TYPE_EXTEND_BOUNCING_BACKWARDS;
    postOnAnimationCompat();
  }

  public void autoScrollTo(int rate, boolean autoStop, @Nullable ScrollFinishedCallback callback) {
    stopAnimating(false);
    startAutoScrollInternal(rate);
    mScrollFinishedCallback = callback;
  }

  public void animatedScrollTo(
      int offsetX, int offsetY, @Nullable ScrollFinishedCallback callback) {
    stopAnimating(false);
    mProgrammaticScroller.startScroll(scrollView.getScrollX(), scrollView.getScrollY(),
        offsetX - scrollView.getScrollX(), offsetY - scrollView.getScrollY());
    mScrollFinishedCallback = callback;
    mScrollType = TYPE_NON_TOUCH;
    postOnAnimationCompat();
  }

  public void fling(int velocityX, int velocityY, @Nullable ScrollFinishedCallback callback) {
    stopAnimating(false);
    scrollView.startNestedScroll(getNestedScrollAxis(), TYPE_NON_TOUCH);
    int[] scrollOffset = scrollView.getScrollOffset();
    int[] scrollRange = scrollView.getScrollRange();
    boolean isVertical = scrollView.isVertical();

    mScrollFinishedCallback = callback;

    int minX =
        scrollOffset[0]; // mEnableBounces? (scrollOffset[0] < scrollRange[0] ? scrollOffset[0] -
    // 50 : scrollRange[0] - 300) : Integer.MIN_VALUE;
    int maxX =
        scrollOffset[0]; // mEnableBounces? (scrollOffset[0] > scrollRange[1] ? scrollOffset[0] +
    // 50 : scrollRange[1] + 300) : Integer.MAX_VALUE;
    int minY =
        scrollOffset[1]; // mEnableBounces? (scrollOffset[1] < scrollRange[2] ? scrollOffset[1] -
    // 50 : scrollRange[2] - 300) : Integer.MIN_VALUE;
    int maxY =
        scrollOffset[1]; // mEnableBounces? (scrollOffset[1] > scrollRange[3] ? scrollOffset[1] +
    // 50 : scrollRange[3] + 300) : Integer.MAX_VALUE;

    int[] flingDist = calculateMaxFlingDistance(
        scrollView, isVertical ? velocityY > 0 : velocityX > 0, isVertical);

    if (scrollView.hasNestedScrollingParent(TYPE_NON_TOUCH)) {
      LynxBaseScrollViewNestedInternal scrollParent =
          scrollView.getNestedScrollingParentForType(TYPE_NON_TOUCH);
      int velocity = isVertical ? velocityY : velocityX;
      if (velocity != 0 && scrollParent instanceof LynxBaseScrollViewNestedInternal) {
        int[] parentFlingDist = calculateMaxFlingDistance(scrollParent,
            ((LynxBaseScrollView) scrollParent).isVertical() ? velocityY > 0 : velocityX > 0,
            ((LynxBaseScrollView) scrollParent).isVertical());

        switch (velocity > 0 ? scrollView.getForwardNestedScrollMode()
                             : scrollView.getBackwardNestedScrollMode()) {
          case NESTED_SCROLL_MODE_PARALLEL:
            flingDist[0] = Math.min(flingDist[0], parentFlingDist[0]);
            flingDist[1] = Math.max(flingDist[1], parentFlingDist[1]);
            break;
          default:
            break;
        }
      }
    }

    if (isVertical) {
      minY = flingDist[0] + scrollOffset[1];
      maxY = flingDist[1] + scrollOffset[1];
    } else {
      minX = flingDist[0] + scrollOffset[0];
      maxX = flingDist[1] + scrollOffset[0];
    }

    mProgrammaticScroller.fling(scrollView.getScrollX(), scrollView.getScrollY(), velocityX,
        velocityY, minX, maxX, minY, maxY);
    ;

    mScrollType = TYPE_EXTEND_FLING;
    postOnAnimationCompat();
  }

  private int[] calculateMaxFlingDistance(
      LynxBaseScrollViewNestedInternal view, boolean isForward, boolean isVertical) {
    int[] maxFlingRange = view.getFlingRange(isVertical);
    int scrollOffset =
        isVertical ? view.getScrollOffsetVertically() : view.getScrollOffsetHorizontally();
    LynxBaseScrollViewNestedInternal parent = view.getNestedScrollingParentForType(TYPE_NON_TOUCH);

    switch (isForward ? view.getForwardNestedScrollMode() : view.getBackwardNestedScrollMode()) {
      case NESTED_SCROLL_MODE_SELF_ONLY:
        break;
      case NESTED_SCROLL_MODE_SELF_FIRST:
        if (parent != null) {
          int[] parentMaxFlingDistance = calculateMaxFlingDistance(parent, isForward, isVertical);
          maxFlingRange[0] += parentMaxFlingDistance[0];
          maxFlingRange[1] += parentMaxFlingDistance[1];
        }
        break;
      case NESTED_SCROLL_MODE_PARENT_FIRST:
        if (parent != null) {
          int[] parentMaxFlingDistance = calculateMaxFlingDistance(parent, isForward, isVertical);
          maxFlingRange[0] += parentMaxFlingDistance[0];
          maxFlingRange[1] += parentMaxFlingDistance[1];
        }
        break;
      case NESTED_SCROLL_MODE_PARALLEL:
        if (parent != null) {
          int[] parentMaxFlingDistance = calculateMaxFlingDistance(parent, isForward, isVertical);
          maxFlingRange[0] = Math.max(maxFlingRange[0], parentMaxFlingDistance[0]);
          maxFlingRange[1] = Math.max(maxFlingRange[1], parentMaxFlingDistance[1]);
        }
        break;
    }

    return new int[] {maxFlingRange[0] - scrollOffset, maxFlingRange[1] - scrollOffset};
  }

  public void stopAnimating(boolean completed) {
    scrollView.getView().removeCallbacks(this);
    mProgrammaticScroller.abortAnimation();
    if (completed) {
      //        stopNestedScroll(TYPE_NON_TOUCH);
    }
    if (mScrollFinishedCallback != null) {
      ScrollFinishedCallback callback = mScrollFinishedCallback;
      mScrollFinishedCallback = null;
      callback.finished(completed);
    }
  }

  @Override
  public void run() {
    if (mProgrammaticScroller.computeScrollOffset()) {
      boolean isVertical = scrollView.isVertical();
      scrollView.dispatchScroll(
          isVertical ? 0 : mProgrammaticScroller.getCurrX() - mProgrammaticScroller.getPreviousX(),
          isVertical ? mProgrammaticScroller.getCurrY() - mProgrammaticScroller.getPreviousY() : 0,
          mScrollType, null, null);
      postOnAnimationCompat();
    } else {
      stopAnimating(true);
    }
  }

  private void postOnAnimationCompat() {
    scrollView.getView().removeCallbacks(this);
    ViewCompat.postOnAnimation(scrollView.getView(), this);
  }

  private int getNestedScrollAxis() {
    return scrollView.isVertical() ? ViewCompat.SCROLL_AXIS_VERTICAL
                                   : ViewCompat.SCROLL_AXIS_HORIZONTAL;
  }
}
