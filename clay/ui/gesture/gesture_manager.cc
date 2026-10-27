// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/gesture_manager.h"

#include <cmath>
#include <memory>
#include <utility>

#include "base/include/fml/time/timer.h"
#include "clay/common/service/service_manager.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/gesture_recognizer.h"
#include "clay/ui/gesture/macros.h"

namespace clay {
namespace {
// treat the scrolling events those time difference within 500ms as the same
// scrolling event.
// TODO(yuchi): on mac platfrom, the touchpad will simulate the fling effect by
// hardware, so the delay will be longer than 500ms. A better solution will be
// researched in future.
static const int kWheelScrollDelay = 500;

}  // namespace

GestureManager::GestureManager(fml::RefPtr<fml::TaskRunner> task_runner,
                               std::shared_ptr<ServiceManager> service_manager)
    : arena_manager_(std::make_unique<ArenaManager>()),
      task_runner_(std::move(task_runner)) {
  if (service_manager) {
    gesture_mediate_puppet_ =
        service_manager->GetService<GestureMediateService>();
  }
}

bool GestureManager::HandlePointerEvents(
    HitTestable* root, const std::vector<PointerEvent>& events) {
  FML_DCHECK(root);
  GESTURE_LOG << "Manager" << this
              << " HandlePointerEvents size:" << events.size();
  bool consumed = false;
  for (const auto& event : events) {
    GESTURE_LOG << "Handle pointer: " << event.ToString();
    consumed |= HandlePointerEvent(root, event);
  }
  return consumed;
}

bool GestureManager::HandlePointerEvent(HitTestable* root,
                                        const PointerEvent& event) {
  bool consumed = true;
  if (event.type == PointerEvent::EventType::kDownEvent ||
      event.type == PointerEvent::EventType::kSignalEvent ||
      event.type == PointerEvent::EventType::kPanZoomStartEvent) {
    // Due to the possibility of multiple touch events, reset is only performed
    // on the first DownEvent. Since the Tap gesture is recognized after the
    // UpEvent, clean up to retain only the initial DownEvent of the first
    // finger.
    if (gesture_mediate_puppet_ && hit_tests_.empty() &&
        (event.type == PointerEvent::EventType::kDownEvent)) {
      bool has_outer_gestures = false;
      gesture_mediate_puppet_.Act([&](auto& impl) {
        static_assert(
            is_owner_same_thread<Owner::kPlatform, Owner::kUI>::value,
            "Currently GestureMediateService relies on UI and Platform in the "
            "same thread");
        has_outer_gestures =
            impl.GetOuterScrollableDirection() != ScrollableDirection::kNone;
      });
      arena_manager_->SetHasOuterGestures(has_outer_gestures);
      down_event_point_ = event.position;
      down_event_pointer_id_ = event.pointer_id;
      consume_slide_event_status_ = ConsumeSlideEventStatus::kUndefined;
      ResetHitTestTargetResponsive();
    }
    // First pointer, do hittest
    // Override old value.
    hit_tests_.insert_or_assign(event.pointer_id, HitTestResult{});
    gesture_accepted_map_.insert_or_assign(event.pointer_id,
                                           GestureRecognizerType::kNone);
    root->HitTest(event, hit_tests_[event.pointer_id]);

    if ((!hit_tests_[event.pointer_id].empty()) &&
        hit_tests_[event.pointer_id].front().get()->ShouldPassEventToNative()) {
      // clicked in a event pass node , clay should not handle this event;
      // we achive this by clear hit test result, since the following gesture
      // should not be handled as well.
      hit_tests_[event.pointer_id].clear();
      consumed = false;
    }
    DispatchEvent(event, &hit_tests_[event.pointer_id]);
  } else if (event.type == PointerEvent::EventType::kMoveEvent ||
             event.type == PointerEvent::EventType::kPanZoomUpdateEvent) {
    DispatchEvent(event, &hit_tests_[event.pointer_id]);
  } else if (event.type == PointerEvent::EventType::kUpEvent ||
             event.type == PointerEvent::EventType::kCancel ||
             event.type == PointerEvent::EventType::kPanZoomEndEvent) {
    auto iter = hit_tests_.find(event.pointer_id);
    if (iter != hit_tests_.end()) {
      DispatchEvent(event, &iter->second);
      hit_tests_.erase(iter);
      gesture_accepted_map_.erase(event.pointer_id);
    } else {
      consumed = false;
    }
    if (hit_tests_.empty()) {
      // All pointers are empty, current gesture is over. Reset hit test target
      // responsive.
      ResetHitTestTargetResponsive();
    }
  } else {
    // TODO(yulitao): Events like hover / cancel may enter here.
    consumed = false;
  }

  if (wheel_end_dispatch_timer_ && !wheel_end_dispatch_timer_->Stopped() &&
      event.type == PointerEvent::EventType::kHoverEvent) {
    wheel_end_dispatch_timer_->FireImmediately();
  }
  if (event.type == PointerEvent::EventType::kDownEvent ||
      event.type == PointerEvent::EventType::kPanZoomStartEvent) {
    GESTURE_LOG << "close arena";
    arena_manager_->Close(event);
  } else if (event.type == PointerEvent::EventType::kUpEvent ||
             event.type == PointerEvent::EventType::kPanZoomEndEvent) {
    GESTURE_LOG << "try sweep arena";
    arena_manager_->Sweep(event.pointer_id);
  } else if (event.type == PointerEvent::EventType::kSignalEvent) {
    HandleSignalEvent(event);
  }
  return consumed;
}

// This logic should keep consistent with Lynx. See:
// TouchEventDispatcher.java#consumeSlideEvent
bool GestureManager::ConsumeSlideEvent(const PointerEvent& event) {
  const auto& hit_test_result = hit_tests_[event.pointer_id];

  if (event.type == PointerEvent::EventType::kDownEvent) {
    consume_slide_event_status_ = ConsumeSlideEventStatus::kUndefined;
    can_consume_slide_event_ = false;
    if (!hit_test_result.empty()) {
      for (auto target : hit_test_result) {
        if (target && target->HasConsumeSlideEventAngles()) {
          can_consume_slide_event_ = true;
          break;
        }
      }
    }
    return false;
  } else if (event.type == PointerEvent::EventType::kMoveEvent) {
    if (!can_consume_slide_event_) {
      return false;
    }

    float distance_x = event.position.x() - down_event_point_.x();
    float distance_y = event.position.y() - down_event_point_.y();

    if (abs(distance_x) <= ConvertFrom<kPixelTypeLogical>(kTouchSlop) &&
        abs(distance_y) <= ConvertFrom<kPixelTypeLogical>(kTouchSlop)) {
      return false;
    }

    if (consume_slide_event_status_ == ConsumeSlideEventStatus::kUndefined) {
      consume_slide_event_status_ = ConsumeSlideEventStatus::kUnconsumed;
      float angle = atan2(distance_y, distance_x) * 180 / M_PI;
      for (auto target : hit_test_result) {
        if (target && target->ConsumeSlideEvent(angle)) {
          consume_slide_event_status_ = ConsumeSlideEventStatus::kConsumed;
          break;
        }
      }
    }
  }
  return consume_slide_event_status_ == ConsumeSlideEventStatus::kConsumed;
}

void GestureManager::DispatchEvent(const PointerEvent& event,
                                   HitTestResult* hit_test_result) {
  auto event_copy = event;
  bool consumed = ConsumeSlideEvent(event);
  UpdateHitTestTargetResponsive(
      event.pointer_id, event.type == PointerEvent::EventType::kDownEvent);
  if (consumed) {
    if (event.type == PointerEvent::EventType::kUpEvent ||
        event.type == PointerEvent::EventType::kCancel) {
      // We must send the cancel event, otherwise the tracking pointers of
      // gestures won't be cleaned and DidStopTrackingLastPointer can't be
      // called.
      event_copy.type = PointerEvent::EventType::kCancel;
    } else {
      return;
    }
  }

  if (hit_test_result) {
    for (auto target : *hit_test_result) {
      if (target) {
        target->HandleEvent(event_copy);
      }
    }
  }
  DispatchEventByRouter(event_copy);
}

void GestureManager::DispatchEventByRouter(const PointerEvent& event) {
  pointer_router_.Route(event);
}

void GestureManager::FireEndSignalEvent() {
  PointerEvent event(PointerEvent::EventType::kSignalEvent);
  event.signal_kind = PointerEvent::SignalKind::kEndScroll;
  if (signal_event_route_) {
    signal_event_route_(event);
    signal_event_route_ = nullptr;
  }
  prev_event_ = nullptr;
  more_likely_scroll_on_x_ = false;
}

void GestureManager::OnGestureAccepted(int pointer_id,
                                       GestureRecognizerType type) {
  if (const auto& it = gesture_accepted_map_.find(pointer_id);
      it != gesture_accepted_map_.end()) {
    it->second = type;
    UpdateHitTestTargetResponsive(pointer_id, false);
  }
}

void GestureManager::HandleSignalEvent(const PointerEvent& event) {
  // if mouse moving or scrolling direction changing during scroll event, end
  // current gesture.
  bool same_direction =
      prev_event_
          ? (event.scroll_delta_x * prev_event_->scroll_delta_x >= 0) &&
                (event.scroll_delta_y * prev_event_->scroll_delta_y >= 0)
          : true;
  if (event.type != PointerEvent::EventType::kSignalEvent || !same_direction) {
    if (wheel_end_dispatch_timer_ && !wheel_end_dispatch_timer_->Stopped()) {
      wheel_end_dispatch_timer_->FireImmediately();
    }
    return;
  }
  if (signal_event_route_) {
    if (prev_event_ == nullptr) {
      if (event.scroll_delta_x == 0.f && event.scroll_delta_y == 0.f) {
        return;
      }
      PointerEvent temp_event(PointerEvent::EventType::kSignalEvent);
      temp_event.signal_kind = PointerEvent::SignalKind::kStartScroll;
      if (std::abs(event.scroll_delta_x) > std::abs(event.scroll_delta_y)) {
        more_likely_scroll_on_x_ = true;
        temp_event.scroll_delta_x = event.scroll_delta_x;
        temp_event.scroll_delta_y = 0.f;
      } else {
        more_likely_scroll_on_x_ = false;
        temp_event.scroll_delta_y = event.scroll_delta_y;
        temp_event.scroll_delta_x = 0.f;
      }
      signal_event_route_(temp_event);
      prev_event_ = std::make_shared<PointerEvent>(event);
    }

    PointerEvent temp_event = PointerEvent(event);
    if (more_likely_scroll_on_x_) {
      temp_event.scroll_delta_y = 0.f;
    } else {
      temp_event.scroll_delta_x = 0.f;
    }
    signal_event_route_(temp_event);

    wheel_end_dispatch_timer_ =
        std::make_unique<fml::OneshotTimer>(GetTaskRunner());
    wheel_end_dispatch_timer_->Start(
        fml::TimeDelta::FromMilliseconds(kWheelScrollDelay),
        [this] { FireEndSignalEvent(); });
  } else {
    if (wheel_end_dispatch_timer_) wheel_end_dispatch_timer_->Stop();
  }
}

void GestureManager::RegisterSignalRoute(const SignalEventRoute& route) {
  if (!signal_event_route_) {
    signal_event_route_ = route;
  }
}

void GestureManager::ResetHitTestTargetResponsive() {
  hit_test_responsive_result_ = {};
  if (gesture_mediate_puppet_) {
    gesture_mediate_puppet_.Act(
        [responsive_result = hit_test_responsive_result_](auto& impl) {
          impl.UpdateResponsiveResult(responsive_result);
        });
  }
}

void GestureManager::UpdateHitTestTargetResponsive(int pointer_id,
                                                   bool is_down) {
  HitTestResponsiveResult old_value = hit_test_responsive_result_;
  if (is_down) {
    hit_test_responsive_result_ = HitTestResponsiveResult();
  }
  hit_test_responsive_result_.scrollable_direction = ScrollableDirection::kNone;
  for (auto target : hit_tests_[pointer_id]) {
    if (!target) {
      continue;
    }
    hit_test_responsive_result_.scrollable_direction =
        hit_test_responsive_result_.scrollable_direction |
        target->GetScrollableDirection();
    if (is_down) {
      // These values don't change on touch move.
      hit_test_responsive_result_.tappable |=
          target->HasTapGestureRecognizer() || target->HasTapEvent();
      hit_test_responsive_result_.should_block_native_event |=
          target->ShouldBlockNativeEvent();
      hit_test_responsive_result_.has_consume_slide_event |=
          target->HasConsumeSlideEventAngles();
      // Considering that longpress callback bound in the node is accomplished
      // by IsolatedGestureDetector, such node only has longpress in its
      // events_. So we don't take `HasLongPressGestureRecognizer` into
      // consideration.
      hit_test_responsive_result_.has_longpress_event |=
          target->HasLongPressEvent();
    }
  }

  if (const auto& it = gesture_accepted_map_.find(pointer_id);
      it != gesture_accepted_map_.end()) {
    hit_test_responsive_result_.recognized_gesture_type = it->second;
  }
  hit_test_responsive_result_.slide_event_consumed =
      consume_slide_event_status_ == ConsumeSlideEventStatus::kConsumed;

  if (gesture_mediate_puppet_ &&
      hit_test_responsive_result_.value != old_value.value) {
    gesture_mediate_puppet_.Act(
        [responsive_result = hit_test_responsive_result_](auto& impl) {
          impl.UpdateResponsiveResult(responsive_result);
        });
  }
}

}  // namespace clay
