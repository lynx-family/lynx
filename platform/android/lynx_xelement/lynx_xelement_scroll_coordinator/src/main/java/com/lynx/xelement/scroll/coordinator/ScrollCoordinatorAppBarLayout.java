// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.OverScroller;
import androidx.annotation.NonNull;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import com.google.android.material.appbar.AppBarLayout;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

@CoordinatorLayout.DefaultBehavior(ScrollCoordinatorAppBarLayout.Behavior.class)
public class ScrollCoordinatorAppBarLayout
    extends BaseScrollCoordinatorAppBarLayout implements CoordinatorLayout.AttachedBehavior {
  private AppBarLayoutScrollListener scrollListener;
  private boolean enableTouchStopFling = true;
  private Behavior behavior;

  public ScrollCoordinatorAppBarLayout(Context context) {
    super(context);
  }

  public ScrollCoordinatorAppBarLayout(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  @NonNull
  @Override
  public CoordinatorLayout.Behavior getBehavior() {
    if (behavior == null) {
      behavior = new Behavior();
    }
    return behavior;
  }

  public interface AppBarLayoutScrollListener {
    void onScrollStart();

    void onScrollStop();

    void onFling();
  }

  public int getTopAndBottomOffset() {
    if (behavior != null) {
      return behavior.getTopAndBottomOffset();
    } else {
      return 0;
    }
  }

  public void setScrollListener(AppBarLayoutScrollListener listener) {
    scrollListener = listener;
  }

  public boolean isTouchStopFlingEnabled() {
    return enableTouchStopFling;
  }

  public void setTouchStopFlingEnabled(boolean enabled) {
    enableTouchStopFling = enabled;
  }

  class Behavior extends AppBarLayout.Behavior {
    private static final int TYPE_FLING = 1;
    private boolean isFlinging;
    private boolean shouldBlockNestedScroll;
    private Field cachedFlingRunnableField;
    private Method scrollMethod;
    private Method getDownNestedScrollRange;
    private Method updateAccessibilityActions;

    public Behavior() {
      super();
      setDefaultDragCallback();
    }

    public Behavior(Context context, AttributeSet attrs) {
      super(context, attrs);
      setDefaultDragCallback();
    }

    public boolean getScrollEnabled(AppBarLayout appBarLayout) {
      if (appBarLayout instanceof BaseScrollCoordinatorAppBarLayout) {
        return ((BaseScrollCoordinatorAppBarLayout) appBarLayout).isScrollEnabled();
      }
      return true;
    }

    private boolean getTouchStopFlingEnabled(AppBarLayout appBarLayout) {
      if (appBarLayout instanceof ScrollCoordinatorAppBarLayout) {
        return ((ScrollCoordinatorAppBarLayout) appBarLayout).isTouchStopFlingEnabled();
      }
      return true;
    }

    public void setDefaultDragCallback() {
      setDragCallback(new BaseDragCallback() {
        @Override
        public boolean canDrag(@NonNull AppBarLayout appBarLayout) {
          return true;
        }
      });
    }

    @Override
    public boolean onInterceptTouchEvent(
        CoordinatorLayout parent, AppBarLayout child, MotionEvent event) {
      shouldBlockNestedScroll = isFlinging;
      if (event.getAction() == MotionEvent.ACTION_DOWN && getTouchStopFlingEnabled(child)) {
        stopAppBarLayoutFling(child);
      }
      return super.onInterceptTouchEvent(parent, child, event);
    }

    @Override
    public boolean onStartNestedScroll(CoordinatorLayout parent, AppBarLayout child,
        View directTargetChild, View target, int nestedScrollAxes, int type) {
      if (getTouchStopFlingEnabled(child)) {
        stopAppBarLayoutFling(child);
      }
      if (scrollListener != null) {
        scrollListener.onScrollStart();
      }
      return super.onStartNestedScroll(
          parent, child, directTargetChild, target, nestedScrollAxes, type);
    }

    @Override
    public void onNestedPreScroll(CoordinatorLayout coordinatorLayout, AppBarLayout child,
        View target, int dx, int dy, int[] consumed, int type) {
      if (type == TYPE_FLING) {
        isFlinging = true;
      }
      if (!shouldBlockNestedScroll && getScrollEnabled(child)) {
        super.onNestedPreScroll(coordinatorLayout, child, target, dx, dy, consumed, type);
      }
    }

    private Field getScrollerField() throws NoSuchFieldException {
      Class<?> superclass = getClass().getSuperclass();
      if (superclass != null) {
        superclass = superclass.getSuperclass();
      }
      try {
        Class<?> headerBehaviorType = null;
        if (headerBehaviorType != null) {
          return headerBehaviorType.getDeclaredField("mScroller");
        } else {
          return null;
        }
      } catch (NoSuchFieldException e) {
        e.printStackTrace();
        Class<?> headerBehaviorType = superclass.getSuperclass().getSuperclass();
        if (headerBehaviorType != null) {
          return headerBehaviorType.getDeclaredField("scroller");
        } else {
          return null;
        }
      }
    }

    private Field getFlingRunnableField() throws NoSuchFieldException {
      if (cachedFlingRunnableField != null) {
        return cachedFlingRunnableField;
      }

      Class<?> superclass = getClass().getSuperclass();
      try {
        Class<?> headerBehaviorType = null;
        if (superclass != null) {
          headerBehaviorType = superclass.getSuperclass();
        }
        if (headerBehaviorType != null) {
          cachedFlingRunnableField = headerBehaviorType.getDeclaredField("mFlingRunnable");
          return cachedFlingRunnableField;
        } else {
          return null;
        }
      } catch (NoSuchFieldException e) {
        e.printStackTrace();
        Class<?> headerBehaviorType = superclass.getSuperclass().getSuperclass();
        if (headerBehaviorType != null) {
          cachedFlingRunnableField = headerBehaviorType.getDeclaredField("flingRunnable");
          return cachedFlingRunnableField;
        } else {
          return null;
        }
      }
    }

    protected void stopAppBarLayoutFling(AppBarLayout appBarLayout) {
      try {
        Field flingRunnableField = getFlingRunnableField();
        Runnable flingRunnable;
        if (flingRunnableField != null) {
          flingRunnableField.setAccessible(true);
          flingRunnable = (Runnable) flingRunnableField.get(this);
          if (flingRunnable != null) {
            appBarLayout.removeCallbacks(flingRunnable);
            flingRunnableField.set(this, null);
          }
        }

        Field scrollerField = getScrollerField();
        if (scrollerField != null) {
          scrollerField.setAccessible(true);
          OverScroller overScroller = (OverScroller) scrollerField.get(this);
          if (overScroller != null && !overScroller.isFinished()) {
            overScroller.abortAnimation();
          }
        }
      } catch (NoSuchFieldException e) {
        e.printStackTrace();
      } catch (IllegalAccessException e) {
        e.printStackTrace();
      }
    }

    @Override
    public boolean onNestedFling(@NonNull CoordinatorLayout coordinatorLayout,
        @NonNull AppBarLayout child, @NonNull View target, float velocityX, float velocityY,
        boolean consumed) {
      if (scrollListener != null) {
        scrollListener.onFling();
      }
      return super.onNestedFling(coordinatorLayout, child, target, velocityX, velocityY, consumed);
    }

    public void onNestedScroll(CoordinatorLayout coordinatorLayout, AppBarLayout child, View target,
        int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed, int type) {
      if (!shouldBlockNestedScroll) {
        try {
          if (getScrollEnabled(child)) {
            super.onNestedScroll(coordinatorLayout, child, target, dxConsumed, dyConsumed,
                dxUnconsumed, dyUnconsumed, type);
          }
        } catch (Throwable t) {
          t.printStackTrace();
        }
      }
    }

    public void onNestedScroll(CoordinatorLayout coordinatorLayout, AppBarLayout child, View target,
        int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed, int type,
        int[] consumed) {
      if (!shouldBlockNestedScroll) {
        try {
          if (dyUnconsumed < 0) {
            if (scrollMethod == null) {
              Class headerBehaviorClass =
                  getClass().getSuperclass().getSuperclass().getSuperclass();
              scrollMethod = headerBehaviorClass.getDeclaredMethod(
                  "scroll", CoordinatorLayout.class, View.class, int.class, int.class, int.class);
              scrollMethod.setAccessible(true);
            }
            if (getDownNestedScrollRange == null) {
              Class appBarClass = child.getClass().getSuperclass().getSuperclass();
              getDownNestedScrollRange = appBarClass.getDeclaredMethod("getDownNestedScrollRange");
              getDownNestedScrollRange.setAccessible(true);
            }
            int downRange = (int) getDownNestedScrollRange.invoke(child);
            consumed[1] = (int) scrollMethod.invoke(
                this, coordinatorLayout, child, dyUnconsumed, -downRange, 0);
          }

          if (dyUnconsumed == 0) {
            if (updateAccessibilityActions == null) {
              Class appBarLayoutClass = getClass().getSuperclass().getSuperclass();
              updateAccessibilityActions = appBarLayoutClass.getDeclaredMethod(
                  "updateAccessibilityActions", CoordinatorLayout.class, AppBarLayout.class);
              updateAccessibilityActions.setAccessible(true);
            }
            updateAccessibilityActions.invoke(this, coordinatorLayout, child);
          }
        } catch (Throwable t) {
          t.printStackTrace();
        }
      }
    }

    @Override
    public void onStopNestedScroll(
        CoordinatorLayout coordinatorLayout, AppBarLayout appBarLayout, View target, int type) {
      super.onStopNestedScroll(coordinatorLayout, appBarLayout, target, type);
      isFlinging = false;
      shouldBlockNestedScroll = false;
      if (scrollListener != null) {
        scrollListener.onScrollStop();
      }
    }
  }
}
