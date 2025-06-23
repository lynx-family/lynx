// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_ARENA_GESTURE_ARENA_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_ARENA_GESTURE_ARENA_MANAGER_H_

#include <arkui/ui_input_event.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

namespace harmony {

class LynxContext;
class TouchEvent;
class EventTarget;
class GestureArenaMember;
class GestureDetectorManager;
class GestureHandlerTrigger;

/**
 * Manages the gesture arenas for handling touch events and dispatching them to
 * the appropriate members. Supports adding, removing, and updating gesture
 * members, as well as resolving touch events and determining the winner.
 */
class GestureArenaManager
    : public std::enable_shared_from_this<GestureArenaManager> {
 public:
  GestureArenaManager(bool enable, LynxContext* context);

  ~GestureArenaManager();

  void DispatchTouchEventToArena(const ArkUI_UIInputEvent* event,
                                 std::shared_ptr<TouchEvent> lynx_touch_event);
  void DispatchBubbleTouchEvent(const std::string& type,
                                std::shared_ptr<TouchEvent> touch_event);
  void SetActiveUIToArenaAtDownEvent(std::weak_ptr<EventTarget> target);
  void SetVelocity(float velocity_x, float velocity_y);
  int AddMember(std::weak_ptr<GestureArenaMember> member);
  bool IsMemberExist(int member_id);
  void SetGestureDetectorState(int member_id, int gesture_id, int state);
  void RemoveMember(std::weak_ptr<GestureArenaMember> member);
  std::weak_ptr<GestureArenaMember> GetMemberById(int id);
  void OnDestroy();

 private:
  bool IsEnableNewGesture() const;
  void ClearCurrentGesture();
  void RegisterGestureDetectors(int member_id,
                                const GestureMap& gesture_detectors);
  void UnRegisterGestureDetectors(int member_id,
                                  const GestureMap& gesture_detectors);

  void EnsureGestureDetectorAndHandler();

  LynxContext* context_;
  std::unordered_map<int, std::weak_ptr<GestureArenaMember>> arena_member_map_;
  std::vector<std::weak_ptr<GestureArenaMember>> compete_chain_candidates_;
  std::vector<std::weak_ptr<GestureArenaMember>> bubble_candidate_;
  std::shared_ptr<GestureDetectorManager> gesture_detector_manager_;
  std::shared_ptr<GestureHandlerTrigger> gesture_handler_trigger_;
  bool is_enable_new_gesture_;
  std::weak_ptr<GestureArenaMember> winner_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_ARENA_GESTURE_ARENA_MANAGER_H_
