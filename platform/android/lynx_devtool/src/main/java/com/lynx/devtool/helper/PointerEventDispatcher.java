// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LLog;
import java.lang.ref.WeakReference;

public class PointerEventDispatcher {
  private static final String TAG = "PointerEventDispatcher";
  public static final int POINTER_EVENT_DOWN = 0;
  public static final int POINTER_EVENT_MOVE = 1;
  public static final int POINTER_EVENT_UP = 2;
  public static final int POINTER_EVENT_CANCEL = 3;
  public static final int POINTER_EVENT_SCROLL = 4;
  private static final int SCROLL_DELTA_SCALE = 10;
  private static final long SCROLL_END_DELAY_MS = 100;

  private WeakReference<LynxView> mLynxView;
  private boolean mScrollGestureActive;
  private float mLastX;
  private float mLastY;
  private final Handler mScrollHandler;
  private long mDownTime;
  private boolean mPointerSequenceActive;
  private int mActivePointerId;
  private int mLastModifiers;
  private long mLastEventTime;
  private long mTimestampOffsetMs;

  public PointerEventDispatcher(WeakReference<LynxView> lynxView) {
    mLynxView = lynxView;
    mScrollGestureActive = false;
    mScrollHandler = new Handler(Looper.getMainLooper());
    mDownTime = 0;
    mPointerSequenceActive = false;
    mActivePointerId = 0;
    mLastModifiers = 0;
    mLastEventTime = 0;
    mTimestampOffsetMs = 0;
  }

  public void attach(LynxView lynxView) {
    LynxView previousView = mLynxView.get();
    mScrollHandler.removeCallbacksAndMessages(null);
    if (previousView != null) {
      cancelActiveSequence(previousView, SystemClock.uptimeMillis());
    } else {
      resetActiveSequence();
    }
    mLynxView = new WeakReference<>(lynxView);
  }

  public void detach() {
    LynxView view = mLynxView.get();
    mScrollHandler.removeCallbacksAndMessages(null);
    if (view != null) {
      cancelActiveSequence(view, SystemClock.uptimeMillis());
    } else {
      resetActiveSequence();
    }
    mLynxView.clear();
  }

  public boolean injectPointerEvent(int type, float x, float y, float deltaX, float deltaY,
      int pointerId, int modifiers, long timestampUs) {
    LynxView view = mLynxView.get();
    if (view == null) {
      LLog.e(TAG, "injectPointerEvent: view is null");
      return false;
    }

    long eventTime = toEventTime(timestampUs);
    switch (type) {
      case POINTER_EVENT_DOWN:
        if (mPointerSequenceActive) {
          if (!mScrollGestureActive) {
            return false;
          }
          cancelActiveSequence(view, eventTime);
        }
        mScrollHandler.removeCallbacksAndMessages(null);
        mDownTime = eventTime;
        mPointerSequenceActive = true;
        mActivePointerId = pointerId;
        mLastModifiers = modifiers;
        mLastX = x;
        mLastY = y;
        if (!dispatchMotionEvent(view, MotionEvent.ACTION_DOWN, x, y, eventTime, modifiers)) {
          resetActiveSequence();
          return false;
        }
        return true;
      case POINTER_EVENT_MOVE:
        if (!hasActivePointer(pointerId)) {
          return false;
        }
        mLastModifiers = modifiers;
        mLastX = x;
        mLastY = y;
        if (!dispatchMotionEvent(view, MotionEvent.ACTION_MOVE, x, y, eventTime, modifiers)) {
          resetActiveSequence();
          return false;
        }
        return true;
      case POINTER_EVENT_UP:
        return finishActiveSequence(
            view, MotionEvent.ACTION_UP, x, y, eventTime, pointerId, modifiers);
      case POINTER_EVENT_CANCEL:
        return finishActiveSequence(
            view, MotionEvent.ACTION_CANCEL, x, y, eventTime, pointerId, modifiers);
      case POINTER_EVENT_SCROLL:
        return injectScroll(view, x, y, deltaX, deltaY, eventTime, pointerId, modifiers);
      default:
        return false;
    }
  }

