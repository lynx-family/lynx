// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.content.Context;
import android.text.TextUtils;
import android.view.View;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.LayoutManager;
import androidx.recyclerview.widget.StaggeredGridLayoutManager;
import com.lynx.react.bridge.Callback;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.list.ListViewHolder.WrapView;
import java.lang.ref.WeakReference;

public class ListScroller {
  private static int SCROLL_PX_PER_FRAME = 80;
  private final static int DECELERATE_FACTOR = 3;
  private final static int SCROLL_DIRECTION_UP = -1;
  private final static int SCROLL_DIRECTION_DOWN = 1;

  private final RecyclerView mRecyclerview;
  private SmoothScroller mSmoothScroller;

  ListScroller(Context context, RecyclerView recyclerview) {
    SCROLL_PX_PER_FRAME = calculatePxPerFrame(context.getResources().getDisplayMetrics());
    mRecyclerview = recyclerview;
    mSmoothScroller = new SmoothScroller(mRecyclerview);
  }

  /**
   * set the layout orientation of list
   * @param isVerticalOrientation
   */
  void setVerticalOrientation(boolean isVerticalOrientation) {
    mSmoothScroller.setVerticalOrientation(isVerticalOrientation);
  }

  void scrollToPositionInner(int position) {
    scrollToPositionInner(position, 0, null);
  }

