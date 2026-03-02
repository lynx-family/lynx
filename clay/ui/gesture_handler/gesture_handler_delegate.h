// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DELEGATE_H_
#define CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DELEGATE_H_

#include <vector>

#include "clay/public/value.h"

namespace clay {

class GestureHandlerDelegate {
 public:
  enum class LynxGestureState : unsigned int {
    ACTIVE = 1,
    FAIL = 2,
    END = 3,
  };
  /**
   * Set the state of the gesture detector to the specified state.
   *
   * @param gestureId The identifier of the specific native gesture.
   * @param state     An integer value of type GestureState representing the
   * state to set the gesture detector to.
   */
  virtual void SetGestureDetectorState(int gesture_id, int state) = 0;

  /**
   * Handle whether internal lynxUI of the current gesture node consume the
   * gesture and whether native view outside the current node (outside of
   * lynxView) consume the gesture.
   *
   * @param gestureId The identifier of the specific native gesture.
   * @param params    A map containing parameters, such as {internal: true,
   * isConsume: true}
   */
  virtual void ConsumeGesture(int gesture_id, const Value& params) = 0;

  /**
   * Scrolls the content by the specified amount in the x and y directions.
   *
   * @param deltaX The amount to scroll in the x direction.
   * @param deltaY The amount to scroll in the y direction.
   *
   * @return A vector containing the new x and y positions of the content after
   * scrolling.
   */
  virtual std::vector<float> ScrollBy(float delta_x, float delta_y) = 0;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DELEGATE_H_
