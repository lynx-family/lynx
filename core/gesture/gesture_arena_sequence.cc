// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena_impl.h"

namespace lynx::tasm::gesture {

void GestureArenaImpl::HandleInput(
    const InputEvent &input, const std::vector<MemberId> &response_chain) {
  if (resetting_ || reset_pending_) {
    return;
  }
  // A new down owns the sequence; all later events must match its ID.
  if (input.type == InputType::kDown) {
    if (has_sequence_ && !interaction_terminated_) {
      InputEvent cancelled = last_input_;
      cancelled.type = InputType::kCancel;
      cancelled.monotonic_time_ms = input.monotonic_time_ms;
      cancelled.epoch_time_ms = input.epoch_time_ms;
      TerminateInteraction(cancelled, GestureState::kCancel);
    }
    BeginSequence(input, response_chain);
  } else if (!has_sequence_ || input.sequence_id != sequence_id_) {
    return;
  }

  if (input.type != InputType::kDown && interaction_terminated_) {
    return;
  }

  InputEvent event = input;
  last_input_ = event;
  if (event.type != InputType::kFlingFrame) {
    DispatchTouches(event);
  }
  if (interaction_terminated_) {
    return;
  }

  ReevaluateCompetition();
  if (event.type == InputType::kCancel) {
    DispatchWinner(event);
    TerminateInteraction(event, GestureState::kCancel);
    return;
  }

  DispatchWinner(event);
  if (event.type == InputType::kUp) {
    StartOrFinishFling(event);
  } else if (event.type == InputType::kFlingFrame && event.fling_finished) {
    fling_active_ = false;
    TerminateInteraction(event, GestureState::kEnd);
  }
}

void GestureArenaImpl::HandleTimer(TimerToken token, double monotonic_time_ms,
                                   int64_t epoch_time_ms) {
  if (resetting_ || reset_pending_) {
    return;
  }
  const auto timer_it = timers_.find(token);
  if (timer_it == timers_.end()) {
    return;
  }
  // Cancellation can race with delivery; validate sequence and generation
  // below.
  const TimerRecord timer = timer_it->second;
  timers_.erase(timer_it);
  if (interaction_terminated_ || timer.sequence_id != sequence_id_ ||
      !IsCurrentGeneration(timer.key.member_id, timer.generation)) {
    return;
  }
  auto *handler = FindHandler(timer.key);
  if (!handler) {
    return;
  }

  InputEvent event = timer.event;
  event.monotonic_time_ms = monotonic_time_ms;
  event.epoch_time_ms = epoch_time_ms;
  CallHandler(timer.key, [token, &event](GestureHandler &current) {
    current.HandleTimer(token, event);
  });
  ReevaluateCompetition();
}

void GestureArenaImpl::SetGestureState(MemberId member_id, uint32_t gesture_id,
                                       GestureStateCommand command) {
  if (resetting_ || reset_pending_ || command == GestureStateCommand::kActive ||
      invalidated_members_.find(member_id) != invalidated_members_.end()) {
    return;
  }
  const HandlerKey key{member_id, gesture_id};
  if (!FindHandler(key)) {
    return;
  }
  CallHandler(key, [command](GestureHandler &handler) {
    if (command == GestureStateCommand::kFail) {
      handler.FailFromCompetition();
    } else if (command == GestureStateCommand::kEnd) {
      handler.EndFromCommand();
    }
  });
  ReevaluateCompetition();
}

void GestureArenaImpl::Reset() {
  if (resetting_) {
    return;
  }
  if (dispatch_depth_ != 0 || flushing_mutations_) {
    reset_pending_ = true;
    interaction_terminated_ = true;
    for (const auto &[member_id, member] : members_) {
      invalidated_members_.insert(member_id);
    }
    return;
  }
  ApplyReset();
}

void GestureArenaImpl::ApplyReset() {
  resetting_ = true;
  reset_pending_ = false;
  interaction_terminated_ = true;
  invalidated_members_.clear();
  InputEvent event = last_input_;
  event.type = InputType::kCancel;
  for (const auto member_id : RegisteredMemberIds()) {
    TerminateMember(member_id, event, GestureState::kCancel);
  }
  for (const auto &[token, timer] : timers_) {
    delegate_.CancelTimer(token);
  }
  timers_.clear();
  if (fling_active_) {
    fling_active_ = false;
    delegate_.StopFling();
  }
  members_.clear();
  gesture_members_.clear();
  response_chain_.clear();
  compete_chain_.clear();
  simultaneous_members_.clear();
  active_generations_.clear();
  pending_mutations_.clear();
  invalidated_members_.clear();
  winner_index_.reset();
  has_sequence_ = false;
  interaction_terminated_ = true;
  resetting_ = false;
}

void GestureArenaImpl::BeginSequence(
    const InputEvent &event, const std::vector<MemberId> &response_chain) {
  if (fling_active_) {
    fling_active_ = false;
    delegate_.StopFling();
  }
  has_sequence_ = true;
  interaction_terminated_ = false;
  sequence_id_ = event.sequence_id;
  last_input_ = event;
  context_ = {};
  winner_index_.reset();
  simultaneous_members_.clear();
  active_generations_.clear();
  response_chain_.clear();
  for (const auto member_id : response_chain) {
    if (ContainsMember(member_id) && delegate_.IsMemberValid(member_id)) {
      response_chain_.push_back(member_id);
    }
  }
  compete_chain_ = ConvertResponseChain(response_chain_);
  for (const auto member_id : response_chain_) {
    ActivateMember(member_id);
  }
  for (const auto member_id : compete_chain_) {
    ActivateMember(member_id);
  }
  winner_index_ = FindNextWinner(std::nullopt);
  UpdateSimultaneousMembers();
}

void GestureArenaImpl::ActivateMember(MemberId member_id) {
  const auto member = members_.find(member_id);
  if (member == members_.end() ||
      active_generations_.find(member_id) != active_generations_.end()) {
    return;
  }
  active_generations_[member_id] = member->second.generation;
  std::vector<uint32_t> gesture_ids;
  for (const auto &[gesture_id, handler] : member->second.handlers) {
    gesture_ids.push_back(gesture_id);
  }
  for (const auto gesture_id : gesture_ids) {
    const HandlerKey key{member_id, gesture_id};
    CallHandler(key, [](GestureHandler &handler) { handler.Reset(); });
  }
}

void GestureArenaImpl::TerminateInteraction(const InputEvent &event,
                                            GestureState state) {
  if (interaction_terminated_) {
    return;
  }
  interaction_terminated_ = true;
  if (fling_active_) {
    fling_active_ = false;
    delegate_.StopFling();
  }
  const auto members = ActiveMemberIds();
  for (const auto member_id : members) {
    TerminateMember(member_id, event, state);
  }
  winner_index_.reset();
  simultaneous_members_.clear();
}

void GestureArenaImpl::TerminateMember(MemberId member_id,
                                       const InputEvent &event,
                                       GestureState state) {
  for (const auto &key : HandlerKeys(member_id)) {
    CallHandler(key, [&event, state](GestureHandler &handler) {
      if (handler.state() != GestureState::kBegin &&
          handler.state() != GestureState::kActive) {
        return;
      }
      if (state == GestureState::kCancel) {
        handler.CancelForTermination(event);
      } else {
        handler.EndFromCommand();
      }
    });
  }
}

void GestureArenaImpl::CallHandler(
    const HandlerKey &key,
    const std::function<void(GestureHandler &)> &callback) {
  auto *handler = FindHandler(key);
  if (!handler ||
      invalidated_members_.find(key.member_id) != invalidated_members_.end()) {
    return;
  }
  // Keep handler objects alive until the outermost callback has returned.
  ++dispatch_depth_;
  callback(*handler);
  --dispatch_depth_;
  FlushMutations();
}

bool GestureArenaImpl::IsActiveMember(MemberId member_id) const {
  const auto generation = active_generations_.find(member_id);
  return generation != active_generations_.end() &&
         IsCurrentGeneration(member_id, generation->second) &&
         delegate_.IsMemberValid(member_id);
}

std::vector<MemberId> GestureArenaImpl::ActiveMemberIds() const {
  std::vector<MemberId> result;
  result.reserve(active_generations_.size());
  for (const auto &[member_id, generation] : active_generations_) {
    result.push_back(member_id);
  }
  return result;
}

}  // namespace lynx::tasm::gesture
