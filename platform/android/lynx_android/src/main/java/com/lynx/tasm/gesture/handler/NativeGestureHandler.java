// Copyright 2023 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.detector.GestureDetector;

/**
 * The NativeGestureHandler class is a concrete implementation of the BaseGestureHandler class
 * for handling native gestures. It handles native gestures on android, iOS and other platforms
 */
public class NativeGestureHandler extends PanGestureHandler {
  /**
   * Constructs a NativeGestureHandler object with the specified properties.
   *
   * @param sign               the sign indicating the direction of the gesture
   * @param lynxContext        the LynxContext associated with the gesture handler
   * @param gestureDetector    the GestureDetector associated with the gesture handler
   * @param gestureArenaMember the GestureArenaMember associated with the gesture handler
   */
  public NativeGestureHandler(int sign, LynxContext lynxContext,
      @NonNull GestureDetector gestureDetector, GestureArenaMember gestureArenaMember) {
    super(sign, lynxContext, gestureDetector, gestureArenaMember);
  }
}
