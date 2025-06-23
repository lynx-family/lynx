// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_GESTURE_HANDLER_TRIGGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_GESTURE_HANDLER_TRIGGER_H_

#include <arkui/ui_input_event.h>

#include <deque>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/fling_scroller.h"

namespace lynx {
namespace tasm {

class GestureDetector;

namespace harmony {

class LynxContext;
class TouchEvent;
class EventTarget;
class GestureArenaMember;
class GestureArenaManager;

// class OverScroller;
class LynxContext;
class TouchEvent;
class GestureDetectorManager;
class GestureArenaMember;
class GestureArenaManager;
class BaseGestureHandler;

/**
 * This class represents a Gesture Handler Trigger that manages touch gestures
 * and dispatches events to appropriate gesture handlers. It facilitates the
 * recognition and handling of touch events and manages the state of the active
 * gestures. The class coordinates interactions between various gesture
 * detectors and their associated handlers.
 *
 * The GestureHandlerTrigger is responsible for identifying the current winner
 * of the touch event, updating simultaneous winners, computing scrolls, and
 * dispatching events to the respective gesture handlers based on the type of
 * gesture and event.
 *
 * The class maintains a list of GestureArenaMembers to compete with and handles
 * the bubbling of touch events to the corresponding gesture handlers.
 *
 * This class is typically used in conjunction with GestureDetectorManager to
 * coordinate touch interactions and support complex gesture handling in various
 * applications.
 */
class GestureHandlerTrigger {
 public:
  GestureHandlerTrigger() = default;

  GestureHandlerTrigger(LynxContext* context,
                        std::shared_ptr<GestureDetectorManager> manager);

  void InitCurrentWinnerWhenDown(std::weak_ptr<GestureArenaMember> member);
  void ResolveTouchEvent(
      const ArkUI_UIInputEvent* event,
      std::vector<std::weak_ptr<GestureArenaMember>>& compete_chain_candidates,
      std::shared_ptr<TouchEvent> lynx_touch_event,
      std::vector<std::weak_ptr<GestureArenaMember>>& bubble_chain_candidates);

  void HandleGestureDetectorState(std::weak_ptr<GestureArenaMember> member,
                                  int gesture_id, int state);
  void DispatchBubbleTouchEvent(
      const std::string& type, std::shared_ptr<TouchEvent> touch_event,
      std::vector<std::weak_ptr<GestureArenaMember>>& bubble_candidate,
      std::weak_ptr<GestureArenaMember> winner);
  void SetVelocity(float velocity_x, float velocity_y);

  void Destroy();

 private:
  void ResetCandidatesGestures(
      std::vector<std::weak_ptr<GestureArenaMember>>& members);
  void FailOthersMembersInRaceRelation(
      std::weak_ptr<GestureArenaMember> member, int current_gesture_id,
      std::unordered_set<int>& simultaneous_gesture_ids);
  void StopFlingByLastFlingMember(
      std::shared_ptr<TouchEvent> lynx_touch_event,
      std::vector<std::weak_ptr<GestureArenaMember>>& compete_chain_candidates,
      std::vector<std::weak_ptr<GestureArenaMember>>& bubble_candidates,
      const ArkUI_UIInputEvent* motion_event);

  void FindNextWinnerInBegin(
      std::shared_ptr<TouchEvent> lynx_touch_event,
      std::vector<std::weak_ptr<GestureArenaMember>>& compete_chain_candidates,
      float x, float y, const ArkUI_UIInputEvent* event);
  void UpdateSimultaneousWinner(std::weak_ptr<GestureArenaMember> winner);
  void UpdateLastWinner(std::weak_ptr<GestureArenaMember> winner);
  std::weak_ptr<GestureArenaMember> ReCompeteByGestures(
      std::vector<std::weak_ptr<GestureArenaMember>>&
          competitor_chain_candidates,
      std::weak_ptr<GestureArenaMember> current);
  void DispatchMotionEventOnCurrentWinner(
      const ArkUI_UIInputEvent* event, std::weak_ptr<GestureArenaMember> member,
      std::shared_ptr<TouchEvent> lynx_touch_event, float delta_x,
      float delta_y);
  int GetCurrentMemberState(std::weak_ptr<GestureArenaMember> node);
  void ResetGestureHandlerAndSimultaneous(
      std::weak_ptr<GestureArenaMember> member);
  void ResetGestureHandler(std::weak_ptr<GestureArenaMember> member);
  void DispatchMotionEventWithSimultaneous(
      std::weak_ptr<GestureArenaMember> winner, float x, float y,
      std::shared_ptr<TouchEvent> lynx_touch_event,
      const ArkUI_UIInputEvent* event);
  void DispatchMotionEventWithSimultaneousAndReCompete(
      std::weak_ptr<GestureArenaMember> winner, float x, float y,
      std::shared_ptr<TouchEvent> lynx_touch_event,
      std::vector<std::weak_ptr<GestureArenaMember>>& compete_chain_candidates,
      const ArkUI_UIInputEvent* motion_event);
  void FlingCallback(int status, float velocity_x, float velocity_y);
  std::shared_ptr<BaseGestureHandler> GetGestureHandlerById(
      std::weak_ptr<GestureArenaMember> member, int gesture_id);

  //    OverScroller* scroller_;
  LynxContext* context_;

  int last_fling_target_id_;
  float velocity_x_;
  float velocity_y_;

  std::set<std::weak_ptr<GestureArenaMember>, GestureArenaMemberCompare>
      simultaneous_winners_;
  std::vector<std::weak_ptr<GestureArenaMember>> compete_chain_candidates_;
  std::unordered_set<int> simultaneous_gesture_ids_;
  std::weak_ptr<GestureArenaMember> duplicated_member_;
  std::weak_ptr<GestureArenaMember> winner_;
  std::weak_ptr<GestureArenaMember> last_winner_;
  std::shared_ptr<GestureDetectorManager> gesture_detector_manager_;
  std::unique_ptr<FlingScroller> fling_scroller_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_GESTURE_HANDLER_TRIGGER_H_
