// Copyright 2024 The Lynx Authors. All rights reserved.
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

public class LongPressGestureHandler extends BaseGestureHandler {
  // Minimum time, expressed in milliseconds, that a finger must remain pressed on the corresponding
  // view. The default value is 500.
  private long mMinDuration = 500L;
  // Maximum distance in X and Y axis,  If the finger travels further than the defined distance
  // along the X axis and the gesture hasn't yet activated, it will fail to recognize the gesture.
  private float mMaxDistance = PixelUtils.dipToPx(10);

  private Handler mHandler = null;

  // record x axis when action down
  private float mStartX = 0f;
  // record y axis when action down
  private float mStartY = 0f;
  // record x axis when action up
  private float mLastX = 0f;
  // record x axis when action up
  private float mLastY = 0f;
  // record lynx touch event
  private LynxTouchEvent mLynxTouchEvent = null;
  // is onEnd invoked or not
  private boolean mIsInvokedEnd = false;

  private final Runnable mDelayActivateRunnable = () -> {
    if (mStatus != GestureConstants.LYNX_STATE_FAIL
        && mStatus != GestureConstants.LYNX_STATE_ACTIVE) {
      activate();
      onStart(mLastX, mLastY, mLynxTouchEvent);
    }
  };

  public LongPressGestureHandler(int sign, LynxContext lynxContext,
      @NonNull GestureDetector gestureDetector, @NonNull GestureArenaMember gestureArenaMember) {
    super(sign, lynxContext, gestureDetector, gestureArenaMember);
    handleConfigMap(gestureDetector.getConfigMap());
  }

  @Override
  protected void handleConfigMap(@Nullable ReadableMap config) {
    if (config == null) {
      return;
    }
    mMinDuration = config.getLong(GestureConstants.MIN_DURATION, 500);
    mMaxDistance = PixelUtils.dipToPx(config.getLong(GestureConstants.MAX_DISTANCE, 10));
  }

  private void startLongPress() {
    if (mHandler == null) {
      mHandler = new Handler(Looper.getMainLooper());
    } else {
      mHandler.removeCallbacksAndMessages(null);
    }
    mHandler.postDelayed(mDelayActivateRunnable, mMinDuration);
  }

  private void endLongPress() {
    if (mHandler != null) {
      mHandler.removeCallbacksAndMessages(null);
      mHandler = null;
    }
  }

  @Override
  protected void onHandle(@Nullable MotionEvent event, @Nullable LynxTouchEvent lynxTouchEvent,
      float flingDeltaX, float flingDeltaY) {
    mLynxTouchEvent = lynxTouchEvent;
    // If the event is empty, it means the finger not touches the screen
    if (event == null) {
      ignore();
      return;
    }

    if (mStatus >= GestureConstants.LYNX_STATE_FAIL) {
      endLongPress();
      return;
    }
    switch (event.getActionMasked()) {
      case MotionEvent.ACTION_DOWN:
        mStartX = event.getX();
        mStartY = event.getY();
        mIsInvokedEnd = false;
        begin();
        onBegin(mStartX, mStartY, lynxTouchEvent);
        startLongPress();
        break;
      case MotionEvent.ACTION_MOVE:
        mLastX = event.getX();
        mLastY = event.getY();
        if (shouldFail()) {
          fail();
          endLongPress();
        }
        break;
      case MotionEvent.ACTION_UP:
        endLongPress();
        fail();
        break;
      default:
        break;
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

  @Override
  protected void onBegin(float x, float y, @Nullable LynxTouchEvent event) {
    if (!isOnBeginEnable()) {
      return;
    }
    sendGestureEvent(GestureConstants.ON_BEGIN, getEventParamsFromTouchEvent(event));
  }

  @Override
  protected void onUpdate(float deltaX, float deltaY, @Nullable LynxTouchEvent event) {
    // empty implementation, because long press gesture is not continuous gesture
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
