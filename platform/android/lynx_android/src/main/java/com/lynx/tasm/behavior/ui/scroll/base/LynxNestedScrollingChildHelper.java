// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

import static androidx.core.view.ViewCompat.TYPE_TOUCH;

import android.view.View;
import android.view.ViewParent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.core.view.ViewParentCompat;

/**
 * Helper class for implementing nested scrolling child views compatible with Android platform
 * versions earlier than Android 5.0 Lollipop (API 21).
 *
 * <p>{@link android.view.View View} subclasses should instantiate a final instance of this
 * class as a field at construction. For each <code>View</code> method that has a matching
 * method signature in this class, delegate the operation to the helper instance in an overridden
 * method implementation. This implements the standard framework policy for nested scrolling.</p>
 *
 * <p>Views invoking nested scrolling functionality should always do so from the relevant
 * {@link androidx.core.view.ViewCompat}, {@link androidx.core.view.ViewGroupCompat} or
 * {@link androidx.core.view.ViewParentCompat} compatibility
 * shim static methods. This ensures interoperability with nested scrolling views on Android
 * 5.0 Lollipop and newer.</p>
 */

public class LynxNestedScrollingChildHelper {
  private ViewParent mNestedScrollingParentTouch;
  private ViewParent mNestedScrollingParentNonTouch;
  private final View mView;
  private boolean mIsNestedScrollingEnabled;
  private int[] mTempNestedScrollConsumed;

  /**
   * Construct a new helper for a given view.
   */
  public LynxNestedScrollingChildHelper(@NonNull View view) {
    mView = view;
  }

  /**
   * Enable nested scrolling.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @param enabled true to enable nested scrolling dispatch from this view, false otherwise
   */
  public void setNestedScrollingEnabled(boolean enabled) {
    if (mIsNestedScrollingEnabled) {
      ViewCompat.stopNestedScroll(mView);
    }
    mIsNestedScrollingEnabled = enabled;
  }

  /**
   * Check if nested scrolling is enabled for this view.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if nested scrolling is enabled for this view
   */
  public boolean isNestedScrollingEnabled() {
    return mIsNestedScrollingEnabled;
  }

  /**
   * Check if this view has a nested scrolling parent view currently receiving events for
   * a nested scroll in progress with the type of touch.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if this view has a nested scrolling parent, false otherwise
   */
  public boolean hasNestedScrollingParent() {
    return hasNestedScrollingParent(TYPE_TOUCH);
  }

  /**
   * Check if this view has a nested scrolling parent view currently receiving events for
   * a nested scroll in progress with the given type.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if this view has a nested scrolling parent, false otherwise
   */
  public boolean hasNestedScrollingParent(int type) {
    return getNestedScrollingParentForType(type) != null;
  }

  /**
   * Start a new nested scroll for this view.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @param axes Supported nested scroll axes.
   *             See {@link androidx.core.view.NestedScrollingChild#startNestedScroll(int)}.
   * @return true if a cooperating parent view was found and nested scrolling started successfully
   */
  public boolean startNestedScroll(@ViewCompat.ScrollAxis int axes) {
    return startNestedScroll(axes, TYPE_TOUCH);
  }

  /**
   * Start a new nested scroll for this view.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild2} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @param axes Supported nested scroll axes.
   *             See {@link androidx.core.view.NestedScrollingChild2#startNestedScroll(int,
   *             int)}.
   * @return true if a cooperating parent view was found and nested scrolling started successfully
   */
  public boolean startNestedScroll(
      @ViewCompat.ScrollAxis int axes, @ViewCompat.NestedScrollType int type) {
    if (hasNestedScrollingParent(type)) {
      // Already in progress
      return true;
    }
    if (isNestedScrollingEnabled()) {
      ViewParent p = mView.getParent();
      View child = mView;
      while (p != null) {
        if (ViewParentCompat.onStartNestedScroll(p, child, mView, axes, type)) {
          setNestedScrollingParentForType(type, p);
          ViewParentCompat.onNestedScrollAccepted(p, child, mView, axes, type);
          return true;
        }
        if (p instanceof View) {
          child = (View) p;
        }
        p = p.getParent();
      }
    }
    return false;
  }

  /**
   * Stop a nested scroll in progress.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   */
  public void stopNestedScroll() {
    stopNestedScroll(TYPE_TOUCH);
  }

  /**
   * Stop a nested scroll in progress.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild2} interface method with the same
   * signature to implement the standard policy.</p>
   */
  public void stopNestedScroll(@ViewCompat.NestedScrollType int type) {
    ViewParent parent = getNestedScrollingParentForType(type);
    if (parent != null) {
      ViewParentCompat.onStopNestedScroll(parent, mView, type);
      setNestedScrollingParentForType(type, null);
    }
  }

  /**
   * Dispatch one step of a nested scrolling operation to the current nested scrolling parent.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if the parent consumed any of the nested scroll
   */
  public boolean dispatchNestedScroll(int dxConsumed, int dyConsumed, int dxUnconsumed,
      int dyUnconsumed, @Nullable int[] offsetInWindow) {
    return dispatchNestedScroll(
        dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow, TYPE_TOUCH);
  }

