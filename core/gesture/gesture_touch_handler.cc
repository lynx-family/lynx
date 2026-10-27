// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena_impl.h"

namespace lynx::tasm::gesture {

namespace {

class PanGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent &event,
              GestureInteractionContext &context) override {
    last_event_ = event;
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
        start_ = event.page;
        Begin(event);
        return;
      case InputType::kMove:
        // Competition can select this handler after the initial down event.
        if (state_ == GestureState::kInit) {
          start_ = event.page;
          Begin(event);
          return;
        }
        if (state_ == GestureState::kBegin &&
            (std::abs(event.page.x - start_.x) >
                 definition_.config.min_distance ||
             std::abs(event.page.y - start_.y) >
                 definition_.config.min_distance)) {
          Activate(event);
        }
        if (state_ == GestureState::kActive) {
          Update(event);
        }
        return;
      case InputType::kUp:
        Fail(event);
        return;
      case InputType::kCancel:
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        Fail(event);
        return;
    }
  }

 private:
  Point start_;
};

class TapGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent &event,
              GestureInteractionContext &context) override {
    last_event_ = event;
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
        start_ = event.page;
        start_time_ms_ = event.monotonic_time_ms;
        Begin(event);
        timer_ = ScheduleTimer(definition_.config.max_duration_ms);
        return;
      case InputType::kMove:
        if (state_ == GestureState::kBegin && ExceedsDistance(event.page)) {
          CancelTimer(timer_);
          Fail(event);
        }
        return;
      case InputType::kUp:
        CancelTimer(timer_);
        if (state_ == GestureState::kBegin) {
          // A delayed timer must not allow an already expired tap to activate.
          if (ExceedsDistance(event.page) ||
              event.monotonic_time_ms - start_time_ms_ >
                  definition_.config.max_duration_ms) {
            Fail(event);
          } else {
            Activate(event);
            End(event);
          }
        }
        return;
      case InputType::kCancel:
        CancelTimer(timer_);
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        Fail(event);
        return;
    }
  }

  void HandleTimer(TimerToken token, const InputEvent &event) override {
    if (token != timer_) {
      return;
    }
    timer_ = 0;
    TimerFired(token);
    if (state_ == GestureState::kBegin) {
      Fail(event);
    }
  }

 private:
  bool ExceedsDistance(const Point &point) const {
    return std::abs(point.x - start_.x) > definition_.config.max_distance ||
           std::abs(point.y - start_.y) > definition_.config.max_distance;
  }

  Point start_;
  double start_time_ms_ = 0;
  TimerToken timer_ = 0;
};

class LongPressGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent &event,
              GestureInteractionContext &context) override {
    last_event_ = event;
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
        start_ = event.page;
        Begin(event);
        timer_ = ScheduleTimer(definition_.config.min_duration_ms);
        return;
      // Distance limits apply only while waiting for long-press activation.
      case InputType::kMove:
        if (state_ == GestureState::kBegin && ExceedsDistance(event.page)) {
          CancelTimer(timer_);
          Fail(event);
        }
        return;
      case InputType::kUp:
        CancelTimer(timer_);
        state_ == GestureState::kActive ? End(event) : Fail(event);
        return;
      case InputType::kCancel:
        CancelTimer(timer_);
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        Fail(event);
        return;
    }
  }

  void HandleTimer(TimerToken token, const InputEvent &event) override {
    if (token != timer_) {
      return;
    }
    timer_ = 0;
    TimerFired(token);
    if (state_ == GestureState::kBegin) {
      Activate(event);
    }
  }

 private:
  bool ExceedsDistance(const Point &point) const {
    return std::abs(point.x - start_.x) > definition_.config.max_distance ||
           std::abs(point.y - start_.y) > definition_.config.max_distance;
  }

  Point start_;
  TimerToken timer_ = 0;
};

}  // namespace

std::unique_ptr<GestureHandler> CreateTouchGestureHandler(
    MemberId member_id, GestureDefinition definition, GestureArenaImpl &host) {
  switch (definition.gesture_type) {
    case GestureType::PAN:
    case GestureType::NATIVE:
      return std::make_unique<PanGestureHandler>(member_id,
                                                 std::move(definition), host);
    case GestureType::TAP:
      return std::make_unique<TapGestureHandler>(member_id,
                                                 std::move(definition), host);
    case GestureType::LONG_PRESS:
      return std::make_unique<LongPressGestureHandler>(
          member_id, std::move(definition), host);
    default:
      return nullptr;
  }
}

}  // namespace lynx::tasm::gesture
