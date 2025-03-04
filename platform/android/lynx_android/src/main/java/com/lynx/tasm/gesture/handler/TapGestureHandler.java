// Copyright 2023 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.utils.PixelUtils;

public class TapGestureHandler extends BaseGestureHandler {
  // Maximum distance in X and Y axis,  If the finger travels further than the defined distance
  // along the X axis and the gesture hasn't yet activated, it will fail to recognize the gesture.
  private float mMaxDistance = PixelUtils.dipToPx(10);
  // Maximum time, expressed in milliseconds, that defines how fast a finger must be released after
  // a touch. The default value is 500.
  private long mMaxDuration = 500L;

  // record x axis when action down
  private float mStartX = 0f;
  // record y axis when action down
  private float mStartY = 0f;
  // record x axis when action move / up
  private float mLastX = 0f;
  // record x axis when action move / up
  private float mLastY = 0f;
  // is onEnd invoked or not
  private boolean mIsInvokedEnd = false;

  // record lynx touch event
  private LynxTouchEvent mLynxTouchEvent = null;

  private Handler mHandler = null;

  private final Runnable mDelayFailRunnable = this::fail;

  public TapGestureHandler(int sign, LynxContext lynxContext,
      @NonNull GestureDetector gestureDetector, @NonNull GestureArenaMember gestureArenaMember) {
    super(sign, lynxContext, gestureDetector, gestureArenaMember);
    handleConfigMap(gestureDetector.getConfigMap());
  }

  @Override
  protected void handleConfigMap(@Nullable ReadableMap config) {
    if (config == null) {
      return;
    }
    mMaxDuration = config.getLong(GestureConstants.MAX_DURATION, 500);
    mMaxDistance = PixelUtils.dipToPx(config.getLong(GestureConstants.MAX_DISTANCE, 10));
  }

  @Override
  protected void onHandle(@Nullable MotionEvent event, @Nullable LynxTouchEvent lynxTouchEvent,
      float flingDeltaX, float flingDeltaY) {
    mLynxTouchEvent = lynxTouchEvent;
    // If the event is empty, it means the finger not touches the screen
    if (event == null) {
      ignore();
      endTap();
      return;
    }
    if (mStatus >= GestureConstants.LYNX_STATE_FAIL) {
      endTap();
      return;
    }
    switch (event.getActionMasked()) {
      case MotionEvent.ACTION_DOWN:
        mStartX = event.getX();
        mStartY = event.getY();
        mIsInvokedEnd = false;
        begin();
        onBegin(mStartX, mStartY, lynxTouchEvent);
        startTap();
        break;
      case MotionEvent.ACTION_MOVE:
        mLastX = event.getX();
        mLastY = event.getY();
        if (shouldFail()) {
          fail();
        }
        break;
      case MotionEvent.ACTION_UP:
        mLastX = event.getX();
        mLastY = event.getY();
        if (mStatus >= GestureConstants.LYNX_STATE_FAIL) {
          fail();
        } else {
          activate();
          onStart(mLastX, mLastY, lynxTouchEvent);
          onEnd(mLastX, mLastY, lynxTouchEvent);
        }
        endTap();
        break;
      default:
        break;
    }
  }

  @Override
  public void fail() {
    if (mStatus != GestureConstants.LYNX_STATE_FAIL) {
      mStatus = GestureConstants.LYNX_STATE_FAIL;
      onEnd(mLastX, mLastY, mLynxTouchEvent);
    }
  }

  @Override
  public void end() {
    if (mStatus != GestureConstants.LYNX_STATE_END) {
      mStatus = GestureConstants.LYNX_STATE_END;
      onEnd(mLastX, mLastY, mLynxTouchEvent);
    }
  }

  @Override
  public void reset() {
    super.reset();
    mIsInvokedEnd = false;
  }

  private void startTap() {
    if (mHandler == null) {
      mHandler = new Handler(Looper.getMainLooper());
    } else {
      mHandler.removeCallbacksAndMessages(null);
    }
    mHandler.postDelayed(mDelayFailRunnable, mMaxDuration);
  }

  private void endTap() {
    if (mHandler != null) {
      mHandler.removeCallbacksAndMessages(null);
      mHandler = null;
    }
  }

  private boolean shouldFail() {
    float dx = Math.abs(mLastX - mStartX);
    float dy = Math.abs(mLastY - mStartY);
    if (dx > mMaxDistance || dy > mMaxDistance) {
      return true;
    }
    return false;
  }

  @Override
  protected void onBegin(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnBeginEnable()) {
      return;
    }
    sendGestureEvent(GestureConstants.ON_BEGIN, getEventParamsFromTouchEvent(event));
  }

  @Override
  protected void onUpdate(float deltaX, float deltaY, @Nullable LynxTouchEvent event) {
    // empty implementation, because tap gesture is not continuous gesture
  }

  @Override
  protected void onStart(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnStartEnable()) {
      return;
    }
    sendGestureEvent(GestureConstants.ON_START, getEventParamsFromTouchEvent(event));
  }

  @Override
  protected void onEnd(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnEndEnable() || mIsInvokedEnd) {
      return;
    }
    mIsInvokedEnd = true;
    sendGestureEvent(GestureConstants.ON_END, getEventParamsFromTouchEvent(event));
  }
}
