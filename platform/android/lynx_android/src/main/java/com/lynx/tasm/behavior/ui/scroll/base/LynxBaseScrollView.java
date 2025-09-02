// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ui.IDrawChildHook;

public class LynxBaseScrollView
    extends LynxBaseScrollViewNested implements IDrawChildHook.IDrawChildHookBinding {
  private IDrawChildHook mDrawChildHook;

  private boolean mLayoutFromEnd = false;

  @Override
  public void bindDrawChildHook(IDrawChildHook hook) {
    mDrawChildHook = hook;
  }

  /********* Public begin *********/

  public LynxBaseScrollView(@NonNull Context context) {
    this(context, null);
  }

  public LynxBaseScrollView(@NonNull Context context, @Nullable AttributeSet attrs) {
    this(context, attrs, 0);
  }

  public LynxBaseScrollView(
      @NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
    setNestedScrollingEnabled(true);
    setOnTouchListener(new OnTouchListener() {
      @Override
      public boolean onTouch(View v, MotionEvent event) {
        return !scrollEnabled();
      }
    });
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

  public void setForwardNestedScrollMode(int mode) {
    mForwardNestedScrollMode = mode;
  }

  public void setBackwardNestedScrollMode(int mode) {
    mBackwardNestedScrollMode = mode;
  }

  public void setVertical(boolean vertical) {
    mIsVertical = vertical;
  }

  public void setLayoutFromEnd(boolean layoutFromEnd) {
    mLayoutFromEnd = layoutFromEnd;
  }

  public boolean isLayoutFromEnd() {
    return mLayoutFromEnd;
  }

  public void autoScrollWithRate(
      int rate, boolean autoStop, LynxBaseScrollViewScroller.ScrollFinishedCallback callback) {
    tryToUpdateScrollState(SCROLL_STATE_ANIMATING);
    updateProgrammaticallyScrollFinishedCallback(callback);
    mScrollHelper.autoScrollTo(rate, autoStop, callback);
  }

  public void setHeader(View view) {}

  public void setFooter(View view) {}

  /********* Internal begin *********/

  @Override
  protected void onScrollChanged(int l, int t, int oldl, int oldt) {
    super.onScrollChanged(l, t, oldl, oldt);
    mScrollDelegate.scrollViewDidScroll(this);
  }

  /********* Internal end *********/

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    if (!getRootView().isLayoutRequested() && mDrawChildHook != null) {
      // it means this onLayout is not from rootview's performTraversals, it may be triggered from
      // ReccyleView we should performLayoutChildrenUI just like LynxView!
      mDrawChildHook.performLayoutChildrenUI();
    }
  }

  /********* Rendering begin *********/

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    setMeasuredDimension(
        MeasureSpec.getSize(widthMeasureSpec), MeasureSpec.getSize(heightMeasureSpec));
    if (!getRootView().isLayoutRequested() && mDrawChildHook != null) {
      // it means this onMeasure is not from rootview's performTraversals, it may be triggered from
      // Swiper's ViewPager we should performMeasureChildrenUI just like LynxView!
      mDrawChildHook.performMeasureChildrenUI();
    }
  }

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
