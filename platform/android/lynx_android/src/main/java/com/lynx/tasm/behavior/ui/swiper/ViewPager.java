// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.swiper;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.animation.LinearInterpolator;
import android.widget.Scroller;
import androidx.core.math.MathUtils;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.base.LLog;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public class ViewPager extends ViewGroup {
  public static final String TAG = "LynxSwiperUI#ViewPager";
  public static final boolean DEBUG = false;
  public static final boolean DEBUG_GESTURE = false;
  public static final int SCROLL_DIRECTION_BEGIN = 0;
  public static final int SCROLL_DIRECTION_END = 1;
  private static final int INVALID_TOUCH_POINTER_ID = -1;
  static final int INIT_ITEM_INDEX = -1;
  // If the pager is at least this close to its final position, complete the scroll
  // on touch down and let the user interact with the content inside instead of
  // "catching" the flinging pager.
  private static final int CLOSE_ENOUGH = 2; // dp
  /**
   * SCROLL_STATE_IDLE: Indicates that the pager is in an idle, settled state. The current page is
   * fully in view and no animation is in progress.
   */
  static final int SCROLL_STATE_IDLE = 0;
  /**
   * SCROLL_STATE_DRAGGING: Indicates that the pager is currently being dragged by the user.
   */
  static final int SCROLL_STATE_DRAGGING = 1;
  /**
   * SCROLL_STATE_SETTLING: Indicates that the pager is in the process of settling to a final
   * position.
   */
  static final int SCROLL_STATE_SETTLING = 2;
  static final int INVALID_OFFSET = Integer.MIN_VALUE;
  private boolean mPendingSmoothScroll = false;
  private boolean mHLayoutUpdated = false;
  private boolean mVLayoutUpdated = false;
  private boolean mPropsUpdated = false;
  private int mTotalCount = 0;
  private int mCurrentIndex = INIT_ITEM_INDEX;
  private int mOldCurrentIndex = INIT_ITEM_INDEX;
  private int mPendingCurrentIndex = INIT_ITEM_INDEX;
  private final Scroller mScroller;
  protected boolean mTriggerEvent = false;
  private boolean mIsInit = true;
  private boolean mTouchable = true;
  private boolean mEnableNestedChild = false;
  private boolean mLoop = false;
  private boolean isRTL = false;
  private boolean mKeepItemView = false;
  private boolean mForceCanScroll = false;
  private boolean mEnableViceLoop = false;
  private boolean mLoopChanged = false;
  private int mAnimDuration = 300; // ms
  private int mOrientation = SwiperView.ORIENTATION_HORIZONTAL;
  private int mScrollState = SCROLL_STATE_IDLE;
  private PageTransformer mTransformer;
  private final int mMaxVelocityX;
  private final int mMaxVelocityY;
  private final float mTouchSlop;
  private boolean mFling = false;
  private boolean mIsBeingDragged = false;
  private boolean mIsUnableToDrag = false;
  private boolean mReadyToScroll = false;
  private int mActivePointerId = INVALID_TOUCH_POINTER_ID;
  private int mDragDistance = 0;
  private int mCloseEnough = 0;
  private float mLastX = 0.f;
  private float mLastY = 0.f;
  private float mInitialMotionX = 0.f;
  private float mInitialMotionY = 0.f;
  private Adapter mAdapter;
  private int mPageSize = -1;
  private int mOffset = 0;
  private int mExpectChildSize = -1;
  private int mExpectSize = -1;
  private int mExpectOffset = -1;
  private int mPageMargin = 0;
  private int mMinScrollBoundary = Integer.MIN_VALUE;
  private int mMaxScrollBoundary = Integer.MAX_VALUE;
  private final List<OnPageScrollListener> mPageScrollListeners;
  private boolean mEnableBounce = false;
  private boolean mScrollInToBeginBounce = false;
  private boolean mScrollInToEndBounce = false;
  private boolean mTriggerBounceEvent = false;
  private boolean mIgnoreLayoutUpdate = false;
  private boolean mHandleGesture = true;
  private int mBounceDuration = 300;
  private float mBounceBeginThreshold = -1.f;
  private float mBounceEndThreshold = -1.f;
  protected int mFinalPosition = INVALID_OFFSET;

  /**
   * A PageTransformer is invoked whenever a visible/attached page is scrolled.
   */
  public interface PageTransformer {
    /**
     * Apply a property transformation to the given page.
     *
     * @param viewPager Current viewPager
     * @param page Apply the transformation to this page
     * @param isVertical Scroll direction of this page
     * @param offset Position of page relative to the initial offset
     */
    void transformPage(ViewPager viewPager, View page, boolean isVertical, int offset);

    /**
     * When the page is removed, reset the page's translation, zoom, rotation
     * and other animation properties.
     *
     * @param page Apply the reset operations to this page
     */
    void reset(View page);
  }

  /**
   * Callback interface for responding to scrolling page
   */
  public interface OnPageScrollListener {
    /**
     * This method will be invoked when the current page starts to scroll, either initiated by
     * auto-play or user initiated touch scroll.
     *
     * @param position Position index of middle pager
     * @param isDragged Whether it is currently in the dragging state
     */
    void onPageScrollStart(int position, boolean isDragged);

    /**
     * This method will be invoked when scroll is finished, either initiated by auto-play
     * or user initiated touch scroll.
     *
     * @param position Position index of middle pager
     */
    void onPageScrollEnd(int position);

    /**
     * This method will be invoked when the pager is scrolling, either initiated by auto-play or
     * user initiated touch scroll.
     *
     * @param position Position index of middle pager
     * @param isDragged Whether it is currently in the dragging state
     * @param offsetX Coordinates in the horizontal direction
     * @param offsetY Coordinates in the vertical direction
     */
    void onPageScrolling(int position, boolean isDragged, float offsetX, float offsetY);

    /**
     * This method will be invoked when the position of page is changed, either initiated
     * by auto-play or user initiated touch scroll.
     *
     * @param oldPosition Old position index of middle pager
     * @param newPosition New position index of middle pager
     * @param isInit Initialization flag
     */
    void onPageChange(int oldPosition, int newPosition, boolean isInit);

    /**
     * Called when the scroll state changes.
     * @see ViewPager#SCROLL_STATE_IDLE
     * @see ViewPager#SCROLL_STATE_DRAGGING
     * @see ViewPager#SCROLL_STATE_SETTLING
     */
    void onPageScrollStateChanged(int oldState, int newState);

    /**
     * Called when the scroll to bounce.
     * @param toBegin Scroll to begin bounce
     * @param toEnd Scroll to end bounce
     */
    void onScrollToBounce(boolean toBegin, boolean toEnd);
  }

  public interface Adapter {
    /**
     * Return the number of pages.
     */
    int getCount();

    /**
     * Get the page view for the given position.
     *
     * @param container The containing View in which the page will be shown.
     * @param position The page position.
     */
    View get(ViewGroup container, int position);

    /**
     * Recycle the page view for the given position.
     *
     * @param container The containing View in which the page is shown.
     * @param position The page position.
     * @param view The page view which will be recycled.
     */
    void recycle(ViewGroup container, int position, View view);
  }

  private static class LayoutParams extends ViewGroup.LayoutParams {
    int position;

    public LayoutParams() {
      super(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
    }
  }

  public ViewPager(Context context) {
    this(context, null);
  }

  public ViewPager(Context context, AttributeSet attrs) {
    super(context, attrs);
    final float density = getResources().getDisplayMetrics().density;
    mScroller = new Scroller(context, new LinearInterpolator());
    mMaxVelocityX = mMaxVelocityY = (int) (600 * density);
    mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
    mCloseEnough = (int) (CLOSE_ENOUGH * density);
    mPageScrollListeners = new ArrayList<>();
  }

  public void setCurrentIndex(int index, boolean smooth, int direction) {
    if (mTotalCount < 1 || getChildCount() < 1) {
      return;
    }
    int targetIndex = MathUtils.clamp(index, 0, mTotalCount - 1);
    // Note: need to obtain position info without directly using mCurrentIndex(may be -1)
    View child = getCurrentView();
    int currentPosition = ((LayoutParams) child.getLayoutParams()).position;
    if (DEBUG) {
      LLog.i(TAG,
          "setCurrentIndex: " + currentPosition + "/" + mCurrentIndex + " -> " + targetIndex + "/"
              + index);
    }
    if (currentPosition == targetIndex) {
      return;
    }
    int childExpectSize = getChildExpectSize();
    int scrollDistance = getScrollDistance();
    int pile = getPile() + childExpectSize / 2;
    int childCenterScrollX = (getBegin(child) + getEnd(child)) / 2 - scrollDistance;
    int distance = (targetIndex - currentPosition) * (childExpectSize + mPageMargin);

    if (mLoop || mEnableViceLoop) {
      int expectedTargetIndex = targetIndex;
      if (direction == SCROLL_DIRECTION_END) {
        expectedTargetIndex =
            targetIndex > currentPosition ? targetIndex : targetIndex + mTotalCount;
        distance = (expectedTargetIndex - currentPosition) * (childExpectSize + mPageMargin);
      } else if (direction == SCROLL_DIRECTION_BEGIN) {
        expectedTargetIndex =
            targetIndex > currentPosition ? targetIndex - mTotalCount : targetIndex;
        distance = (expectedTargetIndex - currentPosition) * (childExpectSize + mPageMargin);
      }
      if (!mLoop) {
        mLoopChanged = true;
        mLoop = (direction == SCROLL_DIRECTION_END && targetIndex < currentPosition)
            || (direction == SCROLL_DIRECTION_BEGIN && targetIndex > currentPosition);
      }
    }
    triggerScrollStartEvent();
    if (isRTL()) {
      mScroller.startScroll(
          scrollDistance, 0, childCenterScrollX - pile - distance, 0, smooth ? mAnimDuration : 0);
    } else {
      if (isVertical()) {
        mScroller.startScroll(
            0, scrollDistance, 0, childCenterScrollX - pile + distance, smooth ? mAnimDuration : 0);
      } else {
        mScroller.startScroll(
            scrollDistance, 0, childCenterScrollX - pile + distance, 0, smooth ? mAnimDuration : 0);
      }
    }
    setScrollState(SCROLL_STATE_SETTLING);
    mFinalPosition = isVertical() ? mScroller.getFinalY() : mScroller.getFinalX();
    invalidate();
  }

  public void setAdapter(Adapter adapter) {
    if (mAdapter != null) {
      for (int i = getChildCount() - 1; i > -1; i--) {
        View child = getChildAt(i);
        recycleView(child, false);
      }
    } else {
      removeAllViews();
    }
    mAdapter = adapter;
    if (mAdapter == null) {
      return;
    }
    mTotalCount = mAdapter.getCount();
  }

  public void setOffset(int offset, boolean triggerScrollTo) {
    mOffset = offset;
    // Note: set offset to 0 in another direction in the case of horizontal and vertical switching
    if (triggerScrollTo) {
      if (isVertical()) {
        if (DEBUG) {
          LLog.i(TAG, "setScrollY: " + getScrollY() + " -> " + -offset);
        }
        scrollTo(0, -offset);
      } else {
        if (DEBUG) {
          LLog.i(TAG, "setScrollX: " + getScrollX() + " -> " + -offset);
        }
        scrollTo(-offset, 0);
      }
    }
  }

  protected void computeScrollPosition() {
    if (mAdapter != null && mTotalCount > 0) {
      // The layout information of ViewPager is known, and now calling setOffset -> scrollTo
      // directly will update scrollRange and execute relayoutChildren()
      boolean triggerScroll = mHLayoutUpdated || mVLayoutUpdated || mPropsUpdated;
      if (DEBUG) {
        LLog.i(TAG,
            "computeScrollPosition: triggerScroll = " + triggerScroll
                + ", ignoreLayoutUpdate = " + mIgnoreLayoutUpdate);
      }
      if (mIgnoreLayoutUpdate && !triggerScroll) {
        return;
      }
      setOffset(mOffset, triggerScroll);
      boolean smooth = false;
      int expectedCurrentIndex = INIT_ITEM_INDEX;
      if (mPendingCurrentIndex != INIT_ITEM_INDEX && mPendingCurrentIndex != mCurrentIndex) {
        expectedCurrentIndex = mPendingCurrentIndex;
        smooth = mPendingSmoothScroll;
      } else if (mCurrentIndex != INIT_ITEM_INDEX) {
        expectedCurrentIndex = mCurrentIndex;
      }
      if (DEBUG) {
        LLog.i(TAG, "computeScrollPosition: [" + expectedCurrentIndex + "/" + mTotalCount + "]");
      }
      if (expectedCurrentIndex >= 0 && expectedCurrentIndex < mTotalCount) {
        setCurrentIndex(expectedCurrentIndex, smooth, SCROLL_DIRECTION_END);
      }
    }
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    if (DEBUG) {
      LLog.i(TAG,
          "onSizeChanged: width [" + oldw + " -> " + w + "], height: [" + oldh + " -> " + h + "]");
    }
    super.onSizeChanged(w, h, oldw, oldh);
    if (mHLayoutUpdated || mVLayoutUpdated) {
      computeScrollPosition();
      mHLayoutUpdated = false;
      mVLayoutUpdated = false;
    }
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    if (DEBUG) {
      LLog.i(TAG,
          "onMeasure: [width, height] = [" + getDefaultSize(0, widthMeasureSpec) + ", "
              + getDefaultSize(0, heightMeasureSpec) + "]");
    }
    setMeasuredDimension(getDefaultSize(0, widthMeasureSpec), getDefaultSize(0, heightMeasureSpec));
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {
    if (DEBUG) {
      LLog.i(TAG, "onLayout: [l, t, r, b] = [" + l + ", " + t + ", " + r + ", " + b + "]");
    }
    reMeasureChildren();
    if (mPropsUpdated || mHLayoutUpdated || mVLayoutUpdated) {
      computeScrollPosition();
      mPropsUpdated = false;
      mHLayoutUpdated = false;
      mVLayoutUpdated = false;
    } else {
      relayoutChildren();
    }
  }

  @Override
  public void setPadding(int left, int top, int right, int bottom) {
    int old = getPaddingLeft();
    super.setPadding(left, top, right, bottom);
    if (isVertical()) {
      scrollBy(0, old - left);
    } else {
      scrollBy(old - left, 0);
    }
  }

  @Override
  protected void onDetachedFromWindow() {
    setScrollState(SCROLL_STATE_IDLE);
    if (mScroller != null && !mScroller.isFinished()) {
      mScroller.abortAnimation();
    }
    super.onDetachedFromWindow();
  }

  @Override
  public void addView(View child, int index, ViewGroup.LayoutParams params) {
    throw new IllegalStateException();
  }

  @Override
  public void requestLayout() {
    if (DEBUG) {
      LLog.i(TAG, "trigger requestLayout");
    }
    super.requestLayout();
  }

  @Override
  public boolean canScrollHorizontally(int direction) {
    return (mTouchable && canScrollHorizontallyInternal(direction)) || mForceCanScroll;
  }

  private boolean canScrollHorizontallyInternal(int direction) {
    if (!isVertical() && mAdapter != null && mTotalCount > 1 && mCurrentIndex != -1) {
      // direction negative to check scrolling left, positive to check scrolling right.
      return mLoop
          || (direction > 0 && (isRTL ? mCurrentIndex > 0 : mCurrentIndex < mTotalCount - 1))
          || (direction < 0 && (isRTL ? mCurrentIndex < mTotalCount - 1 : mCurrentIndex > 0));
    }
    return false;
  }

  @Override
  public boolean canScrollVertically(int direction) {
    return (mTouchable && canScrollVerticallyInternal(direction)) || mForceCanScroll;
  }

  private boolean canScrollVerticallyInternal(int direction) {
    if (isVertical() && mAdapter != null && mTotalCount > 1 && mCurrentIndex != -1) {
      // direction negative to check scrolling up, positive to check scrolling down.
      return mLoop || (direction > 0 && mCurrentIndex < mTotalCount - 1)
          || (direction < 0 && mCurrentIndex > 0);
    }
    return false;
  }

  private void reMeasureChildren() {
    for (int i = getChildCount() - 1; i > -1; i--) {
      View child = getChildAt(i);
      measureChild(child);
    }
  }

  private void measureChild(View child) {
    int wMeasureSpec, hMeasureSpec;
    if (isVertical()) {
      wMeasureSpec = getChildWidthMeasureSpec(child);
      hMeasureSpec = MeasureSpec.makeMeasureSpec(getChildExpectSize(), MeasureSpec.EXACTLY);
    } else {
      wMeasureSpec = MeasureSpec.makeMeasureSpec(getChildExpectSize(), MeasureSpec.EXACTLY);
      hMeasureSpec = getChildHeightMeasureSpec(child);
    }
    child.measure(wMeasureSpec, hMeasureSpec);
  }

  private int getChildHeightMeasureSpec(View child) {
    LayoutParams params = (LayoutParams) child.getLayoutParams();
    if (params.height >= 0) {
      return MeasureSpec.makeMeasureSpec(params.height, MeasureSpec.EXACTLY);
    }
    int remain = getHeight() - getPaddingTop() - getPaddingBottom();
    remain = Math.max(0, remain);
    if (params.height == LayoutParams.MATCH_PARENT) {
      return MeasureSpec.makeMeasureSpec(remain, MeasureSpec.EXACTLY);
    } else if (params.height == LayoutParams.WRAP_CONTENT) {
      return MeasureSpec.makeMeasureSpec(remain, MeasureSpec.AT_MOST);
    } else {
      return MeasureSpec.makeMeasureSpec(0, MeasureSpec.EXACTLY);
    }
  }

  private int getChildWidthMeasureSpec(View child) {
    LayoutParams params = (LayoutParams) child.getLayoutParams();
    if (params.width >= 0) {
      return MeasureSpec.makeMeasureSpec(params.width, MeasureSpec.EXACTLY);
    }
    int remain = getWidth() - getPaddingLeft() - getPaddingRight();
    remain = Math.max(0, remain);
    if (params.width == LayoutParams.MATCH_PARENT) {
      return MeasureSpec.makeMeasureSpec(remain, MeasureSpec.EXACTLY);
    } else if (params.width == LayoutParams.WRAP_CONTENT) {
      return MeasureSpec.makeMeasureSpec(remain, MeasureSpec.AT_MOST);
    } else {
      return MeasureSpec.makeMeasureSpec(0, MeasureSpec.EXACTLY);
    }
  }

  private void relayoutChildren() {
    if (mTotalCount < 1) {
      return;
    }

    int childExpectSize = getChildExpectSize();
    if (childExpectSize <= 0) {
      return;
    }
    int scrollDistance = getScrollDistance();
    int w = childExpectSize + mPageMargin;
    if (w <= 0) {
      return;
    }
    int begin = 0;
    int end = 0;
    int count = scrollDistance / w;
    int remain = scrollDistance % w;
    int position = 0;

    // Using mMinScrollBoundary as the starting point, calculate the index of the start item-view
    // according to the scrollDistance (multiple item-views may be displayed in the viewpager, such
    // as cover-flow mode), and then layout the item-views to the right in turn until one item-view
    // exceeds the right side of the viewpager. If mLoop == true and index >= mTotalCount, the index
    // reset to 0.
    if (isRTL()) {
      if (mLoop) {
        // rtl and loop
        if (scrollDistance > 0) {
          if (remain != 0) {
            count++;
          } else {
            count %= mTotalCount;
          }
          end = scrollDistance + getWidth() - remain
              + ((remain == 0) ? 0 : childExpectSize + mPageMargin);
        } else {
          end = scrollDistance + getWidth() - remain;
        }
        position = (mTotalCount - count) % mTotalCount;
        if (position < 0) {
          position += mTotalCount;
        }
      } else {
        // rtl and !loop
        int offset = getPaddingRight() - mOffset;
        for (int i = 0; i < mTotalCount - 1; ++i) {
          if (scrollDistance < mMaxScrollBoundary - offset + getWidth() - (i + 1) * w) {
            position = i;
            break;
          }
        }
        end = mMaxScrollBoundary - offset + getWidth() - position * w;
      }
    } else {
      if (mLoop) {
        // !rtl and loop
        if (scrollDistance < 0) {
          if (remain != 0) {
            count--;
          }
          count %= mTotalCount;
          position = (mTotalCount + count) % mTotalCount;
          begin = scrollDistance - ((remain == 0) ? -mPageMargin : childExpectSize) - remain
              - mPageMargin;
        } else {
          position = count % mTotalCount;
          begin = scrollDistance - remain;
        }
      } else {
        // !rtl and !loop
        int offset = getPaddingLeft() + mOffset;
        for (int i = 0; i < mTotalCount; ++i) {
          if (scrollDistance < mMinScrollBoundary + offset + (i + 1) * w) {
            position = i;
            break;
          }
        }
        begin = mMinScrollBoundary + offset + position * w;
      }
    }

    int min = scrollDistance;
    int max = scrollDistance + Math.max(0, getPageGap());
    if (DEBUG) {
      LLog.i(TAG, "[min, max] = [" + min + ", " + max + "]");
    }
    List<View> children = new ArrayList<>();
    if (!mKeepItemView) {
      // When layout item-views, need to determine whether the index to be placed has been added to
      // the ViewGroup:
      //   (1) if it already exists, we can directly call the layout method of the item-view.
      //   (2) if not, you need to obtain the corresponding item-view through the adapter, then
      //   manually measure and layout.
      //   (3) after scrolling, remove the item-view from the ViewGroup if it is not on the screen.
      children = getAllChildren();
      while (true) {
        View child = getChildByPosition(position);
        children.remove(child);
        if (isRTL()) {
          begin = end - childExpectSize;
        } else {
          end = begin + childExpectSize;
        }
        relayoutChild(child, position, begin, end);
        transformIfNeeded();
        if (isRTL()) {
          if (begin <= min) {
            break;
          }
          end = begin - mPageMargin;
        } else {
          if (end >= max) {
            break;
          }
          begin = end + mPageMargin;
        }
        position++;
        if (position >= mTotalCount && !mLoop) {
          break;
        }
        position = position % mTotalCount;
      }
    } else {
      int initBegin = begin;
      int initEnd = end;
      if (!mLoop) {
        // layout swiper item view from 0 to position - 1
        for (int i = position - 1; i >= 0; --i) {
          View child = getChildByPosition(i);
          if (isRTL()) {
            begin = end + mPageMargin;
            end = begin + childExpectSize;
          } else {
            end = begin - mPageMargin;
            begin = end - childExpectSize;
          }
          if ((isRTL() && (begin - mPageMargin) >= max)
              || (!isRTL() && (end + mPageMargin) <= min)) {
            children.add(child);
          }
          relayoutChild(child, i, begin, end);
        }
        // layout swiper item view from position to mTotalCount - 1
        begin = initBegin;
        end = initEnd;
        for (int i = position; i < mTotalCount; ++i) {
          View child = getChildByPosition(i);
          if (isRTL()) {
            begin = end - childExpectSize;
          } else {
            end = begin + childExpectSize;
          }
          if ((isRTL() && end <= min) || (!isRTL() && begin >= max)) {
            children.add(child);
          }
          relayoutChild(child, i, begin, end);
          if (isRTL()) {
            end = begin - mPageMargin;
          } else {
            begin = end + mPageMargin;
          }
        }
      } else {
        int initPosition = position;
        while (true) {
          View child = getChildByPosition(position);
          if (isRTL()) {
            begin = end - childExpectSize;
          } else {
            end = begin + childExpectSize;
          }
          if ((isRTL() && end <= min) || (!isRTL() && begin >= max)) {
            children.add(child);
          }
          relayoutChild(child, position, begin, end);
          if (isRTL()) {
            end = begin - mPageMargin;
          } else {
            begin = end + mPageMargin;
          }
          position++;
          position = position % mTotalCount;
          if (position == initPosition) {
            break;
          }
        }
      }
      transformIfNeeded();
    }
    for (View child : children) {
      recycleView(child, mKeepItemView);
    }
  }

  private void relayoutChild(final View child, int position, int begin, int end) {
    int top, bottom, left, right;
    if (isVertical()) {
      top = begin;
      bottom = end;
      left = getPaddingLeft();
      right = left + child.getMeasuredWidth();
    } else {
      top = getPaddingTop();
      bottom = top + child.getMeasuredHeight();
      left = begin;
      right = end;
    }
    if (DEBUG) {
      LLog.i(TAG,
          "relayoutChildren at " + position + ": [l, t, r, b] = [" + left + ", " + top + ", "
              + right + ", " + bottom + "]");
    }
    child.layout(left, top, right, bottom);
  }

  private List<View> getAllChildren() {
    int count = getChildCount();
    List<View> list = new LinkedList<View>();
    for (int i = 0; i < count; i++) {
      list.add(getChildAt(i));
    }
    return list;
  }

  private View getChildByPosition(int position) {
    for (int i = getChildCount() - 1; i > -1; i--) {
      View child = getChildAt(i);
      if (((LayoutParams) child.getLayoutParams()).position == position) {
        return child;
      }
    }
    View child = mAdapter.get(this, position);
    LayoutParams params = new LayoutParams();
    params.position = position;
    addViewInLayout(child, 0, params, true);
    measureChild(child);
    return child;
  }

  private void transformIfNeeded() {
    if (mTransformer != null) {
      for (int i = getChildCount() - 1; i > -1; i--) {
        View child = getChildAt(i);
        if (!isBounceBeginView(child) && !isBounceEndView(child)) {
          transformChild(child);
        }
      }
    }
  }

  private void transformChild(final View child) {
    if (child != null) {
      int x;
      if (isRTL()) {
        x = child.getLeft() - getScrollX() + getOffset() - getPaddingLeft();
      } else {
        if (isVertical()) {
          x = child.getTop() - getScrollY() - getOffset() - getPaddingTop();
        } else {
          x = child.getLeft() - getScrollX() - getOffset() - getPaddingLeft();
        }
      }
      mTransformer.transformPage(this, child, isVertical(), x);
    }
  }

  private void recycleView(View child, boolean keepItemView) {
    if (DEBUG) {
      LLog.i(
          TAG, "recycleView: " + (!keepItemView ? "remove child: " : "not remove child: ") + child);
    }
    if (mTransformer != null) {
      mTransformer.reset(child);
    }
    if (!keepItemView) {
      // addViewInLayout() + removeView() will trigger requestLayout one time
      removeView(child);
      int position = ((LayoutParams) child.getLayoutParams()).position;
      mAdapter.recycle(this, position, child);
    }
  }

  private void triggerScrollStartEvent() {
    if (!mIsInit && !mTriggerEvent) {
      boolean isDragged = mTouchable && mScrollState == SCROLL_STATE_DRAGGING;
      for (OnPageScrollListener listener : mPageScrollListeners) {
        listener.onPageScrollStart(mCurrentIndex, isDragged);
      }
      mTriggerEvent = true;
    }
  }

  private void triggerTransitionEvent() {
    if (!mIsInit && mTriggerEvent) {
      boolean isDragged = mTouchable && mScrollState == SCROLL_STATE_DRAGGING;
      for (OnPageScrollListener listener : mPageScrollListeners) {
        listener.onPageScrolling(mCurrentIndex, isDragged, getScrollX(), getScrollY());
      }
    }
  }

  private void triggerScrollToBounce(boolean toBegin, boolean toEnd) {
    if (!mIsInit && mTriggerEvent) {
      for (OnPageScrollListener listener : mPageScrollListeners) {
        listener.onScrollToBounce(toBegin, toEnd);
      }
    }
  }

  private void triggerScrollEndEvent() {
    if (!mIsInit && mTriggerEvent) {
      for (OnPageScrollListener listener : mPageScrollListeners) {
        listener.onPageScrollEnd(mCurrentIndex);
      }
      mTriggerEvent = false;
    }
  }

  private void triggerPageChangeEvent() {
    if (mOldCurrentIndex != mCurrentIndex) {
      for (OnPageScrollListener listener : mPageScrollListeners) {
        listener.onPageChange(mOldCurrentIndex, mCurrentIndex, mIsInit);
      }
    }
  }

  @Override
  public void computeScroll() {
    if (mScroller.computeScrollOffset()) {
      mFinalPosition = isVertical() ? mScroller.getFinalY() : mScroller.getFinalX();
      int oldX = getScrollX();
      int oldY = getScrollY();
      int x = mScroller.getCurrX();
      int y = mScroller.getCurrY();
      if (isVertical() && oldY != y) {
        scrollTo(oldX, y);
      } else if (!isVertical() && oldX != x) {
        scrollTo(x, oldY);
      }
      ViewCompat.postInvalidateOnAnimation(this);
    } else {
      // Note: setting visibility: hidden / visible will trigger re-draw and invoke computeScroll()
      if (getChildCount() > 0 && mScrollState != SCROLL_STATE_DRAGGING) {
        View child = getCurrentView();
        mCurrentIndex = ((LayoutParams) child.getLayoutParams()).position;
        if (DEBUG) {
          LLog.i(TAG,
              "Finish computeScrollOffset with index from " + mOldCurrentIndex + " to "
                  + mCurrentIndex);
        }
        triggerPageChangeEvent();
        if (mEnableViceLoop && mLoopChanged) {
          mLoop = false;
          mLoopChanged = false;
          requestLayout();
        }
        triggerScrollEndEvent();
        if (mOldCurrentIndex == INIT_ITEM_INDEX && mCurrentIndex != INIT_ITEM_INDEX) {
          mIsInit = false;
        }
        mOldCurrentIndex = mCurrentIndex;
        setScrollState(SCROLL_STATE_IDLE);
      }
    }
  }

  protected void scrollToFinalPositionDirectly() {
    if (mFinalPosition != INVALID_OFFSET) {
      if (isVertical()) {
        scrollTo(mScroller.getCurrX(), mFinalPosition);
      } else {
        scrollTo(mFinalPosition, mScroller.getCurrY());
      }
    }
    mFinalPosition = INVALID_OFFSET;
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent event) {
    if (!mTouchable) {
      return super.onInterceptTouchEvent(event);
    }
    final int action = event.getActionMasked();
    final int actionIndex = event.getActionIndex();
    if (action == MotionEvent.ACTION_CANCEL || action == MotionEvent.ACTION_UP) {
      if (DEBUG_GESTURE) {
        LLog.i(TAG, "Intercept ACTION_CANCEL / UP");
      }
      // No need to intercept event at the end of the stream of events.
      mActivePointerId = INVALID_TOUCH_POINTER_ID;
      mIsBeingDragged = false;
      mIsUnableToDrag = false;
      mFling = false;
      mReadyToScroll = false;
      mDragDistance = 0;
      return false;
    }
    if (action != MotionEvent.ACTION_DOWN) {
      if (mIsBeingDragged) {
        if (DEBUG_GESTURE) {
          LLog.i(TAG,
              "onInterceptTouchEvent: is being dragged, returning true directly with action = "
                  + action);
        }
        return true;
      }
      if (mIsUnableToDrag) {
        if (DEBUG_GESTURE) {
          LLog.i(TAG,
              "onInterceptTouchEvent: is unable to drag, returning false directly with action = "
                  + action);
        }
        return false;
      }
    }
    switch (action) {
      case MotionEvent.ACTION_POINTER_DOWN:
        mActivePointerId = event.getPointerId(actionIndex);
        if (DEBUG_GESTURE) {
          LLog.i(TAG,
              "onInterceptTouchEvent ACTION_POINTER_DOWN, use the new finger that touches screen as the active finger, and update mActivePointerId = "
                  + mActivePointerId);
        }
        break;
      case MotionEvent.ACTION_POINTER_UP:
        if (DEBUG_GESTURE) {
          LLog.i(TAG, "onInterceptTouchEvent ACTION_POINTER_UP -> onSecondaryPointerUp");
        }
        onSecondaryPointerUp(event);
        break;
      case MotionEvent.ACTION_DOWN:
        // Reset flags at the beginning of the stream of events.
        mLastX = mInitialMotionX = event.getX();
        mLastY = mInitialMotionY = event.getY();
        mActivePointerId = event.getPointerId(actionIndex);
        mIsBeingDragged = false;
        mIsUnableToDrag = false;
        mReadyToScroll = false;
        mFling = false;
        mDragDistance = 0;
        mScroller.computeScrollOffset();
        int scrollDiff = isVertical() ? mScroller.getFinalY() - mScroller.getCurrY()
                                      : mScroller.getFinalX() - mScroller.getCurrX();
        // In the case of CoordinateLayout nesting horizontal viewpager, we should not invoke
        // requestDisallowInterceptTouchEvent(true) if we want CoordinateLayout to consume vertical
        // gesture, because CoordinateLayout#requestDisallowInterceptTouchEvent(true) will invoke
        // resetTouchBehaviors(false) to trigger an ACTION_CANCEL event to it's all behaviors and
        // lead behaviors to ignore subsequent event.
        // See more details in support framework:
        // https://github.com/aosp-mirror/platform_frameworks_support/blob/master/coordinatorlayout/src/main/java/androidx/coordinatorlayout/widget/CoordinatorLayout.java
        if (mHandleGesture) {
          // Need request parent not intercept touch event in ACTION_DOWN, ViewPager can receive the
          // next ACTION_MOVE event and handle touch conflicts. For example, in the case of
          // horizontal scroll-view nesting horizontal viewpager, if the first ACTION_MOVE is
          // intercepted by scroll-view(Eg: abs(dx) > mTouchSlop), the viewpager will receive
          // ACTION_CANCEL and can not handle touch conflicts.
          requestParentDisallowInterceptTouchEvent(true);
        }
        if (mScrollState == SCROLL_STATE_SETTLING && Math.abs(scrollDiff) > mCloseEnough) {
          // mScroller ends animation and set ViewPager's state from SETTLING to DRAGGING.
          mScroller.abortAnimation();
          // In settling state, always let the user catch the pager.
          requestParentDisallowInterceptTouchEvent(true);
          setScrollState(SCROLL_STATE_DRAGGING);
          mIsBeingDragged = true;
        }
        if (DEBUG_GESTURE) {
          LLog.i(TAG,
              "onInterceptTouchEvent DOWN: " + mIsBeingDragged + ", update mActivePointerId = "
                  + mActivePointerId + ", [mLastX, mLastY] = " + mLastX + ", " + mLastY);
        }
        break;
      case MotionEvent.ACTION_MOVE:
        if (mActivePointerId == INVALID_TOUCH_POINTER_ID) {
          // If we don't have a valid id, the touch down wasn't on content.
          break;
        }
        final int pointerIndex = event.findPointerIndex(mActivePointerId);
        final float x = event.getX(pointerIndex);
        final float y = event.getY(pointerIndex);
        final float dx = x - mLastX;
        final float dy = y - mLastY;
        final float xDiff = x - mInitialMotionX;
        final float yDiff = y - mInitialMotionY;
        final int delta = (int) (isVertical() ? dy : dx);
        // Note: Check scrollable within child views. If any child can scroll, viewpager will not
        // intercept ACTION_MOVE to let child consume.
        if (mEnableNestedChild && delta != 0
            && canScroll(this, isVertical(), false, delta, (int) x, (int) y)) {
          mLastX = x;
          mLastY = y;
          mIsUnableToDrag = true;
          return false;
        }
        if (isVertical()) {
          if (Math.abs(dy) > mTouchSlop && Math.abs(dy) > 0.5f * Math.abs(xDiff)) {
            requestParentDisallowInterceptTouchEvent(true);
            setScrollState(SCROLL_STATE_DRAGGING);
            mIsBeingDragged = true;
            mReadyToScroll = true;
            mLastY = dy > 0 ? mInitialMotionY + mTouchSlop : mInitialMotionY - mTouchSlop;
            mLastX = x;
            if (DEBUG_GESTURE) {
              LLog.i(TAG,
                  "onInterceptTouchEvent MOVE: Starting drag in Y direction, pointId: "
                      + mActivePointerId + ", [mLastX, mLastY] = " + mLastX + ", " + mLastY);
            }
          } else if (xDiff > mTouchSlop) {
            mIsUnableToDrag = true;
            if (DEBUG_GESTURE) {
              LLog.i(TAG, "onInterceptTouchEvent MOVE: Failed to drag in Y direction");
            }
          }
        } else {
          if (Math.abs(dx) > mTouchSlop && Math.abs(dx) > 0.5f * Math.abs(yDiff)) {
            requestParentDisallowInterceptTouchEvent(true);
            setScrollState(SCROLL_STATE_DRAGGING);
            mIsBeingDragged = true;
            mReadyToScroll = true;
            mLastX = dx > 0 ? mInitialMotionX + mTouchSlop : mInitialMotionX - mTouchSlop;
            mLastY = y;
            if (DEBUG_GESTURE) {
              LLog.i(TAG,
                  "onInterceptTouchEvent MOVE: Starting drag in X direction, pointId: "
                      + mActivePointerId + ", [mLastX, mLastY] = " + mLastX + ", " + mLastY);
            }
          } else if (yDiff > mTouchSlop) {
            mIsUnableToDrag = true;
            if (DEBUG_GESTURE) {
              LLog.i(TAG, "onInterceptTouchEvent MOVE: Failed to drag in X direction");
            }
          }
        }
        break;
    }
    return mIsBeingDragged;
  }

  /**
   * Check scrollable within child views of thisView given a distance.
   * @see androidx.viewpager.widget.ViewPager#canScroll(View, boolean, int, int, int)
   *
   * @param thisView View to test for horizontal scrollable
   * @param isVertical The scroll direction
   * @param checkSelfScrollable Whether the thisView passed should itself be checked for scrollable
   *     (true), or just its children (false).
   * @param delta Distance scrolled in pixels
   * @param x X coordinate of the active touch point
   * @param y Y coordinate of the active touch point
   * @return true if child views of thisView can be scrolled by distance
   */
  protected boolean canScroll(
      View thisView, boolean isVertical, boolean checkSelfScrollable, int delta, int x, int y) {
    if (thisView instanceof ViewGroup) {
      final ViewGroup group = (ViewGroup) thisView;
      final int scrollX = thisView.getScrollX();
      final int scrollY = thisView.getScrollY();
      final int count = group.getChildCount();
      for (int i = count - 1; i >= 0; i--) {
        final View child = group.getChildAt(i);
        if (x + scrollX >= child.getLeft() && x + scrollX < child.getRight()
            && y + scrollY >= child.getTop() && y + scrollY < child.getBottom()
            && canScroll(child, isVertical, true, delta, x + scrollX - child.getLeft(),
                y + scrollY - child.getTop())) {
          return true;
        }
      }
    }
    return checkSelfScrollable
        && (isVertical ? thisView.canScrollVertically(-delta)
                       : thisView.canScrollHorizontally(-delta));
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (DEBUG_GESTURE) {
      LLog.i(TAG, "onTouchEvent: " + event.getActionMasked());
    }
    final int actionIndex = event.getActionIndex();
    switch (event.getActionMasked()) {
      case MotionEvent.ACTION_UP:
      case MotionEvent.ACTION_CANCEL:
        mActivePointerId = INVALID_TOUCH_POINTER_ID;
        if (mFling == false) {
          scrollToFinalPosition();
        }
        break;
      case MotionEvent.ACTION_POINTER_UP:
        if (DEBUG_GESTURE) {
          LLog.i(TAG, "onTouchEvent: ACTION_POINTER_UP -> onSecondaryPointerUp");
        }
        onSecondaryPointerUp(event);
        break;
      case MotionEvent.ACTION_POINTER_DOWN:
        mActivePointerId = event.getPointerId(actionIndex);
        mLastX = event.getX(actionIndex);
        mLastY = event.getY(actionIndex);
        if (DEBUG_GESTURE) {
          LLog.i(TAG,
              "onTouchEvent ACTION_POINTER_DOWN, use the new finger that touches screen as the active finger, and update mActivePointerId = "
                  + mActivePointerId);
        }
        break;
    }
    if (mTouchable) {
      return mDetector.onTouchEvent(event);
    } else {
      return super.onTouchEvent(event);
    }
  }

  private final GestureDetector mDetector =
      new GestureDetector(new GestureDetector.SimpleOnGestureListener() {
        /**
         * @param e1 The ACTION_DOWN recorded in onDown(). Note: e1 may be null, eg: in ViewPager
         *     contains ScrollView case, if we scroll ViewPager, e2 is the ACTION_MOVE intercepted
         *     by onInterceptTouchEvent() and trigger onScroll() directly, e1 can't be recorded.
         * @param e2 The ACTION_MOVE that triggers onScroll().
         * @param distanceX The distance along the X axis that has been scrolled since the last
         *                  call to onScroll. This is NOT the distance between {@code e1}
         *                  and {@code e2}.
         *                  - if e1 == null, distanceX = 0 - e2.getX()
         *                  - if e1 != null, distanceX = mLastFocusX - focusX
         * @param distanceY The distance along the Y axis that has been scrolled since the last
         *                  call to onScroll. This is NOT the distance between {@code e1}
         *                  and {@code e2}.
         * @Note Not directly compare whether to slide horizontally or vertically through the values
         * of distanceX and distanceY
         * @return true if the event is consumed, else false
         */
        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
          if (DEBUG_GESTURE) {
            LLog.i(TAG,
                "GestureDetector#onScroll: distance: [x, y] = " + distanceX + ", " + distanceY);
          }
          if (!mIsBeingDragged) {
            // There is no child view of ViewPager consuming ACTION_DOWN in onTouchEvent(),
            // ACTION_DOWN is consumed in onDown() and ACTION_MOVE is consumed in onScroll() by
            // ViewPager.
            int pointerIndex = e2.findPointerIndex(mActivePointerId);
            if (pointerIndex == INVALID_TOUCH_POINTER_ID) {
              return super.onScroll(e1, e2, distanceX, distanceY);
            }
            final float xDiff = Math.abs(distanceX);
            final float yDiff = Math.abs(distanceY);
            if ((isVertical() && yDiff > xDiff && canScrollVerticallyInternal((int) (distanceY)))
                || (!isVertical() && xDiff > yDiff
                    && canScrollHorizontallyInternal((int) (distanceX)))) {
              mIsBeingDragged = true;
              setScrollState(SCROLL_STATE_DRAGGING);
              requestParentDisallowInterceptTouchEvent(true);
            } else {
              // Since requestParentDisallowInterceptTouchEvent(true) is invoked in
              // onInterceptTouchEvent(ACTION_DOWN), here if it is confirmed that viewPager dose not
              // consume ACTION_MOVE, requestParentDisallowInterceptTouchEvent(false) should be
              // invoked, then viewPager's parent can consume ACTION_MOVE.
              requestParentDisallowInterceptTouchEvent(false);
            }
          }
          if (mIsBeingDragged) {
            int pointerIndex = e2.findPointerIndex(mActivePointerId);
            final float x = e2.getX(pointerIndex);
            final float y = e2.getY(pointerIndex);
            final float dx = mLastX - x;
            final float dy = mLastY - y;
            mDragDistance += isVertical() ? dy : dx;
            mLastX = x;
            mLastY = y;
            if (mReadyToScroll) {
              triggerScrollStartEvent();
              mReadyToScroll = false;
            }
            if (isVertical()) {
              scrollBy(0, (int) (dy + 0.5));
            } else {
              scrollBy((int) (dx + 0.5), 0);
            }
          }
          return mIsBeingDragged;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
          boolean result = super.onFling(e1, e2, velocityX, velocityY);
          if ((isVertical() && velocityY >= -mMaxVelocityY && velocityY <= mMaxVelocityY)
              || (!isVertical() && velocityX >= -mMaxVelocityX && velocityX <= mMaxVelocityX)) {
            return result;
          }
          if (DEBUG_GESTURE) {
            LLog.i(TAG, "GestureDetector#onFling: [velX, velY] = " + velocityX + ", " + velocityY);
          }
          mFling = true;
          mScroller.abortAnimation();
          flingToPosition(velocityX, velocityY);
          return super.onFling(e1, e2, velocityX, velocityY);
        }

        /**
         * Invoke timing:
         * (1) There is no child view of ViewPager consuming ACTION_DOWN in onTouchEvent(),
         * ACTION_DOWN is consumed by ViewPager (2) ViewPager's state changes from SETTLING to
         * DRAGGING, and intercept ACTION_DOWN in onInterceptTouchEvent()
         */
        @Override
        public boolean onDown(MotionEvent e) {
          mScroller.abortAnimation();
          mDragDistance = 0;
          mReadyToScroll = true;
          mFling = false;
          mLastX = mInitialMotionX = e.getX();
          mLastY = mInitialMotionY = e.getY();
          mActivePointerId = e.getPointerId(e.getActionIndex());
          if (DEBUG_GESTURE) {
            LLog.i(TAG, "GestureDetector#onDown: [mLastX, mLastY] = " + mLastX + ", " + mLastY);
          }
          return true;
        }
      }, new Handler(Looper.getMainLooper()));

  private void onSecondaryPointerUp(MotionEvent event) {
    final int pointerIndex = event.getActionIndex();
    final int pointerId = event.getPointerId(pointerIndex);
    if (pointerId == mActivePointerId) {
      // If the active finger is released, let the last finger still on the screen be the active
      // finger and pointerIndex is continuous from 0, 1, 2.
      final int newPointerIndex = pointerIndex == 0 ? 1 : 0;
      mLastX = event.getX(newPointerIndex);
      mLastY = event.getY(newPointerIndex);
      mActivePointerId = event.getPointerId(newPointerIndex);
      if (DEBUG_GESTURE) {
        LLog.i(TAG,
            "The active finger is released and update mActivePointerId -> " + mActivePointerId);
      }
    } else {
      if (DEBUG_GESTURE)
        LLog.i(TAG,
            "The inactive finger is released and no need to update mActivePointerId = "
                + mActivePointerId);
    }
  }

  private void setScrollState(int newState) {
    if (DEBUG) {
      LLog.i(TAG,
          "setScrollState from " + getStateName(mScrollState) + " -> " + getStateName(newState));
    }
    if (mScrollState == newState) {
      return;
    }
    for (OnPageScrollListener listener : mPageScrollListeners) {
      listener.onPageScrollStateChanged(mScrollState, newState);
    }
    mScrollState = newState;
  }

  private String getStateName(int state) {
    switch (state) {
      case SCROLL_STATE_IDLE:
        return "STATE_IDLE";
      case SCROLL_STATE_DRAGGING:
        return "STATE_DRAGGING";
      case SCROLL_STATE_SETTLING:
        return "STATE_SETTLING";
    }
    return "";
  }

  private void scrollToFinalPosition() {
    if (getChildCount() < 1) {
      return;
    }
    View child = getCurrentView();
    int scrollDistance = getScrollDistance();
    int begin = getBegin(child) - scrollDistance;
    int end = getEnd(child) - scrollDistance;
    if (isBounceBeginView(child)) {
      int pile = getPile();
      if (isVertical()) {
        mScroller.startScroll(0, scrollDistance, 0, end + mPageMargin - pile, mBounceDuration);
      } else {
        if (isRTL) {
          pile = getPile() + getChildExpectSize();
          mScroller.startScroll(scrollDistance, 0, begin - mPageMargin - pile, 0, mBounceDuration);
        } else {
          mScroller.startScroll(scrollDistance, 0, end + mPageMargin - pile, 0, mBounceDuration);
        }
      }
    } else if (isBounceEndView(child)) {
      int pile = getPile() + getChildExpectSize();
      if (isVertical()) {
        mScroller.startScroll(0, scrollDistance, 0, begin - mPageMargin - pile, mBounceDuration);
      } else {
        if (isRTL) {
          pile = getPile();
          mScroller.startScroll(scrollDistance, 0, end + mPageMargin - pile, 0, mBounceDuration);
        } else {
          mScroller.startScroll(scrollDistance, 0, begin - mPageMargin - pile, 0, mBounceDuration);
        }
      }
    } else {
      int pile = getPile() + getChildExpectSize() / 2;
      int center = (begin + end) / 2;
      if (isVertical()) {
        mScroller.startScroll(0, scrollDistance, 0, center - pile, mAnimDuration);
      } else {
        mScroller.startScroll(scrollDistance, 0, center - pile, 0, mAnimDuration);
      }
    }
    setScrollState(SCROLL_STATE_SETTLING);
    invalidate();
  }

  private void flingToPosition(float velocityX, float velocityY) {
    if (getChildCount() < 1) {
      return;
    }

    float velocity = isVertical() ? velocityY : velocityX;

    int pile = getPile();
    int scrollDistance = getScrollDistance();

    // Note: need to obtain position info without directly using mCurrentIndex(may be -1)
    View currentView = getCurrentView();
    int currentPosition = ((LayoutParams) currentView.getLayoutParams()).position;

    if (velocity < 0) { // acceleration to the left or top
      int start = Integer.MAX_VALUE;
      for (int i = 0, count = getChildCount(); i < count; i++) {
        View child = getChildAt(i);
        int begin = getBegin(child) - scrollDistance;
        if (begin > pile && begin < start) {
          start = begin;
        }
      }
      if (start == Integer.MAX_VALUE) {
        // If not get the target location, scroll to the next item directly.
        flingToPositionInner(currentPosition, true);
        return;
      }
      if (isVertical()) {
        mScroller.startScroll(0, scrollDistance, 0, start - pile, mAnimDuration);
      } else {
        mScroller.startScroll(scrollDistance, 0, start - pile, 0, mAnimDuration);
      }
      setScrollState(SCROLL_STATE_SETTLING);
    } else { // acceleration to the right or bottom
      int start = Integer.MIN_VALUE;
      for (int i = 0, count = getChildCount(); i < count; i++) {
        View child = getChildAt(i);
        int begin = getBegin(child) - scrollDistance;
        if (begin < pile && begin > start) {
          start = begin;
        }
      }
      if (start == Integer.MIN_VALUE) {
        // If not get the target location, scroll to the previous item directly.
        flingToPositionInner(currentPosition, false);
        return;
      }
      if (isVertical()) {
        mScroller.startScroll(0, scrollDistance, 0, start - pile, mAnimDuration);
      } else {
        mScroller.startScroll(scrollDistance, 0, start - pile, 0, mAnimDuration);
      }
      setScrollState(SCROLL_STATE_SETTLING);
    }
    invalidate();
  }

  /**
   * If not get the target location, scroll to the previous item directly.
   *
   * @param currentPosition current item position
   * @param isFlingToStart whether fling to left or top.
   */
  private void flingToPositionInner(int currentPosition, boolean isFlingToStart) {
    int targetPosition = currentPosition;
    if (isFlingToStart) {
      // fling to the left or top
      if (isRTL()) {
        targetPosition = (mLoop && currentPosition - 1 < 0) ? mTotalCount - 1 : currentPosition - 1;
      } else {
        targetPosition = (mLoop && currentPosition + 1 >= mTotalCount) ? 0 : currentPosition + 1;
      }
      setCurrentIndex(
          targetPosition, true, isRTL() ? SCROLL_DIRECTION_BEGIN : SCROLL_DIRECTION_END);
    } else {
      // fling to the right or bottom
      if (isRTL()) {
        targetPosition = (mLoop && currentPosition + 1 >= mTotalCount) ? 0 : currentPosition + 1;
      } else {
        targetPosition = (mLoop && currentPosition - 1 < 0) ? mTotalCount - 1 : currentPosition - 1;
      }
      setCurrentIndex(
          targetPosition, true, isRTL() ? SCROLL_DIRECTION_END : SCROLL_DIRECTION_BEGIN);
    }
  }

  private View getCurrentView() {
    int pile = getPile() + getChildExpectSize() / 2;
    int distance = getScrollDistance();
    int count = getChildCount();
    for (int i = 0; i < count; i++) {
      View child = getChildAt(i);
      int begin = getBegin(child) - distance;
      int end = getEnd(child) - distance;
      if (mDragDistance > 0) { // finger displacement is left / top
        end += mPageMargin;
      } else if (mDragDistance < 0) { // finger displacement is right / bottom
        begin -= mPageMargin;
      }
      if (begin <= pile && end >= pile) {
        return child;
      }
    }
    return getChildAt(0);
  }

  /**
   * Since the viewpager needs to support limited transform and infinite transform, override this
   * method to set the scrollable range.
   */
  @Override
  public void scrollTo(int x, int y) {
    resetScrollRange();
    if (!mLoop) {
      int minScrollBoundary = 0;
      int maxScrollBoundary = 0;
      if (isVertical()) {
        minScrollBoundary = enableBounceBegin() ? (int) (mMinScrollBoundary
                                + mBounceBeginThreshold * (getChildExpectSize() + mPageMargin))
                                                : mMinScrollBoundary;
        maxScrollBoundary = enableBounceEnd() ? (int) (mMaxScrollBoundary
                                - mBounceEndThreshold * (getChildExpectSize() + mPageMargin))
                                              : mMaxScrollBoundary;
        mScrollInToBeginBounce = enableBounceBegin() && y <= minScrollBoundary;
        mScrollInToEndBounce = enableBounceEnd() && y >= maxScrollBoundary;
        y = Math.max(y, minScrollBoundary);
        y = Math.min(y, maxScrollBoundary);
      } else {
        minScrollBoundary = mMinScrollBoundary;
        maxScrollBoundary = mMaxScrollBoundary;
        if (enableBounceBegin()) {
          if (isRTL) {
            maxScrollBoundary = (int) (mMaxScrollBoundary
                - mBounceBeginThreshold * (getChildExpectSize() + mPageMargin));
          } else {
            minScrollBoundary = (int) (mMinScrollBoundary
                + mBounceBeginThreshold * (getChildExpectSize() + mPageMargin));
          }
        }
        if (enableBounceEnd()) {
          if (isRTL) {
            minScrollBoundary = (int) (mMinScrollBoundary
                + mBounceEndThreshold * (getChildExpectSize() + mPageMargin));
          } else {
            maxScrollBoundary = (int) (mMaxScrollBoundary
                - mBounceEndThreshold * (getChildExpectSize() + mPageMargin));
          }
        }
        mScrollInToBeginBounce =
            enableBounceBegin() && (isRTL ? x >= maxScrollBoundary : x <= minScrollBoundary);
        mScrollInToEndBounce =
            enableBounceEnd() && (isRTL ? x <= minScrollBoundary : x >= maxScrollBoundary);
        x = Math.max(x, minScrollBoundary);
        x = Math.min(x, maxScrollBoundary);
      }
    }
    super.scrollTo(x, y);
    updateScrollRange();
    if (DEBUG) {
      LLog.i(TAG,
          "scrollTo: [" + x + ", " + y + "], scrollRange: [" + mMinScrollBoundary + ", "
              + mMaxScrollBoundary + "]");
    }
    relayoutChildren();
    triggerTransitionEvent();
    if (!mLoop) {
      if (mScrollInToBeginBounce || mScrollInToEndBounce) {
        scrollToFinalPosition();
        if (!mTriggerBounceEvent) {
          triggerScrollToBounce(mScrollInToBeginBounce, mScrollInToEndBounce);
        }
      }
      mTriggerBounceEvent = mScrollInToBeginBounce || mScrollInToEndBounce;
      mScrollInToBeginBounce = false;
      mScrollInToEndBounce = false;
    }
  }

  /**
   * Return whether the scrollable range has changed.
   */
  private boolean scrollRangeChanged() {
    final int expectChildSize = getChildExpectSize() + mPageMargin;
    final int expectSize = mTotalCount * expectChildSize;
    final int expectOffset = getPaddingLeft() + mOffset;
    boolean res =
        (mMinScrollBoundary == Integer.MIN_VALUE && mMaxScrollBoundary == Integer.MAX_VALUE)
        || (mExpectChildSize != expectChildSize || mExpectSize != expectSize
            || mExpectOffset != expectOffset);
    mExpectChildSize = expectChildSize;
    mExpectSize = expectSize;
    mExpectOffset = expectOffset;
    return res;
  }

  /**
   * Re-calculate min and max scrollable boundary in the condition of:
   *   (1) scroll range is invalid (== -1) in the init of viewpager
   *   (2) the single page's size is changed if swiper ui sets new layout properties
   *   (3) the total count of pages is changed if swiper ui sets new adapter
   */
  private void resetScrollRange() {
    if (scrollRangeChanged()) {
      int expectOffset = 0;
      if (isRTL()) {
        expectOffset = getPaddingRight() - mOffset;
        mMinScrollBoundary =
            -(mTotalCount - 1) * (getChildExpectSize() + mPageMargin) + expectOffset;
        mMaxScrollBoundary = expectOffset;
      } else {
        int padding = isVertical() ? getPaddingTop() : getPaddingLeft();
        expectOffset = padding + mOffset;
        mMinScrollBoundary = -expectOffset;
        mMaxScrollBoundary =
            (mTotalCount - 1) * (getChildExpectSize() + mPageMargin) - expectOffset;
      }
    }
  }

  /**
   * Update scrollable range according to current scrolling distance and this is called at #scrollto
   * method. In the condition of mLoop == true, We update scrollable range and using this range to
   * limit scrolling distance if mLoop changed to false.
   */
  private void updateScrollRange() {
    final int expectChildSize = getChildExpectSize() + mPageMargin;
    final int expectSize = mTotalCount * expectChildSize;
    if (!mLoop || expectSize <= 0 || expectChildSize <= 0) {
      return;
    }
    int scrollDistance = getScrollDistance();
    int offset = 0;
    if (isRTL()) {
      offset = getPaddingRight() - mOffset;
      scrollDistance -= expectChildSize / 2;
      if (scrollDistance >= 0) {
        mMinScrollBoundary = (scrollDistance / expectSize) * expectSize + expectChildSize + offset;
        mMaxScrollBoundary = (scrollDistance / expectSize + 1) * expectSize + offset;
      } else {
        mMinScrollBoundary =
            -(Math.abs(scrollDistance) / expectSize + 1) * expectSize + expectChildSize + offset;
        mMaxScrollBoundary = -(Math.abs(scrollDistance) / expectSize) * expectSize + offset;
      }
    } else {
      int padding = isVertical() ? getPaddingTop() : getPaddingLeft();
      offset = padding + mOffset;
      scrollDistance += expectChildSize / 2;
      if (scrollDistance >= 0) {
        mMinScrollBoundary = (scrollDistance / expectSize) * expectSize - offset;
        mMaxScrollBoundary =
            (scrollDistance / expectSize + 1) * expectSize - expectChildSize - offset;
      } else {
        mMinScrollBoundary = -(Math.abs(scrollDistance) / expectSize + 1) * expectSize - offset;
        mMaxScrollBoundary =
            -(Math.abs(scrollDistance) / expectSize) * expectSize - expectChildSize - offset;
      }
    }
  }

  private void requestParentDisallowInterceptTouchEvent(boolean disallowIntercept) {
    ViewParent parent = getParent();
    if (parent != null) {
      parent.requestDisallowInterceptTouchEvent(disallowIntercept);
    }
  }

  public void addPageScrollListener(OnPageScrollListener listener) {
    this.mPageScrollListeners.add(listener);
  }

  public void removePageScrollListener(OnPageScrollListener listener) {
    this.mPageScrollListeners.remove(listener);
  }

  private int getContentSize() {
    return isVertical() ? getContentHeight() : getContentWidth();
  }

  private int getPile() {
    if (isVertical()) {
      return getPaddingTop() + getOffset();
    }

    if (isRTL()) {
      return getPaddingLeft() + getOffset() + getWidth() - getChildExpectSize();
    } else {
      return getPaddingLeft() + getOffset();
    }
  }

  private int getBegin(View view) {
    return isVertical() ? view.getTop() : view.getLeft();
  }

  private int getEnd(View view) {
    return isVertical() ? view.getBottom() : view.getRight();
  }

  private int getScrollDistance() {
    return isVertical() ? getScrollY() : getScrollX();
  }

  private int getContentWidth() {
    return getWidth() - getPaddingLeft() - getPaddingRight();
  }

  private int getContentHeight() {
    return getHeight() - getPaddingTop() - getPaddingBottom();
  }

  private int getPageGap() {
    if (isVertical()) {
      return getHeight() - getPaddingBottom() - mPageMargin;
    } else {
      return getWidth() - getPaddingRight() - mPageMargin;
    }
  }

  private boolean isRTL() {
    return isRTL && mOrientation == SwiperView.ORIENTATION_HORIZONTAL;
  }

  public Adapter getAdapter() {
    return mAdapter;
  }

  public int getTotalCount() {
    return mTotalCount;
  }

  public boolean isVertical() {
    return mOrientation == SwiperView.ORIENTATION_VERTICAL;
  }

  public int getCurrentIndex() {
    return mCurrentIndex;
  }

  public int getChildExpectSize() {
    if (mPageSize > 0) {
      return mPageSize;
    }
    int origin = getContentSize();
    return origin;
  }

  public int getOffset() {
    return mOffset;
  }

  public void setTransformer(PageTransformer transformer) {
    if (mTransformer != null) {
      for (int i = getChildCount() - 1; i > -1; i--) {
        View view = getChildAt(i);
        mTransformer.reset(view);
      }
    }
    this.mTransformer = transformer;
    transformIfNeeded();
  }

  public void setIsRTL(boolean isRTL) {
    this.isRTL = isRTL;
  }

  public void setOrientation(int orientation) {
    this.mOrientation = orientation;
  }

  public void setHLayoutUpdated(boolean value) {
    if (this.mHLayoutUpdated) {
      return;
    }
    this.mHLayoutUpdated = value;
  }

  public void setVLayoutUpdated(boolean value) {
    if (this.mVLayoutUpdated) {
      return;
    }
    this.mVLayoutUpdated = value;
  }

  public void setPropsUpdated(boolean value) {
    if (this.mPropsUpdated) {
      return;
    }
    this.mPropsUpdated = value;
  }

  public void setPendingCurrentIndex(int pendingCurrentIndex, boolean pendingSmoothScroll) {
    this.mPendingCurrentIndex = pendingCurrentIndex;
    this.mPendingSmoothScroll = pendingSmoothScroll;
  }

  public void setAnimDuration(int animDuration) {
    this.mAnimDuration = animDuration;
  }

  public void setTouchable(boolean touchable) {
    this.mTouchable = touchable;
  }

  public void setLoop(boolean loop) {
    this.mLoop = loop;
    if (mEnableViceLoop && mAdapter != null && mTotalCount > 1 && mCurrentIndex != -1) {
      requestLayout();
    }
  }

  public void setPageSize(int pageSize) {
    this.mPageSize = pageSize;
  }

  public void setPageMargin(int pageMargin) {
    if (pageMargin < 0) {
      pageMargin = 0;
    }
    this.mPageMargin = pageMargin;
  }

  public void setKeepItemView(boolean value) {
    this.mKeepItemView = value;
  }

  public void setForceCanScroll(boolean value) {
    this.mForceCanScroll = value;
  }

  public void setEnableViceLoop(boolean value) {
    this.mEnableViceLoop = value;
  }

  public void setEnableNestedChild(boolean value) {
    mEnableNestedChild = value;
  }

  public boolean enableBounceBegin() {
    return mEnableBounce && (!mLoop) && (!mIsInit) && mBounceBeginThreshold > 0.f
        && mBounceBeginThreshold < 1.f && mAdapter != null
        && (mBounceEndThreshold < 0.f ? mTotalCount >= 2 : mTotalCount >= 3)
        && !(mPropsUpdated || mHLayoutUpdated || mVLayoutUpdated);
  }

  public boolean enableBounceEnd() {
    return mEnableBounce && (!mLoop) && (!mIsInit) && mBounceEndThreshold > 0.f
        && mBounceEndThreshold < 1.f && mAdapter != null
        && (mBounceBeginThreshold < 0.f ? mTotalCount >= 2 : mTotalCount >= 3)
        && !(mPropsUpdated || mHLayoutUpdated || mVLayoutUpdated);
  }

  public void setEnableBounce(boolean value) {
    this.mEnableBounce = value;
  }

  public void setBounceBeginThreshold(float value) {
    if (value > 0.f && value < 1.f) {
      mBounceBeginThreshold = 1.f - value;
    } else {
      mBounceBeginThreshold = -1.f;
    }
  }

  public void setBounceEndThreshold(float value) {
    if (value > 0.f && value < 1.f) {
      mBounceEndThreshold = 1.f - value;
    } else {
      mBounceEndThreshold = -1.f;
    }
  }

  public void setBounceDuration(int value) {
    mBounceDuration = value;
  }

  public void setIgnoreLayoutUpdate(boolean value) {
    mIgnoreLayoutUpdate = value;
  }

  public void setHandleGesture(boolean value) {
    mHandleGesture = value;
  }

  private boolean isBounceBeginView(View view) {
    return enableBounceBegin() && ((LayoutParams) view.getLayoutParams()).position == 0;
  }

  private boolean isBounceEndView(View view) {
    return enableBounceEnd() && ((LayoutParams) view.getLayoutParams()).position == mTotalCount - 1;
  }
}
