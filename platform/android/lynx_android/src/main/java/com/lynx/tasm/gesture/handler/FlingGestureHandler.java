// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

import android.view.MotionEvent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.detector.GestureDetector;
import java.util.HashMap;

/**
 * The FlingGestureHandler class is a concrete implementation of the BaseGestureHandler class
 * for handling fling gestures. It handles fling gesture for component, triggered when the finger is
 * lifted and speed is not zero.
 */
public class FlingGestureHandler extends BaseGestureHandler {
  // is onBegin invoked or not
  private boolean mIsInvokedBegin = false;
  // is onStart invoked or not
  private boolean mIsInvokedStart = false;
  // is onEnd invoked or not
  private boolean mIsInvokedEnd = false;

  private final HashMap<String, Object> mEventParams;

  /**
   * Constructs a FlingGestureHandler object with the specified properties.
   *
   * @param sign                the sign indicating the direction of the gesture
   * @param lynxContext         the LynxContext associated with the gesture handler
   * @param gestureDetector     the GestureDetector associated with the gesture handler
   * @param gestureArenaMember  the GestureArenaMember associated with the gesture handler
   */
  public FlingGestureHandler(int sign, LynxContext lynxContext,
      @NonNull GestureDetector gestureDetector, GestureArenaMember gestureArenaMember) {
    super(sign, lynxContext, gestureDetector, gestureArenaMember);
    handleConfigMap(gestureDetector.getConfigMap());
    mEventParams = new HashMap<>();
  }

  @Override
  protected void handleConfigMap(@Nullable ReadableMap config) {
    if (config == null) {
      return;
    }
  }

  @Override
  protected void onHandle(@Nullable MotionEvent event, @Nullable LynxTouchEvent lynxTouchEvent,
      float flingDeltaX, float flingDeltaY) {
    if (mGestureArenaMember != null) {
      mGestureArenaMember.onInvalidate();
    }

    if (event != null
        && (event.getActionMasked() == MotionEvent.ACTION_DOWN
            || event.getActionMasked() == MotionEvent.ACTION_MOVE)) {
      // If the event is not empty, it means the finger on the screen, no need to handle fling
      // gesture
      ignore();
      return;
    }

    if (event != null && event.getActionMasked() == MotionEvent.ACTION_UP) {
      begin();
      onBegin(0, 0, null);
      return;
    }

    if (mStatus >= GestureConstants.LYNX_STATE_FAIL && mStatus <= GestureConstants.LYNX_STATE_END) {
      onEnd(0, 0, null);
      return;
    }

    if (flingDeltaX == Float.MIN_VALUE && flingDeltaY == Float.MIN_VALUE) {
      fail();
      onEnd(0, 0, null);
      return;
    }

    if (mStatus == GestureConstants.LYNX_STATE_INIT
        || mStatus == GestureConstants.LYNX_STATE_UNDETERMINED) {
      begin();
      activate();
      onBegin(0, 0, null);
      onStart(0, 0, null);
      return;
    }

    onUpdate(flingDeltaX, flingDeltaY, null);
  }

  protected HashMap<String, Object> getEventParamsInActive(float deltaX, float deltaY) {
    mEventParams.put("scrollX", px2dip(mGestureArenaMember.getMemberScrollX()));
    mEventParams.put("scrollY", px2dip(mGestureArenaMember.getMemberScrollY()));
    mEventParams.put("deltaX", px2dip(deltaX));
    mEventParams.put("deltaY", px2dip(deltaY));
    mEventParams.put("isAtStart", mGestureArenaMember.isAtBorder(true));
    mEventParams.put("isAtEnd", mGestureArenaMember.isAtBorder(false));
    return mEventParams;
  }

  @Override
  public void fail() {
    super.fail();
    onEnd(0, 0, null);
  }

  @Override
  public void end() {
    super.end();
    onEnd(0, 0, null);
  }

  @Override
  public void reset() {
    super.reset();
    mIsInvokedBegin = false;
    mIsInvokedStart = false;
    mIsInvokedEnd = false;
  }

  @Override
  protected void onBegin(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnBeginEnable() || mIsInvokedBegin) {
      return;
    }
    mIsInvokedBegin = true;
    sendGestureEvent(GestureConstants.ON_BEGIN, getEventParamsInActive(x, y));
  }

  @Override
  protected void onUpdate(float deltaX, float deltaY, @Nullable LynxTouchEvent event) {
    if (!isOnUpdateEnable()) {
      return;
    }
    sendGestureEvent(GestureConstants.ON_UPDATE, getEventParamsInActive(deltaX, deltaY));
  }

  @Override
  protected void onStart(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnStartEnable() || mIsInvokedStart || !mIsInvokedBegin) {
      return;
    }
    mIsInvokedStart = true;
    sendGestureEvent(GestureConstants.ON_START, getEventParamsInActive(x, y));
  }

  @Override
  protected void onEnd(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnEndEnable() || mIsInvokedEnd || !mIsInvokedBegin) {
      return;
    }
    mIsInvokedEnd = true;
    sendGestureEvent(GestureConstants.ON_END, getEventParamsInActive(x, y));
  }
}