  private boolean injectScroll(LynxView view, float x, float y, float deltaX, float deltaY,
      long eventTime, int pointerId, int modifiers) {
    mScrollHandler.removeCallbacksAndMessages(null);
    if (!mScrollGestureActive) {
      cancelActiveSequence(view, eventTime);
      mScrollGestureActive = true;
      mDownTime = eventTime;
      mPointerSequenceActive = true;
      mActivePointerId = pointerId;
      mLastModifiers = modifiers;
      mLastX = x;
      mLastY = y;
      if (!dispatchMotionEvent(view, MotionEvent.ACTION_DOWN, x, y, eventTime, modifiers)) {
        resetActiveSequence();
        return false;
      }
    }

    mLastModifiers = modifiers;
    mLastX += deltaX / SCROLL_DELTA_SCALE;
    mLastY += deltaY / SCROLL_DELTA_SCALE;
    if (!dispatchMotionEvent(view, MotionEvent.ACTION_MOVE, mLastX, mLastY, eventTime, modifiers)) {
      resetActiveSequence();
      return false;
    }
    mScrollHandler.postDelayed(this::stopScroll, SCROLL_END_DELAY_MS);
    return true;
  }

  private boolean finishActiveSequence(
      LynxView view, int action, float x, float y, long eventTime, int pointerId, int modifiers) {
    if (!hasActivePointer(pointerId)) {
      return false;
    }
    boolean dispatched = dispatchMotionEvent(view, action, x, y, eventTime, modifiers);
    resetActiveSequence();
    return dispatched;
  }

  private void cancelActiveSequence(LynxView view, long eventTime) {
    if (mPointerSequenceActive) {
      dispatchMotionEvent(
          view, MotionEvent.ACTION_CANCEL, mLastX, mLastY, eventTime, mLastModifiers);
    }
    resetActiveSequence();
  }

  private void stopScroll() {
    LynxView view = mLynxView.get();
    if (view == null) {
      resetActiveSequence();
      return;
    }
    if (mScrollGestureActive && mPointerSequenceActive) {
      dispatchMotionEvent(
          view, MotionEvent.ACTION_UP, mLastX, mLastY, SystemClock.uptimeMillis(), mLastModifiers);
    }
    resetActiveSequence();
  }

  private boolean dispatchMotionEvent(
      LynxView view, int action, float x, float y, long eventTime, int modifiers) {
    View windowRoot = resolveWindowRoot(view);
    if (windowRoot == null) {
      return false;
    }
    int[] viewLocation = new int[2];
    int[] rootLocation = new int[2];
    view.getLocationInWindow(viewLocation);
    windowRoot.getLocationInWindow(rootLocation);

    MotionEvent.PointerProperties properties = new MotionEvent.PointerProperties();
    properties.id = 0;
    properties.toolType = MotionEvent.TOOL_TYPE_FINGER;
    MotionEvent.PointerCoords coordinates = new MotionEvent.PointerCoords();
    coordinates.x = x + viewLocation[0] - rootLocation[0];
    coordinates.y = y + viewLocation[1] - rootLocation[1];
    coordinates.pressure =
        action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL ? 0.f : 1.f;
    coordinates.size = 1.f;
    MotionEvent event = MotionEvent.obtain(mDownTime, eventTime, action, 1,
        new MotionEvent.PointerProperties[] {properties},
        new MotionEvent.PointerCoords[] {coordinates}, toMetaState(modifiers), 0, 1.f, 1.f, 0, 0,
        InputDevice.SOURCE_TOUCHSCREEN, 0);
    try {
      windowRoot.dispatchTouchEvent(event);
    } finally {
      event.recycle();
    }
    return true;
  }

  private View resolveWindowRoot(LynxView view) {
    IBinder windowToken = view.getWindowToken();
    View windowRoot = view.getRootView();
    if (windowToken == null || windowRoot == null
        || !windowToken.equals(windowRoot.getWindowToken())) {
      LLog.e(TAG, "injectPointerEvent: LynxView is not attached to a window");
      return null;
    }
    return windowRoot;
  }

  private boolean hasActivePointer(int pointerId) {
    return mPointerSequenceActive && pointerId == mActivePointerId;
  }

  private long toEventTime(long timestampUs) {
    long now = SystemClock.uptimeMillis();
    long eventTime;
    if (timestampUs <= 0) {
      eventTime = now;
    } else {
      long timestampMs = timestampUs / 1000;
      if (!mPointerSequenceActive) {
        mTimestampOffsetMs = now - timestampMs;
      }
      eventTime = Math.min(now, timestampMs + mTimestampOffsetMs);
    }
    mLastEventTime = Math.max(mLastEventTime, eventTime);
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

  private void resetActiveSequence() {
    mScrollGestureActive = false;
    mPointerSequenceActive = false;
    mDownTime = 0;
    mActivePointerId = 0;
    mLastModifiers = 0;
    mLastEventTime = 0;
    mTimestampOffsetMs = 0;
  }
}
