// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import static com.lynx.tasm.behavior.ui.scroll.UIScrollView.INVALID_INDEX;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.OverScroller;
import android.widget.ScrollView;
import androidx.core.math.MathUtils;
import androidx.core.view.ViewCompat;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.behavior.ui.IDrawChildHook.IDrawChildHookBinding;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.utils.BorderRadius;
import com.lynx.tasm.event.LynxScrollEvent;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.utils.PixelUtils;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.util.ArrayList;

public class AndroidScrollView extends NestedScrollView implements IDrawChildHookBinding {
  public static final int HORIZONTAL = 0;
  public static final int VERTICAL = 1;
  private static final int INTERNAL_FOR_SCROLL_END_CHECK = 100; // ms
  // Whether the down event has been processed, gesture starts from the down event, so if you want
  // to handle the gesture with one gesture, you need to convert one of the move events into a down
  // event
  private boolean mIsDownEventHandled = true;
  // Whether to consume gestures
  private Boolean mConsumeGesture = null;

  private UIScrollView mUIScrollView;
  private LinearLayout mLinearLayout;
  private boolean isLinearLayoutExist = false;
  private CustomHorizontalScrollView mHorizontalScrollView;
  private int mLastScrollY;
  private int mLastScrollX;

  boolean isHorizontal = false;
  boolean isUserDragging = false;
  private int mMeasuredWidth = 0;
  private int mMeasuredHeight = 0;
  private ArrayList<OnScrollListener> mOnScrollListenerList;
  private IDrawChildHook mDrawChildHook;

  private Runnable mScrollerEndDetectionTask;
  private int initialPositionY = 0;
  private int initialPositionX = 0;
  private Rect mClipRect;

  private SmoothScrollRunnable mSmoothScrollRunnable = null;
  private boolean mNeedAutoScroll = false;
  private boolean mBlockDescendantFocusability = false;
  private int mAutoScrollRate = 0;
  private int mScrollRange = 0;
  protected boolean mDirectionChanged = false;
  private boolean mForbidFocusChangeAfterFling = false;
  private boolean mHandleTouchMove = false;

  /**
   * SCROLL_STATE_IDLE: Indicates that the scrollview is in an idle, settled state.
   */
  public static final int SCROLL_STATE_IDLE = 0;
  /**
   * SCROLL_STATE_DRAGGING: Indicates that the scrollview is currently being dragged by the user.
   */
  public static final int SCROLL_STATE_DRAGGING = 1;
  /**
   * SCROLL_STATE_SETTLING: Indicates that the scrollview is in the process of fling to a final
   * position.
   */
  public static final int SCROLL_STATE_FLING = 2;
  /**
   * SCROLL_STATE_SETTLING: Indicates that the scrollview is in the process of scrolling triggered
   * by UI.
   */
  public static final int SCROLL_STATE_ANIMATION = 3;
  /**
   * SCROLL_NESTED_SCROLL: Indicates that the scrollview is in nested scroll.
   */
  public static final int SCROLL_NESTED_SCROLL = 4;

  // Used to handle gesture conflicts and control the return value of canScrollHorizontally()
  // and canScrollVertically(). In the viewpager nested scrollview scenario, if set to true, the
  // external viewpager will not respond to touch event.
  private boolean mForceCanScroll = false;

  int mScrollState = SCROLL_STATE_IDLE;
  int mLastScrollState = SCROLL_STATE_IDLE;
  private Rect mUiBound = null;

  private static class ScrollerEndDetectionTask implements Runnable {
    private WeakReference<AndroidScrollView> mWeakScrollView;

    public ScrollerEndDetectionTask(AndroidScrollView view) {
      mWeakScrollView = new WeakReference<>(view);
    }

    public void run() {
      if (mWeakScrollView.get() != null) {
        AndroidScrollView scrollView = mWeakScrollView.get();
        int newPositionY = scrollView.getScrollY();
        int newPositionX = scrollView.mHorizontalScrollView.getScrollX();
        boolean stoped =
            (scrollView.isHorizontal && scrollView.initialPositionX - newPositionX == 0)
            || (!scrollView.isHorizontal && scrollView.initialPositionY - newPositionY == 0);
        if (scrollView.isUserDragging || !stoped) {
          scrollView.initialPositionY = newPositionY;
          scrollView.initialPositionX = newPositionX;
          scrollView.postDelayed(this, scrollView.INTERNAL_FOR_SCROLL_END_CHECK);
        } else { // has stopped
          scrollView.notifyStateChange(SCROLL_STATE_IDLE);
          scrollView.triggerOnScrollStop();
          scrollView.mUIScrollView.scrollToBounce(true);
        }
      }
    }
  }

  void autoScroll(ReadableMap params) {
    double ratePerSecond = params.getDouble("rate", 0.0);
    boolean start = params.getBoolean("start", true);
    if (start) {
      int ratePerFrame = (int) PixelUtils.dipToPx(ratePerSecond / 60);
      if (ratePerSecond == 0) {
        LLog.e(UIScrollView.TAG, "the rate of speed  is not right, current value is 0");
        return;
      }
      if (!mNeedAutoScroll) {
        mNeedAutoScroll = true;
        mAutoScrollRate = ratePerFrame;
        mSmoothScrollRunnable = new SmoothScrollRunnable(this);
        post(mSmoothScrollRunnable);
      }
    } else {
      mNeedAutoScroll = false;
      mAutoScrollRate = 0;
    }
  }

