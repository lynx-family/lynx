// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/unified_gesture_handler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>
#include <vector>

#include "base/include/auto_reset.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture_handler/gesture_handler_dispatcher.h"
#include "clay/ui/gesture_handler/handler/fling_scroller.h"

namespace clay {
namespace {

constexpr float kFlingSpeedThreshold = 300;

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

UnifiedGestureHandler::UnifiedGestureHandler(PageView* page_view)
    : page_view_(page_view),
      fling_scroller_(
          std::make_unique<FlingScroller>(page_view->GetAnimationHandler())),
      arena_(*this) {}

UnifiedGestureHandler::~UnifiedGestureHandler() { Reset(); }

UnifiedGestureHandler::EntryScope::EntryScope(UnifiedGestureHandler& owner)
    : owner_(owner) {
  ++owner_.entry_depth_;
}

UnifiedGestureHandler::EntryScope::~EntryScope() { owner_.LeaveEntry(); }

void UnifiedGestureHandler::LeaveEntry() {
  --entry_depth_;
  if (entry_depth_ == 0 && reset_requested_ && !reset_complete_) {
    ResetNow();
  }
}

void UnifiedGestureHandler::Reset() {
  if (reset_requested_ || reset_complete_) {
    return;
  }
  reset_requested_ = true;
  if (entry_depth_ == 0) {
    ResetNow();
  }
}

void UnifiedGestureHandler::ResetNow() {
  reset_complete_ = true;
  active_pointer_.reset();
  arena_.Reset();
  timers_.clear();
  StopFling();
  members_.clear();
}

int UnifiedGestureHandler::ReplaceMember(BaseView* view,
                                         const GestureMap& detectors) {
  EntryScope entry(*this);
  if (reset_requested_) {
    return 0;
  }
  int member_id = view->GestureArenaMemberId();
  std::vector<unified_gesture::GestureDefinition> definitions;
  for (const auto& entry : detectors) {
    definitions.push_back(entry.second);
  }
  if (definitions.empty()) {
    RemoveMember(member_id);
    return 0;
  }
  if (!members_.count(member_id)) {
    member_id = next_member_id_++;
    members_.emplace(member_id, view->GetWeakPtr());
  }
  arena_.ReplaceMemberGestures(member_id, std::move(definitions));
  return arena_.ContainsMember(member_id) ? member_id : 0;
}

void UnifiedGestureHandler::RemoveMember(int member_id) {
  EntryScope entry(*this);
  if (reset_requested_) {
    return;
  }
  arena_.RemoveMember(member_id);
  members_.erase(member_id);
}

void UnifiedGestureHandler::SetGestureState(int member_id, uint32_t gesture_id,
                                            int state) {
  EntryScope entry(*this);
  if (reset_requested_ || state < 1 || state > 3) {
    return;
  }
  arena_.SetGestureState(
      member_id, gesture_id,
      static_cast<unified_gesture::GestureStateCommand>(state));
}

bool UnifiedGestureHandler::ContainsGesture(int member_id,
                                            uint32_t gesture_id) const {
  return arena_.ContainsGesture(member_id, gesture_id);
}

void UnifiedGestureHandler::HandlePointerEvent(const PointerEvent& event,
                                               const HitTestResult& hits) {
  EntryScope entry(*this);
  if (reset_requested_) {
    return;
  }
  const bool down = event.type == PointerEvent::EventType::kDownEvent;
  if (down) {
    if (active_pointer_) {
      return;
    }
    std::vector<unified_gesture::MemberId> expired_members;
    for (const auto& [member_id, view] : members_) {
      if (!view) {
        expired_members.push_back(member_id);
      }
    }
    for (const auto member_id : expired_members) {
      RemoveMember(member_id);
    }
    active_pointer_ = event.pointer_id;
    velocity_tracker_.Clear();
    ++last_input_.sequence_id;
    const double now = MonotonicTimeMs();
    native_to_monotonic_ms_ = now - event.timestamp / 1000.0;
    monotonic_to_epoch_ms_ = EpochTimeMs() - now;
  } else if (!active_pointer_ || *active_pointer_ != event.pointer_id) {
    return;
  }

  auto input = last_input_;
  switch (event.type) {
    case PointerEvent::EventType::kDownEvent:
      input.type = unified_gesture::InputType::kDown;
      break;
    case PointerEvent::EventType::kMoveEvent:
      input.type = unified_gesture::InputType::kMove;
      break;
    case PointerEvent::EventType::kUpEvent:
      input.type = unified_gesture::InputType::kUp;
      break;
    case PointerEvent::EventType::kCancel:
      input.type = unified_gesture::InputType::kCancel;
      break;
    default:
      return;
  }
  input.pointer_id = event.pointer_id;
  input.monotonic_time_ms = event.timestamp / 1000.0 + native_to_monotonic_ms_;
  input.epoch_time_ms =
      static_cast<int64_t>(input.monotonic_time_ms + monotonic_to_epoch_ms_);
  input.page = ToLogical(event.position.x(), event.position.y());
  input.screen = input.client = input.page;
  input.delta = down
                    ? unified_gesture::Point{}
                    : unified_gesture::Point{last_input_.page.x - input.page.x,
                                             last_input_.page.y - input.page.y};
  input.velocity = {};
  input.fling_finished = false;
  velocity_tracker_.AddPosition(event.position, event.timestamp,
                                input.type == unified_gesture::InputType::kUp);
  if (input.type == unified_gesture::InputType::kUp) {
    const auto velocity = velocity_tracker_.GetVelocityEstimate();
    input.velocity = ToLogical(velocity.pixels_per_second.width(),
                               velocity.pixels_per_second.height());
  }

  std::vector<unified_gesture::MemberId> chain;
  if (down) {
    for (const auto& hit : hits) {
      if (hit) {
        auto* view = static_cast<BaseView*>(hit.get());
        if (arena_.ContainsMember(view->GestureArenaMemberId())) {
          chain.push_back(view->GestureArenaMemberId());
        }
      }
    }
  }
  last_input_ = input;
  arena_.HandleInput(input, chain);
  if (input.type == unified_gesture::InputType::kUp ||
      input.type == unified_gesture::InputType::kCancel) {
    active_pointer_.reset();
  }
}

void UnifiedGestureHandler::SetVelocity(FloatPoint velocity) {
  last_input_.velocity = ToLogical(velocity.x(), velocity.y());
}

BaseView* UnifiedGestureHandler::FindMember(
    unified_gesture::MemberId member_id) const {
  if (reset_requested_) {
    return nullptr;
  }
  const auto it = members_.find(member_id);
  return it == members_.end() ? nullptr : it->second.get();
}

bool UnifiedGestureHandler::IsMemberValid(
    unified_gesture::MemberId member_id) const {
  return FindMember(member_id) != nullptr;
}

unified_gesture::Point UnifiedGestureHandler::ToLogical(float x,
                                                        float y) const {
  return {page_view_->ConvertTo<kPixelTypeLogical>(x),
          page_view_->ConvertTo<kPixelTypeLogical>(y)};
}

unified_gesture::Point UnifiedGestureHandler::ConvertPageToMember(
    unified_gesture::MemberId member_id,
    const unified_gesture::Point& page) const {
  if (auto* view = FindMember(member_id)) {
    FloatPoint page_in_member_tree{
        static_cast<float>(page_view_->ConvertFrom<kPixelTypeLogical>(page.x)),
        static_cast<float>(page_view_->ConvertFrom<kPixelTypeLogical>(page.y))};
    for (auto* ancestor = view; ancestor; ancestor = ancestor->Parent()) {
      if (!ancestor->Is<OverlayView>()) {
        continue;
      }
      auto* overlay = static_cast<OverlayView*>(ancestor);
      if (overlay->ShouldChangeOffset()) {
        const auto offset = overlay->GetTouchOffset();
        page_in_member_tree.Move(-offset.x(), -offset.y());
      }
      break;
    }
    const auto local = view->GetPointBySelf(page_in_member_tree);
    return ToLogical(local.x(), local.y());
  }
  return {};
}

unified_gesture::ScrollState UnifiedGestureHandler::GetScrollState(
    unified_gesture::MemberId member_id) const {
  if (auto* view = FindMember(member_id)) {
    const auto position = ToLogical(view->ScrollX(), view->ScrollY());
    return {position.x, position.y, view->IsAtBorder(true),
            view->IsAtBorder(false)};
  }
  return {};
}

unified_gesture::GestureDirection UnifiedGestureHandler::GetScrollDirection(
    unified_gesture::MemberId member_id) const {
  if (auto* view = FindMember(member_id)) {
    switch (view->GetScrollContainerDirection()) {
      case -1:
        return unified_gesture::GestureDirection::kHorizontal;
      case 1:
        return unified_gesture::GestureDirection::kVertical;
    }
  }
  return unified_gesture::GestureDirection::kUndetermined;
}

bool UnifiedGestureHandler::CanConsumeGesture(
    unified_gesture::MemberId member_id,
    unified_gesture::GestureDirection direction,
    const unified_gesture::Point& delta) const {
  auto* view = FindMember(member_id);
  return view && view->CanConsumeGesture(
                     page_view_->ConvertFrom<kPixelTypeLogical>(delta.x),
                     page_view_->ConvertFrom<kPixelTypeLogical>(delta.y));
}

bool UnifiedGestureHandler::ShouldConsumeGesture(
    unified_gesture::MemberId member_id) const {
  auto* view = FindMember(member_id);
  return view && view->ShouldConsumeGesture() &&
         !page_view_->gesture_manager()->ShouldInterceptGesture();
}

unified_gesture::ScrollResult UnifiedGestureHandler::ScrollBy(
    unified_gesture::MemberId member_id, const unified_gesture::Point& delta) {
  if (auto* view = FindMember(member_id)) {
    const auto result = view->GestureScrollBy(
        page_view_->ConvertFrom<kPixelTypeLogical>(delta.x),
        page_view_->ConvertFrom<kPixelTypeLogical>(delta.y));
    return {ToLogical(result[0], result[1]), ToLogical(result[2], result[3])};
  }
  return {{}, delta};
}

void UnifiedGestureHandler::OnGestureRecognized(
    unified_gesture::MemberId member_id) {
  if (auto* view = FindMember(member_id)) {
    page_view_->GetGestureHandlerDispatcher()->OnGestureRecognizedWithSign(
        view->Sign());
  }
}

void UnifiedGestureHandler::OnGestureStateChanged(
    unified_gesture::MemberId member_id, uint32_t gesture_id,
    unified_gesture::GestureState state) {}

void UnifiedGestureHandler::DispatchGestureEvent(
    const unified_gesture::GestureEvent& event) {
  auto* view = FindMember(event.member_id);
  auto* delegate = page_view_->GetEventDelegate();
  if (!view || !delegate) {
    return;
  }
  const char* name = unified_gesture::GestureCallbackName(event.callback);
  Value::Map params;
  params.emplace("type",
                 Value(unified_gesture::GestureInputTypeName(event.source)));
  params.emplace("timestamp", Value(event.timestamp_epoch_ms));
  params.emplace("x", Value(event.local.x));
  params.emplace("y", Value(event.local.y));
  params.emplace("pageX", Value(event.page.x));
  params.emplace("pageY", Value(event.page.y));
  params.emplace("clientX", Value(event.client.x));
  params.emplace("clientY", Value(event.client.y));
  params.emplace("scrollX", Value(event.scroll.x));
  params.emplace("scrollY", Value(event.scroll.y));
  params.emplace("deltaX", Value(event.delta.x));
  params.emplace("deltaY", Value(event.delta.y));
  params.emplace("isAtStart", Value(event.scroll.is_at_start));
  params.emplace("isAtEnd", Value(event.scroll.is_at_end));
  delegate->OnUnifiedGestureHandlerEvent(name, view->Sign(), event.gesture_id,
                                         Value(std::move(params)));
}

void UnifiedGestureHandler::ScheduleTimer(unified_gesture::TimerToken token,
                                          double delay_ms) {
  auto timer = std::make_unique<fml::OneshotTimer>(page_view_->GetTaskRunner());
  std::weak_ptr<UnifiedGestureHandler> weak_self = weak_from_this();
  timer->Start(fml::TimeDelta::FromMicroseconds(std::max<int64_t>(
                   1, static_cast<int64_t>(std::ceil(delay_ms * 1000)))),
               [weak_self, token]() {
                 auto self = weak_self.lock();
                 if (!self) {
                   return;
                 }
                 EntryScope entry(*self);
                 if (self->reset_requested_) {
                   return;
                 }
                 const auto it = self->timers_.find(token);
                 if (it == self->timers_.end()) {
                   return;
                 }
                 auto fired_timer = std::move(it->second);
                 self->timers_.erase(it);
                 self->arena_.HandleTimer(token, MonotonicTimeMs(),
                                          EpochTimeMs());
               });
  timers_.insert_or_assign(token, std::move(timer));
}

void UnifiedGestureHandler::CancelTimer(unified_gesture::TimerToken token) {
  timers_.erase(token);
}

bool UnifiedGestureHandler::StartFling(const unified_gesture::Point& velocity) {
  const float x = page_view_->ConvertFrom<kPixelTypeLogical>(velocity.x);
  const float y = page_view_->ConvertFrom<kPixelTypeLogical>(velocity.y);
  if (std::abs(x) <= kFlingSpeedThreshold &&
      std::abs(y) <= kFlingSpeedThreshold) {
    return false;
  }
  std::weak_ptr<UnifiedGestureHandler> weak_self = weak_from_this();
  fling_scroller_->Start(
      x, y,
      [weak_self, sequence_id = last_input_.sequence_id](uint8_t state,
                                                         float dx, float dy) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        EntryScope entry(*self);
        if (self->reset_requested_ || self->stopping_fling_ ||
            self->last_input_.sequence_id != sequence_id) {
          return;
        }
        auto input = self->last_input_;
        input.type = unified_gesture::InputType::kFlingFrame;
        input.monotonic_time_ms = MonotonicTimeMs();
        input.epoch_time_ms = EpochTimeMs();
        input.delta = self->ToLogical(dx, dy);
        input.fling_finished =
            state == static_cast<uint8_t>(FlingScroller::FlingState::IDLE);
        self->arena_.HandleInput(input);
      });
  return true;
}

void UnifiedGestureHandler::StopFling() {
  lynx::base::AutoReset<bool> resetter(&stopping_fling_, true);
  if (!fling_scroller_->IsIdle()) {
    fling_scroller_->Stop();
  }
}

}  // namespace clay
