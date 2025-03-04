// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import android.content.Context;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewParent;
import android.widget.HorizontalScrollView;
import android.widget.OverScroller;
import android.widget.ScrollView;
import androidx.core.math.MathUtils;
import androidx.core.view.NestedScrollingChild2;
import androidx.core.view.NestedScrollingChildHelper;
import androidx.core.view.NestedScrollingParent2;
import androidx.core.view.NestedScrollingParentHelper;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.base.LLog;
import java.lang.reflect.Field;

public class NestedHorizontalScrollView
    extends HorizontalScrollView implements NestedScrollingParent2, NestedScrollingChild2 {
  private static final String TAG = "LynxNestedHorizontalScrollView";
  public static final boolean DEBUG_GESTURE = false;

  private static final int INVALID_POINTER = -1;
  private boolean mEnableNewNested = false;
  private boolean mIsBeingDragged = false;
  protected boolean mEnableNewBounce = false;
  private int mActivePointerId = INVALID_POINTER;
  private int mLastMotionX;
  private int mTouchSlop;
  private int mMinimumVelocity;
  private int mMaximumVelocity;
  private int mNestedXOffset;
  private int mLastFlingScrollX;
  private final int[] mScrollOffset = new int[2];
  private final int[] mScrollConsumed = new int[2];
  private UIScrollView mUIScrollView;
  private OverScroller mHScroller;
  private VelocityTracker mVelocityTracker;
  private NestedScrollingParentHelper mParentHelper;
  private NestedScrollingChildHelper mChildHelper;
  protected BounceGestureHelper mBounceGestureHelper;

  public NestedHorizontalScrollView(Context context, UIScrollView uiScrollView) {
    super(context);
    mUIScrollView = uiScrollView;
    initHorizontalScrollView();
  }

  private void initHorizontalScrollView() {
    mParentHelper = new NestedScrollingParentHelper(this);
    mChildHelper = new NestedScrollingChildHelper(this);
    mBounceGestureHelper = new BounceGestureHelper(this, false, mParentHelper);
    final ViewConfiguration configuration = ViewConfiguration.get(getContext());
    mTouchSlop = configuration.getScaledTouchSlop();
    mMinimumVelocity = configuration.getScaledMinimumFlingVelocity();
    mMaximumVelocity = configuration.getScaledMaximumFlingVelocity();
    this.setNestedScrollingEnabled(false);
  }

  void setPagingTouchSlopIfNeeded() {
    try {
      final ViewConfiguration configuration = ViewConfiguration.get(getContext());
      Field touchSlopField = HorizontalScrollView.class.getDeclaredField("mTouchSlop");
      if (configuration != null && touchSlopField != null) {
        int touchSlop = configuration.getScaledPagingTouchSlop();
        touchSlopField.setAccessible(true);
        touchSlopField.set(this, touchSlop);
        mTouchSlop = touchSlop;
      }
    } catch (NoSuchFieldException e) {
      LLog.e(TAG, "Failed to get mTouchSlop field of HorizontalScrollView!");
    } catch (IllegalAccessException e) {
      LLog.e(TAG, "Failed to get mTouchSlop field of HorizontalScrollView!");
    }
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev) {
    if (!mEnableNewNested) {
      return super.onInterceptTouchEvent(ev);
    }
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      return mBounceGestureHelper.onInterceptTouchEvent(ev);
    }
    final int action = ev.getAction();
    if (action == MotionEvent.ACTION_MOVE && mIsBeingDragged) {
      return true;
    }
    switch (ev.getActionMasked()) {
      case MotionEvent.ACTION_DOWN: {
        final int x = (int) ev.getX();
        final int y = (int) ev.getY();
        if (!inChild(x, y)) {
          mIsBeingDragged = false;
          recycleVelocityTracker();
          break;
        }
        initOrResetVelocityTracker();
        mVelocityTracker.addMovement(ev);
        mActivePointerId = ev.getPointerId(0);
        mLastMotionX = x;

        OverScroller scroller = getHScroller();
        if (scroller != null) {
          // If being fling and user touches the screen, initiate drag;
          // We need to call computeScrollOffset() first so that isFinished() is correct.
          scroller.computeScrollOffset();
          mIsBeingDragged = !getHScroller().isFinished();
        }
        this.startNestedScroll(ViewCompat.SCROLL_AXIS_HORIZONTAL, ViewCompat.TYPE_TOUCH);
        if (DEBUG_GESTURE) {
          LLog.i(TAG, "onInterceptTouchEvent -> ACTION_DOWN: mIsBeingDragged = " + mIsBeingDragged);
        }
        break;
      }
      case MotionEvent.ACTION_MOVE: {
        if (mActivePointerId == INVALID_POINTER) {
          break;
        }
        final int pointerIndex = ev.findPointerIndex(mActivePointerId);
        if (pointerIndex == INVALID_POINTER) {
          break;
        }
        final int x = (int) ev.getX(pointerIndex);
        final int xDiff = Math.abs(x - mLastMotionX);
        // In the nested scroll scenario, the parent view will satisfy xDiff > mTouchSlop first,
        // but since (this.getNestedScrollAxes() & ViewCompat.SCROLL_AXIS_HORIZONTAL) == 0
        // is not satisfied, the parent view cannot intercept the ACTION_MOVE, which guarantees
        // that the ACTION_MOVE will be consumed by the child view first.
        // When the child view satisfy xDiff > mTouchSlop, it calls the
        // requestDisallowInterceptTouchEvent method to ensure that it handles all subsequent
        // ACTION_MOVE. Note: The mNestedScrollAxes value will be modified in
        // NestedScrollingParent#onNestedScrollAccepted()
        if (xDiff > mTouchSlop
            && (this.getNestedScrollAxes() & ViewCompat.SCROLL_AXIS_HORIZONTAL) == 0) {
          mIsBeingDragged = true;
          mLastMotionX = x;
          initVelocityTrackerIfNotExists();
          mVelocityTracker.addMovement(ev);
          final ViewParent parent = getParent();
          if (parent != null) {
            parent.requestDisallowInterceptTouchEvent(true);
          }
          // No nested scroll, reset mNestedXOffset to 0
          mNestedXOffset = 0;
          if (DEBUG_GESTURE) {
            LLog.i(TAG,
                "onInterceptTouchEvent -> ACTION_MOVE: with no nested scroll child and intercept this event");
          }
        }
        break;
      }
      case MotionEvent.ACTION_POINTER_UP: {
        if (DEBUG_GESTURE) {
          LLog.i(TAG, "onInterceptTouchEvent -> ACTION_POINTER_UP: onSecondaryPointerUp");
        }
        onSecondaryPointerUp(ev);
        break;
      }
      case MotionEvent.ACTION_CANCEL:
      case MotionEvent.ACTION_UP:
        mIsBeingDragged = false;
        mActivePointerId = INVALID_POINTER;
        recycleVelocityTracker();
        final int scrollRange = mUIScrollView.getScrollRange();
        if (getHScroller() != null
            && getHScroller().springBack(getScrollX(), getScrollY(), 0, scrollRange, 0, 0)) {
          ViewCompat.postInvalidateOnAnimation(this);
        }
        this.stopNestedScroll(ViewCompat.TYPE_TOUCH);
        break;
    }
    return mIsBeingDragged;
  }

  @Override
  public boolean onTouchEvent(MotionEvent ev) {
    if (!mEnableNewNested) {
      return super.onTouchEvent(ev);
    }
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      return mBounceGestureHelper.onTouchEvent(ev);
    }
    final int actionMasked = ev.getActionMasked();
    initVelocityTrackerIfNotExists();

    // Note: In the nested scroll scenario, an additional MotionEvent tempEv need to be created to
    // properly calculate the fling velocity.
    MotionEvent tempEv = MotionEvent.obtain(ev);
    if (actionMasked == MotionEvent.ACTION_DOWN) {
      mNestedXOffset = 0;
    }
    tempEv.offsetLocation(mNestedXOffset, 0);

    switch (actionMasked) {
      case MotionEvent.ACTION_DOWN: {
        if (getChildCount() == 0) {
          return false;
        }
        OverScroller scroller = getHScroller();
        if (scroller != null) {
          boolean isFinished = scroller.isFinished();
          mIsBeingDragged = !isFinished;
          if (mIsBeingDragged) {
            final ViewParent parent = getParent();
            if (parent != null) {
              parent.requestDisallowInterceptTouchEvent(true);
            }
          }
          // If being fling and user touches stop the fling.
          scroller.abortAnimation();
        }
        mLastMotionX = (int) ev.getX();
        mActivePointerId = ev.getPointerId(0);
        this.startNestedScroll(ViewCompat.SCROLL_AXIS_HORIZONTAL, ViewCompat.TYPE_TOUCH);
        if (DEBUG_GESTURE) {
          LLog.i(TAG, "onTouchEvent -> ACTION_DOWN: mIsBeingDragged = " + mIsBeingDragged);
        }
        break;
      }
      case MotionEvent.ACTION_MOVE: {
        final int activePointerIndex = ev.findPointerIndex(mActivePointerId);
        if (activePointerIndex == INVALID_POINTER) {
          break;
        }
        final int x = (int) ev.getX(activePointerIndex);
        int deltaX = mLastMotionX - x;
        if (this.dispatchNestedPreScroll(
                deltaX, 0, mScrollConsumed, mScrollOffset, ViewCompat.TYPE_TOUCH)) {
          // If the parent consumed some or all of the scroll delta
          if (DEBUG_GESTURE) {
            LLog.i(TAG,
                "onTouchEvent -> ACTION_MOVE: dispatch nested pre scroll with scroll distance = "
                    + deltaX + ", and parent view consume " + mScrollConsumed[0]);
          }
          deltaX -= mScrollConsumed[0];
          tempEv.offsetLocation(mScrollOffset[0], 0);
          // Accumulate the the offset in local view.
          mNestedXOffset += mScrollOffset[0];
        }
        if (!mIsBeingDragged && Math.abs(deltaX) > mTouchSlop) {
          // Math.abs(deltaX) > mTouchSlop means the scroll distance exceeds the touch threshold and
          // current view should consume ACTION_MOVE event which cannot be intercepted by parent
          // view.
          mIsBeingDragged = true;
          final ViewParent parent = getParent();
          if (parent != null) {
            parent.requestDisallowInterceptTouchEvent(true);
          }
          // Note: need to modify the deltaX by mTouchSlop to prevent the view from jumping.
          if (deltaX > 0) {
            deltaX -= mTouchSlop;
          } else {
            deltaX += mTouchSlop;
          }
        }
        if (mIsBeingDragged) {
          // recalculate the last motion X due to nested scroll parent may consume scroll distance.
          mLastMotionX = x - mScrollOffset[0];
          final int scrollRange = mUIScrollView.getScrollRange();
          final int oldScrollX = getScrollX();
          int newScrollX = oldScrollX + deltaX;
          final boolean clampedX = newScrollX < 0 || newScrollX > scrollRange;
          newScrollX = MathUtils.clamp(newScrollX, 0, scrollRange);
          OverScroller scroller = getHScroller();
          if (clampedX && !this.hasNestedScrollingParent(ViewCompat.TYPE_NON_TOUCH)
              && scroller != null) {
            // If view scroll to boundary and has no nested scroll parent with TYPE_NON_TOUCH,
            // invoke springBack() to let child scroll into a valid coordinate range.
            scroller.springBack(newScrollX, 0, 0, scrollRange, 0, 0);
          }
          if (DEBUG_GESTURE) {
            LLog.i(TAG,
                "onTouchEvent -> ACTION_MOVE: consume scroll distance by self from " + oldScrollX
                    + " to " + newScrollX);
          }
          // consume scroll distance by self
          super.scrollTo(newScrollX, getScrollY());
          if (clampedX && !this.hasNestedScrollingParent(ViewCompat.TYPE_TOUCH)) {
            mVelocityTracker.clear();
          }
          final int scrolledDeltaX = getScrollX() - oldScrollX;
          final int unconsumedX = deltaX - scrolledDeltaX;
          if (this.dispatchNestedScroll(
                  scrolledDeltaX, 0, unconsumedX, 0, mScrollOffset, ViewCompat.TYPE_TOUCH)) {
            if (DEBUG_GESTURE) {
              LLog.i(TAG,
                  "onTouchEvent -> ACTION_MOVE: dispatch nested scroll with scroll distance = "
                      + unconsumedX);
            }
            mLastMotionX -= mScrollOffset[0];
            mNestedXOffset += mScrollOffset[0];
            tempEv.offsetLocation(mScrollOffset[0], 0);
          }
        }
        break;
      }
      case MotionEvent.ACTION_POINTER_DOWN: {
        final int index = ev.getActionIndex();
        mLastMotionX = (int) ev.getX(index);
        mActivePointerId = ev.getPointerId(index);
        if (DEBUG_GESTURE) {
          LLog.i(TAG,
              "onTouchEvent -> ACTION_POINTER_DOWN: use the new finger that"
                  + " touches screen as the active finger, and update mActivePointerId = "
                  + mActivePointerId + ", mLastMotionX = " + mLastMotionX);
        }
        break;
      }
      case MotionEvent.ACTION_POINTER_UP: {
        if (DEBUG_GESTURE) {
          LLog.i(TAG, "onTouchEvent -> ACTION_POINTER_UP: onSecondaryPointerUp");
        }
        onSecondaryPointerUp(ev);
        final int pointerIndex = ev.findPointerIndex(mActivePointerId);
        if (pointerIndex == INVALID_POINTER) {
          break;
        }
        mLastMotionX = (int) ev.getX(pointerIndex);
        break;
      }
      case MotionEvent.ACTION_UP: {
        final VelocityTracker velocityTracker = mVelocityTracker;
        velocityTracker.computeCurrentVelocity(1000, mMaximumVelocity);
        int initialVelocity = -(int) velocityTracker.getXVelocity(mActivePointerId);
        final int scrollRange = mUIScrollView.getScrollRange();
        OverScroller scroller = getHScroller();
        if (getChildCount() > 0) {
          if (Math.abs(initialVelocity) > mMinimumVelocity) {
            final boolean canFling = (getScrollX() > 0 || initialVelocity > 0)
                && (getScrollX() < scrollRange || initialVelocity < 0);
            // The parent view has chance to fully consume the fling in a nested before the child
            // view consumes it
            if (!this.dispatchNestedPreFling(initialVelocity, 0)) {
              dispatchNestedFling(initialVelocity, 0, canFling);
              if (DEBUG_GESTURE) {
                LLog.i(TAG,
                    "onTouchEvent -> ACTION_UP: dispatch a fling to a nested scrolling parent and parent not consumes, fling by self with velocity = "
                        + initialVelocity);
              }
              fling(initialVelocity);
            }
          } else if (scroller != null
              && scroller.springBack(getScrollX(), getScrollY(), 0, scrollRange, 0, 0)) {
            ViewCompat.postInvalidateOnAnimation(this);
          }
        }
        endDrag();
        break;
      }
      case MotionEvent.ACTION_CANCEL: {
        final int scrollRange = mUIScrollView.getScrollRange();
        OverScroller scroller = getHScroller();
        if (mIsBeingDragged && getChildCount() > 0 && scroller != null
            && scroller.springBack(getScrollX(), getScrollY(), 0, scrollRange, 0, 0)) {
          ViewCompat.postInvalidateOnAnimation(this);
        }
        endDrag();
        break;
      }
    }
    if (mVelocityTracker != null) {
      mVelocityTracker.addMovement(tempEv);
    }
    tempEv.recycle();
    return true;
  }

  /**
   * Fling the scroll view and start a nested scroll operation along the horizontal axes if
   * mEnableNewNested is true.
   *
   * @param velocityX The initial velocity in the X direction. Positive numbers mean that we want to
   *     scroll towards the left.
   */
  @Override
  public void fling(int velocityX) {
    OverScroller scroller;
    if (!mEnableNewNested || (scroller = getHScroller()) == null) {
      super.fling(velocityX);
      return;
    }
    if (getChildCount() > 0) {
      // Start nested scroll for TYPE_NON_TOUCH
      this.startNestedScroll(ViewCompat.SCROLL_AXIS_HORIZONTAL, ViewCompat.TYPE_NON_TOUCH);
      scroller.fling(getScrollX(), getScrollY(), // start position
          velocityX, 0, // fling velocity
          Integer.MIN_VALUE, Integer.MAX_VALUE, // min and max scrollX
          0, 0, // min and max scrollY
          0, 0); // overscroll
      ViewCompat.postInvalidateOnAnimation(this);
      mLastFlingScrollX = getScrollX();
    }
  }

  /**
   * Called by a parent to request that a child update its values for mScrollX and mScrollY if
   * necessary. If scroller is not finished, we dispatch nested scroll and parent view can consume
   * scroll distance in onNestedPreScroll() or onNestedScroll()
   */
  @Override
  public void computeScroll() {
    OverScroller scroller;
    if (!mEnableNewNested || (scroller = getHScroller()) == null) {
      super.computeScroll();
      return;
    }
    if (scroller.computeScrollOffset()) {
      final int x = scroller.getCurrX();
      int dx = x - mLastFlingScrollX;
      if (dispatchNestedPreScroll(dx, 0, mScrollConsumed, null, ViewCompat.TYPE_NON_TOUCH)) {
        dx -= mScrollConsumed[0];
      }
      if (dx != 0) {
        final int scrollRange = mUIScrollView.getScrollRange();
        final int oldScrollX = getScrollX();
        int newScrollX = oldScrollX + dx;
        final boolean clampedX = newScrollX < 0 || newScrollX > scrollRange;
        newScrollX = MathUtils.clamp(newScrollX, 0, scrollRange);
        if (clampedX && !this.hasNestedScrollingParent(ViewCompat.TYPE_NON_TOUCH)) {
          scroller.springBack(newScrollX, 0, 0, scrollRange, 0, 0);
        }
        super.scrollTo(newScrollX, getScrollY());
        final int scrolledDeltaX = getScrollX() - oldScrollX;
        final int unconsumedX = dx - scrolledDeltaX;
        this.dispatchNestedScroll(
            scrolledDeltaX, 0, unconsumedX, 0, null, ViewCompat.TYPE_NON_TOUCH);
      }
      // Finally update the scroll positions and post an invalidation
      mLastFlingScrollX = x;
      ViewCompat.postInvalidateOnAnimation(this);
    } else {
      if (this.hasNestedScrollingParent(ViewCompat.TYPE_NON_TOUCH)) {
        this.stopNestedScroll(ViewCompat.TYPE_NON_TOUCH);
      }
      mLastFlingScrollX = 0;
      if (mBounceGestureHelper != null && mEnableNewBounce) {
        mBounceGestureHelper.checkNestedStateByDeadLine();
      }
    }
  }

  protected void smoothScrollToInternal(int x, int y) {
    if (mEnableNewNested && getHScroller() != null) {
      // Note: we should update mLastFlingScrollX value for calculating correct scroll distance in
      // computeScroll() method.
      mLastFlingScrollX = getScrollX();
    }
    super.smoothScrollTo(x, y);
  }

  protected OverScroller getHScroller() {
    if (mHScroller != null) {
      return mHScroller;
    }
    try {
      Field scrollerField = HorizontalScrollView.class.getDeclaredField("mScroller");
      if (scrollerField != null) {
        scrollerField.setAccessible(true);
        Object scroller = scrollerField.get(this);
        if (scroller instanceof OverScroller) {
          mHScroller = (OverScroller) scroller;
        }
      }
    } catch (NoSuchFieldException e) {
      LLog.e(TAG, "Failed to get mScroller field of HorizontalScrollView!");
    } catch (IllegalAccessException e) {
      LLog.e(TAG, "Failed to get mScroller of HorizontalScrollView!");
    }
    return mHScroller;
  }

  private boolean inChild(int x, int y) {
    if (getChildCount() > 0) {
      final int scrollX = getScrollX();
      final View child = getChildAt(0);
      return !(y < child.getTop() || y >= child.getBottom() || x < child.getLeft() - scrollX
          || x >= child.getRight() - scrollX);
    }
    return false;
  }

  private void recycleVelocityTracker() {
    if (mVelocityTracker != null) {
      mVelocityTracker.recycle();
      mVelocityTracker = null;
    }
  }

  private void initOrResetVelocityTracker() {
    if (mVelocityTracker == null) {
      mVelocityTracker = VelocityTracker.obtain();
    } else {
      mVelocityTracker.clear();
    }
  }

  private void initVelocityTrackerIfNotExists() {
    if (mVelocityTracker == null) {
      mVelocityTracker = VelocityTracker.obtain();
    }
  }

  private void onSecondaryPointerUp(MotionEvent ev) {
    final int pointerIndex = ev.getActionIndex();
    final int pointerId = ev.getPointerId(pointerIndex);
    if (pointerId == mActivePointerId) {
      // If the active finger is released, let the last finger still on the screen be the active
      // finger and pointerIndex is continuous from 0, 1, 2.
      final int newPointerIndex = pointerIndex == 0 ? 1 : 0;
      mLastMotionX = (int) ev.getX(newPointerIndex);
      mActivePointerId = ev.getPointerId(newPointerIndex);
      if (mVelocityTracker != null) {
        mVelocityTracker.clear();
      }
      if (DEBUG_GESTURE) {
        LLog.i(TAG,
            "The active finger is released and update mActivePointerId -> " + mActivePointerId
                + ", and mLastMotionX = " + mLastMotionX);
      }
    } else if (DEBUG_GESTURE) {
      LLog.i(TAG,
          "The inactive finger is released with pointerId = " + pointerId
              + ", no need to update mActivePointerId = " + mActivePointerId
              + " and mLastMotionX = " + mLastMotionX);
    }
  }

  private void endDrag() {
    mIsBeingDragged = false;
    mActivePointerId = INVALID_POINTER;
    recycleVelocityTracker();
    stopNestedScroll(ViewCompat.TYPE_TOUCH);
  }

  public void setEnableNewNested(boolean value) {
    mEnableNewNested = value;
  }

  public void setEnableNewBounce(boolean value) {
    mEnableNewBounce = value;
  }

  public void setBounceScrollRange(int scrollRange, int bounceScrollRange) {
    if (mBounceGestureHelper != null) {
      if (scrollRange != mBounceGestureHelper.getScrollRange()
          || bounceScrollRange != mBounceGestureHelper.getBounceScrollRange()) {
        requestLayout();
      }
      mBounceGestureHelper.setScrollRange(scrollRange);
      mBounceGestureHelper.setBounceScrollRange(bounceScrollRange);
    }
  }

  /********* NestedScrollingChild2 begin *********/

  @Override
  public void setNestedScrollingEnabled(boolean enabled) {
    mChildHelper.setNestedScrollingEnabled(enabled);
  }

  @Override
  public boolean isNestedScrollingEnabled() {
    return mChildHelper.isNestedScrollingEnabled();
  }

  @Override
  public boolean hasNestedScrollingParent(int type) {
    return mChildHelper.hasNestedScrollingParent(type);
  }

  @Override
  public boolean hasNestedScrollingParent() {
    return this.hasNestedScrollingParent(ViewCompat.TYPE_TOUCH);
  }

  @Override
  public boolean startNestedScroll(int axes, int type) {
    return mChildHelper.startNestedScroll(axes, type);
  }

  @Override
  public boolean startNestedScroll(int axes) {
    return this.startNestedScroll(axes, ViewCompat.TYPE_TOUCH);
  }

  @Override
  public void stopNestedScroll(int type) {
    mChildHelper.stopNestedScroll(type);
  }

  @Override
  public void stopNestedScroll() {
    this.stopNestedScroll(ViewCompat.TYPE_TOUCH);
  }

  @Override
  public boolean dispatchNestedScroll(int dxConsumed, int dyConsumed, int dxUnconsumed,
      int dyUnconsumed, int[] offsetInWindow, int type) {
    return mChildHelper.dispatchNestedScroll(
        dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow, type);
  }

  @Override
  public boolean dispatchNestedScroll(
      int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed, int[] offsetInWindow) {
    return this.dispatchNestedScroll(
        dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow, ViewCompat.TYPE_TOUCH);
  }

  @Override
  public boolean dispatchNestedPreScroll(
      int dx, int dy, int[] consumed, int[] offsetInWindow, int type) {
    return mChildHelper.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, type);
  }

  @Override
  public boolean dispatchNestedPreScroll(int dx, int dy, int[] consumed, int[] offsetInWindow) {
    return this.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, ViewCompat.TYPE_TOUCH);
  }

  @Override
  public boolean dispatchNestedFling(float velocityX, float velocityY, boolean consumed) {
    return mChildHelper.dispatchNestedFling(velocityX, velocityY, consumed);
  }

  @Override
  public boolean dispatchNestedPreFling(float velocityX, float velocityY) {
    return mChildHelper.dispatchNestedPreFling(velocityX, velocityY);
  }

  /********* NestedScrollingChild2 end *********/

  /********* NestedScrollingParent2 begin *********/

  @Override
  public boolean onStartNestedScroll(View child, View target, int nestedScrollAxes, int type) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      return mBounceGestureHelper.onStartNestedScroll(child, target, nestedScrollAxes, type);
    }
    return (nestedScrollAxes & ViewCompat.SCROLL_AXIS_HORIZONTAL) != 0;
  }

  @Override
  public boolean onStartNestedScroll(View child, View target, int nestedScrollAxes) {
    return this.onStartNestedScroll(child, target, nestedScrollAxes, ViewCompat.TYPE_TOUCH);
  }

  @Override
  public void onNestedScrollAccepted(View child, View target, int nestedScrollAxes, int type) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      mBounceGestureHelper.onNestedScrollAccepted(child, target, nestedScrollAxes, type);
      return;
    }
    mParentHelper.onNestedScrollAccepted(child, target, nestedScrollAxes, type);
    startNestedScroll(ViewCompat.SCROLL_AXIS_HORIZONTAL, type);
  }

  @Override
  public void onNestedScrollAccepted(View child, View target, int nestedScrollAxes) {
    this.onNestedScrollAccepted(child, target, nestedScrollAxes, ViewCompat.TYPE_TOUCH);
  }

  @Override
  public void onStopNestedScroll(View target, int type) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      mBounceGestureHelper.onStopNestedScroll(target, type);
      return;
    }
    mParentHelper.onStopNestedScroll(target, type);
    this.stopNestedScroll(type);
  }

  @Override
  public void onStopNestedScroll(View target) {
    this.onStopNestedScroll(target, ViewCompat.TYPE_TOUCH);
  }

  @Override
  public void onNestedScroll(
      View target, int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed, int type) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      mBounceGestureHelper.onNestedScroll(
          target, dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, type);
      return;
    }
    final int oldScrollX = getScrollX();
    scrollBy(dxUnconsumed, 0);
    final int myConsumed = getScrollX() - oldScrollX;
    final int myUnconsumed = dxUnconsumed - myConsumed;
    this.dispatchNestedScroll(myConsumed, 0, myUnconsumed, 0, null, type);
  }

  @Override
  public void onNestedScroll(
      View target, int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed) {
    this.onNestedScroll(
        target, dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, ViewCompat.TYPE_TOUCH);
  }

  @Override
  public void onNestedPreScroll(View target, int dx, int dy, int[] consumed, int type) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      mBounceGestureHelper.onNestedPreScroll(target, dx, dy, consumed, type);
      return;
    }
    this.dispatchNestedPreScroll(dx, dy, consumed, (int[]) null, type);
  }

  @Override
  public void onNestedPreScroll(View target, int dx, int dy, int[] consumed) {
    this.dispatchNestedPreScroll(dx, dy, consumed, (int[]) null, ViewCompat.TYPE_TOUCH);
  }

  @Override
  public boolean onNestedFling(View target, float velocityX, float velocityY, boolean consumed) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      return mBounceGestureHelper.onNestedFling(target, velocityX, velocityY, consumed);
    }
    if (!consumed) {
      final int scrollX = getScrollX();
      final int scrollRange = mUIScrollView.getScrollRange();
      final boolean canFling =
          (scrollX > 0 || velocityX > 0) && (scrollX < scrollRange || velocityX < 0);
      if (!this.dispatchNestedPreFling(velocityX, 0)) {
        this.dispatchNestedFling(velocityX, 0, canFling);
        fling((int) velocityX);
      }
      return true;
    }
    return false;
  }

  @Override
  public boolean onNestedPreFling(View target, float velocityX, float velocityY) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      return mBounceGestureHelper.onNestedPreFling(target, velocityX, velocityY);
    }
    return this.dispatchNestedPreFling(velocityX, velocityY);
  }

  @Override
  public int getNestedScrollAxes() {
    return mParentHelper.getNestedScrollAxes();
  }

  /********* NestedScrollingParent2 end *********/
}
