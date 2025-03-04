// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll;

import android.view.MotionEvent;
import android.view.View;
import android.widget.OverScroller;
import androidx.core.view.NestedScrollingParentHelper;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.base.LLog;

public class BounceGestureHelper {
  private static final String TAG = "BounceGestureHelper";
  private static final boolean DEBUG = false;
  private boolean mIsVertical;
  private boolean mHasTriggerStartBounce;
  private boolean mHasTriggerEndBounce;
  private final View mView;
  private int mScrollRange;
  private int mBounceScrollRange;
  private final NestedScrollingParentHelper mParentHelper;

  public BounceGestureHelper(
      View view, boolean isVertical, NestedScrollingParentHelper parentHelper) {
    mView = view;
    mIsVertical = isVertical;
    mParentHelper = parentHelper;
  }

  public void setBounceScrollRange(int bounceScrollRange) {
    mBounceScrollRange = bounceScrollRange;
  }

  public void setScrollRange(int scrollRange) {
    mScrollRange = scrollRange;
  }

  public boolean onInterceptTouchEvent(MotionEvent ev) {
    OverScroller scroller;
    // If scroller of BounceLayout is not finished, we force intercept motion event and nested child
    // can not consume.
    if (!mIsVertical && mView instanceof NestedHorizontalScrollView) {
      NestedHorizontalScrollView nestedHorizontalScrollView = (NestedHorizontalScrollView) mView;
      scroller = nestedHorizontalScrollView.getHScroller();
      if (scroller != null && !scroller.isFinished()) {
        return true;
      }
    } else if (mIsVertical && mView instanceof NestedScrollView) {
      NestedScrollView nestedScrollView = (NestedScrollView) mView;
      scroller = nestedScrollView.getVScroller();
      if (scroller != null && !scroller.isFinished()) {
        return true;
      }
    }
    return false;
  }

  public boolean onTouchEvent(MotionEvent ev) {
    // If used as BounceLayout, no need to handle any motion event.
    return false;
  }

  public boolean onStartNestedScroll(View child, View target, int nestedScrollAxes, int type) {
    return (nestedScrollAxes
               & (mIsVertical ? ViewCompat.SCROLL_AXIS_VERTICAL
                              : ViewCompat.SCROLL_AXIS_HORIZONTAL))
        != 0;
  }

  public void onNestedScrollAccepted(View child, View target, int nestedScrollAxes, int type) {
    mParentHelper.onNestedScrollAccepted(child, target, nestedScrollAxes, type);
  }

  /**
   * React to a TYPE_TOUCH nested scroll operation ending. This method will be called when a nested
   * scroll stops and invoke smoothScrollToInternal() to fold bounce view.
   */
  public void onStopNestedScroll(View target, int type) {
    final int scrollX = mView.getScrollX();
    final int scrollY = mView.getScrollY();
    if (DEBUG) {
      LLog.i(TAG,
          "onStopNestedScroll -> [scrollX, scrollY] = [" + scrollX + ", " + scrollY + "]"
              + ", mBounceScrollRange = " + mBounceScrollRange + ", type = " + type);
    }
    if (ViewCompat.TYPE_NON_TOUCH == type) {
      return;
    }
    if (!mIsVertical && mView instanceof NestedHorizontalScrollView) {
      NestedHorizontalScrollView nestedHorizontalScrollView = (NestedHorizontalScrollView) mView;
      if (scrollX != mBounceScrollRange) {
        nestedHorizontalScrollView.smoothScrollToInternal(mBounceScrollRange, scrollY);
      }
      nestedHorizontalScrollView.stopNestedScroll(type);
    } else if (mIsVertical && mView instanceof NestedScrollView) {
      NestedScrollView nestedScrollView = (NestedScrollView) mView;
      if (scrollY != mBounceScrollRange) {
        nestedScrollView.smoothScrollToInternal(scrollX, mBounceScrollRange);
      }
      nestedScrollView.stopNestedScroll(type);
    }
  }

  /**
   * React to a TYPE_TOUCH nested scroll in progress before the target view consumes a portion of
   * the scroll, and BounceLayout can consume dx or dy first to response gesture.
   */
  public void onNestedPreScroll(View target, int dx, int dy, int[] consumed, int type) {
    if (ViewCompat.TYPE_NON_TOUCH == type) {
      return;
    }
    final int scrollX = mView.getScrollX();
    final int scrollY = mView.getScrollY();
    if (!mIsVertical && mView instanceof NestedHorizontalScrollView) {
      NestedHorizontalScrollView nestedHorizontalScrollView = (NestedHorizontalScrollView) mView;
      onNestedPreScrollHorizontally(nestedHorizontalScrollView, target, scrollX, scrollY, dx,
          mScrollRange, mBounceScrollRange, consumed);
    } else if (mIsVertical && mView instanceof NestedScrollView) {
      NestedScrollView nestedScrollView = (NestedScrollView) mView;
      onNestedPreScrollVertically(nestedScrollView, target, scrollX, scrollY, dy, mScrollRange,
          mBounceScrollRange, consumed);
    }
  }