  /**
   * Dispatch one step of a nested scrolling operation to the current nested scrolling parent.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild2} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if the parent consumed any of the nested scroll
   */
  public boolean dispatchNestedScroll(int dxConsumed, int dyConsumed, int dxUnconsumed,
      int dyUnconsumed, @Nullable int[] offsetInWindow, @ViewCompat.NestedScrollType int type) {
    if (isNestedScrollingEnabled()) {
      final ViewParent parent = getNestedScrollingParentForType(type);
      if (parent == null) {
        return false;
      }

      if (dxConsumed != 0 || dyConsumed != 0 || dxUnconsumed != 0 || dyUnconsumed != 0) {
        int startX = 0;
        int startY = 0;
        if (offsetInWindow != null) {
          mView.getLocationInWindow(offsetInWindow);
          startX = offsetInWindow[0];
          startY = offsetInWindow[1];
        }

        ViewParentCompat.onNestedScroll(
            parent, mView, dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, type);

        if (offsetInWindow != null) {
          mView.getLocationInWindow(offsetInWindow);
          offsetInWindow[0] -= startX;
          offsetInWindow[1] -= startY;
        }
        return true;
      } else if (offsetInWindow != null) {
        // No motion, no dispatch. Keep offsetInWindow up to date.
        offsetInWindow[0] = 0;
        offsetInWindow[1] = 0;
      }
    }
    return false;
  }

  /**
   * Dispatch one step of a nested pre-scrolling operation to the current nested scrolling parent.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if the parent consumed any of the nested scroll
   */
  public boolean dispatchNestedPreScroll(
      int dx, int dy, @Nullable int[] consumed, @Nullable int[] offsetInWindow) {
    return dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, TYPE_TOUCH);
  }

  /**
   * Dispatch one step of a nested pre-scrolling operation to the current nested scrolling parent.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild2} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if the parent consumed any of the nested scroll
   */
  public boolean dispatchNestedPreScroll(int dx, int dy, @Nullable int[] consumed,
      @Nullable int[] offsetInWindow, @ViewCompat.NestedScrollType int type) {
    if (isNestedScrollingEnabled()) {
      final ViewParent parent = getNestedScrollingParentForType(type);
      if (parent == null) {
        return false;
      }

      if (dx != 0 || dy != 0) {
        int startX = 0;
        int startY = 0;
        if (offsetInWindow != null) {
          mView.getLocationInWindow(offsetInWindow);
          startX = offsetInWindow[0];
          startY = offsetInWindow[1];
        }

        if (consumed == null) {
          if (mTempNestedScrollConsumed == null) {
            mTempNestedScrollConsumed = new int[2];
          }
          consumed = mTempNestedScrollConsumed;
        }
        consumed[0] = 0;
        consumed[1] = 0;
        ViewParentCompat.onNestedPreScroll(parent, mView, dx, dy, consumed, type);

        if (offsetInWindow != null) {
          mView.getLocationInWindow(offsetInWindow);
          offsetInWindow[0] -= startX;
          offsetInWindow[1] -= startY;
        }
        return consumed[0] != 0 || consumed[1] != 0;
      } else if (offsetInWindow != null) {
        offsetInWindow[0] = 0;
        offsetInWindow[1] = 0;
      }
    }
    return false;
  }

  /**
   * Dispatch a nested fling operation to the current nested scrolling parent.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if the parent consumed the nested fling
   */
  public boolean dispatchNestedFling(float velocityX, float velocityY, boolean consumed) {
    if (isNestedScrollingEnabled()) {
      ViewParent parent = getNestedScrollingParentForType(TYPE_TOUCH);
      if (parent != null) {
        return ViewParentCompat.onNestedFling(parent, mView, velocityX, velocityY, consumed);
      }
    }
    return false;
  }

  /**
   * Dispatch a nested pre-fling operation to the current nested scrolling parent.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @return true if the parent consumed the nested fling
   */
  public boolean dispatchNestedPreFling(float velocityX, float velocityY) {
    if (isNestedScrollingEnabled()) {
      ViewParent parent = getNestedScrollingParentForType(TYPE_TOUCH);
      if (parent != null) {
        return ViewParentCompat.onNestedPreFling(parent, mView, velocityX, velocityY);
      }
    }
    return false;
  }

  /**
   * View subclasses should always call this method on their
   * <code>NestedScrollingChildHelper</code> when detached from a window.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   */
  public void onDetachedFromWindow() {
    ViewCompat.stopNestedScroll(mView);
  }

  /**
   * Called when a nested scrolling child stops its current nested scroll operation.
   *
   * <p>This is a delegate method. Call it from your {@link android.view.View View} subclass
   * method/{@link androidx.core.view.NestedScrollingChild} interface method with the same
   * signature to implement the standard policy.</p>
   *
   * @param child Child view stopping its nested scroll. This may not be a direct child view.
   */
  public void onStopNestedScroll(@NonNull View child) {
    ViewCompat.stopNestedScroll(mView);
  }

  public ViewParent getNestedScrollingParentForType(int type) {
    return type == TYPE_TOUCH ? mNestedScrollingParentTouch : mNestedScrollingParentNonTouch;
  }

  private void setNestedScrollingParentForType(
      @ViewCompat.NestedScrollType int type, ViewParent p) {
    if (type == TYPE_TOUCH) {
      mNestedScrollingParentTouch = p;
    } else {
      mNestedScrollingParentNonTouch = p;
    }
  }
}
