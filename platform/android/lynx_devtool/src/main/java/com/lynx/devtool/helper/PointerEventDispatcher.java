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
  private static final int MAX_TOUCH_POINTS = 2;

  private WeakReference<LynxView> mLynxView;
  private boolean mPointerSequenceActive;
  private int[] mActivePointerIds = new int[0];
  private long mDownTime;
  private long mLastEventTime;
  private long mTimestampOffsetMs;
  private float[] mLastXs = new float[0];
  private float[] mLastYs = new float[0];
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

  // Scroll injection is reserved for future support. DevToolPlatformAndroidDelegate still accepts
  // deltaX/deltaY from native, but this dispatcher only forwards touch pointer sequences for now.
  public boolean injectPointerEvent(int type, int[] pointerIds, float[] xs, float[] ys,
      int actionPointerId, int modifiers, long timestampUs) {
    LynxView lynxView = mLynxView.get();
    if (lynxView == null || !isValidPointerData(pointerIds, xs, ys)) {
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

    int actionIndex = indexOf(pointerIds, actionPointerId);
    if (!isValidTransition(type, pointerIds, actionPointerId, actionIndex)) {
      return false;
    }

    float density = resolveDensity(lynxView);
    float[] rootXs = new float[xs.length];
    float[] rootYs = new float[ys.length];
    for (int index = 0; index < pointerIds.length; ++index) {
      rootXs[index] = xs[index] * density;
      rootYs[index] = ys[index] * density;
      if (!isFinite(rootXs[index]) || !isFinite(rootYs[index])
          || !target.contains(rootXs[index], rootYs[index])) {
        if (mPointerSequenceActive) {
          cancelActiveSequence(SystemClock.uptimeMillis());
        }
        return false;
      }
    }

    boolean startsSequence = type == POINTER_EVENT_DOWN && !mPointerSequenceActive;
    long eventTime = toEventTime(timestampUs, startsSequence);
    int action = toMotionEventAction(type, actionIndex);
    if (action < 0) {
      return false;
    }

    if (startsSequence) {
      mPointerSequenceActive = true;
      mDownTime = eventTime;
      mActiveRoot = new WeakReference<>(target.root);
      mActiveWindowToken = target.windowToken;
    }
    boolean dispatched =
        dispatchMotionEvent(target.root, action, rootXs, rootYs, eventTime, modifiers, pointerIds);
    if (!dispatched) {
      if (startsSequence) {
        resetActiveSequence();
      } else {
        cancelActiveSequence(eventTime);
      }
      return false;
    }

    mLastModifiers = modifiers;
    switch (type) {
      case POINTER_EVENT_DOWN:
      case POINTER_EVENT_MOVE:
        setActivePointers(pointerIds, rootXs, rootYs);
        return true;
      case POINTER_EVENT_UP:
        if (pointerIds.length == 1) {
          resetActiveSequence();
        } else {
          removeActivePointer(pointerIds, rootXs, rootYs, actionPointerId);
        }
        return true;
      case POINTER_EVENT_CANCEL:
        resetActiveSequence();
        return true;
      default:
        return false;
    }
  }

  private boolean isValidPointerData(int[] pointerIds, float[] xs, float[] ys) {
    if (pointerIds == null || xs == null || ys == null || pointerIds.length == 0
        || pointerIds.length > MAX_TOUCH_POINTS || pointerIds.length != xs.length
        || pointerIds.length != ys.length) {
      return false;
    }
    for (int index = 0; index < pointerIds.length; ++index) {
      if (pointerIds[index] < 0 || pointerIds[index] > MAX_POINTER_ID || !isFinite(xs[index])
          || !isFinite(ys[index])) {
        return false;
      }
      for (int previous = 0; previous < index; ++previous) {
        if (pointerIds[previous] == pointerIds[index]) {
          return false;
        }
      }
    }
    return true;
  }

  private boolean isValidTransition(
      int type, int[] pointerIds, int actionPointerId, int actionIndex) {
    if (actionIndex < 0) {
      return false;
    }
    if (type == POINTER_EVENT_DOWN) {
      if (!mPointerSequenceActive) {
        return pointerIds.length == 1;
      }
      return pointerIds.length == mActivePointerIds.length + 1 && !hasActivePointer(actionPointerId)
          && containsAll(pointerIds, mActivePointerIds);
    }
    return mPointerSequenceActive && pointerIds.length == mActivePointerIds.length
        && containsAll(pointerIds, mActivePointerIds)
        && (type == POINTER_EVENT_MOVE || type == POINTER_EVENT_UP || type == POINTER_EVENT_CANCEL);
  }

  private int toMotionEventAction(int type, int actionIndex) {
    switch (type) {
      case POINTER_EVENT_DOWN:
        return mPointerSequenceActive ? MotionEvent.ACTION_POINTER_DOWN
                | (actionIndex << MotionEvent.ACTION_POINTER_INDEX_SHIFT)
                                      : MotionEvent.ACTION_DOWN;
      case POINTER_EVENT_MOVE:
        return MotionEvent.ACTION_MOVE;
      case POINTER_EVENT_UP:
        return mActivePointerIds.length == 1 ? MotionEvent.ACTION_UP
                                             : MotionEvent.ACTION_POINTER_UP
                | (actionIndex << MotionEvent.ACTION_POINTER_INDEX_SHIFT);
      case POINTER_EVENT_CANCEL:
        return MotionEvent.ACTION_CANCEL;
      default:
        return -1;
    }
  }

  private void cancelActiveSequence(long eventTime) {
    if (mPointerSequenceActive && mActiveRoot != null) {
      View root = mActiveRoot.get();
      if (root != null) {
        dispatchMotionEvent(root, MotionEvent.ACTION_CANCEL, mLastXs, mLastYs,
            Math.max(eventTime, mLastEventTime), mLastModifiers, mActivePointerIds);
      }
    }
    resetActiveSequence();
  }

  private boolean dispatchMotionEvent(View root, int action, float[] xs, float[] ys, long eventTime,
      int modifiers, int[] pointerIds) {
    int pointerCount = pointerIds.length;
    MotionEvent.PointerProperties[] properties = new MotionEvent.PointerProperties[pointerCount];
    MotionEvent.PointerCoords[] coordinates = new MotionEvent.PointerCoords[pointerCount];
    int actionMasked = action & MotionEvent.ACTION_MASK;
    int actionIndex =
        (action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
    for (int index = 0; index < pointerCount; ++index) {
      properties[index] = new MotionEvent.PointerProperties();
      properties[index].id = pointerIds[index];
      properties[index].toolType = MotionEvent.TOOL_TYPE_FINGER;

      coordinates[index] = new MotionEvent.PointerCoords();
      coordinates[index].x = xs[index];
      coordinates[index].y = ys[index];
      coordinates[index].pressure = actionMasked == MotionEvent.ACTION_CANCEL
              || (actionMasked == MotionEvent.ACTION_UP
                     || actionMasked == MotionEvent.ACTION_POINTER_UP)
                  && index == actionIndex
          ? 0.f
          : 1.f;
      coordinates[index].size = 1.f;
    }

    MotionEvent event = MotionEvent.obtain(mDownTime, eventTime, action, pointerCount, properties,
        coordinates, toMetaState(modifiers), 0, 1.f, 1.f, 0, 0, InputDevice.SOURCE_TOUCHSCREEN, 0);
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
    return mPointerSequenceActive && indexOf(mActivePointerIds, pointerId) >= 0;
  }

  private static boolean containsAll(int[] values, int[] expected) {
    for (int value : expected) {
      if (indexOf(values, value) < 0) {
        return false;
      }
    }
    return true;
  }

  private static int indexOf(int[] values, int value) {
    for (int index = 0; index < values.length; ++index) {
      if (values[index] == value) {
        return index;
      }
    }
    return -1;
  }

  private void setActivePointers(int[] pointerIds, float[] xs, float[] ys) {
    mActivePointerIds = pointerIds.clone();
    mLastXs = xs.clone();
    mLastYs = ys.clone();
  }

  private void removeActivePointer(int[] pointerIds, float[] xs, float[] ys, int actionPointerId) {
    int remainingCount = pointerIds.length - 1;
    int[] remainingIds = new int[remainingCount];
    float[] remainingXs = new float[remainingCount];
    float[] remainingYs = new float[remainingCount];
    int outputIndex = 0;
    for (int index = 0; index < pointerIds.length; ++index) {
      if (pointerIds[index] == actionPointerId) {
        continue;
      }
      remainingIds[outputIndex] = pointerIds[index];
      remainingXs[outputIndex] = xs[index];
      remainingYs[outputIndex] = ys[index];
      ++outputIndex;
    }
    setActivePointers(remainingIds, remainingXs, remainingYs);
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
    mActivePointerIds = new int[0];
    mDownTime = 0;
    mLastEventTime = 0;
    mTimestampOffsetMs = 0;
    mLastXs = new float[0];
    mLastYs = new float[0];
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