  protected void onNestedPreScrollHorizontally(NestedHorizontalScrollView bounceLayoutView,
      final View target, final int scrollX, final int scrollY, final int delta,
      final int scrollRange, final int bounceScrollRange, int[] consumed) {
    if (!target.canScrollHorizontally(-1) && delta < 0) {
      if (scrollX + delta < 0) {
        // Note: We should keep mScrollX in the scope of [0, scrollRange]
        bounceLayoutView.scrollTo(0, scrollY);
      } else {
        bounceLayoutView.requestDisallowInterceptTouchEvent(true);
        bounceLayoutView.scrollBy(delta, 0);
      }
      consumed[0] = delta;
    } else if (scrollX < bounceScrollRange && delta > 0) {
      bounceLayoutView.requestDisallowInterceptTouchEvent(true);
      if (scrollX + delta > bounceScrollRange) {
        // Only can consume scroll distance bounceScrollRange - scrollX instead of dx
        bounceLayoutView.scrollBy(bounceScrollRange - scrollX, 0);
      } else {
        bounceLayoutView.scrollBy(delta, 0);
      }
      consumed[0] = delta;
    }

    if (!target.canScrollHorizontally(1) && delta > 0) {
      if (scrollX + delta > scrollRange) {
        // Note: We should keep mScrollX in the scope of [0, scrollRange]
        bounceLayoutView.scrollTo(scrollRange, scrollY);
      } else {
        bounceLayoutView.requestDisallowInterceptTouchEvent(true);
        bounceLayoutView.scrollBy(delta, 0);
      }
      consumed[0] = delta;
    } else if (bounceScrollRange < scrollX && scrollX <= scrollRange && delta < 0) {
      bounceLayoutView.requestDisallowInterceptTouchEvent(true);
      if (scrollX + delta < bounceScrollRange) {
        // Only can consume scroll distance bounceScrollRange - scrollX instead of dx
        bounceLayoutView.scrollBy(bounceScrollRange - scrollX, 0);
      } else {
        bounceLayoutView.scrollBy(delta, 0);
      }
      consumed[0] = delta;
    }
  }

  protected void onNestedPreScrollVertically(NestedScrollView bounceLayoutView, View target,
      final int scrollX, final int scrollY, final int delta, final int scrollRange,
      final int bounceScrollRange, int[] consumed) {
    if (!target.canScrollVertically(-1) && delta < 0) {
      if (scrollY + delta < 0) {
        // Note: We should keep mScrollY in the scope of [0, scrollRange]
        bounceLayoutView.scrollTo(scrollX, 0);
      } else {
        bounceLayoutView.requestDisallowInterceptTouchEvent(true);
        bounceLayoutView.scrollBy(0, delta);
      }
      consumed[1] = delta;
    } else if (scrollY < bounceScrollRange && delta > 0) {
      bounceLayoutView.requestDisallowInterceptTouchEvent(true);
      if (scrollY + delta > bounceScrollRange) {
        // Only can consume scroll distance bounceScrollRange - scrollY instead of dy
        bounceLayoutView.scrollBy(0, bounceScrollRange - scrollY);
      } else {
        bounceLayoutView.scrollBy(0, delta);
      }
      consumed[1] = delta;
    }

    if (!target.canScrollVertically(1) && delta > 0) {
      if (scrollY + delta > scrollRange) {
        // Note: We should keep mScrollY in the scope of [0, scrollRange]
        bounceLayoutView.scrollTo(scrollX, scrollRange);
      } else {
        bounceLayoutView.requestDisallowInterceptTouchEvent(true);
        bounceLayoutView.scrollBy(0, delta);
      }
      consumed[1] = delta;
    } else if (bounceScrollRange < scrollY && scrollY <= scrollRange && delta < 0) {
      bounceLayoutView.requestDisallowInterceptTouchEvent(true);
      if (scrollY + delta < bounceScrollRange) {
        // Only can consume scroll distance bounceScrollRange - scrollY instead of dy
        bounceLayoutView.scrollBy(0, bounceScrollRange - scrollY);
      } else {
        bounceLayoutView.scrollBy(0, delta);
      }
      consumed[1] = delta;
    }
  }

