// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
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

public class NestedScrollView
    extends ScrollView implements NestedScrollingParent2, NestedScrollingChild2 {
  private static final String TAG = "LynxNestedScrollView";

  private NestedScrollingParentHelper mParentHelper;
  private NestedScrollingChildHelper mChildHelper;
  private UIScrollView mUIScrollView;
  private boolean mEnableNewNested = false;
  protected boolean mEnableNewBounce = false;
  private int mLastFlingScrollY;
  private final int[] mScrollConsumed = new int[2];
  private OverScroller mVScroller;
  protected BounceGestureHelper mBounceGestureHelper;

  public NestedScrollView(Context context, UIScrollView uiScrollView) {
    super(context);
    this.mUIScrollView = uiScrollView;
    mParentHelper = new NestedScrollingParentHelper(this);
    mChildHelper = new NestedScrollingChildHelper(this);
    mBounceGestureHelper = new BounceGestureHelper(this, true, mParentHelper);
    this.setNestedScrollingEnabled(true);
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      return mBounceGestureHelper.onInterceptTouchEvent(ev);
    }
    return super.onInterceptTouchEvent(ev);
  }

  @Override
  public boolean onTouchEvent(MotionEvent ev) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      return mBounceGestureHelper.onTouchEvent(ev);
    }
    return super.onTouchEvent(ev);
  }

  /**
   * Fling the scroll view and start a nested scroll operation along the vertical axes if
   * mEnableNewNested is true.
   *
   * @param velocityY The initial velocity in the Y direction. Positive numbers mean that we want to
   *     scroll towards the top.
   */
  @Override
  public void fling(int velocityY) {
    LLog.i(TAG, "fling with vel = " + velocityY);
    OverScroller scroller;
    if (!mEnableNewNested || (scroller = getVScroller()) == null) {
      super.fling(velocityY);
      return;
    }
    if (getChildCount() > 0) {
      // Start nested scroll for TYPE_NON_TOUCH
      this.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL, ViewCompat.TYPE_NON_TOUCH);
      scroller.fling(getScrollX(), getScrollY(), // start position
          0, velocityY, // fling velocity
          0, 0, // min and max scrollX
          Integer.MIN_VALUE, Integer.MAX_VALUE, // min and max scrollY
          0, 0); // overscroll
      ViewCompat.postInvalidateOnAnimation(this);
      mLastFlingScrollY = getScrollY();
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
    if (!mEnableNewNested || (scroller = getVScroller()) == null) {
      super.computeScroll();
      return;
    }
    if (scroller.computeScrollOffset()) {
      final int y = scroller.getCurrY();
      int dy = y - mLastFlingScrollY;
      if (dispatchNestedPreScroll(0, dy, mScrollConsumed, null, ViewCompat.TYPE_NON_TOUCH)) {
        dy -= mScrollConsumed[1];
      }
      if (dy != 0) {
        final int scrollRange = mUIScrollView.getScrollRange();
        final int oldScrollY = getScrollY();
        int newScrollY = oldScrollY + dy;
        final boolean clampedY = newScrollY < 0 || newScrollY > scrollRange;
        newScrollY = MathUtils.clamp(newScrollY, 0, scrollRange);
        if (clampedY && !this.hasNestedScrollingParent(ViewCompat.TYPE_NON_TOUCH)) {
          scroller.springBack(0, newScrollY, 0, 0, 0, scrollRange);
        }
        super.scrollTo(getScrollX(), newScrollY);
        final int scrolledDeltaY = getScrollY() - oldScrollY;
        final int unconsumedY = dy - scrolledDeltaY;
        this.dispatchNestedScroll(
            0, scrolledDeltaY, 0, unconsumedY, null, ViewCompat.TYPE_NON_TOUCH);
      }
      // Finally update the scroll positions and post an invalidation
      mLastFlingScrollY = y;
      ViewCompat.postInvalidateOnAnimation(this);
    } else {
      if (this.hasNestedScrollingParent(ViewCompat.TYPE_NON_TOUCH)) {
        this.stopNestedScroll(ViewCompat.TYPE_NON_TOUCH);
      }
      mLastFlingScrollY = 0;
      if (mBounceGestureHelper != null && mEnableNewBounce) {
        mBounceGestureHelper.checkNestedStateByDeadLine();
      }
    }
  }

  protected void smoothScrollToInternal(int x, int y) {
    if (mEnableNewNested && getVScroller() != null) {
      // Note: we should update mLastFlingScrollY value for calculating correct scroll distance in
      // computeScroll() method.
      mLastFlingScrollY = getScrollY();
    }
    super.smoothScrollTo(x, y);
  }

  protected OverScroller getVScroller() {
    if (mVScroller != null) {
      return mVScroller;
    }
    try {
      Field scrollerField = ScrollView.class.getDeclaredField("mScroller");
      if (scrollerField != null) {
        scrollerField.setAccessible(true);
        Object scroller = scrollerField.get(this);
        if (scroller instanceof OverScroller) {
          mVScroller = (OverScroller) scroller;
        }
      }
    } catch (NoSuchFieldException e) {
      LLog.e(TAG, "Failed to get mScroller field of ScrollView!");
    } catch (IllegalAccessException e) {
      LLog.e(TAG, "Failed to get mScroller of ScrollView!");
    }
    return mVScroller;
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

  void setPagingTouchSlopIfNeeded() {
    try {
      final ViewConfiguration configuration = ViewConfiguration.get(getContext());
      Field touchSlopField = ScrollView.class.getDeclaredField("mTouchSlop");
      if (configuration != null && touchSlopField != null) {
        int touchSlop = configuration.getScaledPagingTouchSlop();
        touchSlopField.setAccessible(true);
        touchSlopField.set(this, touchSlop);
      }
    } catch (NoSuchFieldException e) {
      LLog.e(TAG, "Failed to get mTouchSlop field of NestedScrollView!");
    } catch (IllegalAccessException e) {
      LLog.e(TAG, "Failed to get mTouchSlop field of NestedScrollView!");
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
  public boolean dispatchNestedPreFling(float velocityX, float velocityY) {
    return mChildHelper.dispatchNestedPreFling(velocityX, velocityY);
  }

  @Override
  public boolean dispatchNestedFling(float velocityX, float velocityY, boolean consumed) {
    return mChildHelper.dispatchNestedFling(velocityX, velocityY, consumed);
  }

  /********* NestedScrollingChild2 end *********/

  /********* NestedScrollingParent2 begin *********/

  @Override
  public boolean onStartNestedScroll(View child, View target, int nestedScrollAxes, int type) {
    if (mEnableNewBounce && mBounceGestureHelper != null) {
      return mBounceGestureHelper.onStartNestedScroll(child, target, nestedScrollAxes, type);
    }
    return (nestedScrollAxes & ViewCompat.SCROLL_AXIS_VERTICAL) != 0;
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
    this.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL, type);
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
    final int oldScrollY = getScrollY();
    scrollBy(0, dyUnconsumed);
    final int myConsumed = getScrollY() - oldScrollY;
    final int myUnconsumed = dyUnconsumed - myConsumed;
    this.dispatchNestedScroll(0, myConsumed, 0, myUnconsumed, null, type);
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
      final int scrollY = getScrollY();
      final int scrollRange = mUIScrollView.getScrollRange();
      final boolean canFling =
          (scrollY > 0 || velocityY > 0) && (scrollY < scrollRange || velocityY < 0);
      if (!this.dispatchNestedPreFling(0, velocityY)) {
        this.dispatchNestedFling(0, velocityY, canFling);
        fling((int) velocityY);
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
