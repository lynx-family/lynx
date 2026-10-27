// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena_impl.h"

namespace lynx::tasm::gesture {

GestureHandler::GestureHandler(MemberId member_id, GestureDefinition definition,
                               GestureArenaImpl &host)
    : key_{member_id, definition.gesture_id},
      definition_(std::move(definition)),
      host_(host) {}

GestureHandler::~GestureHandler() { CancelAllTimers(); }

void GestureHandler::HandleSimultaneous(const InputEvent &event,
                                        const Point &delta,
                                        GestureInteractionContext &context) {
  Handle(event, context);
}

void GestureHandler::HandleTimer(TimerToken token, const InputEvent &event) {
  TimerFired(token);
}

void GestureHandler::DispatchTouchCallback(GestureCallbackType callback,
                                           const InputEvent &event) {
  if (IsSubscribed(callback)) {
    host_.Emit(key_, callback, event, {});
  }
}

void GestureHandler::Reset() {
  CancelAllTimers();
  state_ = GestureState::kInit;
  last_event_ = {};
  began_ = false;
  started_ = false;
  ended_ = false;
}

void GestureHandler::FailFromCompetition() { Fail(last_event_); }

void GestureHandler::EndFromCommand() { End(last_event_); }

void GestureHandler::CancelForTermination(const InputEvent &event) {
  Cancel(event);
}

bool GestureHandler::IsSubscribed(GestureCallbackType callback) const {
  return std::find(definition_.callbacks.begin(), definition_.callbacks.end(),
                   callback) != definition_.callbacks.end();
}

void GestureHandler::Begin(const InputEvent &event) {
  if (state_ != GestureState::kInit) {
    return;
  }
  last_event_ = event;
  // Set guards before callbacks, which may synchronously end this handler.
  began_ = true;
  TransitionTo(GestureState::kBegin);
  if (IsSubscribed(GestureCallbackType::kBegin)) {
    host_.Emit(key_, GestureCallbackType::kBegin, event, {});
  }
}

void GestureHandler::Activate(const InputEvent &event) {
  if (state_ == GestureState::kInit) {
    Begin(event);
  }
  if (state_ != GestureState::kBegin && state_ != GestureState::kActive) {
    return;
  }
  if (state_ != GestureState::kActive) {
    TransitionTo(GestureState::kActive);
  }
  if (!started_) {
    started_ = true;
    if (IsSubscribed(GestureCallbackType::kStart)) {
      host_.Emit(key_, GestureCallbackType::kStart, event, {});
    }
  }
}

void GestureHandler::Update(const InputEvent &event, const Point &delta) {
  if (state_ == GestureState::kActive &&
      IsSubscribed(GestureCallbackType::kUpdate)) {
    host_.Emit(key_, GestureCallbackType::kUpdate, event, delta);
  }
}

void GestureHandler::Fail(const InputEvent &event) {
  if (state_ == GestureState::kFail || IsTerminal()) {
    return;
  }
  Finish(event, GestureState::kFail);
}

void GestureHandler::End(const InputEvent &event) {
  if (state_ == GestureState::kEnd || state_ == GestureState::kCancel) {
    return;
  }
  Finish(event, GestureState::kEnd);
}

void GestureHandler::Cancel(const InputEvent &event) {
  if (state_ == GestureState::kEnd || state_ == GestureState::kCancel) {
    return;
  }
  Finish(event, GestureState::kCancel);
}

TimerToken GestureHandler::ScheduleTimer(double delay_ms) {
  if ((state_ != GestureState::kBegin && state_ != GestureState::kActive) ||
      !host_.IsMemberValid(member_id())) {
    return 0;
  }
  const auto token = host_.ScheduleTimer(key_, delay_ms);
  if (token == 0) {
    return 0;
  }
  // Scheduling can reenter platform code and invalidate the member.
  if ((state_ != GestureState::kBegin && state_ != GestureState::kActive) ||
      !host_.IsMemberValid(member_id())) {
    host_.CancelTimer(token);
    return 0;
  }
  timers_.push_back(token);
  return token;
}

void GestureHandler::CancelTimer(TimerToken &token) {
  if (token == 0) {
    return;
  }
  host_.CancelTimer(token);
  TimerFired(token);
  token = 0;
}

void GestureHandler::TimerFired(TimerToken token) {
  timers_.erase(std::remove(timers_.begin(), timers_.end(), token),
                timers_.end());
}

bool GestureHandler::IsTerminal() const {
  return state_ == GestureState::kEnd || state_ == GestureState::kCancel;
}

void GestureHandler::CancelAllTimers() {
  for (const auto token : timers_) {
    host_.CancelTimer(token);
  }
  timers_.clear();
}

void GestureHandler::TransitionTo(GestureState state) {
  state_ = state;
  host_.OnGestureStateChanged(key_, state);
}

void GestureHandler::Finish(const InputEvent &event, GestureState state) {
  CancelAllTimers();
  TransitionTo(state);
  if (began_ && !ended_) {
    // Mark delivery before emitting to prevent reentrant duplicate end
    // callbacks.
    ended_ = true;
    if (IsSubscribed(GestureCallbackType::kEnd)) {
      host_.Emit(key_, GestureCallbackType::kEnd, event, {});
    }
  }
}

}  // namespace lynx::tasm::gesture
