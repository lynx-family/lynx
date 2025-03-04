// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.event;

import android.graphics.Rect;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.event.EventTargetBase;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

public class LynxTouchEvent extends LynxEvent {
  public static final String EVENT_TOUCH_START = "touchstart";
  public static final String EVENT_TOUCH_MOVE = "touchmove";
  public static final String EVENT_TOUCH_CANCEL = "touchcancel";
  public static final String EVENT_TOUCH_END = "touchend";
  public static final String EVENT_TAP = "tap";
  public static final String EVENT_LONG_PRESS = "longpress";
  public static final String EVENT_CLICK = "click";
  public static final int kPseudoStateNone = 0;
  public static final int kPseudoStateHover = 1;
  public static final int kPseudoStateHoverTransition = 1 << 1;
  public static final int kPseudoStateActive = 1 << 3;
  public static final int kPseudoStateActiveTransition = 1 << 4;
  public static final int kPseudoStateFocus = 1 << 6;
  public static final int kPseudoStateFocusTransition = 1 << 7;
  public static final int kPseudoStateAll = ~0;

  public static class Point {
    float x;
    float y;

    public Point() {
      this.x = 0;
      this.y = 0;
    }

    public Point(float x, float y) {
      this.x = x;
      this.y = y;
    }

    public float getX() {
      return x;
    }

    public float getY() {
      return y;
    }

    public Point convert(Rect fromRect, Rect toRect) {
      float newX = fromRect.left - toRect.left + x;
      float newY = fromRect.top - toRect.top + y;
      return new Point(newX, newY);
    }

    @Override
    public boolean equals(Object o) {
      if (this == o) {
        return true;
      }
      if (!(o instanceof Point)) {
        return false;
      }
      Point point = (Point) o;
      return Float.compare(point.x, x) == 0 && Float.compare(point.y, y) == 0;
    }

    @Override
    public int hashCode() {
      float[] seq = {x, y};
      return Arrays.hashCode(seq);
    }

    @NonNull
    @Override
    public String toString() {
      return "Point{"
          + "x=" + x + ", y=" + y + '}';
    }
  }

  private Point mClientPoint;
  private Point mPagePoint;
  private Point mViewPoint;
  private boolean mIsMultiTouch;
  private Map<Integer, Point> mTouchMap;
  private JavaOnlyMap mUITouchMap;
  private HashMap<Integer, EventTargetBase> mActiveTargetMap;
  private MotionEvent mMotionEvent;

  public LynxTouchEvent(int tag, String name) {
    super(tag, name, LynxEventType.kTouch);
  }

  public LynxTouchEvent(int tag, String name, float x, float y) {
    super(tag, name, LynxEventType.kTouch);

    Point point = new Point(x, y);
    mClientPoint = point;
    mPagePoint = point;
    mViewPoint = point;
  }

  public LynxTouchEvent(int tag, String name, Point clientPoint, Point pagePoint, Point viewPoint) {
    super(tag, name, LynxEventType.kTouch);

    mClientPoint = clientPoint;
    mPagePoint = pagePoint;
    mViewPoint = viewPoint;
  }

  public LynxTouchEvent(int tag, String name, Map<Integer, Point> touchMap) {
    super(tag, name, LynxEventType.kTouch);

    mIsMultiTouch = true;
    mTouchMap = touchMap;
  }

  public LynxTouchEvent(String name, JavaOnlyMap uiTouchMap) {
    super(-1, name, LynxEventType.kTouch);

    mIsMultiTouch = true;
    mUITouchMap = uiTouchMap;
  }

  public Point getClientPoint() {
    return mClientPoint;
  }

  public Point getPagePoint() {
    return mPagePoint;
  }

  public Point getViewPoint() {
    return mViewPoint;
  }

  public boolean getIsMultiTouch() {
    return mIsMultiTouch;
  }

  public Map<Integer, Point> getTouchMap() {
    return mTouchMap;
  }

  public JavaOnlyMap getUITouchMap() {
    return mUITouchMap;
  }

  public void setMotionEvent(MotionEvent motionEvent) {
    mMotionEvent = motionEvent;
  }

  public MotionEvent getMotionEvent() {
    return mMotionEvent;
  }

  public void setActiveTargetMap(HashMap<Integer, EventTargetBase> activeTargetMap) {
    mActiveTargetMap = activeTargetMap;
  }

  public HashMap<Integer, EventTargetBase> getActiveTargetMap() {
    return mActiveTargetMap;
  }
}
