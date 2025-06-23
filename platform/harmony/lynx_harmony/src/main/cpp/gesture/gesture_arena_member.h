// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_GESTURE_ARENA_MEMBER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_GESTURE_ARENA_MEMBER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "core/renderer/utils/base/base_def.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/base_gesture_handler.h"

namespace lynx {
namespace tasm {
namespace harmony {

class GestureArenaMember {
 public:
  /**
   * Called when the gesture should be scrolled by the specified delta values.
   *
   * @param delta_x The delta value for scrolling in the x-axis.
   * @param delta_y The delta value for scrolling in the y-axis.
   */
  virtual std::vector<float> ScrollBy(float delta_x, float delta_y) = 0;

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
  bool operator()(const std::weak_ptr<GestureArenaMember>& lhs,
                  const std::weak_ptr<GestureArenaMember>& rhs) const {
    auto lhs_lock = lhs.lock();
    auto rhs_lock = rhs.lock();
    if (!lhs_lock || !rhs_lock) {
      return lhs.owner_before(rhs);
    }
    return lhs_lock->GestureArenaMemberId() < rhs_lock->GestureArenaMemberId();
  }
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_GESTURE_ARENA_MEMBER_H_
