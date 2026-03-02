// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_GESTURE_ARENA_MEMBER_H_
#define CLAY_UI_GESTURE_HANDLER_GESTURE_ARENA_MEMBER_H_

#include <memory>
#include <vector>

#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

namespace clay {

class GestureArenaMember {
 public:
  /**
   * Called when the gesture should be scrolled by the specified delta values.
   *
   * @param delta_x The delta value for scrolling in the x-axis.
   * @param delta_y The delta value for scrolling in the y-axis.
   */
  virtual std::vector<float> GestureScrollBy(float delta_x, float delta_y) = 0;

  /**
   * Checks if the gesture can consume the specified delta values.
   *
   * @param delta_x The delta value for scrolling in the x-axis.
   * @param delta_y The delta value for scrolling in the y-axis.
   * @return True if the gesture can consume the delta values, false otherwise.
   */
  virtual bool CanConsumeGesture(float delta_x, float delta_y) = 0;

  /**
   * Get sign of lynx ui
   * @return
   */
  virtual int Sign() const = 0;

  /**
   * Retrieves the ID of the gesture arena member.
   *
   * @return The ID of the gesture arena member.
   */
  virtual int GestureArenaMemberId() = 0;

  /**
   * Retrieves the scroll position of the member in the x-axis.
   *
   * @return The scroll position in the x-axis.
   */
  virtual float ScrollX() = 0;

  /**
   * get scroll container direction
   * @return 1 —— vertical  -1 —— horizontal 0 —— not scroll container
   */
  virtual int8_t GetScrollContainerDirection() = 0;

  /**
   * Get current scroller container is at border edge or not
   * @param isStart if it is at start or end
   * @return true — at border edge, false — not at border edge
   */
  virtual bool IsAtBorder(bool is_start) = 0;

  /**
   * Retrieves the scroll position of the member in the y-axis.
   *
   * @return The scroll position in the y-axis.
   */
  virtual float ScrollY() = 0;

  /**
   * Retrieves the map of gesture detectors associated with the member.
   *
   * @return The map of gesture detectors, or null if not available.
   */
  virtual const GestureMap& GetGestureDetectorMap() = 0;
  /**
   * Retrieves the map of gesture handler associated with the member.
   *
   * @return The map of gesture handlers, or null if not available.
   * @see GestureDetector key — GestureDetector type value — gesture handler
   */
  virtual const GestureHandlerMap& GetGestureHandlers() = 0;
};

struct GestureArenaMemberCompare {
  bool operator()(const fml::WeakPtr<GestureArenaMember>& lhs,
                  const fml::WeakPtr<GestureArenaMember>& rhs) const {
    if (!lhs || !rhs) {
      return lhs ? true : false;
    }
    return lhs->GestureArenaMemberId() < rhs->GestureArenaMemberId();
  }
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_GESTURE_ARENA_MEMBER_H_
