// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/arena/gesture_arena_manager.h"

#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/gesture_handler/detector/gesture_detector_manager.h"
#include "clay/ui/gesture_handler/gesture_arena_member.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_trigger.h"

namespace clay {

GestureArenaManager::GestureArenaManager(bool enable, PageView* page_view)
    : page_view_(page_view), is_enable_new_gesture_(enable) {}

void GestureArenaManager::EnsureGestureDetectorAndHandler() {
  if ((gesture_detector_manager_ && gesture_handler_trigger_) || !page_view_) {
    return;
  }
  gesture_detector_manager_ =
      std::make_shared<GestureDetectorManager>(weak_from_this());
  gesture_handler_trigger_ = std::make_shared<GestureHandlerTrigger>(
      page_view_, gesture_detector_manager_);
}

bool GestureArenaManager::IsEnableNewGesture() const {
  // This function is a placeholder and should check some global config or
  // condition
  return is_enable_new_gesture_;
}

void GestureArenaManager::DispatchTouchEventToArena(
    const PointerEvent& pointer_event) {
  if (!IsEnableNewGesture()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  if (!gesture_handler_trigger_) {
    return;
  }
  gesture_handler_trigger_->ResolveTouchEvent(
      &pointer_event, compete_chain_candidates_, bubble_candidate_);
  DispatchBubbleTouchEvent(pointer_event);
}

void GestureArenaManager::DispatchBubbleTouchEvent(
    const PointerEvent& pointer_event) {
  if (!IsEnableNewGesture()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  if (!gesture_handler_trigger_) {
    return;
  }
  gesture_handler_trigger_->DispatchBubbleTouchEvent(
      &pointer_event, bubble_candidate_, winner_);
}

void GestureArenaManager::SetVelocity(float velocity_x, float velocity_y) {
  EnsureGestureDetectorAndHandler();
  if (!gesture_handler_trigger_) {
    return;
  }
  gesture_handler_trigger_->SetVelocity(velocity_x, velocity_y);
}

void GestureArenaManager::SetActiveUIToArenaAtDownEvent(
    HitTestResult& hit_test_result) {
  if (!IsEnableNewGesture()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  ClearCurrentGesture();
  if (arena_member_map_.empty() || !gesture_handler_trigger_) {
    return;
  }

  for (auto& hit_test_object : hit_test_result) {
    BaseView* view = static_cast<BaseView*>(hit_test_object.get());
    for (auto& pair : arena_member_map_) {
      if (pair.second->GestureArenaMemberId() == view->GestureArenaMemberId()) {
        bubble_candidate_.emplace_back(view->GetWeakPtr());
        break;
      }
    }
  }

  if (gesture_detector_manager_) {
    compete_chain_candidates_ =
        gesture_detector_manager_->ConvertResponseChainToCompeteChain(
            bubble_candidate_);
  }

  if (!compete_chain_candidates_.empty()) {
    winner_ = compete_chain_candidates_.front();
  }

  gesture_handler_trigger_->InitCurrentWinnerWhenDown(winner_);
}

void GestureArenaManager::ClearCurrentGesture() {
  winner_.reset();
  bubble_candidate_.clear();
  compete_chain_candidates_.clear();
}

int GestureArenaManager::AddMember(
    fml::WeakPtr<GestureArenaMember> arena_member) {
  if (!IsEnableNewGesture() || !arena_member) {
    return 0;
  }
  arena_member_map_[arena_member->Sign()] = arena_member;
  RegisterGestureDetectors(arena_member->Sign(),
                           arena_member->GetGestureDetectorMap());
  return arena_member->Sign();
}

bool GestureArenaManager::IsMemberExist(int member_id) {
  if (!IsEnableNewGesture()) {
    return false;
  }
  return arena_member_map_.find(member_id) != arena_member_map_.end();
}

void GestureArenaManager::SetGestureDetectorState(int member_id, int gesture_id,
                                                  int state) {
  if (!IsEnableNewGesture()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  if (!gesture_handler_trigger_) {
    return;
  }

  auto it = arena_member_map_.find(member_id);
  if (it != arena_member_map_.end()) {
    gesture_handler_trigger_->HandleGestureDetectorState(it->second, gesture_id,
                                                         state);
  }
}

void GestureArenaManager::RemoveMember(
    fml::WeakPtr<GestureArenaMember> arena_member) {
  if (!IsEnableNewGesture() || !arena_member) {
    return;
  }
  arena_member_map_.erase(arena_member->GestureArenaMemberId());
  UnRegisterGestureDetectors(arena_member->GestureArenaMemberId(),
                             arena_member->GetGestureDetectorMap());
}

fml::WeakPtr<GestureArenaMember> GestureArenaManager::GetMemberById(int id) {
  auto it = arena_member_map_.find(id);
  if (it != arena_member_map_.end()) {
    return it->second;
  }
  return fml::WeakPtr<GestureArenaMember>();
}

GestureArenaManager::~GestureArenaManager() { OnDestroy(); }

void GestureArenaManager::OnDestroy() {
  arena_member_map_.clear();
  compete_chain_candidates_.clear();
  bubble_candidate_.clear();
  if (gesture_detector_manager_) {
    gesture_detector_manager_->Destroy();
  }
  if (gesture_handler_trigger_) {
    gesture_handler_trigger_->Destroy();
  }
}

void GestureArenaManager::RegisterGestureDetectors(
    int member_id, const GestureMap& gesture_detectors) {
  if (!IsEnableNewGesture() || gesture_detectors.empty()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  if (!gesture_detector_manager_) {
    return;
  }

  for (const auto& entry : gesture_detectors) {
    gesture_detector_manager_->RegisterGestureDetector(member_id, entry.second);
  }
}

void GestureArenaManager::UnRegisterGestureDetectors(
    int member_id, const GestureMap& gesture_detectors) {
  if (!IsEnableNewGesture() || gesture_detectors.empty()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  if (!gesture_detector_manager_) {
    return;
  }
  for (const auto& entry : gesture_detectors) {
    gesture_detector_manager_->UnregisterGestureDetector(member_id,
                                                         entry.second);
  }
}

}  // namespace clay
