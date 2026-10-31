// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena_impl.h"

namespace lynx::tasm::gesture {

std::unique_ptr<GestureHandler> CreateTouchGestureHandler(
    MemberId member_id, GestureDefinition definition, GestureArenaImpl &host);

namespace {

class DefaultGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent &event,
              GestureInteractionContext &context) override {
    if (event.type != InputType::kFlingFrame) {
      last_event_ = event;
    }
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
        last_point_ = event.page;
        Begin(event);
        return;
      case InputType::kMove: {
        if (state_ == GestureState::kInit) {
          last_point_ = event.page;
          Begin(event);
          return;
        }
        // Scrolling consumes content movement, opposite to the finger delta.
        Point delta{last_point_.x - event.page.x, last_point_.y - event.page.y};
        last_point_ = event.page;
        HandleDelta(event, delta, context);
        return;
      }
      case InputType::kUp:
        return;
      case InputType::kCancel:
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        if (event.fling_finished) {
          if (state_ == GestureState::kActive ||
              state_ == GestureState::kBegin) {
            End(event);
          }
          return;
        }
        HandleDelta(event, event.delta, context);
        return;
    }
  }

  void HandleSimultaneous(const InputEvent &event, const Point &delta,
                          GestureInteractionContext &context) override {
    // Simultaneous scrolling still receives the shared delta after failure.
    if (!context.has_simultaneous_delta || state_ == GestureState::kEnd ||
        state_ == GestureState::kCancel || !host_.IsMemberValid(member_id()) ||
        !host_.ShouldConsumeGesture(member_id())) {
      return;
    }
    const auto direction = EffectiveDirection(delta, context);
    if (MatchesMemberDirection(direction) &&
        host_.CanConsumeGesture(member_id(), direction, delta)) {
      host_.ScrollBy(member_id(), delta);
    }
  }

 private:
  GestureDirection EffectiveDirection(
      const Point &delta, GestureInteractionContext &context) const {
    // Keep the first nonzero movement direction for the entire interaction.
    if (context.direction == GestureDirection::kUndetermined) {
      context.direction = std::abs(delta.x) > std::abs(delta.y)
                              ? GestureDirection::kHorizontal
                              : GestureDirection::kVertical;
    }
    return context.direction;
  }

  bool MatchesMemberDirection(GestureDirection direction) const {
    const auto member_direction = host_.GetScrollDirection(member_id());
    return member_direction == GestureDirection::kUndetermined ||
           member_direction == direction;
  }

  void HandleDelta(const InputEvent &event, Point delta,
                   GestureInteractionContext &context) {
    if ((delta.x == 0 && delta.y == 0) || state_ == GestureState::kFail ||
        IsTerminal()) {
      return;
    }
    const auto direction = EffectiveDirection(delta, context);
    if (direction == GestureDirection::kHorizontal) {
      delta.y = 0;
    } else {
      delta.x = 0;
    }
    context.has_simultaneous_delta = true;
    context.simultaneous_delta = delta;

    if (!MatchesMemberDirection(direction) ||
        !host_.ShouldConsumeGesture(member_id()) ||
        !host_.CanConsumeGesture(member_id(), direction, delta)) {
      Fail(event);
      return;
    }
    if (state_ == GestureState::kInit) {
      Begin(event);
    }
    Activate(event);
    if (state_ != GestureState::kActive || !host_.IsMemberValid(member_id())) {
      return;
    }
    host_.ScrollBy(member_id(), delta);
    if (state_ != GestureState::kActive || !host_.IsMemberValid(member_id())) {
      return;
    }
    if (std::abs(delta.x) > definition_.config.tap_slop ||
        std::abs(delta.y) > definition_.config.tap_slop) {
      host_.OnGestureRecognized(member_id());
    }
    Update(event, delta);
  }

  Point last_point_;
};

class FlingGestureHandler : public GestureHandler {
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
      case InputType::kMove:
        return;
      case InputType::kUp:
        Begin(event);
        return;
      case InputType::kCancel:
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        if (event.fling_finished) {
          state_ == GestureState::kActive ? End(event) : Fail(event);
          return;
        }
        if (state_ == GestureState::kInit) {
          Begin(event);
        }
        Activate(event);
        Update(event, event.delta);
        return;
    }
  }
};

}  // namespace

std::unique_ptr<GestureHandler> CreateGestureHandler(
    MemberId member_id, GestureDefinition definition, GestureArenaImpl &host) {
  switch (definition.gesture_type) {
    case GestureType::PAN:
    case GestureType::NATIVE:
    case GestureType::TAP:
    case GestureType::LONG_PRESS:
      return CreateTouchGestureHandler(member_id, std::move(definition), host);
    case GestureType::DEFAULT:
      return std::make_unique<DefaultGestureHandler>(
          member_id, std::move(definition), host);
    case GestureType::FLING:
      return std::make_unique<FlingGestureHandler>(member_id,
                                                   std::move(definition), host);
    case GestureType::ROTATION:
    case GestureType::PINCH:
      return nullptr;
  }
}

}  // namespace lynx::tasm::gesture
