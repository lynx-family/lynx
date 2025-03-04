// Copyright 2023 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

public class GestureConstants {
  // Gesture event names
  public final static String ON_TOUCHES_DOWN = "onTouchesDown";
  public final static String ON_TOUCHES_MOVE = "onTouchesMove";
  public final static String ON_TOUCHES_UP = "onTouchesUp";
  public final static String ON_TOUCHES_CANCEL = "onTouchesCancel";
  // Set the callback that is being called when given gesture handler starts receiving touches. At
  // the moment of this callback the handler is not yet in an active state and we don't know yet if
  // it will recognize the gesture at all.
  public final static String ON_BEGIN = "onBegin";
  // Set the callback that is being called every time the gesture receives an update while it's
  // active.
  public final static String ON_UPDATE = "onUpdate";
  // Set the callback that is being called when the gesture that was recognized by the handler
  // finishes. It will be called only if the handler was previously in the active state.
  public final static String ON_END = "onEnd";
  // Set the callback that is being called when the gesture is recognized by the handler and it
  // transitions to the active state.
  public final static String ON_START = "onStart";

  // Gesture Configs
  public static final String MIN_DURATION = "minDuration";
  public static final String MAX_DURATION = "maxDuration";
  public static final String MIN_DISTANCE = "minDistance";
  public static final String MAX_DISTANCE = "maxDistance";

  // Gesture Status
  public static final int LYNX_STATE_INIT = 0;
  public static final int LYNX_STATE_BEGIN = 1;
  public static final int LYNX_STATE_ACTIVE = 2;
  public static final int LYNX_STATE_FAIL = 3;
  public static final int LYNX_STATE_END = 4;
  // not trigger in current event, for example, flingGesture will not trigger when touching the
  // screen.
  public static final int LYNX_STATE_UNDETERMINED = 5;

  static final int MIN_SCROLL = Integer.MIN_VALUE;
  static final int MAX_SCROLL = Integer.MAX_VALUE;
  static final int FLING_SPEED_THRESHOLD = 300;
}
