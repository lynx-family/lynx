// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/arena/gesture_arena_manager.h"

#include <memory>

#include "core/renderer/events/gesture.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/detector/gesture_detector_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/gesture_handler_trigger.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
GestureArenaManager::GestureArenaManager(bool enable, LynxContext* context)
    : context_(context), is_enable_new_gesture_(false) {
  is_enable_new_gesture_ = enable;
  if (!IsEnableNewGesture()) {
    return;
  }

  arena_member_map_.clear();
  compete_chain_candidates_.clear();
  bubble_candidate_.clear();
}

void GestureArenaManager::EnsureGestureDetectorAndHandler() {
  if ((gesture_detector_manager_ && gesture_handler_trigger_) || !context_) {
    return;
  }
  gesture_detector_manager_ =
      std::make_shared<GestureDetectorManager>(weak_from_this());
  gesture_handler_trigger_ = std::make_shared<GestureHandlerTrigger>(
      context_, gesture_detector_manager_);
}

bool GestureArenaManager::IsEnableNewGesture() const {
  // This function is a placeholder and should check some global config or
  // condition
  return is_enable_new_gesture_;
}

void GestureArenaManager::DispatchTouchEventToArena(
    const ArkUI_UIInputEvent* event,
    std::shared_ptr<TouchEvent> lynx_touch_event) {
  if (!IsEnableNewGesture()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  if (!gesture_handler_trigger_) {
    return;
  }
  gesture_handler_trigger_->ResolveTouchEvent(
      event, compete_chain_candidates_, lynx_touch_event, bubble_candidate_);
  DispatchBubbleTouchEvent(lynx_touch_event->Name(), lynx_touch_event);
}

void GestureArenaManager::DispatchBubbleTouchEvent(
    const std::string& type, std::shared_ptr<TouchEvent> touch_event) {
  if (!IsEnableNewGesture()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  if (!gesture_handler_trigger_) {
    return;
  }
  gesture_handler_trigger_->DispatchBubbleTouchEvent(
      type, touch_event, bubble_candidate_, winner_);
}

void GestureArenaManager::SetVelocity(float velocity_x, float velocity_y) {
  EnsureGestureDetectorAndHandler();
  if (!gesture_handler_trigger_) {
    return;
  }
  gesture_handler_trigger_->SetVelocity(velocity_x, velocity_y);
}

void GestureArenaManager::SetActiveUIToArenaAtDownEvent(
    std::weak_ptr<EventTarget> target) {
  if (!IsEnableNewGesture()) {
    return;
  }
  EnsureGestureDetectorAndHandler();
  ClearCurrentGesture();
  if (arena_member_map_.empty() || !gesture_handler_trigger_) {
    return;
  }

  auto temp = target.lock().get();
  while (temp) {
    for (auto& pair : arena_member_map_) {
      if (auto member = pair.second.lock()) {
        if (member->GestureArenaMemberId() == temp->GestureArenaMemberId()) {
          bubble_candidate_.emplace_back(member);
        }
      }
    }
    temp = temp->ParentTarget();
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
    std::weak_ptr<GestureArenaMember> arena_member) {
  auto member = arena_member.lock();
  if (!IsEnableNewGesture() || !member) {
    return 0;
  }
  arena_member_map_[member->Sign()] = member;
  RegisterGestureDetectors(member->Sign(), member->GetGestureDetectorMap());
  return member->Sign();
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
    gesture_handler_trigger_->HandleGestureDetectorState(it->second.lock(),
                                                         gesture_id, state);
  }
}

void GestureArenaManager::RemoveMember(
    std::weak_ptr<GestureArenaMember> arena_member) {
  auto member = arena_member.lock();
  if (!IsEnableNewGesture() || !member) {
    return;
  }
  arena_member_map_.erase(member->GestureArenaMemberId());
  UnRegisterGestureDetectors(member->GestureArenaMemberId(),
                             member->GetGestureDetectorMap());
}

std::weak_ptr<GestureArenaMember> GestureArenaManager::GetMemberById(int id) {
  auto it = arena_member_map_.find(id);
  if (it != arena_member_map_.end()) {
    return it->second.lock();
  }
  return std::weak_ptr<GestureArenaMember>();
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

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
