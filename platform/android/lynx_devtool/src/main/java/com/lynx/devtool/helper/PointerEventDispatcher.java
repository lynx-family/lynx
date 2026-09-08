// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.content.res.Resources;
import android.os.IBinder;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LLog;
import java.lang.ref.WeakReference;

/**
 * Dispatches synthetic pointer sequences to the window root via {@link View#dispatchTouchEvent}.
 *
 * <p>Threading contract: all public methods ({@link #attach}, {@link #detach} and
 * {@link #injectPointerEvent}) must be invoked on the Android UI thread. {@code dispatchTouchEvent}
 * requires the UI thread, and the mutable pointer-sequence state below is intentionally not
 * synchronized. When the native {@code AndroidInputEventTarget} drives injection, the owning
 * {@code SyntheticGestureController} must therefore be created with the UI task runner so that its
 * {@code injectPointerEvent} callbacks land on this thread, mirroring how {@code EmulateTouch} is
 * wrapped in {@code RunOnUIThread} at the mediator layer.
 */
public class PointerEventDispatcher {
  private static final String TAG = "PointerEventDispatcher";
  public static final int POINTER_EVENT_DOWN = 0;
  public static final int POINTER_EVENT_MOVE = 1;
  public static final int POINTER_EVENT_UP = 2;
  public static final int POINTER_EVENT_CANCEL = 3;
  public static final int POINTER_EVENT_SCROLL = 4;

  private static final int MAX_POINTER_ID = 31;

  private WeakReference<LynxView> mLynxView;
  private boolean mPointerSequenceActive;
  private int mActivePointerId;
  private long mDownTime;
  private long mLastEventTime;
  private long mTimestampOffsetMs;
  private float mLastX;
  private float mLastY;
  private int mLastModifiers;
  private WeakReference<View> mActiveRoot;
  private IBinder mActiveWindowToken;

  public PointerEventDispatcher(LynxView lynxView) {
    mLynxView = new WeakReference<>(lynxView);
  }

  public void attach(LynxView lynxView) {
    cancelActiveSequence(SystemClock.uptimeMillis());
    mLynxView = new WeakReference<>(lynxView);
  }

  public void detach() {
    cancelActiveSequence(SystemClock.uptimeMillis());
    mLynxView.clear();
  }

  public boolean injectPointerEvent(int type, float x, float y, float deltaX, float deltaY,
      int pointerId, int modifiers, long timestampUs) {
    // deltaX/deltaY are unused for tap down/up/cancel; they are kept to match the JNI-called
    // signature and reserved for future scroll support (POINTER_EVENT_SCROLL).
    LynxView lynxView = mLynxView.get();
    if (lynxView == null || pointerId < 0 || pointerId > MAX_POINTER_ID) {
      return false;
    }

    WindowTarget target = resolveWindowTarget(lynxView);
    if (target == null) {
      cancelActiveSequence(SystemClock.uptimeMillis());
      return false;
    }
    if (mPointerSequenceActive && !isActiveTarget(target)) {
      cancelActiveSequence(SystemClock.uptimeMillis());
      return false;
    }

    if (type == POINTER_EVENT_DOWN && mPointerSequenceActive) {
      return false;
    }
    if (type != POINTER_EVENT_DOWN && !hasActivePointer(pointerId)) {
      return false;
    }

    float density = resolveDensity(lynxView);
    float rootX = x * density;
    float rootY = y * density;
    if (!isFinite(rootX) || !isFinite(rootY) || !target.contains(rootX, rootY)) {
      if (mPointerSequenceActive) {
        cancelActiveSequence(SystemClock.uptimeMillis());
      }
      return false;
    }

    long eventTime = toEventTime(timestampUs, type == POINTER_EVENT_DOWN);
    switch (type) {
      case POINTER_EVENT_DOWN:
        return startSequence(target, rootX, rootY, eventTime, pointerId, modifiers);
      case POINTER_EVENT_UP:
        return finishActiveSequence(
            target, MotionEvent.ACTION_UP, rootX, rootY, eventTime, modifiers);
      case POINTER_EVENT_CANCEL:
        return finishActiveSequence(
            target, MotionEvent.ACTION_CANCEL, rootX, rootY, eventTime, modifiers);
      default:
        return false;
    }
  }

  private boolean startSequence(
      WindowTarget target, float x, float y, long eventTime, int pointerId, int modifiers) {
    mPointerSequenceActive = true;
    mActivePointerId = pointerId;
    mDownTime = eventTime;
    mLastEventTime = eventTime;
    mLastX = x;
    mLastY = y;
    mLastModifiers = modifiers;
    mActiveRoot = new WeakReference<>(target.root);
    mActiveWindowToken = target.windowToken;
    boolean dispatched = dispatchMotionEvent(
        target.root, MotionEvent.ACTION_DOWN, x, y, eventTime, modifiers, pointerId);
    if (!dispatched) {
      resetActiveSequence();
    }
    return dispatched;
  }

