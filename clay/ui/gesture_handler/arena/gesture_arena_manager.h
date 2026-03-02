// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_ARENA_GESTURE_ARENA_MANAGER_H_
#define CLAY_UI_GESTURE_HANDLER_ARENA_GESTURE_ARENA_MANAGER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/gesture_handler/gesture_detector.h"

namespace clay {

class GestureArenaMember;
class GestureDetectorManager;
class GestureHandlerTrigger;
class PageView;

/**
 * Manages the gesture arenas for handling touch events and dispatching them to
 * the appropriate members. Supports adding, removing, and updating gesture
 * members, as well as resolving touch events and determining the winner.
 */
class GestureArenaManager
    : public std::enable_shared_from_this<GestureArenaManager> {
 public:
  GestureArenaManager(bool enable, PageView* page_view);

  ~GestureArenaManager();

  void DispatchTouchEventToArena(const PointerEvent& event);
  void DispatchBubbleTouchEvent(const PointerEvent& event);
  void SetActiveUIToArenaAtDownEvent(HitTestResult& hit_test_result);
  void SetVelocity(float velocity_x, float velocity_y);
  int AddMember(fml::WeakPtr<GestureArenaMember> member);
  bool IsMemberExist(int member_id);
  void SetGestureDetectorState(int member_id, int gesture_id, int state);
  void RemoveMember(fml::WeakPtr<GestureArenaMember> member);
  fml::WeakPtr<GestureArenaMember> GetMemberById(int id);
  void OnDestroy();

 private:
  bool IsEnableNewGesture() const;
  void ClearCurrentGesture();
  void RegisterGestureDetectors(int member_id,
                                const GestureMap& gesture_detectors);
  void UnRegisterGestureDetectors(int member_id,
                                  const GestureMap& gesture_detectors);

  void EnsureGestureDetectorAndHandler();

  PageView* page_view_;
  std::unordered_map<int, fml::WeakPtr<GestureArenaMember>> arena_member_map_;
  std::vector<fml::WeakPtr<GestureArenaMember>> compete_chain_candidates_;
  std::vector<fml::WeakPtr<GestureArenaMember>> bubble_candidate_;
  std::shared_ptr<GestureDetectorManager> gesture_detector_manager_;
  std::shared_ptr<GestureHandlerTrigger> gesture_handler_trigger_;
  bool is_enable_new_gesture_;
  fml::WeakPtr<GestureArenaMember> winner_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_ARENA_GESTURE_ARENA_MANAGER_H_
