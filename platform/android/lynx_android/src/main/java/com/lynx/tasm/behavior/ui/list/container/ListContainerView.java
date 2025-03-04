// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list.container;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import androidx.annotation.NonNull;
import com.lynx.tasm.ListNodeInfoFetcher;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.gesture.arena.GestureArenaManager;

public class ListContainerView
    extends NestedScrollContainerView implements IDrawChildHook.IDrawChildHookBinding {
  private static final String TAG = "ListContainerView";
  private static final boolean DEBUG = true;
  private UIListContainer mUiListContainer;
  private CustomLinearLayout mCustomLinearLayout;
  // Whether to consume gestures
  private Boolean mConsumeGesture = null;
  // Whether the down event has been processed, gesture starts from the down event, so if you want
  // to handle the gesture with one gesture, you need to convert one of the move events into a down
  // event
  private boolean mIsDownEventHandled = true;
  private IDrawChildHook mDrawChildHook;
  private boolean mIsVertical = true;
  private int mMeasuredWidth = 0;
  private int mMeasuredHeight = 0;
  private boolean mShouldBlockScrollByListContainer = false;
  private int mPreviousOffsetX;
  private int mPreviousOffsetY;
  private boolean mForceCanScroll = false;

  public ListContainerView(@NonNull Context context, UIListContainer uiListContainer) {
    super(context);
    mUiListContainer = uiListContainer;
    createCustomLinearLayoutIfNeeded();
    addView(mCustomLinearLayout,
        new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
  }

  private void createCustomLinearLayoutIfNeeded() {
    if (mCustomLinearLayout == null) {
      mCustomLinearLayout = new CustomLinearLayout(this.getContext());
    }
    mCustomLinearLayout.setOrientation(LinearLayout.VERTICAL);
    mCustomLinearLayout.setWillNotDraw(true);
    mCustomLinearLayout.setFocusableInTouchMode(true);
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent e) {
    if (mUiListContainer == null) {
      return super.onInterceptTouchEvent(e);
    }
    if (mUiListContainer.isEnableNewGesture() && (mConsumeGesture == null || !mConsumeGesture)
        && e.getActionMasked() != MotionEvent.ACTION_DOWN) {
      // If new gestures are enabled, return false to indicate that the event is not intercept, So
      // this event can be passed to child node, do not intercept the down event, otherwise will not
      // receive other types of events.
      return false;
    }
    return super.onInterceptTouchEvent(e);
  }

  @Override
  public boolean dispatchTouchEvent(MotionEvent ev) {
    if (mUiListContainer == null) {
      return super.dispatchTouchEvent(ev);
    }
    if (mUiListContainer.isEnableNewGesture()) {
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
  public boolean onTouchEvent(MotionEvent ev) {
    if (mUiListContainer == null) {
      return super.onTouchEvent(ev);
    }
    if (mUiListContainer.isEnableNewGesture() && (mConsumeGesture == null || !mConsumeGesture)
        && ev.getActionMasked() != MotionEvent.ACTION_DOWN) {
      // If new gestures are enabled, return false to indicate that the event is not consumed,
      // So this event can be passed to parent node, do not intercept the down event, otherwise will
      // not receive other types of events.
      return false;
    }

    return super.onTouchEvent(ev);
  }

  public void consumeGesture(boolean consume) {
    mConsumeGesture = consume;
    if (consume) {
      mIsDownEventHandled = false;
    }
  }

  @Override
  public void computeScroll() {
    super.computeScroll();
    if (mUiListContainer == null || !mUiListContainer.isEnableNewGesture()) {
      return;
    }
    GestureArenaManager manager = mUiListContainer.getGestureArenaManager();
    if (manager != null) {
      manager.computeScroll();
    }
  }

  protected void setMeasuredSize(int measuredWidth, int measuredHeight) {
    if (mMeasuredWidth != measuredWidth || mMeasuredHeight != measuredHeight) {
      mMeasuredHeight = measuredHeight;
      mMeasuredWidth = measuredWidth;
      if (mCustomLinearLayout != null) {
        mCustomLinearLayout.requestLayout();
      }
    }
  }

  public void setForceCanScroll(boolean forceCanScroll) {
    mForceCanScroll = forceCanScroll;
  }

  void updateContentSizeAndOffset(int contentSize, int deltaX, int deltaY) {
    if (mIsVertical && contentSize != mMeasuredHeight) {
      setMeasuredSize(mMeasuredWidth, contentSize);
    } else if (!mIsVertical && contentSize != mMeasuredWidth) {
      setMeasuredSize(contentSize, mMeasuredHeight);
    }
    mShouldBlockScrollByListContainer = true;
    if (mIsVertical) {
      mPreviousOffsetY += deltaY;
      setScrollY(mPreviousOffsetY);
    } else {
      mPreviousOffsetX += deltaX;
      setScrollX(mUiListContainer.isRtl() ? contentOffsetXRTL(mPreviousOffsetX) : mPreviousOffsetX);
    }
    mShouldBlockScrollByListContainer = false;
  }

  private int contentOffsetXRTL(float originLeft) {
    return (int) Math.max(mMeasuredWidth - originLeft - mUiListContainer.getWidth(), 0);
  }

  @Override
  public boolean canScrollVertically(int direction) {
    return (mForceCanScroll && mIsVertical) || super.canScrollVertically(direction);
  }

  @Override
  public boolean canScrollHorizontally(int direction) {
    return (mForceCanScroll && !mIsVertical) || super.canScrollHorizontally(direction);
  }

  @Override
  protected void onScrollChanged(int l, int t, int oldl, int oldt) {
    super.onScrollChanged(l, t, oldl, oldt);
    if (DEBUG) {
      LLog.i(TAG, "onScrollChanged: " + oldt + " -> " + t + ", " + oldl + " -> " + l);
    }
    if (!mShouldBlockScrollByListContainer && mUiListContainer != null
        && mUiListContainer.getLynxContext() != null) {
      ListNodeInfoFetcher listNodeInfoFetcher =
          mUiListContainer.getLynxContext().getListNodeInfoFetcher();
      if (listNodeInfoFetcher == null) {
        LLog.e(TAG, "onScrollChanged: listNodeInfoFetcher is nullptr");
        return;
      }
      mPreviousOffsetY = t;
      mPreviousOffsetX = mUiListContainer.isRtl() ? contentOffsetXRTL(l) : l;
      listNodeInfoFetcher.scrollByListContainer(
          mUiListContainer.getSign(), mPreviousOffsetX, t, l, t);
      mUiListContainer.updateStickyTops(getScrollY());
      mUiListContainer.updateStickyBottoms(getScrollY());
    }
  }

  @Override
  public void bindDrawChildHook(IDrawChildHook hook) {
    mDrawChildHook = hook;
  }

  public void setOrientation(int orientation) {
    mIsVertical = orientation == LinearLayout.VERTICAL;
    setIsVertical(mIsVertical);
    if (mCustomLinearLayout != null) {
      mCustomLinearLayout.setOrientation(
          orientation == LinearLayout.VERTICAL ? LinearLayout.VERTICAL : LinearLayout.HORIZONTAL);
    }
  }

  void destroy() {
    String sectionName = "ListContainerView.destroy";
    TraceEvent.beginSection(sectionName);
    mDrawChildHook = null;
    mUiListContainer = null;
    mCustomLinearLayout = null;
    setOnScrollStateChangeListener(null);
    TraceEvent.endSection(sectionName);
  }

  LinearLayout getLinearLayout() {
    return mCustomLinearLayout;
  }

  @Override
  protected boolean isRtl() {
    if (mUiListContainer == null) {
      return false;
    }
    return mUiListContainer.isRtl();
  }

  @Override
  public void addView(View child) {
    if (mCustomLinearLayout != null) {
      if (mCustomLinearLayout == child) {
        super.addView(mCustomLinearLayout);
      } else {
        mCustomLinearLayout.addView(child);
      }
    }
  }

  @Override
  public void addView(View child, int index) {
    if (mCustomLinearLayout != null) {
      if (mCustomLinearLayout == child) {
        super.addView(mCustomLinearLayout, index);
      } else {
        mCustomLinearLayout.addView(child, index);
      }
    }
  }

  @Override
  public void addView(View child, ViewGroup.LayoutParams params) {
    if (mCustomLinearLayout != null) {
      if (mCustomLinearLayout == child) {
        super.addView(mCustomLinearLayout, params);
      } else {
        mCustomLinearLayout.addView(child, params);
      }
    }
  }

  @Override
  public void addView(View child, int width, int height) {
    if (mCustomLinearLayout != null) {
      if (mCustomLinearLayout == child) {
        super.addView(mCustomLinearLayout, width, height);
      } else {
        mCustomLinearLayout.addView(child, width, height);
      }
    }
  }

  @Override
  public void addView(View child, int index, ViewGroup.LayoutParams params) {
    if (mCustomLinearLayout != null) {
      if (mCustomLinearLayout == child) {
        super.addView(mCustomLinearLayout, index, params);
      } else {
        mCustomLinearLayout.addView(child, index, params);
      }
    }
  }

  @Override
  public void removeView(View view) {
    if (mCustomLinearLayout != null) {
      if (mCustomLinearLayout == view) {
        super.removeView(mCustomLinearLayout);
      } else {
        mCustomLinearLayout.removeView(view);
      }
    }
  }

  @Override
  public void removeViewAt(int index) {
    if (mCustomLinearLayout != null) {
      mCustomLinearLayout.removeViewAt(index);
    }
  }

  @Override
  public void removeAllViews() {
    if (mCustomLinearLayout != null) {
      mCustomLinearLayout.removeAllViews();
    }
  }

  private class CustomLinearLayout extends LinearLayout {
    public CustomLinearLayout(Context context) {
      super(context);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
      setMeasuredDimension(mMeasuredWidth > 0 ? mMeasuredWidth : mUiListContainer.getWidth(),
          mMeasuredHeight > 0 ? mMeasuredHeight : mUiListContainer.getHeight());
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {}

    @Override
    protected void dispatchDraw(Canvas canvas) {
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
  }
}
