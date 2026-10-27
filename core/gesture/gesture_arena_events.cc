// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena_impl.h"

namespace lynx::tasm::gesture {

void GestureArenaImpl::StartOrFinishFling(const InputEvent &event) {
  if (interaction_terminated_) {
    return;
  }
  if (CurrentWinner()) {
    fling_active_ = true;
    // Starting the platform driver can synchronously remove the winner.
    ++dispatch_depth_;
    const bool started = delegate_.StartFling(event.velocity);
    --dispatch_depth_;
    FlushMutations();
    const auto winner = CurrentWinner();
    if (started && fling_active_ && winner && IsActiveMember(*winner)) {
      return;
    }
    if (started && fling_active_) {
      delegate_.StopFling();
    }
    fling_active_ = false;
    if (interaction_terminated_) {
      return;
    }
  }

  InputEvent finished = event;
  finished.type = InputType::kFlingFrame;
  finished.delta = {};
  finished.velocity = {};
  finished.fling_finished = true;
  DispatchWinner(finished);
  if (!interaction_terminated_) {
    TerminateInteraction(finished, GestureState::kEnd);
  }
}

std::optional<MemberId> GestureArenaImpl::CurrentWinner() const {
  if (!winner_index_ || *winner_index_ >= compete_chain_.size()) {
    return std::nullopt;
  }
  return compete_chain_[*winner_index_];
}

bool GestureArenaImpl::IsMemberValid(MemberId member_id) const {
  return ContainsMember(member_id) && delegate_.IsMemberValid(member_id);
}

GestureDirection GestureArenaImpl::GetScrollDirection(
    MemberId member_id) const {
  return IsMemberValid(member_id) ? delegate_.GetScrollDirection(member_id)
                                  : GestureDirection::kUndetermined;
}

bool GestureArenaImpl::CanConsumeGesture(MemberId member_id,
                                         GestureDirection direction,
                                         const Point &delta) const {
  return IsMemberValid(member_id) &&
         delegate_.CanConsumeGesture(member_id, direction, delta);
}

bool GestureArenaImpl::ShouldConsumeGesture(MemberId member_id) const {
  return IsMemberValid(member_id) && delegate_.ShouldConsumeGesture(member_id);
}

ScrollResult GestureArenaImpl::ScrollBy(MemberId member_id,
                                        const Point &delta) {
  return IsMemberValid(member_id) ? delegate_.ScrollBy(member_id, delta)
                                  : ScrollResult{{}, delta};
}

void GestureArenaImpl::OnGestureRecognized(MemberId member_id) {
  if (IsMemberValid(member_id)) {
    delegate_.OnGestureRecognized(member_id);
  }
}

void GestureArenaImpl::OnGestureStateChanged(const HandlerKey &key,
                                             GestureState state) {
  delegate_.OnGestureStateChanged(key.member_id, key.gesture_id, state);
}

void GestureArenaImpl::Emit(const HandlerKey &key, GestureCallbackType callback,
                            const InputEvent &event, const Point &delta) {
  auto *handler = FindHandler(key);
  if (!handler || !IsMemberValid(key.member_id)) {
    return;
  }
  GestureEvent gesture_event;
  gesture_event.member_id = key.member_id;
  gesture_event.gesture_id = key.gesture_id;
  gesture_event.gesture_type = handler->gesture_type();
  gesture_event.callback = callback;
  gesture_event.source = event.type;
  gesture_event.sequence_id = event.sequence_id;
  gesture_event.pointer_id = event.pointer_id;
  gesture_event.timestamp_epoch_ms = event.epoch_time_ms;
  gesture_event.screen = event.screen;
  gesture_event.page = event.page;
  gesture_event.client = event.client;
  gesture_event.local =
      delegate_.ConvertPageToMember(key.member_id, event.page);
  gesture_event.delta = delta;
  gesture_event.scroll = delegate_.GetScrollState(key.member_id);
  if (IsMemberValid(key.member_id)) {
    delegate_.DispatchGestureEvent(gesture_event);
  }
}

TimerToken GestureArenaImpl::ScheduleTimer(const HandlerKey &key,
                                           double delay_ms) {
  const auto member = members_.find(key.member_id);
  if (member == members_.end() || !IsMemberValid(key.member_id)) {
    return 0;
  }
  const auto token = next_timer_token_++;
  timers_.insert_or_assign(token, TimerRecord{key, member->second.generation,
                                              sequence_id_, last_input_});
  delegate_.ScheduleTimer(token, std::max(0.0, delay_ms));
  return token;
}

void GestureArenaImpl::CancelTimer(TimerToken token) {
  if (token == 0) {
    return;
  }
  const auto timer = timers_.find(token);
  if (timer == timers_.end()) {
    return;
  }
  timers_.erase(timer);
  delegate_.CancelTimer(token);
}

}  // namespace lynx::tasm::gesture
