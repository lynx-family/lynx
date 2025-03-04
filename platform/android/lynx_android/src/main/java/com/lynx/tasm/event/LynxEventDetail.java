// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.event;

import android.view.MotionEvent;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.event.EventTargetBase;
import com.lynx.tasm.event.LynxTouchEvent.Point;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public class LynxEventDetail {
  public static enum EVENT_TYPE {
    TOUCH_EVENT,
    CUSTOM_EVENT,
  }

  private LynxEvent mEvent;
  private EventTargetBase mEventTarget;
  private LynxView mLynxView;
  private MotionEvent mMotionEvent;

  private static String TAG = "LynxEventDetail";

  public LynxEventDetail(@NonNull LynxEvent event, EventTargetBase target, LynxView view) {
    mEvent = event;
    mEventTarget = target;
    mLynxView = view;
    mMotionEvent = null;
  }

  public LynxView getLynxView() {
    return mLynxView;
  }

  public EventTargetBase getEventTarget() {
    return mEventTarget;
  }

  public String getEventName() {
    return mEvent.getName();
  }

  public EVENT_TYPE getEventType() {
    switch (mEvent.getType()) {
      case kTouch:
        return EVENT_TYPE.TOUCH_EVENT;
      default:
        return EVENT_TYPE.CUSTOM_EVENT;
    }
  }

  public void setMotionEvent(MotionEvent motionEvent) {
    mMotionEvent = motionEvent;
  }

  public MotionEvent getMotionEvent() {
    if (mEvent.getType() != LynxEvent.LynxEventType.kTouch) {
      LLog.w(
          TAG, "getMotionEvent error, event type is not touch event. type is " + mEvent.getType());
    }
    return mMotionEvent;
  }

  public Point getTargetPoint() {
    if (mEvent.getType() != LynxEvent.LynxEventType.kTouch) {
      LLog.w(
          TAG, "getTargetPoint error, event type is not touch event. type is " + mEvent.getType());
      return new Point(0, 0);
    }
    Point point = ((LynxTouchEvent) mEvent).getViewPoint();
    return point == null ? new Point(0, 0) : point;
  }

  public boolean getIsMultiTouch() {
    if (mEvent.getType() != LynxEvent.LynxEventType.kTouch) {
      LLog.w(
          TAG, "getIsMultiTouch error, event type is not touch event. type is " + mEvent.getType());
      return false;
    }
    return ((LynxTouchEvent) mEvent).getIsMultiTouch();
  }

  public HashMap<Integer, Point> getTargetPointMap() {
    boolean isMultiTouch = ((LynxTouchEvent) mEvent).getIsMultiTouch();
    if (mEvent.getType() != LynxEvent.LynxEventType.kTouch || !isMultiTouch) {
      LLog.w(TAG,
          "getTargetPointMap error, event type is not touch event. type is " + mEvent.getType()
              + ", isMultiTouch:" + isMultiTouch);
      return new HashMap<>();
    }
    HashMap<Integer, Point> map = (HashMap<Integer, Point>) ((LynxTouchEvent) mEvent).getTouchMap();
    return map == null ? new HashMap<>() : map;
  }

  public HashMap<String, Object> getEventParams() {
    if (mEvent.getType() != LynxEvent.LynxEventType.kCustom) {
      LLog.w(
          TAG, "getEventParams error, event type is not custom event. type is" + mEvent.getType());
      return new HashMap<>();
    }
    HashMap<String, Object> map =
        (HashMap<String, Object>) ((LynxCustomEvent) mEvent).eventParams();
    return map == null ? new HashMap<>() : map;
  }
}