  private static class SmoothScrollRunnable implements Runnable {
    private WeakReference<AndroidScrollView> mWeakScrollView;

    public SmoothScrollRunnable(AndroidScrollView view) {
      mWeakScrollView = new WeakReference<>(view);
    }

    @Override
    public void run() {
      if (mWeakScrollView != null && mWeakScrollView.get() != null
          && mWeakScrollView.get().mLinearLayout != null) {
        AndroidScrollView scrollView = mWeakScrollView.get();
        if (scrollView.mNeedAutoScroll) {
          int x = scrollView.getRealScrollX();
          int y = scrollView.getRealScrollY();
          int autoScrollRate = scrollView.mAutoScrollRate;
          LinearLayout linearLayout = scrollView.mLinearLayout;
          if (!scrollView.isHorizontal) { // for vertical scrollview
            scrollView.setScrollTo(x, y + autoScrollRate, false);
            if (autoScrollRate < 0) {
              // if the scrolling orientation is reverse sequence and the scrollView reaches the
              // upper border, should stop autoScroll
              if (y <= 0) {
                scrollView.mNeedAutoScroll = false;
              } else {
                scrollView.postDelayed(this, 16);
              }
            } else {
              // the scrolling orientation is sequence and the scrollView reaches the bottom border,
              // should stop autoScroll
              if (y + autoScrollRate + scrollView.getMeasuredHeight()
                  < linearLayout.getMeasuredHeight()) {
                // Start a scheduled task for automatic sliding.
                scrollView.postDelayed(this, 16);
              } else {
                scrollView.mNeedAutoScroll = false;
              }
            }
          } else { // for horizontal scrollview
            int targetScrollX =
                ViewCompat.getLayoutDirection(scrollView) == ViewCompat.LAYOUT_DIRECTION_RTL
                ? x - autoScrollRate
                : x + autoScrollRate;
            scrollView.setScrollTo(targetScrollX, y, false);
            if (autoScrollRate < 0) {
              // the scrolling orientation is reverse sequence,the two scenes should stop autoScroll
              // LAYOUT_DIRECTION_LTR:  getScrollX<= 0
              // LAYOUT_DIRECTION_RTL:  getScrollX + measureHeight >= ContentSize
              boolean autoStop =
                  ViewCompat.getLayoutDirection(scrollView) == ViewCompat.LAYOUT_DIRECTION_LTR
                  ? (x <= 0)
                  : (targetScrollX + scrollView.getMeasuredWidth()
                      >= linearLayout.getMeasuredWidth());
              if (autoStop) {
                scrollView.mNeedAutoScroll = false;
              } else {
                scrollView.postDelayed(this, 16);
              }
            } else {
              // the scrolling orientation is reverse sequence and the scrollView reaches the right
              // border, should stop autoScroll
              if (targetScrollX > 0
                  && targetScrollX + scrollView.getMeasuredWidth()
                      < linearLayout.getMeasuredWidth()) {
                scrollView.postDelayed(this, 16);
              } else {
                scrollView.mNeedAutoScroll = false;
              }
            }
          }
        }
      }
    }
  }

  public void consumeGesture(boolean consume) {
    mConsumeGesture = consume;
    if (consume) {
      mIsDownEventHandled = false;
    }
  }

