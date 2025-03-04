// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture;

import com.lynx.react.bridge.ReadableMap;

public interface LynxNewGestureDelegate {
  /**
   * When the gesture wins, it wins the gesture, and handles the gesture
   */
  int STATE_ACTIVE = 1;
  /**
   * When the gesture fails, it will abandon the current gesture and the gesture response will be
   * transferred to the next competitor
   */
  int STATE_FAIL = 2;
  /**
   * When the gesture ends, the current gesture response will be terminated directly, and the
   * re-competition logic will not be executed
   */
  int STATE_END = 3;

  /**
   * Set the state of the gesture detector to the specified state.
   *
   * @param gestureId
   * @param state     An integer value of type GestureState representing the state to set the
   *     gesture
   *                  detector to. Must be one of the following values defined by the GestureState
   * enumeration: GestureState::ACTIVE (1) GestureState::FAIL (2) GestureState::END (3)
   * @note It is important to implement this method in any class that conforms to the
   * LynxNewGestureDelegate interface to ensure the proper functioning of the gesture detector.
   */
  void setGestureDetectorState(int gestureId, int state);

  /**
   * Handle whether internal lynxUI of the current gesture node consume the gesture and whether
   * native view outside the current node (outside of lynxView) consume the gesture.
   *
   * @param gestureId The identifier of the specific native gesture.
   * @param params {internal: boolean, isConsume: boolean, ...}
   */
  void consumeGesture(int gestureId, ReadableMap params);

  /**
   * Scrolls the content by the specified amount in the x and y directions.
   *
   * @param deltaX The amount to scroll in the x direction.
   * @param deltaY The amount to scroll in the y direction.
   *
   * @return An array containing the new x and y positions of the content after scrolling.
   */
  float[] scrollBy(float deltaX, float deltaY);
}
