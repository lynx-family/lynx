// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/unified_gesture_handler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

#include "base/include/auto_reset.h"
#include "base/include/value/table.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_dispatcher.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/fling_scroller.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"

namespace lynx::tasm::harmony {
namespace {
double MonotonicTimeMs() {
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

int64_t EpochTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
}  // namespace

UnifiedGestureHandler::UnifiedGestureHandler(LynxContext* context)
    : context_(context),
      fling_scroller_(std::make_unique<FlingScroller>()),
      arena_(*this) {}

UnifiedGestureHandler::~UnifiedGestureHandler() { Reset(); }

void UnifiedGestureHandler::Reset() {
  arena_.Reset();
  timers_.clear();
  StopFling();
  members_.clear();
  response_chain_.clear();
  active_pointer_.reset();
  velocity_ = {};
}

int UnifiedGestureHandler::ReplaceMember(
    std::weak_ptr<GestureArenaMember> member) {
  auto view = std::static_pointer_cast<UIBase>(member.lock());
  if (!view) {
    return 0;
  }
  const int id = view->Sign();
  std::vector<const GestureDetector*> detectors;
  for (const auto& entry : view->GetGestureDetectorMap()) {
    detectors.push_back(entry.second.get());
  }
  members_.insert_or_assign(id, view);
  arena_.ReplaceMemberGestures(id, detectors);
  if (!arena_.ContainsMember(id)) {
    members_.erase(id);
    return 0;
  }
  return id;
}

void UnifiedGestureHandler::RemoveMember(int member_id) {
  arena_.RemoveMember(member_id);
  members_.erase(member_id);
}

bool UnifiedGestureHandler::ContainsMember(int member_id) const {
  return arena_.ContainsMember(member_id);
}

bool UnifiedGestureHandler::ContainsGesture(int member_id,
                                            uint32_t gesture_id) const {
  return arena_.ContainsGesture(member_id, gesture_id);
}

void UnifiedGestureHandler::SetGestureState(int member_id, uint32_t gesture_id,
                                            int state) {
  if (state >= 1 && state <= 3) {
    arena_.SetGestureState(member_id, gesture_id,
                           static_cast<gesture::GestureStateCommand>(state));
  }
}

void UnifiedGestureHandler::SetResponseChain(
    std::weak_ptr<EventTarget> target) {
  response_chain_.clear();
  auto retained_target = target.lock();
  for (auto* current = retained_target.get(); current;
       current = current->ParentTarget()) {
    const int id = current->GestureArenaMemberId();
    if (arena_.ContainsMember(id)) {
      response_chain_.push_back(id);
    }
  }
}

void UnifiedGestureHandler::HandleInput(const ArkUI_UIInputEvent* event) {
  const auto action = OH_ArkUI_UIInputEvent_GetAction(event);
  auto* dispatcher = context_->GetUIOwner()->GetEventDispatcher();
  auto input = last_input_;
  switch (action) {
    case UI_TOUCH_EVENT_ACTION_DOWN:
      if (active_pointer_ && OH_ArkUI_PointerEvent_GetPointerCount(event) > 1) {
        return;
      }
      input.type = gesture::InputType::kDown;
      break;
    case UI_TOUCH_EVENT_ACTION_MOVE:
      input.type = gesture::InputType::kMove;
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      input.type = gesture::InputType::kUp;
      break;
    case UI_TOUCH_EVENT_ACTION_CANCEL:
      input.type = gesture::InputType::kCancel;
      break;
    default:
      return;
  }
  const bool down = input.type == gesture::InputType::kDown;
  if (!down && !active_pointer_) {
    return;
  }
  const uint32_t count = OH_ArkUI_PointerEvent_GetPointerCount(event);
  uint32_t index = 0;
  for (; index < count; ++index) {
    const int32_t id = OH_ArkUI_PointerEvent_GetPointerId(event, index);
    if ((down || id == *active_pointer_) &&
        dispatcher->IsActiveFinger(event, index)) {
      break;
    }
  }
  if (index == count && input.type != gesture::InputType::kCancel) {
    return;
  }
  const double native_ms = OH_ArkUI_UIInputEvent_GetEventTime(event) / 1e6;
  if (down) {
    active_pointer_ = OH_ArkUI_PointerEvent_GetPointerId(event, index);
    input.sequence_id++;
    velocity_ = {};
    const double now = MonotonicTimeMs();
    native_to_monotonic_ms_ = now - native_ms;
    monotonic_to_epoch_ms_ = EpochTimeMs() - now;
  }
  input.pointer_id = *active_pointer_;
  input.monotonic_time_ms = native_ms + native_to_monotonic_ms_;
  input.epoch_time_ms =
      static_cast<int64_t>(input.monotonic_time_ms + monotonic_to_epoch_ms_);
  if (index < count) {
    const float density = context_->ScaledDensity();
    float page[2];
    dispatcher->GetEventPagePoint(page, event, index, density);
    input.page = {page[0], page[1]};
    input.client = {
        OH_ArkUI_PointerEvent_GetWindowXByIndex(event, index) / density,
        OH_ArkUI_PointerEvent_GetWindowYByIndex(event, index) / density};
    input.screen = {
        OH_ArkUI_PointerEvent_GetDisplayXByIndex(event, index) / density,
        OH_ArkUI_PointerEvent_GetDisplayYByIndex(event, index) / density};
  }
  input.delta = down ? gesture::Point{}
                     : gesture::Point{last_input_.page.x - input.page.x,
                                      last_input_.page.y - input.page.y};
  input.velocity =
      input.type == gesture::InputType::kUp ? velocity_ : gesture::Point{};
  input.fling_finished = false;
  last_input_ = input;
  if (input.type == gesture::InputType::kUp ||
      input.type == gesture::InputType::kCancel) {
    active_pointer_.reset();
    velocity_ = {};
  }
  arena_.HandleInput(input,
                     down ? response_chain_ : std::vector<gesture::MemberId>{});
}

void UnifiedGestureHandler::CancelInteraction() {
  auto input = last_input_;
  input.type = gesture::InputType::kCancel;
  input.monotonic_time_ms = MonotonicTimeMs();
  input.epoch_time_ms = EpochTimeMs();
  input.velocity = {};
  active_pointer_.reset();
  velocity_ = {};
  arena_.HandleInput(input);
}

void UnifiedGestureHandler::SetVelocity(float x, float y) {
  const float density = context_->ScaledDensity();
  velocity_ = {x / density, y / density};
}

std::shared_ptr<UIBase> UnifiedGestureHandler::FindMember(
    gesture::MemberId member_id) const {
  const auto it = members_.find(member_id);
  return it == members_.end() ? nullptr : it->second.lock();
}

bool UnifiedGestureHandler::IsMemberValid(gesture::MemberId member_id) const {
  return FindMember(member_id) != nullptr;
}

gesture::Point UnifiedGestureHandler::ConvertPageToMember(
    gesture::MemberId member_id, const gesture::Point& page) const {
  if (auto view = FindMember(member_id)) {
    float point[2] = {static_cast<float>(page.x), static_cast<float>(page.y)};
    float local[2];
    LynxUIHelper::ConvertPointFromAncestorToDescendant(
        local, context_->GetUIOwner()->Root(), view.get(), point);
    return {local[0], local[1]};
  }
  return {};
}

gesture::ScrollState UnifiedGestureHandler::GetScrollState(
    gesture::MemberId member_id) const {
  if (auto view = FindMember(member_id)) {
    return {view->ScrollX(), view->ScrollY(), view->IsAtBorder(true),
            view->IsAtBorder(false)};
  }
  return {};
}

gesture::GestureDirection UnifiedGestureHandler::GetScrollDirection(
    gesture::MemberId member_id) const {
  if (auto view = FindMember(member_id)) {
    switch (view->GetScrollContainerDirection()) {
      case -1:
        return gesture::GestureDirection::kHorizontal;
      case 1:
        return gesture::GestureDirection::kVertical;
    }
  }
  return gesture::GestureDirection::kUndetermined;
}

bool UnifiedGestureHandler::CanConsumeGesture(
    gesture::MemberId member_id, gesture::GestureDirection direction,
    const gesture::Point& delta) const {
  auto view = FindMember(member_id);
  return view && view->CanConsumeGesture(delta.x, delta.y);
}

bool UnifiedGestureHandler::ShouldConsumeGesture(
    gesture::MemberId member_id) const {
  auto view = FindMember(member_id);
  return view && view->ShouldConsumeGesture() &&
         !context_->GetUIOwner()
              ->GetEventDispatcher()
              ->ShouldInterceptGesture();
}

gesture::ScrollResult UnifiedGestureHandler::ScrollBy(
    gesture::MemberId member_id, const gesture::Point& delta) {
  if (auto view = FindMember(member_id)) {
    const auto result = view->GestureScrollBy(delta.x, delta.y);
    return {{result[0], result[1]}, {result[2], result[3]}};
  }
  return {{}, delta};
}

void UnifiedGestureHandler::OnGestureRecognized(gesture::MemberId member_id) {
  if (auto view = FindMember(member_id)) {
    context_->OnGestureRecognized(view.get());
  }
}

void UnifiedGestureHandler::OnGestureStateChanged(gesture::MemberId member_id,
                                                  uint32_t gesture_id,
                                                  gesture::GestureState state) {
}

void UnifiedGestureHandler::DispatchGestureEvent(
    const gesture::GestureEvent& event) {
  auto view = FindMember(event.member_id);
  if (!view) {
    return;
  }
  const char* name = gesture::GestureCallbackName(event.callback);
  auto params = lepus::Dictionary::Create();
  params->SetValue("type",
                   lepus::Value(gesture::GestureInputTypeName(event.source)));
  params->SetValue("timestamp", lepus::Value(event.timestamp_epoch_ms));
  params->SetValue("x", lepus::Value(event.local.x));
  params->SetValue("y", lepus::Value(event.local.y));
  params->SetValue("pageX", lepus::Value(event.page.x));
  params->SetValue("pageY", lepus::Value(event.page.y));
  params->SetValue("clientX", lepus::Value(event.client.x));
  params->SetValue("clientY", lepus::Value(event.client.y));
  params->SetValue("scrollX", lepus::Value(event.scroll.x));
  params->SetValue("scrollY", lepus::Value(event.scroll.y));
  params->SetValue("deltaX", lepus::Value(event.delta.x));
  params->SetValue("deltaY", lepus::Value(event.delta.y));
  params->SetValue("isAtStart", lepus::Value(event.scroll.is_at_start));
  params->SetValue("isAtEnd", lepus::Value(event.scroll.is_at_end));
  context_->HandleGestureEvent(
      GestureEvent(view->Sign(), event.gesture_id, name, lepus::Value(params)));
}

void UnifiedGestureHandler::ScheduleTimer(gesture::TimerToken token,
                                          double delay_ms) {
  auto timer = std::make_unique<fml::OneshotTimer>(context_->GetUITaskRunner());
  timer->Start(fml::TimeDelta::FromMicroseconds(std::max<int64_t>(
                   1, static_cast<int64_t>(std::ceil(delay_ms * 1000)))),
               [weak = weak_from_this(), token]() {
                 auto self = weak.lock();
                 if (!self) return;
                 const auto it = self->timers_.find(token);
                 if (it == self->timers_.end()) return;
                 auto fired_timer = std::move(it->second);
                 self->timers_.erase(it);
                 self->arena_.HandleTimer(token, MonotonicTimeMs(),
                                          EpochTimeMs());
               });
  timers_.insert_or_assign(token, std::move(timer));
}

void UnifiedGestureHandler::CancelTimer(gesture::TimerToken token) {
  timers_.erase(token);
}

bool UnifiedGestureHandler::StartFling(const gesture::Point& velocity) {
  const float density = context_->ScaledDensity();
  const float x = velocity.x * density;
  const float y = velocity.y * density;
  if (std::abs(x) <= GestureConstants::FLING_SPEED_THRESHOLD &&
      std::abs(y) <= GestureConstants::FLING_SPEED_THRESHOLD) {
    return false;
  }
  fling_scroller_->Start(
      x, y,
      [weak = weak_from_this(), density, sequence_id = last_input_.sequence_id](
          uint8_t state, float dx, float dy) {
        auto self = weak.lock();
        if (!self || self->stopping_fling_ ||
            self->last_input_.sequence_id != sequence_id)
          return;
        auto input = self->last_input_;
        input.type = gesture::InputType::kFlingFrame;
        input.monotonic_time_ms = MonotonicTimeMs();
        input.epoch_time_ms = EpochTimeMs();
        input.delta = {dx / density, dy / density};
        input.fling_finished =
            state == static_cast<uint8_t>(FlingScroller::FlingState::IDLE);
        self->arena_.HandleInput(input);
      });
  return true;
}

void UnifiedGestureHandler::StopFling() {
  base::AutoReset<bool> resetter(&stopping_fling_, true);
  fling_scroller_->Stop();
}

}  // namespace lynx::tasm::harmony
