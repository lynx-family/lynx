// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

import android.view.MotionEvent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.detector.GestureDetector;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * BaseGestureHandler is an abstract class that serves as the base for implementing gesture
 handlers.
 * It provides common functionality and defines abstract methods to handle gesture events.

 * Gesture handlers are responsible for processing touch events and generating corresponding
 * gesture events based on the gesture type and user interactions.

 * The class includes methods for converting gesture detectors to gesture handlers, handling enable
 * gesture callbacks, sending gesture events, and retrieving event parameters from touch events.

 * Subclasses of BaseGestureHandler must implement the abstract methods to handle specific types
 * of gestures such as PanGestureHandler, FlingGestureHandler... and define their behavior when the
 gesture begins, updates, and ends.
 *
 * @see GestureDetector
 * @see GestureArenaMember
 */
public abstract class BaseGestureHandler {
  // LynxUI key
  protected int mSign;

  protected int mStatus = GestureConstants.LYNX_STATE_INIT; // 0 - INIT / 1 - BEGIN / 2 - ACTIVE / 3
                                                            // - FAIL / 4 - END / 5 - UNDETERMINED

  protected final LynxContext mLynxContext;
  protected final Map<String, Boolean> mEnableFlags;
  protected final GestureDetector mGestureDetector;

  protected final GestureArenaMember mGestureArenaMember;

  public BaseGestureHandler(int sign, LynxContext lynxContext,
      @NonNull GestureDetector gestureDetector, @NonNull GestureArenaMember gestureArenaMember) {
    this.mSign = sign;
    this.mLynxContext = lynxContext;
    this.mGestureDetector = gestureDetector;
    this.mGestureArenaMember = gestureArenaMember;
    mEnableFlags = new HashMap<>();
    handleEnableGestureCallback(gestureDetector.getGestureCallbackNames());
  }

  /**
   * Converts gesture detectors to gesture handlers and returns a map of gesture handlers.
   *
   * @param sign              The LynxUI key.
   * @param lynxContext       The Lynx context.
   * @param member            The gesture arena member.
   * @param gestureDetectors  A map of gesture detectors.
   * @return                  A map of gesture handlers.
   */
  public static Map<Integer, BaseGestureHandler> convertToGestureHandler(int sign,
      LynxContext lynxContext, GestureArenaMember member,
      Map<Integer, GestureDetector> gestureDetectors) {
    // Create an empty map to store the gesture handlers
    Map<Integer, BaseGestureHandler> gestureHandlerMap = new HashMap<>();
    // Iterate over the gesture detectors
    for (int i : gestureDetectors.keySet()) {
      GestureDetector detector = gestureDetectors.get(i);
      // Skip if the gesture detector is null
      if (detector == null) {
        continue;
      }
      if (detector.getGestureType() == GestureDetector.GESTURE_TYPE_PAN) {
        gestureHandlerMap.put(
            detector.getGestureType(), new PanGestureHandler(sign, lynxContext, detector, member));
      } else if (detector.getGestureType() == GestureDetector.GESTURE_TYPE_DEFAULT) {
        gestureHandlerMap.put(detector.getGestureType(),
            new DefaultGestureHandler(sign, lynxContext, detector, member));
      } else if (detector.getGestureType() == GestureDetector.GESTURE_TYPE_FLING) {
        gestureHandlerMap.put(detector.getGestureType(),
            new FlingGestureHandler(sign, lynxContext, detector, member));
      } else if (detector.getGestureType() == GestureDetector.GESTURE_TYPE_TAP) {
        gestureHandlerMap.put(
            detector.getGestureType(), new TapGestureHandler(sign, lynxContext, detector, member));
      } else if (detector.getGestureType() == GestureDetector.GESTURE_TYPE_LONG_PRESS) {
        gestureHandlerMap.put(detector.getGestureType(),
            new LongPressGestureHandler(sign, lynxContext, detector, member));
      } else if (detector.getGestureType() == GestureDetector.GESTURE_TYPE_NATIVE) {
        gestureHandlerMap.put(detector.getGestureType(),
            new NativeGestureHandler(sign, lynxContext, detector, member));
      }
    }
    return gestureHandlerMap;
  }

