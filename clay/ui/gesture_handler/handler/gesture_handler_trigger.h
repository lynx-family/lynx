// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_HANDLER_GESTURE_HANDLER_TRIGGER_H_
#define CLAY_UI_GESTURE_HANDLER_HANDLER_GESTURE_HANDLER_TRIGGER_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/gesture_arena_member.h"
#include "clay/ui/gesture_handler/handler/fling_scroller.h"

namespace clay {

class GestureDetector;
class LynxContext;
class TouchEvent;
class EventTarget;
class GestureArenaMember;
class GestureArenaManager;

// class OverScroller;
class GestureDetectorManager;
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

  GestureHandlerTrigger(PageView* page_view,
                        std::shared_ptr<GestureDetectorManager> manager);

  void InitCurrentWinnerWhenDown(fml::WeakPtr<GestureArenaMember> member);
  void ResolveTouchEvent(
      const PointerEvent* pointer_event,
      std::vector<fml::WeakPtr<GestureArenaMember>>& compete_chain_candidates,
      std::vector<fml::WeakPtr<GestureArenaMember>>& bubble_chain_candidates);

  void HandleGestureDetectorState(fml::WeakPtr<GestureArenaMember> member,
                                  uint32_t gesture_id, int state);
  void DispatchBubbleTouchEvent(
      const PointerEvent* pointer_event,
      std::vector<fml::WeakPtr<GestureArenaMember>>& bubble_candidate,
      fml::WeakPtr<GestureArenaMember> winner);
  void SetVelocity(float velocity_x, float velocity_y);

  void Destroy();

 private:
  void ResetCandidatesGestures(
      std::vector<fml::WeakPtr<GestureArenaMember>>& members);
  void FailOthersMembersInRaceRelation(
      fml::WeakPtr<GestureArenaMember> member, uint32_t current_gesture_id,
      std::unordered_set<int>& simultaneous_gesture_ids);
  void StopFlingByLastFlingMember(
      const PointerEvent* pointer_event,
      std::vector<fml::WeakPtr<GestureArenaMember>>& compete_chain_candidates,
      std::vector<fml::WeakPtr<GestureArenaMember>>& bubble_candidates);

  void FindNextWinnerInBegin(
      std::vector<fml::WeakPtr<GestureArenaMember>>& compete_chain_candidates,
      float x, float y, const PointerEvent* event);
  void UpdateSimultaneousWinner(fml::WeakPtr<GestureArenaMember> winner);
  void UpdateLastWinner(fml::WeakPtr<GestureArenaMember> winner);
  fml::WeakPtr<GestureArenaMember> ReCompeteByGestures(
      std::vector<fml::WeakPtr<GestureArenaMember>>&
          competitor_chain_candidates,
      fml::WeakPtr<GestureArenaMember> current);
  void DispatchMotionEventOnCurrentWinner(
      const PointerEvent* event, fml::WeakPtr<GestureArenaMember> member,
      float delta_x, float delta_y, bool handle_by_simultaneous,
      const std::shared_ptr<GestureExtraBundle>& current_extra_bundle);
  int GetCurrentMemberState(fml::WeakPtr<GestureArenaMember> node);
  void ResetGestureHandlerAndSimultaneous(
      fml::WeakPtr<GestureArenaMember> member);
  void ResetGestureHandler(fml::WeakPtr<GestureArenaMember> member);
  void DispatchMotionEventWithSimultaneous(
      fml::WeakPtr<GestureArenaMember> winner, float x, float y,
      const PointerEvent* event);
  void DispatchMotionEventWithSimultaneousAndReCompete(
      fml::WeakPtr<GestureArenaMember> winner, float x, float y,
      std::vector<fml::WeakPtr<GestureArenaMember>>& compete_chain_candidates,
      const PointerEvent* pointer_event);
  void FlingCallback(int status, float velocity_x, float velocity_y);
  std::shared_ptr<BaseGestureHandler> GetGestureHandlerById(
      fml::WeakPtr<GestureArenaMember> member, uint32_t gesture_id);

  //    OverScroller* scroller_;
  PageView* page_view_;

  int last_fling_target_id_;
  float velocity_x_;
  float velocity_y_;

  std::set<fml::WeakPtr<GestureArenaMember>, GestureArenaMemberCompare>
      simultaneous_winners_;
  std::vector<fml::WeakPtr<GestureArenaMember>> compete_chain_candidates_;
  std::unordered_set<int> simultaneous_gesture_ids_;
  fml::WeakPtr<GestureArenaMember> duplicated_member_;
  fml::WeakPtr<GestureArenaMember> winner_;
  fml::WeakPtr<GestureArenaMember> last_winner_;
  std::shared_ptr<GestureDetectorManager> gesture_detector_manager_;
  std::unique_ptr<FlingScroller> fling_scroller_;
  // current gesture extra bundle, reset when touch down
  std::shared_ptr<GestureExtraBundle> current_extra_bundle_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_HANDLER_GESTURE_HANDLER_TRIGGER_H_