  public AndroidScrollView(Context context, UIScrollView uiScrollView) {
    super(context, uiScrollView);
    mUIScrollView = uiScrollView;
    this.setVerticalScrollBarEnabled(false);
    this.setOverScrollMode(HorizontalScrollView.OVER_SCROLL_NEVER);
    this.setFadingEdgeLength(0);
    this.setScrollContainer(false);
    mClipRect = new Rect();
    mOnScrollListenerList = new ArrayList<>();

    if (mLinearLayout == null) {
      createLinearLayout();
      createInnerScrollView(mUIScrollView);
      mHorizontalScrollView.addView(
          mLinearLayout, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
      this.addView(mHorizontalScrollView,
          new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
    }
    mScrollerEndDetectionTask = new ScrollerEndDetectionTask(this);
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {
    super.onLayout(changed, l, t, r, b);
    if (!isHorizontal) {
      final int bounceScrollRange = mBounceGestureHelper.getBounceScrollRange();
      if (mEnableNewBounce && bounceScrollRange > 0 && bounceScrollRange != getScrollY()) {
        setScrollTo(getScrollX(), bounceScrollRange, false);
      }
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent ev) {
    if (!isHorizontal) {
      if (mUIScrollView.isEnableNewGesture() && (mConsumeGesture == null || !mConsumeGesture)
          && ev.getActionMasked() != MotionEvent.ACTION_DOWN) {
        // If new gestures are enabled, return false to indicate that the event is not consumed, So
        // this event can be passed to parent node, do not intercept the down event, otherwise will
        // not receive other types of events.
        if (ev.getAction() == MotionEvent.ACTION_UP) {
          isUserDragging = false;
        }
        return false;
      }

      // Note: we should update mHandleTouchMove before invoke super.onTouchEvent(ev) because
      // mHandleTouchMove will be used to update scroll state in onScrollChanged call back which
      // invoked by super.onTouchEvent(ev).
      mHandleTouchMove = ev.getAction() == MotionEvent.ACTION_MOVE;
      boolean res = false;
      try {
        res = super.onTouchEvent(ev);
      } catch (IllegalStateException exception) {
        LLog.e(UIScrollView.TAG,
            "AndroidScrollView onTouchEvent: " + ev.getAction() + ", " + exception.getMessage());
      } finally {
        if (ev.getAction() == MotionEvent.ACTION_UP) {
          isUserDragging = false;
          mHandleTouchMove = false;
          mUIScrollView.scrollToBounce(true);
          // Note: stopNestedScroll() should be invoked after super#onTouchEvent() for handling
          // fling.
          this.stopNestedScroll(ViewCompat.TYPE_TOUCH);
        } else if (ev.getAction() == MotionEvent.ACTION_DOWN) {
          isUserDragging = true;
          mUIScrollView.recognizeGestureInternal(mScrollState);
        } else if (ev.getAction() == MotionEvent.ACTION_CANCEL) {
          mHandleTouchMove = false;
          this.stopNestedScroll(ViewCompat.TYPE_TOUCH);
        }
        return res;
      }
    } else {
      return false;
    }
  }

  private void transferToScroll() {
    triggerOnScrollStart();
    // Note: Flag isUserDragging is not a sufficient condition which indicates the user is dragging
    // the scroll-view. In the case: scroll-view nested with view-pager, whether the scroll
    // direction is vertical or not, view-pager always consume ACTION_DOWN which leads scroll-view
    // cannot receive ACTION_DOWN in OnTouchEvent() and have no change to modify flag isUserDragging
    // to true.
    notifyStateChange(
        (isUserDragging || mHandleTouchMove) ? SCROLL_STATE_DRAGGING : SCROLL_STATE_ANIMATION);
    initialPositionY = getScrollY();
    initialPositionX = mHorizontalScrollView.getScrollX();
    this.postDelayed(mScrollerEndDetectionTask, INTERNAL_FOR_SCROLL_END_CHECK);
  }

  @Override
  public void bindDrawChildHook(IDrawChildHook hook) {
    mDrawChildHook = hook;
  }

  @Override
  public void addView(View child) {
    if (isLinearLayoutExist) {
      mLinearLayout.addView(child);
    } else {
      super.addView(child);
      isLinearLayoutExist = true;
    }
  }

  @Override
  public void addView(View child, int index) {
    if (isLinearLayoutExist) {
      mLinearLayout.addView(child, index);
    } else {
      super.addView(child, index);
      isLinearLayoutExist = true;
    }
  }

  @Override
  public void addView(View child, int index, ViewGroup.LayoutParams params) {
    if (isLinearLayoutExist) {
      mLinearLayout.addView(child, index, params);
    } else {
      super.addView(child, index, params);
      isLinearLayoutExist = true;
    }
  }

  @Override
  public void addView(View child, ViewGroup.LayoutParams params) {
    if (isLinearLayoutExist) {
      mLinearLayout.addView(child, params);
    } else {
      super.addView(child, params);
      isLinearLayoutExist = true;
    }
  }

  @Override
  public void addView(View child, int width, int height) {
    if (isLinearLayoutExist) {
      mLinearLayout.addView(child, width, height);
    } else {
      super.addView(child, width, height);
      isLinearLayoutExist = true;
    }
  }

  @Override
  public void removeView(View view) {
    if (isLinearLayoutExist) {
      mLinearLayout.removeView(view);
    } else {
      super.removeView(view);
      isLinearLayoutExist = true;
    }
  }

  @Override
  public void removeViewAt(int index) {
    if (isLinearLayoutExist) {
      mLinearLayout.removeViewAt(index);
    } else {
      super.removeViewAt(index);
      isLinearLayoutExist = true;
    }
  }

  @Override
  public void removeAllViews() {
    if (isLinearLayoutExist) {
      mLinearLayout.removeAllViews();
    } else {
      super.removeAllViews();
      isLinearLayoutExist = true;
    }
  }

  /**
   * Computes the scroll and handles gestures if new gestures are enabled.
   */
  @Override
  public void computeScroll() {
    super.computeScroll();
    if (mUIScrollView != null) {
      if (mUIScrollView.mPendingScrollToIndex != INVALID_INDEX) {
        LLog.i(UIScrollView.TAG,
            "computeScroll: apply mPendingScrollToIndex when computing scroll "
                + mUIScrollView.mPendingScrollToIndex);
        setScrollToIndex(mUIScrollView.mPendingScrollToIndex);
        // Note: Don't forget to rest mPendingScrollToIndex to INVALID_INDEX because computeScroll()
        // will be invoked at drawing time.
        mUIScrollView.mPendingScrollToIndex = INVALID_INDEX;
      }
      mUIScrollView.handleComputeScroll();
      if (mUIScrollView.isEnableNewGesture()) {
        GestureArenaManager manager = mUIScrollView.getGestureArenaManager();
        if (manager != null) {
          manager.computeScroll();
        }
      }
    }
  }

  @Override
  protected int computeScrollDeltaToGetChildRectOnScreen(Rect rect) {
    if (!mBlockDescendantFocusability) {
      return super.computeScrollDeltaToGetChildRectOnScreen(rect);
    }
    return 0;
  }

  public void sendScrollToEdgeEvent(int l, int t) {
    int status = mUIScrollView.updateBorderStatus(l, t, 0, 0);
    if (mUIScrollView.mEnableScrollToLowerEdgeEvent && UIScrollView.isLower(status)) {
      mUIScrollView.sendCustomEvent(getScrollX(), getScrollY(), getScrollX(), getScrollY(),
          LynxScrollEvent.EVENT_SCROLL_TO_LOWER_EDGE);
    }
    if (mUIScrollView.mEnableScrollToUpperEdgeEvent && UIScrollView.isUpper(status)) {
      mUIScrollView.sendCustomEvent(getScrollX(), getScrollY(), getScrollX(), getScrollY(),
          LynxScrollEvent.EVENT_SCROLL_TO_UPPER_EDGE);
    }
    if (mUIScrollView.mEnableScrollToNormalStateEvent && !UIScrollView.isUpper(status)
        && !UIScrollView.isLower(status)) {
      mUIScrollView.sendCustomEvent(getScrollX(), getScrollY(), getScrollX(), getScrollY(),
          LynxScrollEvent.EVENT_SCROLL_TO_NORMAL_STATE);
    }
  }

  @Override
  protected void onScrollChanged(int l, int t, int oldl, int oldt) {
    super.onScrollChanged(l, t, oldl, oldt);
    if (t == mLastScrollY) {
      return;
    }
    mLastScrollY = this.getScrollY();
    // scroll by user or by front-end
    if (mScrollState == SCROLL_STATE_IDLE) {
      transferToScroll();
    }
    triggerOnScrollChanged(l, t, oldl, oldt);
    if (!isUserDragging && !mNeedAutoScroll) {
      mUIScrollView.scrollToBounce(true);
    }
    sendScrollToEdgeEvent(l, t);
  }

  @Override
  protected void dispatchDraw(final Canvas canvas) {
    Drawable drawable = getBackground();
    if (drawable instanceof BackgroundDrawable) {
      BackgroundDrawable backgroundDrawable = (BackgroundDrawable) drawable;
      RectF borderWidth = backgroundDrawable.getDirectionAwareBorderInsets();
      BorderRadius borderCornerRadii = backgroundDrawable.getBorderRadius();
      Rect bounds = mUiBound == null ? drawable.getBounds() : mUiBound;
      Path clip = new Path();
      RectF rect =
          new RectF(bounds.left + borderWidth.left, bounds.top + borderWidth.top + mLastScrollY,
              bounds.right - borderWidth.right, bounds.bottom - borderWidth.bottom + mLastScrollY);
      if (borderCornerRadii == null) {
        clip.addRect(rect, Path.Direction.CW);
      } else {
        float[] radiusArray = borderCornerRadii.getArray();
        radiusArray =
            BackgroundDrawable.RoundRectPath.newBorderRadius(radiusArray, borderWidth, 1.0f);
        clip.addRoundRect(rect, radiusArray, Path.Direction.CW);
      }
      int count = canvas.save();
      canvas.clipPath(clip);
      super.dispatchDraw(canvas);
      canvas.restoreToCount(count);
    } else {
      if (getParent() instanceof ViewGroup) {
        ViewGroup parent = (ViewGroup) getParent();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
          if (this.getClipBounds() == null) {
            mClipRect.set(getScrollX(), getScrollY(),
                getScrollX() + (mUiBound == null ? getWidth() : mUiBound.width()),
                getScrollY() + (mUiBound == null ? getHeight() : mUiBound.height()));
            canvas.clipRect(mClipRect);
          }
        } else {
          mClipRect.set(
              getScrollX(), getScrollY(), getScrollX() + getWidth(), getScrollY() + getHeight());
          canvas.clipRect(mClipRect);
        }
      }
      super.dispatchDraw(canvas);
    }
  }

  public int getContentWidth() {
    return mMeasuredWidth;
  }

  public int getContentHeight() {
    return mMeasuredHeight;
  }

  @Override
  public boolean dispatchTouchEvent(MotionEvent ev) {
    if (mUIScrollView.isEnableNewGesture()) {
      if (Boolean.FALSE.equals(mConsumeGesture)) {
        return true;
      }
      if (ev.getActionMasked() == MotionEvent.ACTION_MOVE) {
        if (mConsumeGesture != null && !mIsDownEventHandled) {
          ev.setAction(MotionEvent.ACTION_DOWN);
          mIsDownEventHandled = true;
        }
      }
    }
    return super.dispatchTouchEvent(ev);
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev) {
    if (!isHorizontal) {
      if (mUIScrollView.mPreferenceConsumeGesture) {
        requestDisallowInterceptTouchEvent(true);
      }
      if (mUIScrollView.isEnableNewGesture() && (mConsumeGesture == null || !mConsumeGesture)
          && ev.getActionMasked() != MotionEvent.ACTION_DOWN) {
        // If new gestures are enabled, return false to indicate that the event is not intercept, So
        // this event can be passed to child node, do not intercept the down event, otherwise will
        // not receive other types of events.
        return false;
      }
      return super.onInterceptTouchEvent(ev);
    } else {
      return false;
    }
  }

  @Override
  public boolean dispatchNestedScroll(
      int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed, int[] offsetInWindow) {
    boolean res = super.dispatchNestedScroll(
        dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow);
    if (res) {
      triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
    }
    return res;
  }

  @Override
  public boolean dispatchNestedScroll(int dxConsumed, int dyConsumed, int dxUnconsumed,
      int dyUnconsumed, int[] offsetInWindow, int type) {
    boolean res = super.dispatchNestedScroll(
        dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow, type);
    if (res) {
      triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
    }
    return res;
  }

  @Override
  public boolean dispatchNestedPreScroll(int dx, int dy, int[] consumed, int[] offsetInWindow) {
    boolean res = super.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow);
    if (res) {
      triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
    }
    return res;
  }

  @Override
  public boolean dispatchNestedPreScroll(
      int dx, int dy, int[] consumed, int[] offsetInWindow, int type) {
    boolean res = super.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, type);
    if (res) {
      triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
    }
    return res;
  }

  @Override
  public boolean dispatchNestedFling(float velocityX, float velocityY, boolean consumed) {
    boolean res = super.dispatchNestedFling(velocityX, velocityY, consumed);
    if (res) {
      triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
    }
    return res;
  }

  @Override
  public boolean dispatchNestedPreFling(float velocityX, float velocityY) {
    boolean res = super.dispatchNestedPreFling(velocityX, velocityY);
    if (res) {
      triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
    }
    return res;
  }

  @Override
  public void setClipBounds(Rect clipBounds) {
    // take care: do not call super.setClipBounds, in software drawing process or android 4.4,
    // the system did not update the clipBounds during the scroll, so the content out of bounds will
    // be clipped... just clip canvas in dispatchDraw
    mUiBound = clipBounds;
  }

  @Override
  public void setPadding(int left, int top, int right, int bottom) {
    mLinearLayout.setPadding(left, top, right, bottom);
  }

  public void setScrollBarEnable(boolean enable) {
    setVerticalScrollBarEnabled(enable);
  }

  public void setBlockDescendantFocusability(boolean value) {
    mBlockDescendantFocusability = value;
  }

  public void setScrollTo(int x, int y, boolean animate) {
    if (mLastScrollX == x && mLastScrollY == y) {
      return;
    }
    if (animate) {
      if (isHorizontal) {
        mHorizontalScrollView.setSmoothScrollingEnabled(true);
        mHorizontalScrollView.smoothScrollToInternal(x, y);
      } else {
        this.setSmoothScrollingEnabled(true);
        this.smoothScrollToInternal(x, y);
      }
    } else {
      if (isHorizontal) {
        abortAnimation(mHorizontalScrollView);
        mHorizontalScrollView.scrollTo(x, y);
      } else {
        abortAnimation(this);
        this.scrollTo(x, y);
      }
    }
  }

  void setScrollToIndex(int index) {
    if (mUIScrollView != null) {
      final int childrenCount = mUIScrollView.getChildCount();
      if (childrenCount == 0 || index < 0 || index >= childrenCount
          || mUIScrollView.getChildAt(index) == null) {
        return;
      }
      if (isHorizontal) {
        int originScrollY = mHorizontalScrollView.getScrollY();
        int offsetX = mHorizontalScrollView.getScrollX();
        LynxBaseUI child = mUIScrollView.getChildAt(index);
        if (ViewCompat.getLayoutDirection(this) == ViewCompat.LAYOUT_DIRECTION_RTL) {
          offsetX = child.getLeft() + child.getWidth() - mUIScrollView.getWidth();
        } else {
          offsetX = child.getLeft();
        }
        setScrollTo(offsetX, originScrollY, false);
      } else {
        int originScrollX = getScrollX();
        int offsetY = mUIScrollView.getChildAt(index).getTop();
        setScrollTo(originScrollX, offsetY, false);
      }
    }
  }

  /**
   * get mScroller from mScrollView, and abortAnimation before scrollTo
   * @param mScrollView
   */
  public void abortAnimation(View mScrollView) {
    try {
      Field f = null;
      if (mScrollView instanceof HorizontalScrollView) {
        f = HorizontalScrollView.class.getDeclaredField("mScroller");
      } else if (mScrollView instanceof ScrollView) {
        f = ScrollView.class.getDeclaredField("mScroller");
      }
      if (f == null) {
        LLog.w(
            "AndroidScrollView", "did not find mScroller in " + mScrollView.getClass().getName());
        return;
      }
      f.setAccessible(true);
      OverScroller mScroller = (OverScroller) f.get(mScrollView);
      if (!mScroller.isFinished()) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
          mScroller.abortAnimation();
        }
      }
    } catch (Throwable e) {
      LLog.w("AndroidScrollView", e.getMessage());
    }
  }

  public int getRealScrollX() {
    if (isHorizontal) {
      return mHorizontalScrollView.getScrollX();
    } else {
      return this.getScrollX();
    }
  }

  public boolean isHorizontal() {
    return isHorizontal;
  }

  public int getRealScrollY() {
    if (isHorizontal) {
      return mHorizontalScrollView.getScrollY();
    } else {
      return this.getScrollY();
    }
  }

  public void setEnableScroll(final boolean scroll) {
    View.OnTouchListener listener = new View.OnTouchListener() {
      @Override
      public boolean onTouch(View arg0, MotionEvent arg1) {
        return !scroll;
      }
    };
    mHorizontalScrollView.setOnTouchListener(listener);
    this.setOnTouchListener(listener);
  }

  public void setOrientation(int orientation) {
    if (orientation == HORIZONTAL) {
      mLinearLayout.setOrientation(LinearLayout.HORIZONTAL);
      isHorizontal = true;
    } else if (orientation == VERTICAL) {
      mLinearLayout.setOrientation(LinearLayout.VERTICAL);
      isHorizontal = false;
    }
  }

  public int getOrientation() {
    return mLinearLayout.getOrientation();
  }

  private void createLinearLayout() {
    mLinearLayout = new LinearLayout(this.getContext()) {
      @Override
      protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        setMeasuredDimension(mMeasuredWidth, mMeasuredHeight);
      }

      @Override
      protected void onLayout(boolean changed, int l, int t, int r, int b) {}

      @Override
      protected void dispatchDraw(final Canvas canvas) {
        if (mDrawChildHook != null) {
          mDrawChildHook.beforeDispatchDraw(canvas);
        }

        super.dispatchDraw(canvas);

        if (mDrawChildHook != null) {
          mDrawChildHook.afterDispatchDraw(canvas);
        }
      }

      @Override
      protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
        Rect bound = null;
        if (mDrawChildHook != null) {
          bound = mDrawChildHook.beforeDrawChild(canvas, child, drawingTime);
        }

        boolean ret;
        if (bound != null) {
          canvas.save();
          canvas.clipRect(bound);
          ret = super.drawChild(canvas, child, drawingTime);
          canvas.restore();
        } else {
          ret = super.drawChild(canvas, child, drawingTime);
        }

        if (mDrawChildHook != null) {
          mDrawChildHook.afterDrawChild(canvas, child, drawingTime);
        }

        return ret;
      }

      @Override
      public boolean canScrollHorizontally(int direction) {
        if (mUIScrollView.isEnableNewGesture()) {
          return super.canScrollHorizontally(direction);
        }
        return (mForceCanScroll && isHorizontal) || super.canScrollHorizontally(direction);
      }
    };
    mLinearLayout.setOrientation(LinearLayout.VERTICAL);
    mLinearLayout.setWillNotDraw(true);
    mLinearLayout.setFocusableInTouchMode(true);
  }

  @Override
  public boolean canScrollVertically(int direction) {
    if (mUIScrollView.isEnableNewGesture()) {
      return super.canScrollVertically(direction);
    }
    return (mForceCanScroll && !isHorizontal) || super.canScrollVertically(direction);
  }

  @Override
  public void fling(int velocityY) {
    super.fling(velocityY);
    if (mScrollState == SCROLL_STATE_DRAGGING) {
      notifyStateChange(SCROLL_STATE_FLING);
    }
    triggerOnFling(velocityY);
  }

  private void createInnerScrollView(UIScrollView uiScrollView) {
    mHorizontalScrollView = new CustomHorizontalScrollView(this.getContext(), uiScrollView);
    mHorizontalScrollView.setOverScrollMode(HorizontalScrollView.OVER_SCROLL_NEVER);
    mHorizontalScrollView.setFadingEdgeLength(0);
    mHorizontalScrollView.setWillNotDraw(true);
  }

  private void notifyStateChange(int state) {
    LLog.i(UIScrollView.TAG, "notifyStateChange " + mScrollState + " -> " + state);
    if (mScrollState != state) {
      mLastScrollState = mScrollState;
      mScrollState = state;
      triggerOnScrollStateChanged(state);
    }
  }

  public void setMeasuredSize(int measuredWidth, int measuredHeight) {
    mMeasuredHeight = measuredHeight;
    mMeasuredWidth = measuredWidth;
    // update native content size
    if (mLinearLayout != null) {
      mLinearLayout.requestLayout();
    }
  }

  public CustomHorizontalScrollView getHScrollView() {
    return mHorizontalScrollView;
  }

  public void forbidFocusChangeAfterFling() {
    mForbidFocusChangeAfterFling = true;
  }

  public LinearLayout getLinearLayout() {
    return mLinearLayout;
  }

  public void setForceCanScroll(boolean value) {
    mForceCanScroll = value;
  }

  protected class CustomHorizontalScrollView extends NestedHorizontalScrollView {
    public CustomHorizontalScrollView(Context context, UIScrollView uiScrollView) {
      super(context, uiScrollView);
    }

    @Override
    public void fling(int velocityX) {
      if (mScrollState == SCROLL_STATE_DRAGGING) {
        notifyStateChange(SCROLL_STATE_FLING);
      }
      triggerOnFling(velocityX);
      // By default HorizontalScrollView will request focus after fling,
      // which will cause focus loss even if ignore-focus is set.
      // Override fling implementation to eliminate this behavior.
      if (!mForbidFocusChangeAfterFling) {
        super.fling(velocityX);
      } else {
        OverScroller mScroller;
        try {
          Field f = HorizontalScrollView.class.getDeclaredField("mScroller");
          if (f == null) {
            throw new Exception("can not find mScroller field in HorizontalScrollView");
          }
          f.setAccessible(true);
          mScroller = (OverScroller) f.get(this);
          if (mScroller == null) {
            throw new Exception("failed to get mScroller in HorizontalScrollView");
          }
        } catch (Throwable e) {
          LLog.w("AndroidScrollView", e.getMessage());
          super.fling(velocityX);
          return;
        }

        if (getChildCount() > 0) {
          int width = getWidth() - getPaddingRight() - getPaddingLeft();
          int right = getChildAt(0).getWidth();

          mScroller.fling(getScrollX(), getScrollY(), velocityX, 0, 0, Math.max(0, right - width),
              0, 0, width / 2, 0);

          postInvalidateOnAnimation();
        }
      }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
      // save the scroll distance here,because when the content size changes,the
      // HorizontalScrollView's will call scroll() method, which will lead to the change of scrollX.
      int scrollX = getScrollX();
      super.onLayout(changed, l, t, r, b);
      if (!isHorizontal) {
        return;
      }
      int childWidth = getChildCount() > 0 ? getChildAt(0).getMeasuredWidth() : 0;
      int viewPortSize = r - l - getPaddingLeft() - getPaddingRight();
      int scrollRange = Math.max(0, childWidth - viewPortSize);
      if (mDirectionChanged) {
        // If layout direction changed, reset scrollX to scrollRange in RTL or 0 in LTR.
        setScrollX(ViewCompat.getLayoutDirection(this) == ViewCompat.LAYOUT_DIRECTION_RTL
                ? scrollRange
                : 0);
        mDirectionChanged = false;
        mLastScrollX = getScrollX();
      } else if (ViewCompat.getLayoutDirection(this) == ViewCompat.LAYOUT_DIRECTION_RTL) {
        // Note: In the condition that the scroll range has not changed, scrollX value being
        // modified in super#onLayout() when isLaidOut(this) == false is not as expected.
        if (!ViewCompat.isLaidOut(this) && mScrollRange == scrollRange) {
          setScrollX(mLastScrollX);
        } else if (mScrollRange != scrollRange && mScrollRange >= 0) {
          // Note: Here we should use getScrollX() instead of mLastScrollX, because scrollX value
          // has been modified in super#onLayout()
          scrollX += scrollRange - mScrollRange;
          // Note: Don't forget to clamp value
          scrollX = MathUtils.clamp(scrollX, 0, scrollRange);
          setScrollX(scrollX);
        }
        mLastScrollX = getScrollX();
      }
      mScrollRange = scrollRange;
      final int bounceScrollRange = mBounceGestureHelper.getBounceScrollRange();
      if (mEnableNewBounce && bounceScrollRange > 0 && bounceScrollRange != getScrollX()) {
        setScrollTo(bounceScrollRange, getScrollY(), false);
      }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
      if (mUIScrollView.isEnableNewGesture()) {
        if (Boolean.FALSE.equals(mConsumeGesture)) {
          return true;
        }
        if (ev.getActionMasked() == MotionEvent.ACTION_MOVE) {
          if (mConsumeGesture != null && !mIsDownEventHandled) {
            ev.setAction(MotionEvent.ACTION_DOWN);
            mIsDownEventHandled = true;
          }
        }
      }
      return super.dispatchTouchEvent(ev);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
      if (isHorizontal) {
        if (mUIScrollView.isEnableNewGesture() && (mConsumeGesture == null || !mConsumeGesture)
            && ev.getActionMasked() != MotionEvent.ACTION_DOWN) {
          // If new gestures are enabled, return false to indicate that the event is not intercept,
          // So this event can be passed to child node
          return false;
        }
        return super.onInterceptTouchEvent(ev);
      } else {
        return false;
      }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
      if (isHorizontal) {
        if (mUIScrollView.isEnableNewGesture() && (mConsumeGesture == null || !mConsumeGesture)
            && ev.getActionMasked() != MotionEvent.ACTION_DOWN) {
          // If new gestures are enabled, return false to indicate that the event is not consumed,
          // So this event can be passed to parent node, do not intercept the down event, otherwise
          // will not receive other types of events.
          if (ev.getAction() == MotionEvent.ACTION_UP) {
            isUserDragging = false;
          }
          return false;
        }
        mHandleTouchMove = ev.getAction() == MotionEvent.ACTION_MOVE;
        if (ev.getAction() == MotionEvent.ACTION_UP) {
          isUserDragging = false;
          mHandleTouchMove = false;
          mUIScrollView.scrollToBounce(true);
        } else if (ev.getAction() == MotionEvent.ACTION_DOWN) {
          isUserDragging = true;
          mUIScrollView.recognizeGestureInternal(mScrollState);
        } else if (ev.getAction() == MotionEvent.ACTION_CANCEL) {
          mHandleTouchMove = false;
        }
        boolean res = false;
        try {
          res = super.onTouchEvent(ev);
        } catch (IllegalStateException exception) {
          LLog.e(UIScrollView.TAG,
              "CustomHorizontalScrollView onTouchEvent: " + ev.getAction() + ", "
                  + exception.getMessage());
        } finally {
          return res;
        }
      } else {
        return false;
      }
    }

    @Override
    protected void onScrollChanged(int l, int t, int oldl, int oldt) {
      super.onScrollChanged(l, t, oldl, oldt);
      if (l == mLastScrollX) {
        return;
      }
      mLastScrollX = this.getScrollX();
      // scroll by user or by front-end
      if (mScrollState == SCROLL_STATE_IDLE) {
        transferToScroll();
      }
      triggerOnScrollChanged(l, t, oldl, oldt);
      if (!isUserDragging && !mNeedAutoScroll) {
        mUIScrollView.scrollToBounce(true);
      }
    }

    @Override
    protected int computeScrollDeltaToGetChildRectOnScreen(Rect rect) {
      if (!mBlockDescendantFocusability) {
        return super.computeScrollDeltaToGetChildRectOnScreen(rect);
      }
      return 0;
    }

    @Override
    public boolean dispatchNestedPreScroll(int dx, int dy, int[] consumed, int[] offsetInWindow) {
      boolean res = super.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow);
      if (res) {
        triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
      }
      return res;
    }

    @Override
    public boolean dispatchNestedPreScroll(
        int dx, int dy, int[] consumed, int[] offsetInWindow, int type) {
      boolean res = super.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, type);
      if (res) {
        triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
      }
      return res;
    }

    @Override
    public boolean dispatchNestedScroll(
        int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed, int[] offsetInWindow) {
      boolean res = super.dispatchNestedScroll(
          dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow);
      if (res) {
        triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
      }
      return res;
    }

    @Override
    public boolean dispatchNestedScroll(int dxConsumed, int dyConsumed, int dxUnconsumed,
        int dyUnconsumed, int[] offsetInWindow, int type) {
      boolean res = super.dispatchNestedScroll(
          dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow, type);
      if (res) {
        triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
      }
      return res;
    }

    @Override
    public boolean dispatchNestedPreFling(float velocityX, float velocityY) {
      boolean res = super.dispatchNestedPreFling(velocityX, velocityY);
      if (res) {
        triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
      }
      return res;
    }

    @Override
    public boolean dispatchNestedFling(float velocityX, float velocityY, boolean consumed) {
      boolean res = super.dispatchNestedFling(velocityX, velocityY, consumed);
      if (res) {
        triggerOnScrollStateChanged(SCROLL_NESTED_SCROLL);
      }
      return res;
    }
  }

  private void triggerOnScrollStop() {
    if (mOnScrollListenerList != null) {
      for (OnScrollListener listener : mOnScrollListenerList) {
        if (listener != null) {
          listener.onScrollStop();
        }
      }
    }
  }

  private void triggerOnScrollChanged(int l, int t, int oldl, int oldt) {
    if (mOnScrollListenerList != null) {
      for (OnScrollListener listener : mOnScrollListenerList) {
        if (listener != null) {
          listener.onScrollChanged(l, t, oldl, oldt);
        }
      }
    }
  }

  private void triggerOnScrollStart() {
    if (mOnScrollListenerList != null) {
      for (OnScrollListener listener : mOnScrollListenerList) {
        if (listener != null) {
          listener.onScrollStart();
        }
      }
    }
  }

  private void triggerOnScrollStateChanged(int state) {
    if (mOnScrollListenerList != null) {
      for (OnScrollListener listener : mOnScrollListenerList) {
        if (listener != null) {
          listener.onScrollStateChanged(state);
        }
      }
    }
  }

  private void triggerOnFling(int velocity) {
    if (mOnScrollListenerList != null) {
      for (OnScrollListener listener : mOnScrollListenerList) {
        if (listener != null) {
          listener.onFling(velocity);
        }
      }
    }
  }

  public void setOnScrollListener(OnScrollListener listener) {
    addOnScrollListener(listener);
  }

  public void addOnScrollListener(OnScrollListener listener) {
    if (mOnScrollListenerList == null) {
      mOnScrollListenerList = new ArrayList<>();
    }
    if (listener != null) {
      mOnScrollListenerList.add(listener);
    }
  }

  public void clearOnScrollListener() {
    if (mOnScrollListenerList != null) {
      mOnScrollListenerList.clear();
    }
  }

  public void removeOnScrollListener(OnScrollListener listener) {
    if (mOnScrollListenerList != null && listener != null) {
      mOnScrollListenerList.remove(listener);
    }
  }

  public interface OnScrollListener {
    void onScrollStop();

    void onScrollChanged(int l, int t, int oldl, int oldt);

    void onScrollStart();

    void onScrollStateChanged(int state);

    void onFling(int velocity);
  }
}
