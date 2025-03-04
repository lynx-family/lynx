// Copyright 2024 The Lynx Authors. All rights reserved.
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
import com.lynx.tasm.utils.PixelUtils;
import java.util.HashMap;

/**
 * The PanGestureHandler class is a concrete implementation of the BaseGestureHandler class
 * for handling pan gestures. It handles pan gesture for component, triggered when a finger touches
 * the component.
 */
public class PanGestureHandler extends BaseGestureHandler {
  private final HashMap<String, Object> mEventParams;

  // Min distance in X and Y axis,  If the finger travels further than the defined distance
  // along the X axis and the gesture will activated.
  // otherwise assigned to 0
  private float mMinDistance = PixelUtils.dipToPx(0);

  // record x axis when action down
  private float mStartX = 0f;
  // record y axis when action down
  private float mStartY = 0f;
  // record x axis when action move / up
  private float mLastX = 0f;
  // record x axis when action move / up
  private float mLastY = 0f;

  // record last lynx touch event
  private LynxTouchEvent mLastTouchEvent;

  // is onBegin invoked or not
  private boolean mIsInvokedBegin = false;

  // is onStart invoked or not
  private boolean mIsInvokedStart = false;

  // is onEnd invoked or not
  private boolean mIsInvokedEnd = false;

  /**
   * Constructs a PanGestureHandler object with the specified properties.
   *
   * @param sign                the sign indicating the direction of the gesture
   * @param lynxContext         the LynxContext associated with the gesture handler
   * @param gestureDetector     the GestureDetector associated with the gesture handler
   * @param gestureArenaMember  the GestureArenaMember associated with the gesture handler
   */
  public PanGestureHandler(int sign, LynxContext lynxContext,
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
    mMinDistance = PixelUtils.dipToPx(config.getLong(GestureConstants.MIN_DISTANCE, 0), 0);
  }

  @Override
  protected void onHandle(@Nullable MotionEvent event, @Nullable LynxTouchEvent lynxTouchEvent,
      float flingDeltaX, float flingDeltaY) {
    mLastTouchEvent = lynxTouchEvent;
    // If the event is empty, it means the finger not touches the screen
    if (event == null) {
      ignore();
      return;
    }
    if (mStatus >= GestureConstants.LYNX_STATE_FAIL) {
      return;
    }
    switch (event.getActionMasked()) {
      case MotionEvent.ACTION_DOWN:
        mStartX = event.getX();
        mStartY = event.getY();
        mIsInvokedEnd = false;
        begin();
        onBegin(mStartX, mStartY, lynxTouchEvent);
        break;
      case MotionEvent.ACTION_MOVE:
        mLastX = event.getX();
        mLastY = event.getY();
        if (mStatus == GestureConstants.LYNX_STATE_INIT) {
          begin();
          onBegin(mLastX, mLastY, lynxTouchEvent);
        }

        if (shouldActive()) {
          onStart(mLastX, mStartY, lynxTouchEvent);
          activate();
        }
        if (mStatus == GestureConstants.LYNX_STATE_ACTIVE) {
          onUpdate(mLastX, mLastY, lynxTouchEvent);
        } else if (mStatus >= GestureConstants.LYNX_STATE_FAIL) {
          onEnd(mLastX, mLastY, lynxTouchEvent);
        }
        break;
      case MotionEvent.ACTION_UP:
        fail();
        onEnd(mLastX, mLastY, lynxTouchEvent);
        break;
      default:
        break;
    }
  }

  private boolean shouldActive() {
    if (mStatus >= GestureConstants.LYNX_STATE_FAIL) {
      return false;
    }
    float dx = Math.abs(mLastX - mStartX);
    float dy = Math.abs(mLastY - mStartY);
    if (dx > mMinDistance || dy > mMinDistance) {
      return true;
    }
    return false;
  }

  @Override
  public void fail() {
    super.fail();
    onEnd(mLastX, mLastY, mLastTouchEvent);
  }

  @Override
  public void end() {
    super.end();
    onEnd(mLastX, mLastY, mLastTouchEvent);
  }

  @Override
  public void reset() {
    super.reset();
    mIsInvokedBegin = false;
    mIsInvokedStart = false;
    mIsInvokedEnd = false;
  }

  protected HashMap<String, Object> getEventParamsInActive(@Nullable LynxTouchEvent event) {
    mEventParams.put("scrollX", px2dip(mGestureArenaMember.getMemberScrollX()));
    mEventParams.put("scrollY", px2dip(mGestureArenaMember.getMemberScrollY()));
    mEventParams.put("isAtStart", mGestureArenaMember.isAtBorder(true));
    mEventParams.put("isAtEnd", mGestureArenaMember.isAtBorder(false));
    mEventParams.putAll(getEventParamsFromTouchEvent(event));
    return mEventParams;
  }

  @Override
  protected void onBegin(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnBeginEnable() || mIsInvokedBegin) {
      return;
    }
    mIsInvokedBegin = true;
    sendGestureEvent(GestureConstants.ON_BEGIN, getEventParamsInActive(event));
  }

  @Override
  protected void onUpdate(float deltaX, float deltaY, @Nullable LynxTouchEvent event) {
    if (!isOnUpdateEnable()) {
      return;
    }
    sendGestureEvent(GestureConstants.ON_UPDATE, getEventParamsInActive(event));
  }

  @Override
  protected void onStart(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnStartEnable() || mIsInvokedStart || !mIsInvokedBegin) {
      return;
    }
    mIsInvokedStart = true;
    sendGestureEvent(GestureConstants.ON_START, getEventParamsInActive(event));
  }

  @Override
  protected void onEnd(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnEndEnable() || mIsInvokedEnd || !mIsInvokedBegin) {
      return;
    }
    mIsInvokedEnd = true;
    sendGestureEvent(GestureConstants.ON_END, getEventParamsInActive(event));
  }
}