  /**
   * Handles enable gesture callback by setting enable flags for specific callback names.
   *
   * @param callbackNames  The list of callback names.
   */
  private void handleEnableGestureCallback(List<String> callbackNames) {
    // Set initial enable flags for all callback names to false
    mEnableFlags.put(GestureConstants.ON_TOUCHES_DOWN, false);
    mEnableFlags.put(GestureConstants.ON_TOUCHES_MOVE, false);
    mEnableFlags.put(GestureConstants.ON_TOUCHES_UP, false);
    mEnableFlags.put(GestureConstants.ON_TOUCHES_CANCEL, false);
    mEnableFlags.put(GestureConstants.ON_BEGIN, false);
    mEnableFlags.put(GestureConstants.ON_UPDATE, false);
    mEnableFlags.put(GestureConstants.ON_START, false);
    mEnableFlags.put(GestureConstants.ON_END, false);

    // Check if the callbackNames list is not null
    if (callbackNames != null) {
      // Iterate over the callback names
      for (String callback : callbackNames) {
        if (mEnableFlags.containsKey(callback)) {
          mEnableFlags.put(callback, true);
        }
      }
    }
  }

  /**
   * handle gesture config in specification gesture detector
   * @param config The custom config map of gesture
   */
  protected abstract void handleConfigMap(@Nullable ReadableMap config);

  public void handleMotionEvent(@Nullable MotionEvent event,
      @Nullable LynxTouchEvent lynxTouchEvent, float deltaX, float deltaY) {
    onHandle(event, lynxTouchEvent, deltaX, deltaY);
  }

  /**
   * handle touchEvent and determine whether active or not
   * @param event The origin MotionEvent
   */
  protected abstract void onHandle(@Nullable MotionEvent event,
      @Nullable LynxTouchEvent lynxTouchEvent, float flingDeltaX, float flingDeltaY);

  /**
   * Checks if the current gesture is called end.
   * @return if end or not
   */
  protected boolean isEnd() {
    return mStatus == GestureConstants.LYNX_STATE_END;
  }

  /**
   * Checks if the current gesture is activated.
   *
   * @return {@code true} if the gesture can be activated, {@code false} otherwise.
   */

  protected boolean isActive() {
    return mStatus == GestureConstants.LYNX_STATE_ACTIVE;
  }

  /**
   * Get gesture status
   * status: 0 - INIT / 1 - ACTIVE / 2 - FAIL / 3 - END
   * @return
   */
  protected int getGestureStatus() {
    return mStatus;
  }

  /**
   * Sends a gesture event with the specified event name and parameters.
   *
   * @param eventName   The name of the gesture event.
   * @param eventParams The parameters associated with the gesture event.
   */
  protected void sendGestureEvent(String eventName, HashMap<String, Object> eventParams) {
    if (mGestureDetector == null) {
      return;
    }
    LynxCustomEvent gestureEvent = new LynxCustomEvent(mSign, eventName, eventParams);
    mLynxContext.getEventEmitter().sendGestureEvent(mGestureDetector.getGestureID(), gestureEvent);
  }

  protected boolean isOnBeginEnable() {
    return mEnableFlags.get(GestureConstants.ON_BEGIN);
  }
  protected boolean isOnUpdateEnable() {
    return mEnableFlags.get(GestureConstants.ON_UPDATE);
  }

  protected boolean isOnStartEnable() {
    return mEnableFlags.get(GestureConstants.ON_START);
  }

  protected boolean isOnEndEnable() {
    return mEnableFlags.get(GestureConstants.ON_END);
  }
  /**
   * Retrieves the event parameters from the given touch event.
   *
   * @param touchEvent The touch event to extract the parameters from.
   * @return A HashMap containing the extracted event parameters.
   */
  protected HashMap<String, Object> getEventParamsFromTouchEvent(
      @Nullable LynxTouchEvent touchEvent) {
    HashMap<String, Object> params = new HashMap<>();
    if (touchEvent != null) {
      params.put("timestamp", System.currentTimeMillis());
      params.put("type", touchEvent.getName());
      if (touchEvent.getViewPoint() != null) {
        params.put("x", px2dip(touchEvent.getViewPoint().getX()));
        params.put("y", px2dip(touchEvent.getViewPoint().getY()));
      }
      if (touchEvent.getPagePoint() != null) {
        params.put("pageX", px2dip(touchEvent.getPagePoint().getX()));
        params.put("pageY", px2dip(touchEvent.getPagePoint().getY()));
      }
      if (touchEvent.getClientPoint() != null) {
        params.put("clientX", px2dip(touchEvent.getClientPoint().getX()));
        params.put("clientY", px2dip(touchEvent.getClientPoint().getY()));
      }
    }
    return params;
  }

  /**
   * Converts a pixel value to density-independent pixels (dip).
   *
   * @param pxValue The pixel value to convert.
   * @return  The converted value in density-independent pixels.
   */
  protected int px2dip(float pxValue) {
    LynxContext context = mLynxContext;
    if (context == null || context.getResources() == null
        || context.getResources().getDisplayMetrics() == null) {
      return (int) pxValue;
    }
    float scale = context.getResources().getDisplayMetrics().density;
    return (int) (pxValue / scale + 0.5);
  }

