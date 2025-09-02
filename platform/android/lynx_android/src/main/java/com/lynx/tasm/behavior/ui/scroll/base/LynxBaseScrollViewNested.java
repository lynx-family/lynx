// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

import static androidx.core.view.ViewCompat.TYPE_NON_TOUCH;
import static androidx.core.view.ViewCompat.TYPE_TOUCH;
import static com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARALLEL;
import static com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARENT_FIRST;
import static com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_SELF_FIRST;
import static com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_SELF_ONLY;
import static com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller.TYPE_EXTEND_BOUNCING_BACKWARDS;
import static com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller.TYPE_EXTEND_FLING;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewParent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.NestedScrollingParentHelper;
import androidx.core.view.ViewCompat;

public class LynxBaseScrollViewNested
    extends LynxBaseScrollViewDragging implements LynxBaseScrollViewNestedInternal {
  public final LynxNestedScrollingChildHelper mNestedScrollingChildHelper;
  private final NestedScrollingParentHelper mNestedScrollingParentHelper;
  protected int mForwardNestedScrollMode = NESTED_SCROLL_MODE_SELF_ONLY;
  protected int mBackwardNestedScrollMode = NESTED_SCROLL_MODE_SELF_ONLY;

  public LynxBaseScrollViewNested(
      @NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
    this.mNestedScrollingChildHelper = new LynxNestedScrollingChildHelper(this);
    this.mNestedScrollingParentHelper = new NestedScrollingParentHelper(this);
    mScrollHelper.scrollView = this;
  }

  /******** NestedScrollingChild2 begin *********/

  @Override
  public void setNestedScrollingEnabled(boolean enabled) {
    mNestedScrollingChildHelper.setNestedScrollingEnabled(enabled);
  }

  @Override
  public boolean isNestedScrollingEnabled() {
    return mNestedScrollingChildHelper.isNestedScrollingEnabled();
  }

  @Override
  public boolean hasNestedScrollingParent(int type) {
    return mNestedScrollingChildHelper.hasNestedScrollingParent(type);
  }

  @Override
  public boolean hasNestedScrollingParent() {
    return this.hasNestedScrollingParent(TYPE_TOUCH);
  }

  @Override
  public boolean startNestedScroll(int axes, int type) {
    return mNestedScrollingChildHelper.startNestedScroll(axes, type);
  }

  @Override
  public boolean startNestedScroll(int axes) {
    return this.startNestedScroll(axes, TYPE_TOUCH);
  }

  @Override
  public void stopNestedScroll(int type) {
    mNestedScrollingChildHelper.stopNestedScroll(type);
  }

  @Override
  public void stopNestedScroll() {
    this.stopNestedScroll(TYPE_TOUCH);
  }

  @Override
  public boolean dispatchNestedScroll(int dxConsumed, int dyConsumed, int dxUnconsumed,
      int dyUnconsumed, int[] offsetInWindow, int type) {
    return mNestedScrollingChildHelper.dispatchNestedScroll(
        dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow, type);
  }

  @Override
  public boolean dispatchNestedScroll(
      int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed, int[] offsetInWindow) {
    return this.dispatchNestedScroll(
        dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow, TYPE_TOUCH);
  }

  @Override
  public boolean dispatchNestedPreScroll(
      int dx, int dy, int[] consumed, int[] offsetInWindow, int type) {
    return mNestedScrollingChildHelper.dispatchNestedPreScroll(
        dx, dy, consumed, offsetInWindow, type);
  }

  @Override
  public boolean dispatchNestedPreScroll(int dx, int dy, int[] consumed, int[] offsetInWindow) {
    return this.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, TYPE_TOUCH);
  }

  @Override
  public boolean dispatchNestedPreFling(float velocityX, float velocityY) {
    return mNestedScrollingChildHelper.dispatchNestedPreFling(velocityX, velocityY);
  }

  @Override
  public boolean dispatchNestedFling(float velocityX, float velocityY, boolean consumed) {
    return mNestedScrollingChildHelper.dispatchNestedFling(velocityX, velocityY, consumed);
  }

  /********* NestedScrollingChild2 end *********/

  /********* NestedScrollingParent2 begin *********/
  @Override
  public boolean onStartNestedScroll(
      @NonNull View child, @NonNull View target, int axes, int type) {
    if (mIsVertical) {
      return (axes & ViewCompat.SCROLL_AXIS_VERTICAL) != 0;
    } else {
      return (axes & ViewCompat.SCROLL_AXIS_HORIZONTAL) != 0;
    }
  }

  @Override
  public boolean onStartNestedScroll(View child, View target, int axes) {
    return this.onStartNestedScroll(child, target, axes, TYPE_TOUCH);
  }

  @Override
  public void onNestedScrollAccepted(
      @NonNull View child, @NonNull View target, int axes, int type) {
    mNestedScrollingParentHelper.onNestedScrollAccepted(child, target, axes, type);
    if (mIsVertical) {
      this.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL, type);
    } else {
      this.startNestedScroll(ViewCompat.SCROLL_AXIS_HORIZONTAL, type);
    }
  }

  @Override
  public void onNestedScrollAccepted(View child, View target, int axes) {
    this.onNestedScrollAccepted(child, target, axes, TYPE_TOUCH);
  }

  @Override
  public void onStopNestedScroll(@NonNull View target, int type) {
    mNestedScrollingParentHelper.onStopNestedScroll(target, type);
    this.stopNestedScroll(type);
  }

  @Override
  public void onStopNestedScroll(View child) {
    this.onStopNestedScroll(child, TYPE_TOUCH);
  }

  @Override
  public void onNestedPreScroll(
      @NonNull View target, int dx, int dy, @NonNull int[] consumed, int type) {
    int[] lefts = dispatchScroll(mIsVertical ? 0 : dx, mIsVertical ? dy : 0, type, null, null);
    consumed[0] = dx - lefts[0];
    consumed[1] = dy - lefts[1];
  }

  @Override
  public void onNestedPreScroll(View target, int dx, int dy, int[] consumed) {
    this.onNestedPreScroll(target, dx, dy, consumed, TYPE_TOUCH);
  }

  @Override
  public void onNestedScroll(@NonNull View target, int dxConsumed, int dyConsumed, int dxUnconsumed,
      int dyUnconsumed, int type) {
    dispatchScroll(
        mIsVertical ? 0 : dxUnconsumed, mIsVertical ? dyUnconsumed : 0, type, null, null);
  }

  @Override
  public void onNestedScroll(
      View target, int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed) {
    this.onNestedScroll(target, dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, TYPE_TOUCH);
  }

  @Override
  public boolean onNestedPreFling(View target, float velocityX, float velocityY) {
    return this.dispatchNestedPreFling(velocityX, velocityY);
  }

  @Override
  public boolean onNestedFling(View target, float velocityX, float velocityY, boolean consumed) {
    if (!consumed) {
      return flingWithNestedDispatch((int) velocityX, (int) velocityY);
    }
    return false;
  }

  @Override
  public int getNestedScrollAxes() {
    return mNestedScrollingParentHelper.getNestedScrollAxes();
  }

  /********* NestedScrollingParent2 end *********/

  private boolean flingWithNestedDispatch(int velocityX, int velocityY) {
    if (mIsVertical || Math.abs(velocityX) < mConfig.minFlingVelocity) {
      velocityX = 0;
    }
    if (!mIsVertical || Math.abs(velocityY) < mConfig.minFlingVelocity) {
      velocityY = 0;
    }
    if (velocityX == 0 && velocityY == 0) {
      return false;
    }
    // The parent view has chance to fully consume the fling in a nested before the child
    // view consumes it
    if (!dispatchNestedPreFling(velocityX, velocityY)) {
      dispatchNestedFling(velocityX, velocityY, true);

      startNestedScroll(getNestedScrollAxis(), TYPE_NON_TOUCH);
      velocityX =
          Math.max(-mConfig.maxFlingVelocity, Math.min(velocityX, mConfig.maxFlingVelocity));
      velocityY =
          Math.max(-mConfig.maxFlingVelocity, Math.min(velocityY, mConfig.maxFlingVelocity));
      tryToUpdateScrollState(SCROLL_STATE_FLING);
      mScrollHelper.fling(
          velocityX, velocityY, new LynxBaseScrollViewScroller.ScrollFinishedCallback() {
            @Override
            public void finished(boolean completed) {
              if (completed) {
                tryToUpdateScrollState(SCROLL_STATE_IDLE);
              }
            }
          });
      return true;
    }
    return false;
  }

  @Override
  public boolean canScrollHorizontally(int direction) {
    if (mIsVertical || direction == 0) {
      return false;
    }

    int scrollOffset = getScrollOffsetHorizontally();
    int[] scrollRange = getScrollRangeHorizontally();

    boolean forward = direction > 0;

    switch (forward ? getForwardNestedScrollMode() : mBackwardNestedScrollMode) {
      case NESTED_SCROLL_MODE_SELF_ONLY:
        return true;
      case NESTED_SCROLL_MODE_PARALLEL:
      case NESTED_SCROLL_MODE_SELF_FIRST:
      case NESTED_SCROLL_MODE_PARENT_FIRST:
        return forward ? scrollRange[1] > scrollOffset + 1 : scrollOffset - 1 > scrollRange[0];
      default:
        break;
    }
    return super.canScrollHorizontally(direction);
  }

  @Override
  public boolean canScrollVertically(int direction) {
    if (!mIsVertical || direction == 0) {
      return false;
    }

    int scrollOffset = getScrollOffsetVertically();
    int[] scrollRange = getScrollRangeVertically();

    boolean forward = direction > 0;

    switch (forward ? mForwardNestedScrollMode : mBackwardNestedScrollMode) {
      case NESTED_SCROLL_MODE_SELF_ONLY:
        return true;
      case NESTED_SCROLL_MODE_PARALLEL:
      case NESTED_SCROLL_MODE_SELF_FIRST:
      case NESTED_SCROLL_MODE_PARENT_FIRST:
        return forward ? scrollRange[1] > scrollOffset + 1 : scrollOffset - 1 > scrollRange[0];
      default:
        break;
    }
    return super.canScrollVertically(direction);
  }

  @Override
  public int getForwardNestedScrollMode() {
    return mForwardNestedScrollMode;
  }

  @Override
  public int getBackwardNestedScrollMode() {
    return mBackwardNestedScrollMode;
  }

  @Override
  public LynxBaseScrollViewNestedInternal getNestedScrollingParentForType(int type) {
    ViewParent view = mNestedScrollingChildHelper.getNestedScrollingParentForType(type);
    if (view instanceof LynxBaseScrollViewNestedInternal) {
      return (LynxBaseScrollViewNestedInternal) view;
    } else {
      return null;
    }
  }

  @Override
  public int[] dispatchScroll(
      int deltaX, int deltaY, int type, MotionEvent event, @Nullable int[] offsetInWindow) {
    int[] contentOffset = getScrollOffset();
    int[] scrollRange = getScrollRange();
    int[] lefts = new int[] {0, 0};
    boolean isScrollForward = mIsVertical ? deltaY > 0 : deltaX > 0;

    switch (isScrollForward ? mForwardNestedScrollMode : mBackwardNestedScrollMode) {
      case NESTED_SCROLL_MODE_SELF_ONLY: {
        // for self
        if (bounces()) {
          tryBouncingThanScroll(deltaX, deltaY, isScrollForward, contentOffset, scrollRange, type);
        } else {
          scrollByWithLefts(deltaX, deltaY, lefts);
        }
        break;
      }
      case NESTED_SCROLL_MODE_SELF_FIRST: {
        if (bounces()) {
          // TODO(moonface): Take bounces into account.
        } else {
          // for parent, do nothing
          int[] consumedByNestedParent = new int[2];
          if (dispatchNestedPreScroll(0, 0, consumedByNestedParent, null, type)) {
            //            if (event != null && offsetInWindow != null) {
            //              event.offsetLocation(offsetInWindow[0], offsetInWindow[1]);
            //            }
          }

          // for self
          if (type == TYPE_EXTEND_BOUNCING_BACKWARDS && !bounces()) {
            lefts[0] = deltaX;
            lefts[1] = deltaY;
          } else {
            scrollByWithLefts(deltaX, deltaY, lefts);
          }
          // for parent
          dispatchNestedScroll(
              deltaX - lefts[0], deltaY - lefts[1], lefts[0], lefts[1], offsetInWindow, type);
        }
        break;
      }
      case NESTED_SCROLL_MODE_PARENT_FIRST: {
        // for parent
        int[] consumedByNestedParent = new int[2];
        int[] preScrollOffsetInWindow = new int[2];
        if (type == TYPE_EXTEND_BOUNCING_BACKWARDS && bounces()) {
          consumedByNestedParent[0] = 0;
          consumedByNestedParent[1] = 0;
          preScrollOffsetInWindow[0] = 0;
          preScrollOffsetInWindow[0] = 0;
        } else if (dispatchNestedPreScroll(
                       deltaX, deltaY, consumedByNestedParent, preScrollOffsetInWindow, type)) {
          //          if (event != null && offsetInWindow != null) {
          //            event.offsetLocation(offsetInWindow[0], offsetInWindow[1]);
          //          }
        }
        deltaX -= consumedByNestedParent[0];
        deltaY -= consumedByNestedParent[1];
        // for self
        if (bounces()) {
          tryBouncingThanScroll(deltaX, deltaY, isScrollForward, contentOffset, scrollRange, type);
        } else {
          scrollByWithLefts(deltaX, deltaY, lefts);
        }
        // for parent
        int[] postScrollOffsetInWindow = new int[2];
        if (type == TYPE_EXTEND_BOUNCING_BACKWARDS && bounces()) {
          postScrollOffsetInWindow[0] = 0;
          postScrollOffsetInWindow[0] = 0;
        } else {
          dispatchNestedScroll(deltaX - lefts[0], deltaY - lefts[1], lefts[0], lefts[1],
              postScrollOffsetInWindow, type);
        }
        if (offsetInWindow != null) {
          offsetInWindow[0] = postScrollOffsetInWindow[0] + preScrollOffsetInWindow[0];
          offsetInWindow[1] = postScrollOffsetInWindow[1] + preScrollOffsetInWindow[1];
        }
        break;
      }
      case NESTED_SCROLL_MODE_PARALLEL: {
        // for parent
        int[] preScrollOffsetInWindow = new int[2];
        int[] consumedByNestedParent = new int[2];
        if (dispatchNestedPreScroll(
                deltaX, deltaY, consumedByNestedParent, preScrollOffsetInWindow, type)) {
          //          if (event != null && offsetInWindow != null) {
          //            event.offsetLocation(preScrollOffsetInWindow[0], offsetInWindow[1]);
          //          }
        }
        // for self
        //        deltaX -= preScrollOffsetInWindow[0];
        //        deltaY -= preScrollOffsetInWindow[1];
        if (bounces()) {
          tryBouncingThanScroll(deltaX, deltaY, isScrollForward, contentOffset, scrollRange, type);
        } else {
          scrollByWithLefts(deltaX, deltaY, lefts);
        }
        // for parent
        dispatchNestedScroll(deltaX, deltaY, 0, 0, null, type);

        if (offsetInWindow != null) {
          offsetInWindow[0] = preScrollOffsetInWindow[0];
          offsetInWindow[1] = preScrollOffsetInWindow[1];
        }
        break;
      }
      default:
        break;
    }

    return lefts;
  }

  private void scrollByWithLefts(int deltaX, int deltaY, int[] lefts) {
    if (mIsVertical) {
      int idiomaticOffset = getScrollOffsetVertically() + deltaY;
      int[] scrollRange = getScrollRangeVertically();
      int realOffset = Math.min(Math.max(idiomaticOffset, scrollRange[0]), scrollRange[1]);
      scrollToUnlimitedVertically(realOffset);
      lefts[0] = 0;
      lefts[1] = idiomaticOffset - realOffset;
    } else {
      int idiomaticOffset = getScrollOffsetHorizontally() + deltaX;
      int[] scrollRange = getScrollRangeHorizontally();
      int realOffset = Math.min(Math.max(idiomaticOffset, scrollRange[0]), scrollRange[1]);
      scrollToUnlimitedHorizontally(realOffset);
      lefts[0] = idiomaticOffset - realOffset;
      lefts[1] = 0;
    }
  }

  private int bouncingDist2TouchDist(int bouncing, boolean isVertical) {
    double c = 0.55;
    int d = isVertical ? mHeight : mWidth;
    return (int) (bouncing * d / (c * (d - bouncing)));
  }

  private int touchDist2BouncingDist(int move, boolean isVertical) {
    double c = 0.55;
    int d = isVertical ? mHeight : mWidth;
    return (int) ((1.0 - 1.0 / (move * c / d + 1.0)) * d);
  }

  private int doBounce(
      int delta, int scrollOffset, int[] scrollRange, int type, boolean isVertical) {
    int lefts = 0;
    if (isBouncingForwards(scrollOffset, scrollRange)) {
      if (delta >= 0) {
        // continue forwards
        if (type == TYPE_TOUCH || type == TYPE_EXTEND_FLING) {
          int bouncing = scrollOffset - scrollRange[1];
          int recoverMove = bouncingDist2TouchDist(bouncing, isVertical);
          int targetBouncing = touchDist2BouncingDist(recoverMove + delta, isVertical);
          if (isVertical) {
            scrollToUnlimitedVertically(scrollRange[1] + targetBouncing);
          } else {
            scrollToUnlimitedHorizontally(scrollRange[1] + targetBouncing);
          }
        } else {
          if (isVertical) {
            scrollToUnlimitedVertically(scrollOffset + delta);
          } else {
            scrollToUnlimitedHorizontally(scrollOffset + delta);
          }
        }
      } else {
        // backwards
        if (type == TYPE_TOUCH || type == TYPE_EXTEND_FLING) {
          int bouncing = scrollOffset - scrollRange[1];
          int recoverMove = bouncingDist2TouchDist(bouncing, isVertical);
          int bouncingOffset = Math.max(recoverMove - scrollRange[1], -delta);
          int targetBouncing = touchDist2BouncingDist(Math.max(recoverMove + delta, 0), isVertical);
          if (isVertical) {
            scrollToUnlimitedVertically(scrollRange[1] + targetBouncing);
          } else {
            scrollToUnlimitedHorizontally(scrollRange[1] + targetBouncing);
          }
          lefts = -(-delta - bouncingOffset);
        } else {
          int bouncingTarget = Math.max(scrollOffset + delta, scrollRange[1]);
          if (isVertical) {
            scrollToUnlimitedVertically(bouncingTarget);
          } else {
            scrollToUnlimitedHorizontally(bouncingTarget);
          }
          lefts = delta - (bouncingTarget - scrollOffset);
        }
      }
    } else if (isBouncingBackwards(scrollOffset, scrollRange)) {
      if (delta <= 0) {
        // continue backwards
        if (type == TYPE_TOUCH || type == TYPE_EXTEND_FLING) {
          int bouncing = scrollRange[0] - scrollOffset;
          int recoverMove = bouncingDist2TouchDist(bouncing, isVertical);
          int targetBouncing = touchDist2BouncingDist(recoverMove - delta, isVertical);
          if (isVertical) {
            scrollToUnlimitedVertically(-targetBouncing);
          } else {
            scrollToUnlimitedHorizontally(-targetBouncing);
          }
        } else {
          if (isVertical) {
            scrollToUnlimitedVertically(scrollOffset + delta);
          } else {
            scrollToUnlimitedHorizontally(scrollOffset + delta);
          }
        }
      } else {
        // forwards
        if (type == TYPE_TOUCH || type == TYPE_EXTEND_FLING) {
          int bouncing = scrollRange[0] - scrollOffset;
          int recoverMove = bouncingDist2TouchDist(bouncing, isVertical);
          int bouncingOffset = Math.max(-recoverMove - scrollRange[0], delta);
          int targetBouncing = touchDist2BouncingDist(Math.max(recoverMove - delta, 0), isVertical);
          if (isVertical) {
            scrollToUnlimitedVertically(-targetBouncing);
          } else {
            scrollToUnlimitedHorizontally(-targetBouncing);
          }
          lefts = delta - bouncingOffset;
        } else {
          int bouncingTarget = Math.min(scrollOffset + delta, scrollRange[0]);

          if (isVertical) {
            scrollToUnlimitedVertically(bouncingTarget);
          } else {
            scrollToUnlimitedHorizontally(bouncingTarget);
          }
          lefts = delta - (bouncingTarget - scrollOffset);
        }
      }
    }
    return lefts;
  }

  private int[] bouncing(int deltaX, int deltaY, int[] contentOffset, int[] scrollRange, int type) {
    int[] lefts = new int[] {0, 0};
    if (mIsVertical) {
      lefts[1] = doBounce(
          deltaY, contentOffset[1], new int[] {scrollRange[2], scrollRange[3]}, type, true);
    } else {
      lefts[0] = doBounce(
          deltaX, contentOffset[0], new int[] {scrollRange[0], scrollRange[1]}, type, false);
    }
    return lefts;
  }

  private void tryBouncingThanScroll(int deltaX, int deltaY, boolean isScrollForward,
      int[] scrollOffset, int[] scrollRange, int type) {
    boolean bouncingForwards = mIsVertical
        ? isBouncingForwards(scrollOffset[1], new int[] {scrollRange[2], scrollRange[3]})
        : isBouncingForwards(scrollOffset[0], new int[] {scrollRange[0], scrollRange[1]});
    boolean bouncingBackwards = mIsVertical
        ? isBouncingBackwards(scrollOffset[1], new int[] {scrollRange[2], scrollRange[3]})
        : isBouncingBackwards(scrollOffset[0], new int[] {scrollRange[0], scrollRange[1]});

    if (bouncingForwards) {
      if (isScrollForward) {
        // forwards bouncing
        bouncing(deltaX, deltaY, scrollOffset, scrollRange, type);
      } else {
        // backwards bouncing
        int[] lefts = bouncing(deltaX, deltaY, scrollOffset, scrollRange, type);
        // consume the lefts
        scrollBy(lefts[0], lefts[1]);
      }
    } else if (bouncingBackwards) {
      if (!isScrollForward) {
        // backwards bouncing
        bouncing(deltaX, deltaY, scrollOffset, scrollRange, type);
      } else {
        // forwards bouncing
        int[] lefts = bouncing(deltaX, deltaY, scrollOffset, scrollRange, type);

        // consume the lefts
        scrollBy(lefts[0], lefts[1]);
      }
    } else {
      // scroll to range
      if (type != TYPE_EXTEND_BOUNCING_BACKWARDS) {
        scrollBy(deltaX, deltaY);
        int[] newOffset = getScrollOffset();
        int[] offsetLeft = new int[] {
            deltaX - (newOffset[0] - scrollOffset[0]), deltaY - (newOffset[1] - scrollOffset[1])};

        // than bouncing to another side
        bouncing(offsetLeft[0], offsetLeft[1], scrollOffset, scrollRange, type);
      }
    }
  }
}