  private boolean finishActiveSequence(
      WindowTarget target, int action, float x, float y, long eventTime, int modifiers) {
    mLastX = x;
    mLastY = y;
    mLastModifiers = modifiers;
    boolean dispatched =
        dispatchMotionEvent(target.root, action, x, y, eventTime, modifiers, mActivePointerId);
    resetActiveSequence();
    return dispatched;
  }

  private void cancelActiveSequence(long eventTime) {
    if (mPointerSequenceActive && mActiveRoot != null) {
      View root = mActiveRoot.get();
      if (root != null) {
        dispatchMotionEvent(root, MotionEvent.ACTION_CANCEL, mLastX, mLastY,
            Math.max(eventTime, mLastEventTime), mLastModifiers, mActivePointerId);
      }
    }
    resetActiveSequence();
  }

  private boolean dispatchMotionEvent(
      View root, int action, float x, float y, long eventTime, int modifiers, int pointerId) {
    MotionEvent.PointerProperties properties = new MotionEvent.PointerProperties();
    properties.id = pointerId;
    properties.toolType = MotionEvent.TOOL_TYPE_FINGER;

    MotionEvent.PointerCoords coordinates = new MotionEvent.PointerCoords();
    coordinates.x = x;
    coordinates.y = y;
    coordinates.pressure =
        action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL ? 0.f : 1.f;
    coordinates.size = 1.f;

    MotionEvent event = MotionEvent.obtain(mDownTime, eventTime, action, 1,
        new MotionEvent.PointerProperties[] {properties},
        new MotionEvent.PointerCoords[] {coordinates}, toMetaState(modifiers), 0, 1.f, 1.f, 0, 0,
        InputDevice.SOURCE_TOUCHSCREEN, 0);
    try {
      return root.dispatchTouchEvent(event);
    } finally {
      event.recycle();
    }
  }

  private WindowTarget resolveWindowTarget(LynxView lynxView) {
    IBinder windowToken = lynxView.getWindowToken();
    View root = lynxView.getRootView();
    if (windowToken == null || root == null || !windowToken.equals(root.getWindowToken())) {
      LLog.e(TAG, "injectPointerEvent: LynxView is not attached to a window");
      return null;
    }
    int width = root.getWidth();
    int height = root.getHeight();
    if (width <= 0 || height <= 0) {
      return null;
    }
    return new WindowTarget(root, windowToken, width, height);
  }

  private float resolveDensity(LynxView lynxView) {
    Resources resources = lynxView.getResources();
    if (resources == null || resources.getDisplayMetrics() == null) {
      return Float.NaN;
    }
    float density = resources.getDisplayMetrics().density;
    return isFinite(density) && density > 0.f ? density : Float.NaN;
  }

  private boolean isActiveTarget(WindowTarget target) {
    View activeRoot = mActiveRoot == null ? null : mActiveRoot.get();
    return activeRoot == target.root && mActiveWindowToken != null
        && mActiveWindowToken.equals(target.windowToken);
  }

  private boolean hasActivePointer(int pointerId) {
    return mPointerSequenceActive && pointerId == mActivePointerId;
  }

  private long toEventTime(long timestampUs, boolean startsSequence) {
    long now = SystemClock.uptimeMillis();
    long eventTime = now;
    if (timestampUs > 0) {
      long timestampMs = timestampUs / 1000;
      if (startsSequence) {
        mTimestampOffsetMs = now - timestampMs;
      }
      eventTime = Math.min(now, timestampMs + mTimestampOffsetMs);
    }
    mLastEventTime = startsSequence ? eventTime : Math.max(mLastEventTime, eventTime);
    return mLastEventTime;
  }

  private static int toMetaState(int modifiers) {
    int metaState = 0;
    if ((modifiers & 1) != 0) {
      metaState |= KeyEvent.META_ALT_ON;
    }
    if ((modifiers & 2) != 0) {
      metaState |= KeyEvent.META_CTRL_ON;
    }
    if ((modifiers & 4) != 0) {
      metaState |= KeyEvent.META_META_ON;
    }
    if ((modifiers & 8) != 0) {
      metaState |= KeyEvent.META_SHIFT_ON;
    }
    return metaState;
  }

  private static boolean isFinite(float value) {
    return !Float.isNaN(value) && !Float.isInfinite(value);
  }

  private void resetActiveSequence() {
    mPointerSequenceActive = false;
    mActivePointerId = 0;
    mDownTime = 0;
    mLastEventTime = 0;
    mTimestampOffsetMs = 0;
    mLastX = 0.f;
    mLastY = 0.f;
    mLastModifiers = 0;
    mActiveRoot = null;
    mActiveWindowToken = null;
  }

  private static class WindowTarget {
    final View root;
    final IBinder windowToken;
    final int width;
    final int height;

    WindowTarget(View root, IBinder windowToken, int width, int height) {
      this.root = root;
      this.windowToken = windowToken;
      this.width = width;
      this.height = height;
    }

    boolean contains(float x, float y) {
      return x >= 0.f && y >= 0.f && x < width && y < height;
    }
  }
}
