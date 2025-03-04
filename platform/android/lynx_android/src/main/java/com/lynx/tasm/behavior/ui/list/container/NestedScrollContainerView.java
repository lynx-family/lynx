// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list.container;

import static androidx.core.view.ViewCompat.TYPE_NON_TOUCH;
import static androidx.core.view.ViewCompat.TYPE_TOUCH;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.animation.Interpolator;
import android.widget.FrameLayout;
import android.widget.OverScroller;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.NestedScrollingChild2;
import androidx.core.view.NestedScrollingChildHelper;
import androidx.core.view.NestedScrollingParent2;
import androidx.core.view.NestedScrollingParentHelper;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.list.LynxSnapHelper;

public class NestedScrollContainerView
    extends FrameLayout implements NestedScrollingParent2, NestedScrollingChild2 {
  private static final String TAG = "UIListContainer.NestedScrollContainerView";
  private static final boolean DEBUG = false;
  public LynxSnapHelper mSnapHelper = null;

  public static final float LIST_AUTOMATIC_MAX_FLING_RATIO = Float.MAX_VALUE;

  /** The NestedScrollContainerView is in an idle, settled state. */
  public static final int SCROLL_STATE_IDLE = 1;
  /**
   * The NestedScrollContainerView is currently being dragged by outside input such as user touch
   * input.
   */
  public static final int SCROLL_STATE_DRAGGING = 2;
  /**
   * The NestedScrollContainerView is currently animating to a final position triggered by fling.
   */
  public static final int SCROLL_STATE_FLING = 3;
  /**
   * The NestedScrollContainerView is currently animating to a final position triggered by smooth
   * scroll.
   */
  public static final int SCROLL_STATE_SCROLL_ANIMATION = 4;
  private static final int INVALID_POINTER = -1;
  private boolean mIsVertical = true;
  private int mTouchSlop = 0;
  private int mMinFlingVelocity = 0;
  private int mMaxFlingVelocity = 0;
  private int mInitialMotionX;
  private int mInitialMotionY;
  private int mLastMotionX;
  private int mLastMotionY;
  private int mScrollState = SCROLL_STATE_IDLE;
  private int mActivePointerId = INVALID_POINTER;
  private final int[] mScrollOffset = new int[2];
  private final int[] mScrollConsumed = new int[2];
  private final int[] mNestedOffsets = new int[2];
  private final int[] mScrollStepConsumed = new int[2];
  private final int[] mTargetScrollOffset = new int[2];
  private VelocityTracker mVelocityTracker;
  protected ScrollHelper mScrollHelper;
  private OnScrollStateChangeListener mOnScrollStateChangeListener;
  private final NestedScrollingParentHelper mParentHelper;
  private final NestedScrollingChildHelper mChildHelper;
  private float mMaxFlingDistanceRatio = -1;
  public boolean mIsDuringAutoScroll = false;

  /**
   * Interface definition for a callback to be invoked when the scroll state is changed.
   *
   * @see #setOnScrollStateChangeListener(OnScrollStateChangeListener)
   */
  public interface OnScrollStateChangeListener {
    /**
     * Called when the scroll state of the scrollview changes.
     *
     * @param scrollView The scrollview whose scroll state has changed.
     * @param state New scroll state.
     */
    void onScrollStateChange(NestedScrollContainerView scrollView, int state);
  }

  /**
   * Interface definition for a callback to be invoked in the animation to scroll to a final
   * position, including fling or invoke smooth scroll. {@link #mScrollHelper}
   *
   * @see #setCustomScrollHook(CustomScrollHook)
   */
  public abstract static class CustomScrollHook {
    /**
     * Called when the smooth scroll is start.
     *
     * @param lastScrollX Current horizontal scroll origin.
     * @param lastScrollY Current vertical scroll origin.
     * @param scrollX Target horizontal scroll origin.
     * @param scrollY Target vertical scroll origin.
     */
    public void onSmoothScrollStart(int lastScrollX, int lastScrollY, int scrollX, int scrollY) {}

    /** Called when the smooth scroll is end. */
    public void onSmoothScrollEnd() {}

    /**
     * Called when in the animation to scroll.
     *
     * @param scrollX Target horizontal scroll origin calculated by {@link #mScrollHelper}
     * @param scrollY Target vertical scroll origin calculated by {@link #mScrollHelper}
     * @param targetScrollOffset Output. On return, this will contain the real target scroll origin.
     *                           By default, targetScrollOffset[0] is equal to scrollX and
     * targetScrollOffset[1] is equal to scrollY.
     */
    public void onSmoothScroll(int scrollX, int scrollY, @NonNull final int[] targetScrollOffset) {
      targetScrollOffset[0] = scrollX;
      targetScrollOffset[1] = scrollY;
    }
  }

  /** The scroll interpolator for {@link #mScrollHelper}. */
  static final Interpolator sQuinticInterpolator = new Interpolator() {
    @Override
    public float getInterpolation(float t) {
      t -= 1.0f;
      return t * t * t * t * t + 1.0f;
    }
  };

  public NestedScrollContainerView(@NonNull Context context) {
    this(context, null);
  }

  public NestedScrollContainerView(@NonNull Context context, @Nullable AttributeSet attrs) {
    this(context, attrs, 0);
  }

  public NestedScrollContainerView(
      @NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
    mParentHelper = new NestedScrollingParentHelper(this);
    mChildHelper = new NestedScrollingChildHelper(this);
    mScrollHelper = new ScrollHelper();
    // default enable nested scrolling
    setNestedScrollingEnabled(true);
    initScrollView();
  }

  private void initScrollView() {
    setFocusable(true);
    setDescendantFocusability(FOCUS_AFTER_DESCENDANTS);
    setWillNotDraw(false);
    final ViewConfiguration configuration = ViewConfiguration.get(getContext());
    mTouchSlop = configuration.getScaledTouchSlop();
    mMinFlingVelocity = configuration.getScaledMinimumFlingVelocity();
    mMaxFlingVelocity = configuration.getScaledMaximumFlingVelocity();
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent event) {
    if (mVelocityTracker == null) {
      mVelocityTracker = VelocityTracker.obtain();
    }
    mVelocityTracker.addMovement(event);
    final int action = event.getActionMasked();
    switch (action) {
      case MotionEvent.ACTION_DOWN: {
        mActivePointerId = event.getPointerId(0);
        mInitialMotionX = mLastMotionX = (int) (event.getX() + 0.5f);
        mInitialMotionY = mLastMotionY = (int) (event.getY() + 0.5f);
        if (mScrollState == SCROLL_STATE_FLING || mScrollState == SCROLL_STATE_SCROLL_ANIMATION) {
          getParent().requestDisallowInterceptTouchEvent(true);
          setScrollState(SCROLL_STATE_DRAGGING);
        }
        // Clear the nested offsets
        mNestedOffsets[0] = mNestedOffsets[1] = 0;
        startNestedScroll(getNestedScrollAxis(), TYPE_TOUCH);
        break;
      }
      case MotionEvent.ACTION_POINTER_DOWN: {
        final int actionIndex = event.getActionIndex();
        mActivePointerId = event.getPointerId(actionIndex);
        mInitialMotionX = mLastMotionX = (int) (event.getX(actionIndex) + 0.5f);
        mInitialMotionY = mLastMotionY = (int) (event.getY(actionIndex) + 0.5f);
        break;
      }
      case MotionEvent.ACTION_MOVE: {
        if (mActivePointerId == INVALID_POINTER) {
          break;
        }
        final int pointerIndex = event.findPointerIndex(mActivePointerId);
        if (pointerIndex < 0) {
          return false;
        }
        final int x = (int) (event.getX(pointerIndex) + 0.5f);
        final int y = (int) (event.getY(pointerIndex) + 0.5f);
        if (mScrollState != SCROLL_STATE_DRAGGING) {
          final int dx = x - mInitialMotionX;
          final int dy = y - mInitialMotionY;
          // In the nested scroll scenario, the parent view will satisfy yDiff > mTouchSlop first,
          // but since (this.getNestedScrollAxes() & ViewCompat.SCROLL_AXIS_VERTICAL) == 0
          // is not satisfied, the parent view cannot intercept the ACTION_MOVE, which guarantees
          // that the ACTION_MOVE will be consumed by the child view first.
          // When the child view satisfy yDiff > mTouchSlop, it calls the
          // requestDisallowInterceptTouchEvent method to ensure that it handles all subsequent
          // ACTION_MOVE.
          // Note: The mNestedScrollAxes value will be modified in
          // NestedScrollingParent#onNestedScrollAccepted()
          final boolean isVerticalDragging = mIsVertical && Math.abs(dy) > mTouchSlop
              && (this.getNestedScrollAxes() & ViewCompat.SCROLL_AXIS_VERTICAL) == 0;
          final boolean isHorizontalDragging = !mIsVertical && Math.abs(dx) > mTouchSlop
              && (this.getNestedScrollAxes() & ViewCompat.SCROLL_AXIS_HORIZONTAL) == 0;
          if (isVerticalDragging || isHorizontalDragging) {
            mLastMotionX = x;
            mLastMotionY = y;
            setScrollState(SCROLL_STATE_DRAGGING);
          }
        }
        break;
      }
      case MotionEvent.ACTION_POINTER_UP: {
        onPointerUp(event);
        break;
      }
      case MotionEvent.ACTION_UP: {
        mVelocityTracker.clear();
        stopNestedScroll(TYPE_TOUCH);
        break;
      }
      case MotionEvent.ACTION_CANCEL: {
        resetTouch();
        setScrollState(SCROLL_STATE_IDLE);
        break;
      }
      default: {
        break;
      }
    }
    return mScrollState == SCROLL_STATE_DRAGGING;
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (mVelocityTracker == null) {
      mVelocityTracker = VelocityTracker.obtain();
    }
    boolean eventAddedToVelocityTracker = false;

    // Note: In the nested scroll scenario, an additional MotionEvent tempEv need to be created to
    // properly calculate the fling velocity.
    final MotionEvent tempEv = MotionEvent.obtain(event);
    final int action = event.getActionMasked();
    if (action == MotionEvent.ACTION_DOWN) {
      mNestedOffsets[0] = mNestedOffsets[1] = 0;
    }
    tempEv.offsetLocation(mNestedOffsets[0], mNestedOffsets[1]);
    switch (action) {
      case MotionEvent.ACTION_DOWN: {
        mActivePointerId = event.getPointerId(0);
        mInitialMotionX = mLastMotionX = (int) (event.getX() + 0.5f);
        mInitialMotionY = mLastMotionY = (int) (event.getY() + 0.5f);
        startNestedScroll(getNestedScrollAxis(), TYPE_TOUCH);
        break;
      }
      case MotionEvent.ACTION_POINTER_DOWN: {
        final int actionIndex = event.getActionIndex();
        mActivePointerId = event.getPointerId(actionIndex);
        mInitialMotionX = mLastMotionX = (int) (event.getX(actionIndex) + 0.5f);
        mInitialMotionY = mLastMotionY = (int) (event.getY(actionIndex) + 0.5f);
        break;
      }
      case MotionEvent.ACTION_MOVE: {
        if (mActivePointerId == INVALID_POINTER) {
          break;
        }
        final int pointerIndex = event.findPointerIndex(mActivePointerId);
        if (pointerIndex < 0) {
          return false;
        }
        final int x = (int) (event.getX(pointerIndex) + 0.5f);
        final int y = (int) (event.getY(pointerIndex) + 0.5f);
        int deltaX = mLastMotionX - x;
        int deltaY = mLastMotionY - y;
        if (dispatchNestedPreScroll(deltaX, deltaY, mScrollConsumed, mScrollOffset, TYPE_TOUCH)) {
          if (DEBUG) {
            LLog.i(TAG,
                "onTouchEvent->dispatchNestedPreScroll: delta = " + (mIsVertical ? deltaY : deltaX)
                    + ", consumed = " + (mIsVertical ? mScrollConsumed[1] : mScrollConsumed[0])
                    + ", offset = " + (mIsVertical ? mScrollOffset[1] : mScrollOffset[0]));
          }
          deltaX -= mScrollConsumed[0];
          deltaY -= mScrollConsumed[1];
          tempEv.offsetLocation(mScrollOffset[0], mScrollOffset[1]);
          // Accumulate the offset in local view.
          mNestedOffsets[0] += mScrollOffset[0];
          mNestedOffsets[1] += mScrollOffset[1];
        }
        if (mScrollState != SCROLL_STATE_DRAGGING) {
          // Math.abs(deltaX) > mTouchSlop means the scroll distance exceeds the touch threshold and
          // current view should consume ACTION_MOVE event which cannot be intercepted by parent
          // view.
          if (mIsVertical && Math.abs(deltaY) > mTouchSlop) {
            // Note: need to modify the deltaY by mTouchSlop to prevent the view from jumping.
            if (deltaY > 0) {
              deltaY -= mTouchSlop;
            } else {
              deltaY += mTouchSlop;
            }
            setScrollState(SCROLL_STATE_DRAGGING);
          } else if (!mIsVertical && Math.abs(deltaX) > mTouchSlop) {
            // Note: need to modify the deltaX by mTouchSlop to prevent the view from jumping.
            if (deltaX > 0) {
              deltaX -= mTouchSlop;
            } else {
              deltaX += mTouchSlop;
            }
            setScrollState(SCROLL_STATE_DRAGGING);
          }
        }
        if (mScrollState == SCROLL_STATE_DRAGGING) {
          // recalculate the last motion X and Y due to nested scroll parent may consume scroll
          // distance.
          mLastMotionX = x - mScrollOffset[0];
          mLastMotionY = y - mScrollOffset[1];
          if (scrollByInternal(mIsVertical ? 0 : deltaX, mIsVertical ? deltaY : 0, tempEv)) {
            getParent().requestDisallowInterceptTouchEvent(true);
          }
        }
        break;
      }
      case MotionEvent.ACTION_POINTER_UP: {
        onPointerUp(event);
        break;
      }
      case MotionEvent.ACTION_UP: {
        mVelocityTracker.addMovement(tempEv);
        eventAddedToVelocityTracker = true;
        mVelocityTracker.computeCurrentVelocity(1000, mMaxFlingVelocity);
        final float velX = !mIsVertical ? -mVelocityTracker.getXVelocity(mActivePointerId) : 0;
        final float velY = mIsVertical ? -mVelocityTracker.getYVelocity(mActivePointerId) : 0;
        if (!flingWithNestedDispatch((int) velX, (int) velY) && !mScrollHelper.paging()) {
          setScrollState(SCROLL_STATE_IDLE);
        }
        resetTouch();
        break;
      }
      case MotionEvent.ACTION_CANCEL: {
        resetTouch();
        setScrollState(SCROLL_STATE_IDLE);
        break;
      }
      default: {
        break;
      }
    }
    if (!eventAddedToVelocityTracker) {
      if (DEBUG) {
        LLog.i(TAG, "mVelocityTracker add movement: " + tempEv.getX() + ", " + tempEv.getY());
      }
      mVelocityTracker.addMovement(tempEv);
    }
    tempEv.recycle();
    return true;
  }

  private boolean scrollByInternal(int deltaX, int deltaY, MotionEvent event) {
    int unconsumedX = 0;
    int unconsumedY = 0;
    mScrollStepConsumed[0] = 0;
    mScrollStepConsumed[1] = 0;
    scrollStep(deltaX, deltaY, mScrollStepConsumed);
    unconsumedX = deltaX - mScrollStepConsumed[0];
    unconsumedY = deltaY - mScrollStepConsumed[1];

    if (dispatchNestedScroll(mScrollStepConsumed[0], mScrollStepConsumed[1], unconsumedX,
            unconsumedY, mScrollOffset, TYPE_TOUCH)) {
      mLastMotionX -= mScrollOffset[0];
      mLastMotionY -= mScrollOffset[1];
      if (event != null) {
        event.offsetLocation(mScrollOffset[0], mScrollOffset[1]);
      }
      mNestedOffsets[0] += mScrollOffset[0];
      mNestedOffsets[1] += mScrollOffset[1];
    }
    return mScrollStepConsumed[0] != 0 || mScrollStepConsumed[1] != 0;
  }

  private void scrollStep(int deltaX, int deltaY, int[] consumed) {
    final int scrollX = getScrollX();
    final int scrollY = getScrollY();
    final int scrollRange = getScrollRange();
    // clamp scroll offset to the scroll range.
    int clampDeltaX = clampScrollDelta(deltaX, scrollX, mIsVertical ? 0 : scrollRange);
    int clampDeltaY = clampScrollDelta(deltaY, scrollY, mIsVertical ? scrollRange : 0);
    if (DEBUG) {
      LLog.i(TAG,
          "scrollStep: scroll = [" + scrollX + ", " + scrollY + "], delta = [" + deltaX + ", "
              + deltaY + "], clamp = [" + clampDeltaX + ", " + clampDeltaY + "]");
    }
    if (Math.abs(clampDeltaX) > 0 || Math.abs(clampDeltaY) > 0) {
      super.scrollTo(scrollX + clampDeltaX, scrollY + clampDeltaY);
    }
    if (consumed != null) {
      // This view should always consume the scroll distance which clamped by the scroll range which
      // is aligned with RecyclerView, and in this way, the nested scroll parent can get the correct
      // un-consumed scroll distance.
      consumed[0] = clampDeltaX;
      consumed[1] = clampDeltaY;
    }
    if (DEBUG) {
      LLog.i(TAG,
          "scrollStep end: scroll = [" + getScrollX() + ", " + getScrollY() + "], consumed = ["
              + consumed[0] + ", " + consumed[1] + "]");
    }
  }

  private int clampScrollDelta(int delta, int currentOffset, int scrollRange) {
    int offset = currentOffset + delta;
    if (offset < 0) {
      return -currentOffset;
    } else if (offset > scrollRange) {
      return scrollRange - currentOffset;
    }
    return delta;
  }

  private boolean flingWithNestedDispatch(int velocityX, int velocityY) {
    if (DEBUG) {
      LLog.i(TAG, "flingWithNestedDispatch: " + velocityX + ", " + velocityY);
    }
    if (mIsVertical || Math.abs(velocityX) < mMinFlingVelocity) {
      velocityX = 0;
    }
    if (!mIsVertical || Math.abs(velocityY) < mMinFlingVelocity) {
      velocityY = 0;
    }
    if (velocityX == 0 && velocityY == 0) {
      return false;
    }
    // The parent view has chance to fully consume the fling in a nested before the child
    // view consumes it
    if (!dispatchNestedPreFling(velocityX, velocityY)) {
      dispatchNestedFling(velocityX, velocityY, true);
      if (DEBUG) {
        LLog.i(TAG,
            "onTouchEvent -> ACTION_UP: dispatch a fling to a nested scrolling parent and parent not consumes, fling by self with velocity = "
                + velocityX + ", " + velocityY);
      }
      startNestedScroll(getNestedScrollAxis(), TYPE_NON_TOUCH);
      velocityX = Math.max(-mMaxFlingVelocity, Math.min(velocityX, mMaxFlingVelocity));
      velocityY = Math.max(-mMaxFlingVelocity, Math.min(velocityY, mMaxFlingVelocity));
      mScrollHelper.fling(velocityX, velocityY);
      return true;
    }
    return false;
  }

  protected void setScrollState(int state) {
    LLog.i(TAG, "setScrollState: " + mScrollState + " -> " + state);
    if (state == mScrollState) {
      return;
    }
    if (mIsDuringAutoScroll && state == SCROLL_STATE_IDLE) {
      return;
    }
    mScrollState = state;
    if (mScrollState != SCROLL_STATE_FLING && mScrollState != SCROLL_STATE_SCROLL_ANIMATION) {
      mScrollHelper.stop();
    }
    dispatchOnScrollStateChanged(state);
  }

  public void smoothScrollTo(int scrollX, int scrollY) {
    mScrollHelper.smoothScrollTo(scrollX, scrollY);
  }

  public void stopFling() {
    mScrollHelper.stopFling();
  }

  class ScrollHelper implements Runnable {
    private int mLastScrollX;
    private int mLastScrollY;
    private int mTotalDeltaX;
    private int mTotalDeltaY;
    private ListCustomScroller mScroller;

    private CustomScrollHook mCustomScrollHook;

    public ScrollHelper() {
      mScroller = new ListCustomScroller(getContext(), sQuinticInterpolator);
    }

    private float getAvailableScrollOffsetFromSubviews(boolean forward) {
      float min = Float.MAX_VALUE;
      float max = Float.MIN_VALUE;
      boolean vertical = mIsVertical;
      // TODO:(dingwang.wxx) Define a interface and explicitly obtain the LinearLayout within the
      // ScrollView.
      if (getChildCount() > 0 && getChildAt(0) instanceof ViewGroup) {
        ViewGroup container = (ViewGroup) getChildAt(0);
        if (forward) {
          for (int i = 0; i < container.getChildCount(); i++) {
            View view = container.getChildAt(i);
            float offset =
                vertical ? (view.getY() + view.getHeight()) : (view.getX() + view.getWidth());
            if (max < offset) {
              max = offset;
            }
          }
          max = max - (vertical ? getHeight() : getWidth());
        } else {
          for (int i = 0; i < container.getChildCount(); i++) {
            View view = container.getChildAt(i);
            float offset = vertical ? view.getY() : view.getX();
            if (min > offset) {
              min = offset;
            }
          }
        }
      }
      return forward ? max : min;
    }

    public void fling(int velocityX, int velocityY) {
      setScrollState(SCROLL_STATE_FLING);
      mCustomScrollHook = null;
      mLastScrollX = getScrollX();
      mLastScrollY = getScrollY();
      mTotalDeltaX = 0;
      mTotalDeltaY = 0;
      if (mSnapHelper == null) {
        if (mMaxFlingDistanceRatio > 0) {
          float limitedDistance = 0;
          float forwardFlingDistance = 0;
          float backwardFlingDistance = 0;
          float currentOffset = mIsVertical ? mLastScrollY : mLastScrollX;

          if (mMaxFlingDistanceRatio == LIST_AUTOMATIC_MAX_FLING_RATIO) {
            forwardFlingDistance = getAvailableScrollOffsetFromSubviews(true) - currentOffset;
            backwardFlingDistance = currentOffset - getAvailableScrollOffsetFromSubviews(false);
          } else {
            float maxFlingDistance =
                mMaxFlingDistanceRatio * (mIsVertical ? getHeight() : getWidth());
            forwardFlingDistance = maxFlingDistance;
            backwardFlingDistance = maxFlingDistance;
          }
          mScroller.fling(mLastScrollX, mLastScrollY, velocityX, velocityY, Integer.MIN_VALUE,
              Integer.MAX_VALUE, Integer.MIN_VALUE, Integer.MAX_VALUE, 0, 0,
              (int) forwardFlingDistance, (int) backwardFlingDistance);
        } else {
          mScroller.fling(mLastScrollX, mLastScrollY, velocityX, velocityY, Integer.MIN_VALUE,
              Integer.MAX_VALUE, Integer.MIN_VALUE, Integer.MAX_VALUE);
        }
      } else {
        pagingInternal(velocityX, velocityY);
      }
      postOnAnimationCompat();
    }

    public boolean paging() {
      if (mSnapHelper == null) {
        return false;
      }
      setScrollState(SCROLL_STATE_FLING);
      mCustomScrollHook = null;
      mLastScrollX = getScrollX();
      mLastScrollY = getScrollY();
      mTotalDeltaX = 0;
      mTotalDeltaY = 0;

      pagingInternal(0, 0);

      postOnAnimationCompat();
      return true;
    }

    private void pagingInternal(int velocityX, int velocityY) {
      int[] out = mSnapHelper.findTargetSnapOffset(velocityX, velocityY, mIsVertical, isRtl());
      int dx = out[0] - mLastScrollX;
      int dy = out[1] - mLastScrollY;
      int time = (int) (Math.ceil(Math.ceil(Math.abs(mIsVertical ? dy : dx)
                            * mSnapHelper.mSnapAlignmentMillisecondsPerPx))
          / .3356);
      if (time < 100) {
        time = 100;
      }
      mScroller.startScroll(mLastScrollX, mLastScrollY, dx, dy, time);
    }

    public void stopFling() {
      if (!mScroller.isFinished()) {
        mScroller.abortAnimation();
      }
      mLastScrollX = 0;
      mLastScrollY = 0;
      mTotalDeltaX = 0;
      mTotalDeltaY = 0;
    }

    public void smoothScrollTo(int scrollX, int scrollY) {
      if (DEBUG) {
        LLog.i(TAG, "smoothScrollTo: " + scrollX + ", " + scrollY);
      }
      if (!mScroller.isFinished()) {
        mScroller.abortAnimation();
      }
      setScrollState(SCROLL_STATE_SCROLL_ANIMATION);
      mLastScrollX = getScrollX();
      mLastScrollY = getScrollY();
      if (mCustomScrollHook != null) {
        mCustomScrollHook.onSmoothScrollStart(mLastScrollX, mLastScrollY, scrollX, scrollY);
      }
      mScroller.startScroll(
          mLastScrollX, mLastScrollY, scrollX - mLastScrollX, scrollY - mLastScrollY);
      postOnAnimationCompat();
    }

    private void stop() {
      removeCallbacks(this);
      mScroller.abortAnimation();
      // Note: If in smooth scroll state, we should invoke onSmoothScrollEnd() to notify the native
      // list reset the scrolling info.
      if (mCustomScrollHook != null) {
        mCustomScrollHook.onSmoothScrollEnd();
        mCustomScrollHook = null;
      }
    }

    @Override
    public void run() {
      // Note: Keep a local reference so that if it is changed during onAnimation method, it won't
      // cause unexpected behaviors.

      final ListCustomScroller scroller = mScroller;
      if (scroller.computeScrollOffset()) {
        final int[] scrollConsumed = mScrollConsumed;
        int x = scroller.getCurrX();
        int y = scroller.getCurrY();
        if (DEBUG) {
          LLog.i(TAG,
              "ScrollHelper: computed offset from scroller = " + (mIsVertical ? y : x)
                  + ", mTotalDelta = " + (mIsVertical ? mTotalDeltaY : mTotalDeltaX));
        }
        if (mCustomScrollHook != null) {
          final int[] targetScrollOffset = mTargetScrollOffset;
          mCustomScrollHook.onSmoothScroll(x, y, targetScrollOffset);
          x = targetScrollOffset[0];
          y = targetScrollOffset[1];
        } else {
          x -= mTotalDeltaX;
          y -= mTotalDeltaY;
        }
        int deltaX = x - mLastScrollX;
        int deltaY = y - mLastScrollY;
        if (DEBUG) {
          LLog.i(TAG,
              "ScrollHelper: modified offset = " + (mIsVertical ? y : x)
                  + ", delta = " + (mIsVertical ? deltaY : deltaX));
        }
        if (dispatchNestedPreScroll(deltaX, deltaY, scrollConsumed, null, TYPE_NON_TOUCH)) {
          deltaX -= scrollConsumed[0];
          deltaY -= scrollConsumed[1];
        }
        mScrollStepConsumed[0] = 0;
        mScrollStepConsumed[1] = 0;
        scrollStep(deltaX, deltaY, mScrollStepConsumed);
        int consumedX = mScrollStepConsumed[0];
        int consumedY = mScrollStepConsumed[1];
        int unconsumedX = deltaX - consumedX;
        int unconsumedY = deltaY - consumedY;

        dispatchNestedScroll(consumedX, consumedY, unconsumedX, unconsumedY, null, TYPE_NON_TOUCH);
        final boolean fullyConsumedVertical =
            mIsVertical && (deltaY == 0 || (deltaY != 0 && unconsumedY == 0));
        final boolean fullyConsumedHorizontal =
            !mIsVertical && (deltaX == 0 || (deltaX != 0 && unconsumedX == 0));
        final boolean fullyConsumedAny = fullyConsumedHorizontal || fullyConsumedVertical;
        if (!fullyConsumedAny && !hasNestedScrollingParent(TYPE_NON_TOUCH)) {
          // setting state to idle will stop this.
          setScrollState(SCROLL_STATE_IDLE);
        }
        // Finally update the positions and post
        // Note: the last scroll offset should be got from getScrollX()/getScrollY(), instead of
        // using x/y which is calculated from scroller, and we need to accumulate the
        // mTotalDeltaX/mTotalDeltaY value to modify x/y value in the next execution of this
        // runnable.
        mLastScrollX = getScrollX();
        mLastScrollY = getScrollY();
        mTotalDeltaX += x - mLastScrollX;
        mTotalDeltaY += y - mLastScrollY;
        postOnAnimationCompat();
      } else {
        // setting state to idle will stop this.
        setScrollState(SCROLL_STATE_IDLE);
        if (hasNestedScrollingParent(TYPE_NON_TOUCH)) {
          stopNestedScroll(TYPE_NON_TOUCH);
        }
        mLastScrollX = 0;
        mLastScrollY = 0;
        mTotalDeltaX = 0;
        mTotalDeltaY = 0;
        if (mCustomScrollHook != null) {
          mCustomScrollHook.onSmoothScrollEnd();
        }
      }
    }

    private void postOnAnimationCompat() {
      removeCallbacks(this);
      ViewCompat.postOnAnimation(NestedScrollContainerView.this, this);
    }
  }

  public int getScrollRange() {
    int scrollRange = 0;
    // TODO:(dingwang.wxx) Define a interface and explicitly obtain the LinearLayout within the
    // ScrollView.
    if (getChildCount() > 0 && getChildAt(0) != null) {
      View child = getChildAt(0);
      ViewGroup.LayoutParams layoutParams = child.getLayoutParams();
      if (layoutParams instanceof FrameLayout.LayoutParams) {
        FrameLayout.LayoutParams lp = (LayoutParams) layoutParams;
        int contentSize = 0;
        int parentSize = 0;
        if (mIsVertical) {
          contentSize = child.getHeight() + lp.topMargin + lp.bottomMargin;
          parentSize = getHeight() - getPaddingTop() - getPaddingBottom();
        } else {
          contentSize = child.getWidth() + lp.leftMargin + lp.rightMargin;
          parentSize = getWidth() - getPaddingLeft() - getPaddingRight();
        }
        scrollRange = Math.max(0, contentSize - parentSize);
      }
    }
    return scrollRange;
  }

  public boolean canScrollBy(int delta) {
    final int scrollX = getScrollX() + (mIsVertical ? 0 : delta);
    final int scrollY = getScrollY() + (mIsVertical ? delta : 0);
    final int scrollRange = getScrollRange();
    return mIsVertical ? scrollY > 0 && scrollY < scrollRange
                       : scrollX > 0 && scrollX < scrollRange;
  }

  private int getNestedScrollAxis() {
    return mIsVertical ? ViewCompat.SCROLL_AXIS_VERTICAL : ViewCompat.SCROLL_AXIS_HORIZONTAL;
  }

  private void onPointerUp(MotionEvent event) {
    final int actionIndex = event.getActionIndex();
    if (event.getPointerId(actionIndex) == mActivePointerId) {
      // Pick a new pointer to pick up the slack.
      final int newIndex = actionIndex == 0 ? 1 : 0;
      mActivePointerId = event.getPointerId(newIndex);
      mInitialMotionX = mLastMotionX = (int) (event.getX(newIndex) + 0.5f);
      mInitialMotionY = mLastMotionY = (int) (event.getY(newIndex) + 0.5f);
    }
  }

  private void resetTouch() {
    if (mVelocityTracker != null) {
      mVelocityTracker.clear();
    }
    stopNestedScroll(TYPE_TOUCH);
  }

  public void setIsVertical(boolean isVertical) {
    mIsVertical = isVertical;
  }

  public void setCustomScrollHook(CustomScrollHook customScrollHook) {
    if (mScrollHelper != null) {
      mScrollHelper.mCustomScrollHook = customScrollHook;
    }
  }

  public CustomScrollHook getCustomScrollHook() {
    if (mScrollHelper != null) {
      return mScrollHelper.mCustomScrollHook;
    }
    return null;
  }

  private void dispatchOnScrollStateChanged(int state) {
    if (mOnScrollStateChangeListener != null) {
      mOnScrollStateChangeListener.onScrollStateChange(this, state);
    }
  }

  public void setOnScrollStateChangeListener(OnScrollStateChangeListener listener) {
    mOnScrollStateChangeListener = listener;
  }

  public void setMaxFlingDistanceRatio(float ratio) {
    mMaxFlingDistanceRatio = ratio;
  }

  public void setEnableScroll(boolean enableScroll) {
    setOnTouchListener(new View.OnTouchListener() {
      @Override
      public boolean onTouch(View v, MotionEvent event) {
        return !enableScroll;
      }
    });
  }

  protected boolean isRtl() {
    return false;
  }

  @Override
  public boolean canScrollHorizontally(int direction) {
    return !mIsVertical && super.canScrollHorizontally(direction);
  }

  @Override
  public boolean canScrollVertically(int direction) {
    return mIsVertical && super.canScrollVertically(direction);
  }

  // Override to compute the horizontal range that the horizontal scrollbar represents.
  @Override
  protected int computeHorizontalScrollRange() {
    if (mIsVertical) {
      return 0;
    }
    final int containerSize = getWidth() - getPaddingLeft() - getPaddingRight();
    // TODO:(dingwang.wxx) Define a interface and explicitly obtain the LinearLayout within the
    // ScrollView.
    if (getChildCount() > 0 && getChildAt(0) != null) {
      final View child = getChildAt(0);
      ViewGroup.LayoutParams layoutParams = child.getLayoutParams();
      if (layoutParams instanceof FrameLayout.LayoutParams) {
        FrameLayout.LayoutParams lp = (LayoutParams) layoutParams;
        return child.getWidth() + lp.leftMargin + lp.rightMargin;
      }
    }
    return containerSize;
  }

  // Override to compute the vertical range that the vertical scrollbar represents.
  @Override
  protected int computeVerticalScrollRange() {
    if (!mIsVertical) {
      return 0;
    }
    final int containerSize = getHeight() - getPaddingTop() - getPaddingBottom();
    // TODO:(dingwang.wxx) Define a interface and explicitly obtain the LinearLayout within the
    // ScrollView.
    if (getChildCount() > 0 && getChildAt(0) != null) {
      final View child = getChildAt(0);
      ViewGroup.LayoutParams layoutParams = child.getLayoutParams();
      if (layoutParams instanceof FrameLayout.LayoutParams) {
        FrameLayout.LayoutParams lp = (LayoutParams) layoutParams;
        return child.getHeight() + lp.topMargin + lp.bottomMargin;
      }
    }
    return containerSize;
  }

  @Override
  protected int computeHorizontalScrollOffset() {
    return !mIsVertical ? Math.max(0, super.computeHorizontalScrollOffset()) : 0;
  }

  @Override
  protected int computeVerticalScrollOffset() {
    return mIsVertical ? Math.max(0, super.computeVerticalScrollOffset()) : 0;
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
    return this.hasNestedScrollingParent(TYPE_TOUCH);
  }

  @Override
  public boolean startNestedScroll(int axes, int type) {
    return mChildHelper.startNestedScroll(axes, type);
  }

  @Override
  public boolean startNestedScroll(int axes) {
    return this.startNestedScroll(axes, TYPE_TOUCH);
  }

  @Override
  public void stopNestedScroll(int type) {
    mChildHelper.stopNestedScroll(type);
  }

  @Override
  public void stopNestedScroll() {
    this.stopNestedScroll(TYPE_TOUCH);
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
        dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow, TYPE_TOUCH);
  }

  @Override
  public boolean dispatchNestedPreScroll(
      int dx, int dy, int[] consumed, int[] offsetInWindow, int type) {
    return mChildHelper.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, type);
  }

  @Override
  public boolean dispatchNestedPreScroll(int dx, int dy, int[] consumed, int[] offsetInWindow) {
    return this.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, TYPE_TOUCH);
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
    mParentHelper.onNestedScrollAccepted(child, target, axes, type);
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
    mParentHelper.onStopNestedScroll(target, type);
    this.stopNestedScroll(type);
  }

  @Override
  public void onStopNestedScroll(View child) {
    this.onStopNestedScroll(child, TYPE_TOUCH);
  }

  @Override
  public void onNestedPreScroll(
      @NonNull View target, int dx, int dy, @NonNull int[] consumed, int type) {
    this.dispatchNestedPreScroll(dx, dy, consumed, null, type);
  }

  @Override
  public void onNestedPreScroll(View target, int dx, int dy, int[] consumed) {
    this.onNestedPreScroll(target, dx, dy, consumed, TYPE_TOUCH);
  }

  @Override
  public void onNestedScroll(@NonNull View target, int dxConsumed, int dyConsumed, int dxUnconsumed,
      int dyUnconsumed, int type) {
    if (mIsVertical) {
      final int oldScrollY = getScrollY();
      scrollBy(0, dyUnconsumed);
      final int myConsumed = getScrollY() - oldScrollY;
      final int myUnconsumed = dyUnconsumed - myConsumed;
      this.dispatchNestedScroll(0, myConsumed, 0, myUnconsumed, null, type);
    } else {
      final int oldScrollX = getScrollX();
      scrollBy(dxUnconsumed, 0);
      final int myConsumed = getScrollX() - oldScrollX;
      final int myUnconsumed = dxUnconsumed - myConsumed;
      this.dispatchNestedScroll(myConsumed, 0, myUnconsumed, 0, null, type);
    }
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
    return mParentHelper.getNestedScrollAxes();
  }

  /********* NestedScrollingParent2 end *********/
}
