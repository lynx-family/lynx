// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena_impl.h"

namespace lynx::tasm::gesture {

GestureArena::GestureArena(GestureArenaDelegate &delegate)
    : impl_(std::make_unique<GestureArenaImpl>(delegate)) {}

GestureArena::~GestureArena() = default;

void GestureArena::ReplaceMemberGestures(
    MemberId member_id,
    const std::vector<const GestureDetector *> &gesture_detectors) {
  impl_->ReplaceMemberGestures(member_id,
                               NormalizeGestureDetectors(gesture_detectors));
}

void GestureArena::ReplaceMemberGestures(
    MemberId member_id, std::vector<GestureDefinition> gestures) {
  impl_->ReplaceMemberGestures(member_id, std::move(gestures));
}

void GestureArena::RemoveMember(MemberId member_id) {
  impl_->RemoveMember(member_id);
}

bool GestureArena::ContainsMember(MemberId member_id) const {
  return impl_->ContainsMember(member_id);
}

bool GestureArena::ContainsGesture(MemberId member_id,
                                   uint32_t gesture_id) const {
  return impl_->ContainsGesture(member_id, gesture_id);
}

void GestureArena::HandleInput(const InputEvent &event,
                               const std::vector<MemberId> &response_chain) {
  impl_->HandleInput(event, response_chain);
}

void GestureArena::HandleTimer(TimerToken token, double monotonic_time_ms,
                               int64_t epoch_time_ms) {
  impl_->HandleTimer(token, monotonic_time_ms, epoch_time_ms);
}

void GestureArena::SetGestureState(MemberId member_id, uint32_t gesture_id,
                                   GestureStateCommand command) {
  impl_->SetGestureState(member_id, gesture_id, command);
}

void GestureArena::Reset() { impl_->Reset(); }

}  // namespace lynx::tasm::gesture
