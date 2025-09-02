// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

import static androidx.core.view.ViewCompat.TYPE_TOUCH;
import static androidx.customview.widget.ViewDragHelper.INVALID_POINTER;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;

public class LynxBaseScrollViewDragging
    extends LynxBaseScrollViewScrolling implements LynxBaseScrollViewInternal {
  @Override
  public boolean isBouncingForwards(int scrollOffset, int[] scrollRange) {
    return scrollOffset > scrollRange[1];
  }

  @Override
  public boolean isBouncingBackwards(int scrollOffset, int[] scrollRange) {
    return scrollOffset < scrollRange[0];
  }

  @Override
  public View getView() {
    return this;
  }

  @Override
  public boolean isVertical() {
    return mIsVertical;
  }

  @Override
  public int[] dispatchScroll(
      int deltaX, int deltaY, int type, MotionEvent event, @Nullable int[] offsetInWindow) {
    return new int[2];
  }

  @Override
  public int[] getFlingRange(boolean isVertical) {
    int scrollOffset = isVertical ? getScrollOffsetVertically() : getScrollOffsetHorizontally();
    int[] scrollRange = isVertical ? getScrollRangeVertically() : getScrollRangeHorizontally();

    int min = bounces() ? (scrollOffset < scrollRange[0] ? scrollOffset - 50 : scrollRange[0] - 300)
                        : scrollRange[0];
    int max = bounces() ? (scrollOffset > scrollRange[1] ? scrollOffset + 50 : scrollRange[1] + 300)
                        : scrollRange[1];

    return new int[] {min, max};
  }

  @Override
  public boolean startNestedScroll(int axes, int type) {
    return false;
  }

  @Override
  public void stopNestedScroll(int type) {}

  static class LynxBaseScrollViewDraggingConfig {
    public LynxBaseScrollViewDraggingConfig(
        int touchSlop, int minFlingVelocity, int maxFlingVelocity) {
      this.touchSlop = touchSlop;
      this.minFlingVelocity = minFlingVelocity;
      this.maxFlingVelocity = maxFlingVelocity;
    }
    protected final int touchSlop;
    protected final int minFlingVelocity;
    protected final int maxFlingVelocity;
  }

  public class LynxBaseScrollViewDraggingScrollInfo {
    private int initialMotionX;
    private int initialMotionY;
    private int lastMotionX;
    private int lastMotionY;
    private int activePointerId = INVALID_POINTER;
    private VelocityTracker velocityTracker;
    private final int[] scrollConsumed = new int[2];

    private final int[] offsetInWindow = new int[2];
    private final int[] scrollStepConsumed = new int[2];
    private final int[] targetScrollOffset = new int[2];
  }

  protected final LynxBaseScrollViewDraggingScrollInfo mScrollInfo =
      new LynxBaseScrollViewDraggingScrollInfo();

  protected final LynxBaseScrollViewDraggingConfig mConfig;

  public LynxBaseScrollViewDragging(
      @NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
    final ViewConfiguration viewConfiguration = ViewConfiguration.get(getContext());
    this.mConfig =
        new LynxBaseScrollViewDraggingConfig(viewConfiguration.getScaledWindowTouchSlop(),
            viewConfiguration.getScaledMinimumFlingVelocity(),
            viewConfiguration.getScaledMaximumFlingVelocity());
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent event) {
    if (mScrollInfo.velocityTracker == null) {
      mScrollInfo.velocityTracker = VelocityTracker.obtain();
    }
    mScrollInfo.velocityTracker.addMovement(event);
    final int action = event.getActionMasked();
    switch (action) {
      case MotionEvent.ACTION_DOWN: {
        mScrollInfo.activePointerId = event.getPointerId(0);
        mScrollInfo.initialMotionX = mScrollInfo.lastMotionX = (int) (event.getX() + 0.5f);
        mScrollInfo.initialMotionY = mScrollInfo.lastMotionY = (int) (event.getY() + 0.5f);

        if (mScrollState == SCROLL_STATE_FLING || mScrollState == SCROLL_STATE_ANIMATING) {
          getParent().requestDisallowInterceptTouchEvent(true);
          tryToUpdateScrollState(SCROLL_STATE_DRAGGING);
        }
        // Clear the nested offsets
        //        mNestedOffsets[0] = mNestedOffsets[1] = 0;
        startNestedScroll(getNestedScrollAxis(), TYPE_TOUCH);
        break;
      }
      case MotionEvent.ACTION_POINTER_DOWN: {
        mScrollInfo.activePointerId = event.getPointerId(0);
        mScrollInfo.initialMotionX = mScrollInfo.lastMotionX = (int) (event.getX() + 0.5f);
        mScrollInfo.initialMotionY = mScrollInfo.lastMotionY = (int) (event.getY() + 0.5f);
        break;
      }
      case MotionEvent.ACTION_MOVE: {
        if (mScrollInfo.activePointerId == INVALID_POINTER) {
          break;
        }
        final int pointerIndex = event.findPointerIndex(mScrollInfo.activePointerId);
        if (pointerIndex < 0) {
          return false;
        }
        final int x = (int) (event.getX(pointerIndex) + 0.5f);
        final int y = (int) (event.getY(pointerIndex) + 0.5f);
        if (mScrollState != SCROLL_STATE_DRAGGING) {
          final int dx = x - mScrollInfo.initialMotionX;
          final int dy = y - mScrollInfo.initialMotionY;
          // In the nested scroll scenario, the parent view will satisfy yDiff > mTouchSlop first,
          // but since (this.getNestedScrollAxes() & ViewCompat.SCROLL_AXIS_VERTICAL) == 0
          // is not satisfied, the parent view cannot intercept the ACTION_MOVE, which guarantees
          // that the ACTION_MOVE will be consumed by the child view first.
          // When the child view satisfy yDiff > mTouchSlop, it calls the
          // requestDisallowInterceptTouchEvent method to ensure that it handles all subsequent
          // ACTION_MOVE.
          // Note: The mNestedScrollAxes value will be modified in
          // NestedScrollingParent#onNestedScrollAccepted()
          final boolean isVerticalDragging = mIsVertical && Math.abs(dy) > mConfig.touchSlop
              && (this.getNestedScrollAxes() & ViewCompat.SCROLL_AXIS_VERTICAL) == 0;
          final boolean isHorizontalDragging = !mIsVertical && Math.abs(dx) > mConfig.touchSlop
              && (this.getNestedScrollAxes() & ViewCompat.SCROLL_AXIS_HORIZONTAL) == 0;
          if (isVerticalDragging || isHorizontalDragging) {
            mScrollInfo.lastMotionX = x;
            mScrollInfo.lastMotionY = y;
            tryToUpdateScrollState(SCROLL_STATE_DRAGGING);
          }
        }
        break;
      }
      case MotionEvent.ACTION_POINTER_UP: {
        final int actionIndex = event.getActionIndex();
        if (event.getPointerId(actionIndex) == mScrollInfo.activePointerId) {
          // Pick a new pointer to pick up the slack.
          final int newIndex = actionIndex == 0 ? 1 : 0;
          mScrollInfo.activePointerId = event.getPointerId(newIndex);
          mScrollInfo.initialMotionX = mScrollInfo.lastMotionX =
              (int) (event.getX(newIndex) + 0.5f);
          mScrollInfo.initialMotionY = mScrollInfo.lastMotionY =
              (int) (event.getY(newIndex) + 0.5f);
        }
        break;
      }
      case MotionEvent.ACTION_UP: {
        mScrollInfo.velocityTracker.clear();
        stopNestedScroll(TYPE_TOUCH);
        break;
      }
      case MotionEvent.ACTION_CANCEL: {
        resetTouch();
        tryToUpdateScrollState(SCROLL_STATE_IDLE);
        break;
      }
      default: {
        break;
      }
    }
    return mScrollState == SCROLL_STATE_DRAGGING;
  }

  /********* TouchEvent begin *********/
  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (mScrollInfo.velocityTracker == null) {
      mScrollInfo.velocityTracker = VelocityTracker.obtain();
    }
    boolean eventAddedToVelocityTracker = false;
    final int actionType = event.getActionMasked();
    if (actionType == MotionEvent.ACTION_DOWN) {
      mScrollInfo.offsetInWindow[0] = mScrollInfo.offsetInWindow[1] = 0;
      mScrollHelper.stopAnimating(false);
      startNestedScroll(getNestedScrollAxis(), TYPE_TOUCH);
    }

    final MotionEvent tempEv = MotionEvent.obtain(event);
    tempEv.offsetLocation(mScrollInfo.offsetInWindow[0], mScrollInfo.offsetInWindow[1]);

    switch (actionType) {
      case MotionEvent.ACTION_DOWN: {
        mScrollInfo.activePointerId = event.getPointerId(0);
        mScrollInfo.initialMotionX = mScrollInfo.lastMotionX = (int) (event.getX() + 0.5f);
        mScrollInfo.initialMotionY = mScrollInfo.lastMotionY = (int) (event.getY() + 0.5f);
        break;
      }
      case MotionEvent.ACTION_POINTER_DOWN: {
        final int actionIndex = event.getActionIndex();
        mScrollInfo.activePointerId = event.getPointerId(actionIndex);
        mScrollInfo.initialMotionX = mScrollInfo.lastMotionX =
            (int) (event.getX(actionIndex) + 0.5f);
        mScrollInfo.initialMotionY = mScrollInfo.lastMotionY =
            (int) (event.getY(actionIndex) + 0.5f);
        break;
      }
      case MotionEvent.ACTION_MOVE: {
        if (mScrollInfo.activePointerId == INVALID_POINTER) {
          break;
        }
        final int pointerIndex = event.findPointerIndex(mScrollInfo.activePointerId);
        if (pointerIndex < 0) {
          return false;
        }
        final int x = (int) (event.getX(pointerIndex) + 0.5f);
        final int y = (int) (event.getY(pointerIndex) + 0.5f);
        int deltaX = mScrollInfo.lastMotionX - x;
        int deltaY = mScrollInfo.lastMotionY - y;

        boolean isDragging = mScrollState == SCROLL_STATE_DRAGGING;

        if (!isDragging) {
          // Math.abs(deltaX) > mTouchSlop means the scroll distance exceeds the touch threshold and
          // current view should consume ACTION_MOVE event which cannot be intercepted by parent
          // view.
          if (mIsVertical && Math.abs(deltaY) > mConfig.touchSlop) {
            // Note: need to modify the deltaY by mTouchSlop to prevent the view from jumping.
            if (deltaY > 0) {
              deltaY -= mConfig.touchSlop;
            } else {
              deltaY += mConfig.touchSlop;
            }
            isDragging = true;
          } else if (!mIsVertical && Math.abs(deltaX) > mConfig.touchSlop) {
            // Note: need to modify the deltaX by mTouchSlop to prevent the view from jumping.
            if (deltaX > 0) {
              deltaX -= mConfig.touchSlop;
            } else {
              deltaX += mConfig.touchSlop;
            }
            isDragging = true;
          }
        }

        if (isDragging) {
          if (getParent() != null) {
            getParent().requestDisallowInterceptTouchEvent(true);
          }
          tryToUpdateScrollState(SCROLL_STATE_DRAGGING);
          // recalculate the last motion X and Y due to nested scroll parent may consume scroll
          // distance.
          //          mLastMotionX = x - mScrollOffset[0];
          //          mLastMotionY = y - mScrollOffset[1];
          //          mScrollInfo.lastMotionX = x - nestedParentOffsetInWindow[0];
          //          mScrollInfo.lastMotionY = y - nestedParentOffsetInWindow[1];
          int[] offsetInWindow = new int[] {0, 0};
          int[] lefts = dispatchScroll(mIsVertical ? 0 : deltaX, mIsVertical ? deltaY : 0,
              TYPE_TOUCH, tempEv, offsetInWindow);

          tempEv.offsetLocation(offsetInWindow[0], offsetInWindow[1]);

          mScrollInfo.lastMotionX = x - offsetInWindow[0];
          mScrollInfo.lastMotionY = y - offsetInWindow[1];

          // Accumulate the offset in local view.
          mScrollInfo.offsetInWindow[0] += offsetInWindow[0];
          mScrollInfo.offsetInWindow[1] += offsetInWindow[1];

          if ((mIsVertical && lefts[1] != deltaY) || (!mIsVertical && lefts[0] != deltaX)) {
            getParent().requestDisallowInterceptTouchEvent(true);
          }
        }
        break;
      }
      case MotionEvent.ACTION_UP: {
        mScrollInfo.velocityTracker.addMovement(tempEv);
        mScrollInfo.velocityTracker.computeCurrentVelocity(1000, mConfig.maxFlingVelocity);
        eventAddedToVelocityTracker = true;
        final float velX = !mIsVertical
            ? -mScrollInfo.velocityTracker.getXVelocity(mScrollInfo.activePointerId)
            : 0;
        final float velY = mIsVertical
            ? -mScrollInfo.velocityTracker.getYVelocity(mScrollInfo.activePointerId)
            : 0;
        if ((Math.abs(velX) < 600 & !mIsVertical) || (Math.abs(velY) < 600 && mIsVertical)) {
          if (mScrollHelper.tryBouncesBack((LynxBaseScrollViewNestedInternal) this, mIsVertical,
                  new LynxBaseScrollViewScroller.ScrollFinishedCallback() {
                    @Override
                    public void finished(boolean completed) {
                      if (completed) {
                        tryToUpdateScrollState(SCROLL_STATE_IDLE);
                      }
                    }
                  })) {
            tryToUpdateScrollState(SCROLL_STATE_FLING);
          } else {
            tryToUpdateScrollState(SCROLL_STATE_IDLE);
          }
        } else {
          LynxBaseScrollViewNestedInternal scrollView = (LynxBaseScrollViewNestedInternal) this;
          mScrollHelper.fling(
              (int) velX, (int) velY, new LynxBaseScrollViewScroller.ScrollFinishedCallback() {
                @Override
                public void finished(boolean completed) {
                  if (completed) {
                    if (mScrollHelper.tryBouncesBack(scrollView, mIsVertical,
                            (LynxBaseScrollViewScroller.ScrollFinishedCallback) completed1 -> {
                              if (completed1) {
                                tryToUpdateScrollState(SCROLL_STATE_IDLE);
                              }
                            })) {
                    } else {
                      tryToUpdateScrollState(SCROLL_STATE_IDLE);
                    }
                  }
                }
              });
          tryToUpdateScrollState(SCROLL_STATE_FLING);
        }

        resetTouch();
        //        stopNestedScroll(TYPE_TOUCH);
        break;
      }
      case MotionEvent.ACTION_POINTER_UP: {
        final int actionIndex = event.getActionIndex();
        if (event.getPointerId(actionIndex) == mScrollInfo.activePointerId) {
          // Pick a new pointer to pick up the slack.
          final int newIndex = actionIndex == 0 ? 1 : 0;
          mScrollInfo.activePointerId = event.getPointerId(newIndex);
          mScrollInfo.initialMotionX = mScrollInfo.lastMotionX =
              (int) (event.getX(newIndex) + 0.5f);
          mScrollInfo.initialMotionY = mScrollInfo.lastMotionY =
              (int) (event.getY(newIndex) + 0.5f);
        }
        break;
      }
      case MotionEvent.ACTION_CANCEL: {
        resetTouch();
        tryToUpdateScrollState(SCROLL_STATE_IDLE);
        break;
      }
      default:
        break;
    }

    if (!eventAddedToVelocityTracker) {
      mScrollInfo.velocityTracker.addMovement(tempEv);
    }
    tempEv.recycle();

    return true;
  }

  private void resetTouch() {
    if (mScrollInfo.velocityTracker != null) {
      mScrollInfo.velocityTracker.clear();
    }
    getParent().requestDisallowInterceptTouchEvent(false);
    //    stopNestedScroll(TYPE_TOUCH);
  }

  protected int getNestedScrollAxis() {
    return mIsVertical ? ViewCompat.SCROLL_AXIS_VERTICAL : ViewCompat.SCROLL_AXIS_HORIZONTAL;
  }
}