  /**
   * activate the gesture handler to active state.
   */
  public void activate() {
    mStatus = GestureConstants.LYNX_STATE_ACTIVE;
  }

  /**
   * Resets the gesture handler to init state.
   */
  public void reset() {
    mStatus = GestureConstants.LYNX_STATE_INIT;
  }

  /**
   * Fails the gesture handler, deactivating it and triggering the "onEnd" callback with the given
   * coordinates and touch event.
   */
  public void fail() {
    mStatus = GestureConstants.LYNX_STATE_FAIL;
  }

  /**
   * make the gesture state to begin
   */
  public void begin() {
    mStatus = GestureConstants.LYNX_STATE_BEGIN;
  }

  /**
   * make the gesture state to ignore
   */
  public void ignore() {
    mStatus = GestureConstants.LYNX_STATE_UNDETERMINED;
  }

  /**
   * End the gesture handler, deactivating it and triggering the "onEnd" callback, end current
   * gesture, make winner to null.
   */
  public void end() {
    mStatus = GestureConstants.LYNX_STATE_END;
  }

  /**
   * Handles the "onTouchesDown" event by sending the gesture event if enabled.
   *
   * @param touchEvent The touch event associated with the "onTouchesDown" event.
   */
  public void onTouchesDown(LynxTouchEvent touchEvent) {
    if (mEnableFlags.get(GestureConstants.ON_TOUCHES_DOWN)) {
      sendGestureEvent(GestureConstants.ON_TOUCHES_DOWN, getEventParamsFromTouchEvent(touchEvent));
    }
  }

  /**
   * Handles the "onTouchesMove" event by sending the gesture event if enabled.
   *
   * @param touchEvent The touch event associated with the "onTouchesMove" event.
   */
  public void onTouchesMove(LynxTouchEvent touchEvent) {
    if (mEnableFlags.get(GestureConstants.ON_TOUCHES_MOVE)) {
      sendGestureEvent(GestureConstants.ON_TOUCHES_MOVE, getEventParamsFromTouchEvent(touchEvent));
    }
  }

  /**
   * Handles the "onTouchesUp" event by sending the gesture event if enabled.
   *
   * @param touchEvent The touch event associated with the "onTouchesUp" event.
   */

  public void onTouchesUp(LynxTouchEvent touchEvent) {
    if (mEnableFlags.get(GestureConstants.ON_TOUCHES_UP)) {
      sendGestureEvent(GestureConstants.ON_TOUCHES_UP, getEventParamsFromTouchEvent(touchEvent));
    }
  }

  /**
   * Handles the "onTouchesCancel" event by sending the gesture event if enabled.
   *
   * @param touchEvent The touch event associated with the "onTouchesCancel" event.
   */
  public void onTouchesCancel(LynxTouchEvent touchEvent) {
    if (mEnableFlags.get(GestureConstants.ON_TOUCHES_CANCEL)) {
      sendGestureEvent(
          GestureConstants.ON_TOUCHES_CANCEL, getEventParamsFromTouchEvent(touchEvent));
    }
  }

  /**
   * return gesture detector in handler
   * @return gesture detector
   */
  @NonNull
  protected GestureDetector getGestureDetector() {
    return mGestureDetector;
  }

  /**
   * Called when the gesture begins at the given coordinates.
   *
   * @param x The X-coordinate at the beginning of the gesture.
   * @param y The Y-coordinate at the beginning of the gesture.
   * @param event The touch event associated with the beginning of the gesture.
   */
  protected abstract void onBegin(float x, float y, @Nullable LynxTouchEvent event);

  /**
   * Called when the gesture is updated with the given delta values.
   *
   * @param deltaX The delta value on the X-axis.
   * @param deltaY The delta value on the Y-axis.
   * @param event The touch event associated with the update.
   */
  protected abstract void onUpdate(float deltaX, float deltaY, @Nullable LynxTouchEvent event);

  /**
   * Called when the gesture is recognized by the handler and it transitions to the active state.
   * @param x The X-coordinate at the beginning of the gesture activated.
   * @param y The Y-coordinate at the beginning of the gesture activated.
   * @param event The touch event associated with the beginning of the gesture activated.
   */
  protected abstract void onStart(float x, float y, @Nullable LynxTouchEvent event);

  /**
   * Called when the gesture ends at the given coordinates.
   *
   * @param x The X-coordinate at the end of the gesture.
   * @param y The Y-coordinate at the end of the gesture.
   * @param event The touch event associated with the end of the gesture.
   */
  protected abstract void onEnd(float x, float y, @Nullable LynxTouchEvent event);
}
