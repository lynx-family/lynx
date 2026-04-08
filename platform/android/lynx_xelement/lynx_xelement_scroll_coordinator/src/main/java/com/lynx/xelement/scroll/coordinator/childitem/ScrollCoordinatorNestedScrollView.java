// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator.childitem;

import android.content.Context;
import android.graphics.Canvas;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import androidx.core.widget.NestedScrollView;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import java.util.ArrayList;

public class ScrollCoordinatorNestedScrollView
    extends NestedScrollView implements IDrawChildHook.IDrawChildHookBinding {
  public static final int HORIZONTAL = 0;
  public static final int VERTICAL = 1;
  public static final int SCROLL_STATE_IDLE = 0;
  public static final int SCROLL_STATE_DRAGGING = 1;
  public static final int SCROLL_STATE_SETTLING = 2;

  private LinearLayout linearLayout;
  private boolean linearLayoutExists = false;
  private HorizontalScrollView horizontalScrollView;
  private int lastScrollY;
  private int lastScrollX;
  boolean horizontal = false;
  private int measuredWidth = 0;
  private int measuredHeight = 0;
  private boolean motionDown = false;
  private boolean scrollStarted = false;
  private ArrayList<OnScrollListener> scrollListeners;
  private IDrawChildHook drawChildHook;
  private Runnable scrollerTask;
  private int initialPositionY = 0;
  private int initialPositionX = 0;
  private int newCheck = 300;
  private int state = SCROLL_STATE_IDLE;

  private static class ScrollRunnable implements Runnable {
    private final java.lang.ref.WeakReference<ScrollCoordinatorNestedScrollView> weakScrollView;

    public ScrollRunnable(ScrollCoordinatorNestedScrollView view) {
      weakScrollView = new java.lang.ref.WeakReference<>(view);
    }

    public void run() {
      if (weakScrollView.get() != null) {
        ScrollCoordinatorNestedScrollView scrollView = weakScrollView.get();
        int newPositionY = scrollView.getScrollY();
        int newPositionX = scrollView.horizontalScrollView.getScrollX();
        if ((scrollView.horizontal && scrollView.initialPositionX - newPositionX == 0)
            || (!scrollView.horizontal && scrollView.initialPositionY - newPositionY == 0)) {
          scrollView.notifyScrollStop();
        } else {
          scrollView.initialPositionY = newPositionY;
          scrollView.initialPositionX = newPositionX;
          scrollView.postDelayed(this, scrollView.newCheck);
        }
      }
    }
  }

  public ScrollCoordinatorNestedScrollView(Context context) {
    super(context);
    init();
    createInternalLinearLayout();
    initScrollerTask();
  }

  @Override
  public boolean onTouchEvent(MotionEvent ev) {
    if (!horizontal) {
      analyseMotion(ev);
      if (ev.getAction() == MotionEvent.ACTION_DOWN) {
        notifyScrollStateChanged(state);
      }
      if (ev.getAction() == MotionEvent.ACTION_UP) {
        startScrollerTask();
      }
      return super.onTouchEvent(ev);
    } else {
      return false;
    }
  }

  private void initScrollerTask() {
    scrollerTask = new ScrollRunnable(this);
  }

  public void startScrollerTask() {
    initialPositionY = getScrollY();
    initialPositionX = horizontalScrollView.getScrollX();
    postDelayed(scrollerTask, newCheck);
  }

  protected void init() {
    setVerticalScrollBarEnabled(false);
    setOverScrollMode(HorizontalScrollView.OVER_SCROLL_NEVER);
    setFadingEdgeLength(0);
    setScrollContainer(false);
    scrollListeners = new ArrayList<>();
  }

  @Override
  public void bindDrawChildHook(IDrawChildHook hook) {
    drawChildHook = hook;
  }

  @Override
  public void addView(View child) {
    if (linearLayoutExists) {
      linearLayout.addView(child);
    } else {
      super.addView(child);
      linearLayoutExists = true;
    }
  }

  @Override
  public void addView(View child, int index) {
    if (linearLayoutExists) {
      linearLayout.addView(child, index);
    } else {
      super.addView(child, index);
      linearLayoutExists = true;
    }
  }

  @Override
  public void addView(View child, int index, ViewGroup.LayoutParams params) {
    if (linearLayoutExists) {
      linearLayout.addView(child, index, params);
    } else {
      super.addView(child, index, params);
      linearLayoutExists = true;
    }
  }

  @Override
  public void addView(View child, ViewGroup.LayoutParams params) {
    if (linearLayoutExists) {
      linearLayout.addView(child, params);
    } else {
      super.addView(child, params);
      linearLayoutExists = true;
    }
  }

  @Override
  public void addView(View child, int width, int height) {
    if (linearLayoutExists) {
      linearLayout.addView(child, width, height);
    } else {
      super.addView(child, width, height);
      linearLayoutExists = true;
    }
  }

  @Override
  public void removeView(View view) {
    if (linearLayoutExists) {
      linearLayout.removeView(view);
    } else {
      super.removeView(view);
      linearLayoutExists = true;
    }
  }

  @Override
  public void removeViewAt(int index) {
    if (linearLayoutExists) {
      linearLayout.removeViewAt(index);
    } else {
      super.removeViewAt(index);
      linearLayoutExists = true;
    }
  }

  @Override
  public void removeAllViews() {
    if (linearLayoutExists) {
      linearLayout.removeAllViews();
    } else {
      super.removeAllViews();
      linearLayoutExists = true;
    }
  }

  @Override
  protected void onScrollChanged(int l, int t, int oldl, int oldt) {
    super.onScrollChanged(l, t, oldl, oldt);
    if (t == lastScrollY) {
      return;
    }
    if (motionDown && !scrollStarted) {
      scrollStarted = true;
      notifyScrollStart();
    } else {
      notifyScrollChanged(l, t, oldl, oldt);
    }
    if (lastScrollY != getScrollY()) {
      lastScrollY = getScrollY();
    }
  }

  public int getContentWidth() {
    return measuredWidth;
  }

  public int getContentHeight() {
    return measuredHeight;
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev) {
    if (!horizontal) {
      return super.onInterceptTouchEvent(ev);
    } else {
      return false;
    }
  }

  @Override
  public void setPadding(int left, int top, int right, int bottom) {
    linearLayout.setPadding(left, top, right, bottom);
  }

  public void setOnScrollListener(OnScrollListener listener) {
    scrollListeners.add(listener);
  }

  public void setScrollBarEnable(boolean enable) {
    setVerticalScrollBarEnabled(enable);
    horizontalScrollView.setHorizontalScrollBarEnabled(enable);
  }

  private void analyseMotion(MotionEvent event) {
    if (event.getAction() == MotionEvent.ACTION_DOWN) {
      motionDown = true;
    } else if (event.getAction() == MotionEvent.ACTION_UP) {
      if (scrollStarted) {
        notifyScrollCancel();
      }
      scrollStarted = false;
      motionDown = false;
    }
  }

  public void setScrollTo(int x, int y, boolean animate) {
    if (animate) {
      if (horizontal) {
        horizontalScrollView.smoothScrollTo(x, y);
      } else {
        smoothScrollTo(x, y);
      }
    } else {
      if (horizontal) {
        horizontalScrollView.scrollTo(x, y);
      } else {
        scrollTo(x, y);
      }
    }
  }

  public void setOrientation(int orientation) {
    if (orientation == HORIZONTAL) {
      linearLayout.setOrientation(LinearLayout.HORIZONTAL);
      horizontal = true;
    } else if (orientation == VERTICAL) {
      linearLayout.setOrientation(LinearLayout.VERTICAL);
      horizontal = false;
    }
  }

  public int getOrientation() {
    return linearLayout.getOrientation();
  }

  private void createInnerComponent() {
    linearLayout = new LinearLayout(getContext()) {
      @Override
      protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        setMeasuredDimension(measuredWidth, measuredHeight);
      }

      @Override
      protected void onLayout(boolean changed, int l, int t, int r, int b) {}

      @Override
      protected void dispatchDraw(final Canvas canvas) {
        if (drawChildHook != null) {
          drawChildHook.beforeDispatchDraw(canvas);
        }
        super.dispatchDraw(canvas);
        if (drawChildHook != null) {
          drawChildHook.afterDispatchDraw(canvas);
        }
      }

      @Override
      protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
        if (drawChildHook != null) {
          drawChildHook.beforeDrawChild(canvas, child, drawingTime);
        }
        boolean result = super.drawChild(canvas, child, drawingTime);
        if (drawChildHook != null) {
          drawChildHook.afterDrawChild(canvas, child, drawingTime);
        }
        return result;
      }
    };
    linearLayout.setOrientation(LinearLayout.VERTICAL);
    linearLayout.setWillNotDraw(true);
  }

  private void createInnerScrollView() {
    horizontalScrollView = new HorizontalScrollView(getContext()) {
      @Override
      public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (horizontal) {
          return super.onInterceptTouchEvent(ev);
        } else {
          return false;
        }
      }

      @Override
      public boolean onTouchEvent(MotionEvent ev) {
        if (horizontal) {
          analyseMotion(ev);
          if (ev.getAction() == MotionEvent.ACTION_DOWN) {
            notifyScrollStateChanged(state);
          }
          if (ev.getAction() == MotionEvent.ACTION_UP) {
            startScrollerTask();
          }
          return super.onTouchEvent(ev);
        } else {
          return false;
        }
      }

      @Override
      protected void onScrollChanged(int l, int t, int oldl, int oldt) {
        super.onScrollChanged(l, t, oldl, oldt);
        if (l == lastScrollX) {
          return;
        }
        if (motionDown && !scrollStarted) {
          scrollStarted = true;
          notifyScrollStart();
        } else {
          notifyScrollChanged(l, t, oldl, oldt);
        }
        if (lastScrollX != getScrollX()) {
          lastScrollX = getScrollX();
        }
      }
    };
    horizontalScrollView.setHorizontalScrollBarEnabled(false);
    horizontalScrollView.setOverScrollMode(HorizontalScrollView.OVER_SCROLL_NEVER);
    horizontalScrollView.setFadingEdgeLength(0);
    horizontalScrollView.setWillNotDraw(true);
  }

  private void createInternalLinearLayout() {
    if (linearLayout == null) {
      createInnerComponent();
      createInnerScrollView();
      horizontalScrollView.addView(
          linearLayout, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
      addView(horizontalScrollView,
          new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
    }
  }

  public void setMeasuredSize(int width, int height) {
    measuredHeight = height;
    measuredWidth = width;
    if (linearLayout != null) {
      linearLayout.requestLayout();
    }
  }

  public HorizontalScrollView getHScrollView() {
    return horizontalScrollView;
  }

  public LinearLayout getLinearLayout() {
    return linearLayout;
  }

  private void notifyScrollStart() {
    notifyScrollStateChanged(SCROLL_STATE_DRAGGING);
    for (OnScrollListener listener : scrollListeners) {
      listener.onScrollStart();
    }
  }

  private void notifyScrollChanged(int l, int t, int oldl, int oldt) {
    notifyScrollStateChanged(state);
    for (OnScrollListener listener : scrollListeners) {
      listener.onScrollChanged(l, t, oldl, oldt);
    }
  }

  private void notifyScrollStop() {
    notifyScrollStateChanged(SCROLL_STATE_IDLE);
    for (OnScrollListener listener : scrollListeners) {
      listener.onScrollStop();
    }
  }

  private void notifyScrollCancel() {
    notifyScrollStateChanged(SCROLL_STATE_SETTLING);
    for (OnScrollListener listener : scrollListeners) {
      listener.onScrollCancel();
    }
  }

  private void notifyScrollStateChanged(int scrollState) {
    state = scrollState;
    for (OnScrollListener listener : scrollListeners) {
      listener.onScrollStateChanged(scrollState);
    }
  }

  @Override
  protected void finalize() throws Throwable {
    removeCallbacks(scrollerTask);
    super.finalize();
  }

  public interface OnScrollListener {
    void onScrollStop();

    void onScrollChanged(int l, int t, int oldl, int oldt);

    void onScrollStart();

    void onScrollCancel();

    void onScrollStateChanged(int state);
  }
}
