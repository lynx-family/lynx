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
 * The DefaultGestureHandler class is a concrete implementation of the BaseGestureHandler class
 * for handling default gestures. It handle default pan and fling gesture for scrolling component,
 * which provides aligned scroll rate and decay curves on android and iOS.
 */
public class DefaultGestureHandler extends BaseGestureHandler {
  private final HashMap<String, Object> mEventParams;

  // record x axis when last action
  private float mLastX = 0f;
  // record x axis when last action
  private float mLastY = 0f;
  // is onBegin invoked or not
  private boolean mIsInvokedBegin = false;
  // is onStart invoked or not
  private boolean mIsInvokedStart = false;
  // is onEnd invoked or not
  private boolean mIsInvokedEnd = false;

  // record last lynx touch event
  private LynxTouchEvent mLastTouchEvent;

  /**
   * Constructs a DefaultGestureHandler object with the specified properties.
   *
   * @param sign                the sign indicating the direction of the gesture
   * @param lynxContext         the LynxContext associated with the gesture handler
   * @param gestureDetector     the GestureDetector associated with the gesture handler
   * @param gestureArenaMember  the GestureArenaMember associated with the gesture handler
   */
  public DefaultGestureHandler(int sign, LynxContext lynxContext,
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
    mLastTouchEvent = lynxTouchEvent;
    if (mStatus >= GestureConstants.LYNX_STATE_FAIL) {
      onEnd(mLastX, mLastY, mLastTouchEvent);
      return;
    }
    if (event != null) {
      // in pan status
      switch (event.getActionMasked()) {
        case MotionEvent.ACTION_DOWN:
          mLastX = event.getX();
          mLastY = event.getY();
          mIsInvokedEnd = false;
          begin();
          onBegin(mLastX, mLastY, lynxTouchEvent);
          break;
        case MotionEvent.ACTION_MOVE:
          float deltaX = mLastX - event.getX();
          float deltaY = mLastY - event.getY();
          if (mStatus == GestureConstants.LYNX_STATE_INIT) {
            onBegin(mLastX, mLastY, lynxTouchEvent);
            if (mStatus <= GestureConstants.LYNX_STATE_BEGIN) {
              activate();
            }
          } else {
            if (shouldFail(deltaX, deltaY)) {
              // consume last delta to arrive start or end
              onUpdate(deltaX, deltaY, lynxTouchEvent);
              fail();
              onEnd(mLastX, mLastY, lynxTouchEvent);
            } else {
              activate();
              onUpdate(deltaX, deltaY, lynxTouchEvent);
            }
          }
          mLastX = event.getX();
          mLastY = event.getY();
          break;
        case MotionEvent.ACTION_UP:
          if (mStatus == GestureConstants.LYNX_STATE_ACTIVE && flingDeltaX == Float.MIN_VALUE
              && flingDeltaY == Float.MIN_VALUE) {
            fail();
            onEnd(0, 0, null);
          }
          break;
        default:
          break;
      }
    } else {
      if (mGestureArenaMember != null) {
        mGestureArenaMember.onInvalidate();
      }
      // in fling status
      if (mStatus == GestureConstants.LYNX_STATE_ACTIVE && flingDeltaX == Float.MIN_VALUE
          && flingDeltaY == Float.MIN_VALUE) {
        fail();
        onEnd(0, 0, null);
        return;
      }

      if (shouldFail(flingDeltaX, flingDeltaY)) {
        onUpdate(flingDeltaX, flingDeltaY, null);
        fail();
        onEnd(flingDeltaX, flingDeltaY, null);
      } else {
        if (mStatus == GestureConstants.LYNX_STATE_INIT) {
          onBegin(mLastX, mLastY, lynxTouchEvent);
          if (mStatus <= GestureConstants.LYNX_STATE_BEGIN) {
            activate();
          }
          return;
        }
        onUpdate(flingDeltaX, flingDeltaY, null);
      }
      if (mGestureArenaMember != null) {
        mGestureArenaMember.onInvalidate();
      }
    }
  }

  private boolean shouldFail(float deltaX, float deltaY) {
    return !mGestureArenaMember.canConsumeGesture(deltaX, deltaY);
  }

  protected HashMap<String, Object> getEventParamsInActive(float deltaX, float deltaY) {
    // Set the event parameters for the active state
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
    if (mLastTouchEvent == null) {
      onEnd(0, 0, null);
    } else {
      onEnd(mLastX, mLastY, mLastTouchEvent);
    }
  }

  @Override
  public void activate() {
    super.activate();
  }

  @Override
  public void end() {
    super.end();
    if (mLastTouchEvent == null) {
      onEnd(0, 0, null);
    } else {
      onEnd(mLastX, mLastY, mLastTouchEvent);
    }
  }

  @Override
  public void reset() {
    super.reset();
    mLastX = 0.0f;
    mLastY = 0.0f;
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
    sendGestureEvent(GestureConstants.ON_BEGIN, getEventParamsInActive(0, 0));
  }

  @Override
  protected void onUpdate(float deltaX, float deltaY, @Nullable LynxTouchEvent event) {
    if (mGestureArenaMember != null) {
      mGestureArenaMember.onGestureScrollBy(deltaX, deltaY);
    }
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
    sendGestureEvent(GestureConstants.ON_END, getEventParamsInActive(0, 0));
  }
}