  // non smooth scroll
  void scrollToPositionInner(int position, int offsetVal, Callback callback) {
    LLog.i(UIList.TAG,
        "ListScroller scrollToPositionNoSmooth: position=" + position + ", offset=" + offsetVal);
    if (mSmoothScroller != null && mSmoothScroller.mWorking) {
      LLog.e(UIList.TAG, "ListScroller scrollToPositionSmoothly is scrolling ");
      if (callback != null) {
        callback.invoke(
            LynxUIMethodConstants.UNKNOWN, "dumplicated, scrollToPositionSmoothly is working");
      }
      return;
    }

    // stop fling
    if (mRecyclerview != null) {
      mRecyclerview.stopScroll();
      mRecyclerview.stopNestedScroll();
    }

    LayoutManager layoutManager = mRecyclerview.getLayoutManager();
    if (layoutManager instanceof LinearLayoutManager) {
      LinearLayoutManager llm = (LinearLayoutManager) layoutManager;
      llm.scrollToPositionWithOffset(position, offsetVal);
    } else if (layoutManager instanceof StaggeredGridLayoutManager) {
      StaggeredGridLayoutManager slm = (StaggeredGridLayoutManager) layoutManager;
      slm.scrollToPositionWithOffset(position, offsetVal);
    }
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  // smooth scroll
  void scrollToPositionSmoothly(int position, String alignTo, int offset, Callback callback) {
    LLog.i(UIList.TAG,
        "ListScroller scrollToPositionSmoothly: position=" + position + ", offset=" + offset
            + ", alignTo: " + (alignTo != null ? alignTo : "none"));
    mSmoothScroller.start(position, alignTo, offset, callback);
  }

  private static class SmoothScroller implements Runnable {
    private WeakReference<RecyclerView> mViewRef;
    private Callback mCallback;
    private int mTargetPosition;
    private String mAlignTo;
    private int mOffset;

    private int mTopPosition;
    private int mBottomPosition;
    private View mTargetView;
    // flag:  make sure only one runnable task in view queue
    private boolean mWorking;
    private boolean mVerticalOrientation = true;

    private RecyclerView.SimpleOnItemTouchListener mTouchListener =
        new RecyclerView.SimpleOnItemTouchListener() {
          @Override
          public boolean onInterceptTouchEvent(RecyclerView rv, android.view.MotionEvent e) {
            mWorking = false;
            return false;
          }
        };

    SmoothScroller(RecyclerView recyclerview) {
      mViewRef = new WeakReference<>(recyclerview);
      mCallback = null;
      mTargetView = null;
      mWorking = false;
    }

    void setVerticalOrientation(boolean verticalOrientation) {
      this.mVerticalOrientation = verticalOrientation;
    }

    /* start scroll
     *  1. set parameters
     *  2. post this to queue
     *  */
    void start(int position, String alignTo, int offset, Callback callback) {
      RecyclerView recyclerview = mViewRef.get();
      if (recyclerview == null || recyclerview.getLayoutManager() == null
          || recyclerview.getChildCount() == 0) {
        mWorking = false;
        callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "can not scroll before init");
        return;
      } else if (!recyclerview.isAttachedToWindow()) {
        mWorking = false;
        LLog.e(
            UIList.TAG, "ListScroller start: early return due to view is not attached to window.");
        callback.invoke(LynxUIMethodConstants.INVALID_STATE_ERROR,
            "can not invoke scroll when the view is not attached to window.");
        return;
      }
      mCallback = callback;
      mTargetPosition = position;
      mAlignTo = alignTo;
      mOffset = offset;
      mTargetView = null;
      if (mWorking == false) {
        mWorking = true;
        recyclerview.stopScroll();
        recyclerview.stopNestedScroll();
        recyclerview.addOnItemTouchListener(mTouchListener);
        recyclerview.post(this);
      }
    }

    private void stop() {
      LLog.i(UIList.TAG, "ListScroller stop");
      RecyclerView recyclerview = mViewRef.get();
      mWorking = false;
      if (recyclerview != null) {
        recyclerview.removeOnItemTouchListener(mTouchListener);
      }
    }

    @Override
    public void run() {
      RecyclerView recyclerview = mViewRef.get();
      if (recyclerview == null || recyclerview.getLayoutManager() == null
          || recyclerview.getChildCount() == 0) {
        stop();
        return;
      }
      if (!mWorking) {
        mCallback.invoke(LynxUIMethodConstants.SUCCESS);
        stop();
        return;
      }

      if (work(recyclerview)) {
        recyclerview.post(this);
      } else {
        stop();
      }
    }

    /*  do real work
     *  smooth scroll is achieved by multi scroll
     *  step 1: find next Y to scroll  untill target position is visiable
     *  step 2: compute accurate position/offset, and decelerate scroll multi time
     *  note: each step, waiting for layout completed.
     *  @return  true if need work again on next frame, false if nothing to do
     * */
    private boolean work(RecyclerView recyclerview) {
      LayoutManager layoutManager = recyclerview.getLayoutManager();
      mTargetPosition = Math.min(mTargetPosition, layoutManager.getItemCount() - 1);
      mTargetPosition = Math.max(mTargetPosition, 0);
      // waiting layout finished,  if not finished, delay to next vsync
      if (!isChildrenLayoutFinished(recyclerview)) {
        return true;
      }

      updateChildPosition(recyclerview);
      int dy;
      if (mTargetView == null) {
        // target view is not visable
        int direction =
            (mTopPosition > mTargetPosition) ? SCROLL_DIRECTION_UP : SCROLL_DIRECTION_DOWN;
        int cellsCountOnScreen = mBottomPosition - mTopPosition + 1;
        int cellsCountNeedScroll = (direction == SCROLL_DIRECTION_UP)
            ? (mTopPosition - mTargetPosition)
            : (mTargetPosition - mBottomPosition);

        int containerSize =
            mVerticalOrientation ? recyclerview.getHeight() : recyclerview.getWidth();
        int estimateDistance = cellsCountNeedScroll * containerSize / cellsCountOnScreen;

        boolean farAway = isFarAwayToTarget(cellsCountNeedScroll, estimateDistance, containerSize);
        if (farAway) {
          // scroll to near target by middle point
          int curTarget = (mTopPosition > mTargetPosition)
              ? (mTopPosition - mTargetPosition) / 2 + mTargetPosition
              : (mBottomPosition - mTargetPosition) / 2 + mTargetPosition;
          recyclerview.scrollToPosition(curTarget);
          return true;
        } else {
          // scroll slower when near to target
          dy = Math.min(containerSize, SCROLL_PX_PER_FRAME * cellsCountNeedScroll) * direction;
        }
      } else {
        // target view is visiable, adjust to offset
        int remain = onTargetFound(recyclerview);
        if (remain == 0) {
          // congratulations
          mCallback.invoke(LynxUIMethodConstants.SUCCESS);
          return false;
        } else {
          // decelerate scroll to target
          dy = Math.abs(remain) / DECELERATE_FACTOR;
          dy = Math.max(dy, 1);
          dy = Math.min(dy, SCROLL_PX_PER_FRAME);
          dy = remain > 0 ? dy : -dy;

          int viewSize = mVerticalOrientation ? mTargetView.getHeight() : mTargetView.getWidth();
          if ((viewSize == 0 && dy == 1)) {
            // corner case: a view has zero height, can not align to screen, do not scroll
            mCallback.invoke(LynxUIMethodConstants.SUCCESS);
            return false;
          }
        }
      }
      boolean canScroll = tryScroll(recyclerview, mVerticalOrientation, dy);
      if (!canScroll) {
        mCallback.invoke(LynxUIMethodConstants.PARAM_INVALID, "can not scroll when come to border");
      }
      return canScroll;
    }

    private boolean isFarAwayToTarget(
        int cellsCountNeedScroll, int estimateDistance, int containerSize) {
      return cellsCountNeedScroll > 30 && estimateDistance > 10 * containerSize;
    }

    /* check target view is on the right layout
     *  return offset that need scroll
     * */
    private int onTargetFound(RecyclerView recyclerview) {
      if (mTargetView == null) {
        return 0;
      }
      LayoutManager layoutManager = recyclerview.getLayoutManager();

      // calculate value depending on layout Orientation
      int startOfView = mVerticalOrientation ? layoutManager.getDecoratedTop(mTargetView)
                                             : layoutManager.getDecoratedLeft(mTargetView);
      int targetViewSize = mVerticalOrientation
          ? layoutManager.getDecoratedMeasuredHeight(mTargetView)
          : layoutManager.getDecoratedMeasuredWidth(mTargetView);
      int containerSize =
          mVerticalOrientation ? layoutManager.getHeight() : layoutManager.getWidth();
      int paddingStartSize =
          mVerticalOrientation ? layoutManager.getPaddingTop() : layoutManager.getPaddingLeft();
      int paddingEndSize =
          mVerticalOrientation ? layoutManager.getPaddingBottom() : layoutManager.getPaddingRight();
      int availableHeightOfRv = containerSize - paddingStartSize - paddingEndSize;
      // return delta of view Top and target-Y
      if (TextUtils.equals(mAlignTo, UIList.ALIGN_TO_MIDDLE)) {
        return startOfView
            - (mOffset + paddingStartSize + (availableHeightOfRv - targetViewSize) / 2);
      } else if (TextUtils.equals(mAlignTo, UIList.ALIGN_TO_BOTTOM)) {
        return startOfView - (mOffset + paddingStartSize + availableHeightOfRv - targetViewSize);
      } else {
        return startOfView - (mOffset + paddingStartSize);
      }
    }

    /*  check layout finished
     *  each scroll action will waiting for layout finished
     * */
    private boolean isChildrenLayoutFinished(RecyclerView recyclerview) {
      int childCount = recyclerview.getChildCount();
      for (int i = 0; i < childCount; ++i) {
        WrapView child = (WrapView) recyclerview.getChildAt(i);
        if (child.mLayoutStatus != ListViewHolder.COMPONENT_LAYOUT_EFFECT) {
          ListViewHolder holder = (ListViewHolder) recyclerview.getChildViewHolder(child);
          if (holder.getUIComponent() != null) {
            return false;
          }
        }
      }
      return true;
    }

    /*
     *  find first and last child's position.
     *  it's used to check target position is on screen
     * */
    private void updateChildPosition(RecyclerView recyclerview) {
      LayoutManager layoutManager = recyclerview.getLayoutManager();
      int childCount = recyclerview.getChildCount();
      mTopPosition = layoutManager.getItemCount();
      mBottomPosition = 0;
      for (int i = 0; i < childCount; ++i) {
        WrapView child = (WrapView) recyclerview.getChildAt(i);
        RecyclerView.LayoutParams lp = (RecyclerView.LayoutParams) child.getLayoutParams();
        int pos = lp.getViewLayoutPosition();
        mBottomPosition = Math.max(pos, mBottomPosition);
        mTopPosition = Math.min(pos, mTopPosition);
        if (pos == mTargetPosition) {
          mTargetView = child;
          break;
        }
      }
    }
  }