  /**
   * React to a TYPE_NO_TOUCH nested scroll in progress. This method is used to response child view
   * fling gesture.
   */
  public void onNestedScroll(
      View target, int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed, int type) {
    if (DEBUG) {
      LLog.i(TAG,
          "onNestedScroll -> "
              + "dx consumed = " + dxConsumed + ", dx unconsumed = " + dxUnconsumed
              + ", type = " + type);
    }
    if (ViewCompat.TYPE_TOUCH == type) {
      return;
    }
    final int scrollX = mView.getScrollX();
    final int scrollY = mView.getScrollY();
    if (!mIsVertical && mView instanceof NestedHorizontalScrollView) {
      NestedHorizontalScrollView nestedHorizontalScrollView = (NestedHorizontalScrollView) mView;
      final OverScroller scroller = nestedHorizontalScrollView.getHScroller();
      if (scroller == null) {
        return;
      }
      onNestedScrollHorizontally(nestedHorizontalScrollView, scroller, scrollX, scrollY, dxConsumed,
          dxUnconsumed, mScrollRange, mBounceScrollRange);
    } else if (mIsVertical && mView instanceof NestedScrollView) {
      NestedScrollView nestedScrollView = (NestedScrollView) mView;
      final OverScroller scroller = nestedScrollView.getVScroller();
      if (scroller == null) {
        return;
      }
      onNestedScrollVertically(nestedScrollView, scroller, scrollX, scrollY, dyConsumed,
          dyUnconsumed, mScrollRange, mBounceScrollRange);
    }
  }

  protected void onNestedScrollHorizontally(NestedHorizontalScrollView bounceLayoutView,
      final OverScroller scroller, final int scrollX, final int scrollY, final int deltaConsumed,
      final int deltaUnconsumed, final int scrollRange, final int bounceScrollRange) {
    if (!mHasTriggerStartBounce && deltaConsumed < 0 && deltaUnconsumed < 0) {
      bounceLayoutView.smoothScrollToInternal(0, scrollY);
      mHasTriggerStartBounce = true;
    } else if (mHasTriggerStartBounce && scrollX == 0 && scroller.isFinished()) {
      bounceLayoutView.smoothScrollToInternal(bounceScrollRange, scrollY);
      mHasTriggerStartBounce = false;
    }
    if (!mHasTriggerEndBounce && deltaConsumed > 0 && deltaUnconsumed > 0) {
      bounceLayoutView.smoothScrollToInternal(scrollRange, scrollY);
      mHasTriggerEndBounce = true;
    } else if (mHasTriggerEndBounce && scrollX == scrollRange && scroller.isFinished()) {
      bounceLayoutView.smoothScrollToInternal(bounceScrollRange, scrollY);
      mHasTriggerEndBounce = false;
    }
  }

  protected void onNestedScrollVertically(NestedScrollView bounceLayoutView,
      final OverScroller scroller, final int scrollX, final int scrollY, final int deltaConsumed,
      final int deltaUnconsumed, final int scrollRange, final int bounceScrollRange) {
    if (!mHasTriggerStartBounce && deltaConsumed < 0 && deltaUnconsumed < 0) {
      bounceLayoutView.smoothScrollToInternal(scrollX, 0);
      mHasTriggerStartBounce = true;
    } else if (mHasTriggerStartBounce && scrollY == 0 && scroller.isFinished()) {
      bounceLayoutView.smoothScrollToInternal(scrollX, bounceScrollRange);
      mHasTriggerStartBounce = false;
    }
    if (!mHasTriggerEndBounce && deltaConsumed > 0 && deltaUnconsumed > 0) {
      bounceLayoutView.smoothScrollToInternal(scrollX, scrollRange);
      mHasTriggerEndBounce = true;
    } else if (mHasTriggerEndBounce && scrollY == scrollRange && scroller.isFinished()) {
      bounceLayoutView.smoothScrollToInternal(scrollX, bounceScrollRange);
      mHasTriggerEndBounce = false;
    }
  }

  public boolean onNestedFling(View target, float velocityX, float velocityY, boolean consumed) {
    return false;
  }

  public boolean onNestedPreFling(View target, float velocityX, float velocityY) {
    return false;
  }

  public void checkNestedStateByDeadLine() {
    final int scrollX = mView.getScrollX();
    final int scrollY = mView.getScrollY();
    if (!mIsVertical && mView instanceof NestedHorizontalScrollView) {
      NestedHorizontalScrollView nestedHorizontalScrollView = (NestedHorizontalScrollView) mView;
      if (mHasTriggerStartBounce) {
        nestedHorizontalScrollView.smoothScrollToInternal(mBounceScrollRange, scrollY);
        mHasTriggerStartBounce = false;
      } else if (mHasTriggerEndBounce) {
        nestedHorizontalScrollView.smoothScrollToInternal(mBounceScrollRange, scrollY);
        mHasTriggerEndBounce = false;
      }
    } else if (mIsVertical && mView instanceof NestedScrollView) {
      NestedScrollView nestedScrollView = (NestedScrollView) mView;
      if (mHasTriggerStartBounce) {
        nestedScrollView.smoothScrollToInternal(scrollX, mBounceScrollRange);
        mHasTriggerStartBounce = false;
      } else if (mHasTriggerEndBounce) {
        nestedScrollView.smoothScrollToInternal(scrollX, mBounceScrollRange);
        mHasTriggerEndBounce = false;
      }
    }
  }

  public int getScrollRange() {
    return mScrollRange;
  }

  public int getBounceScrollRange() {
    return mBounceScrollRange;
  }
}