  private int calculatePxPerFrame(android.util.DisplayMetrics displayMetrics) {
    return (int) (displayMetrics.densityDpi / 4);
  }

  /**
   * True if this LayoutManager can scroll the current contents vertically/horizontally
   *
   * @param recyclerView          the container view of list
   * @param isVerticalOrientation layout orientation
   * @param scrollDirection       scrollOrientaion
   * @return
   */
  private static boolean canScroll(
      RecyclerView recyclerView, boolean isVerticalOrientation, int scrollDirection) {
    RecyclerView.LayoutManager layoutManager = recyclerView.getLayoutManager();
    int childCount = recyclerView.getChildCount();
    boolean firstItemVisible = recyclerView.findViewHolderForLayoutPosition(0) != null;
    boolean lastItemVisible =
        recyclerView.findViewHolderForLayoutPosition(layoutManager.getItemCount() - 1) != null;
    if ((firstItemVisible && SCROLL_DIRECTION_UP == scrollDirection)
        || (lastItemVisible && SCROLL_DIRECTION_DOWN == scrollDirection)) {
      // the min and max Y-offset of visible cells.

      int startBoundPx = Integer.MAX_VALUE;
      int endBoundPx = Integer.MIN_VALUE;
      for (int i = 0; i < childCount; ++i) {
        android.view.View child = recyclerView.getChildAt(i);
        startBoundPx = isVerticalOrientation
            ? Math.min(layoutManager.getDecoratedTop(child), startBoundPx)
            : Math.min(layoutManager.getDecoratedLeft(child), startBoundPx);
        endBoundPx = isVerticalOrientation
            ? Math.max(layoutManager.getDecoratedBottom(child), endBoundPx)
            : Math.max(layoutManager.getDecoratedRight(child), endBoundPx);
      }
      int startOfVisibleArea =
          isVerticalOrientation ? layoutManager.getPaddingTop() : layoutManager.getPaddingLeft();
      int endOfVisibleArea = isVerticalOrientation
          ? layoutManager.getHeight() - layoutManager.getPaddingBottom()
          : layoutManager.getWidth() - layoutManager.getPaddingRight();
      if (firstItemVisible && SCROLL_DIRECTION_UP == scrollDirection) {
        // check if the top border reached
        return (startBoundPx < startOfVisibleArea);
      }
      if (lastItemVisible && SCROLL_DIRECTION_DOWN == scrollDirection) {
        // check if the bottom border reached
        return (endBoundPx > endOfVisibleArea);
      }
    }
    return true;
  }

  /* check canScroll before scroll to avoid endless-loop */
  private static boolean tryScroll(
      RecyclerView recyclerView, boolean isVerticalOrientation, int distance) {
    if (distance == 0) {
      return false;
    }
    if (!canScroll(recyclerView, isVerticalOrientation,
            distance > 0 ? SCROLL_DIRECTION_DOWN : SCROLL_DIRECTION_UP)) {
      return false;
    }
    if (isVerticalOrientation) {
      recyclerView.scrollBy(0, distance);
    } else {
      recyclerView.scrollBy(distance, 0);
    }
    return true;
  }
}
