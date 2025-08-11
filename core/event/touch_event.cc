// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/touch_event.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/event/event_target.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace event {

static int64_t g_unique_touch_event_id_ = 0;
static int64_t GetNextUniqueTouchEventID() {
  return g_unique_touch_event_id_++;
}

bool TouchEvent::long_press_consumed_ = false;

TouchEvent::TouchEvent(const std::string& event_name, float x, float y,
                       float page_x, float page_y, float client_x,
                       float client_y, float time_stamp)
    : Event(event_name, time_stamp, Event::EventType::kTouchEvent,
            Event::Bubbles::kYes, Event::Cancelable::kYes,
            Event::ComposedMode::kComposed, Event::PhaseType::kNone),
      x_(x),
      y_(y),
      page_x_(page_x),
      page_y_(page_y),
      client_x_(client_x),
      client_y_(client_y),
      identifier_(GetNextUniqueTouchEventID()) {
  event_type_ = Event::EventType::kTouchEvent;
}

TouchEvent::TouchEvent(const std::string& event_name,
                       const lepus::Value& targets_touches, float time_stamp)
    : Event(event_name, time_stamp, Event::EventType::kTouchEvent,
            Event::Bubbles::kYes, Event::Cancelable::kYes,
            Event::ComposedMode::kComposed, Event::PhaseType::kNone),
      is_multi_touch_(true),
      targets_touches_(targets_touches) {
  event_type_ = Event::EventType::kTouchEvent;
}

void TouchEvent::HandleEventCustomDetail() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TOUCH_EVENT_CUSTOM_DETAIL, "name", type_);
  if (!target_) {
    return;
  }
  BASE_STATIC_STRING_DECL(kX, "x");
  BASE_STATIC_STRING_DECL(kY, "y");
  BASE_STATIC_STRING_DECL(kPageX, "pageX");
  BASE_STATIC_STRING_DECL(kPageY, "pageY");
  BASE_STATIC_STRING_DECL(kClientX, "clientX");
  BASE_STATIC_STRING_DECL(kClientY, "clientY");
  BASE_STATIC_STRING_DECL(kIdentifier, "identifier");
  BASE_STATIC_STRING_DECL(kDetail, "detail");
  BASE_STATIC_STRING_DECL(kTouches, "touches");
  BASE_STATIC_STRING_DECL(kChangedTouches, "changedTouches");

  auto dict = detail_.Table();
  float layouts_unit_per_px = target_->GetLayoutsUnitPerPx();
  auto detail = lepus::Dictionary::Create();

  if (is_multi_touch_) {
    bool enable_multi_touch_params_compatible =
        target_->GetEnableMultiTouchParamsCompatible();

    if (type_ == EVENT_TOUCH_CANCEL) {
      if (enable_multi_touch_params_compatible) {
        dict.get()->SetValue(kChangedTouches,
                             lepus::Value::Clone(GetCurrentTouches()));
        dict.get()->SetValue(kTouches,
                             lepus::Value::Clone(GetCurrentTouches()));
      } else {
        dict.get()->SetValue(kChangedTouches,
                             lepus::Value::Clone(GetCurrentTouches()));
        dict.get()->SetValue(kTouches, lepus::CArray::Create());
      }
      return;
    }

    float detail_x = 0.f, detail_y = 0.f;
    auto changed_touches = lepus::CArray::Create();
    auto touches_with_deleted = lepus::Value::Clone(GetCurrentTouches());
    for (const auto& target_touches : *(targets_touches_.Table())) {
      if (target_touches.second.IsArray()) {
        // touches: includes all touches on the same target.
        auto touches = target_touches.second.Array();
        for (size_t i = 0; i < touches->size(); ++i) {
          const auto& touch_info = touches->get(i).Array();

          int64_t identifier =
              static_cast<int64_t>(touch_info->get(0).Number());
          float client_x = touch_info->get(1).Number();
          float client_y = touch_info->get(2).Number();
          float page_x = touch_info->get(3).Number();
          float page_y = touch_info->get(4).Number();
          float x = touch_info->get(5).Number();
          float y = touch_info->get(6).Number();

          if (identifier == 0) {
            detail_x = page_x / layouts_unit_per_px;
            detail_y = page_y / layouts_unit_per_px;
          }

          auto touch = lepus::Dictionary::Create();
          touch->SetValue(kPageX, page_x / layouts_unit_per_px);
          touch->SetValue(kPageY, page_y / layouts_unit_per_px);
          touch->SetValue(kClientX, client_x / layouts_unit_per_px);
          touch->SetValue(kClientY, client_y / layouts_unit_per_px);
          touch->SetValue(kX, x / layouts_unit_per_px);
          touch->SetValue(kY, y / layouts_unit_per_px);
          touch->SetValue(kIdentifier, identifier);

          auto touch_value = lepus_value(std::move(touch));
          changed_touches->push_back(touch_value);

          // Find whether this touch's identifier is in current_touches_. If
          // it isn't in current_touches_, then insert it, or modify or delete
          // it
          bool in_current_touches = false;
          const auto& current_touches = GetCurrentTouches().Array();
          for (size_t j = 0; j < current_touches->size(); ++j) {
            if (current_touches->get(j)
                    .Table()
                    ->GetValue(kIdentifier)
                    .Number() == identifier) {
              in_current_touches = true;
              // delete this touch when touchend
              if (type_ == EVENT_TOUCH_END) {
                current_touches->Erase(static_cast<uint32_t>(j));
                break;
              }
              // modify this touch when touchmove
              current_touches->set(j, touch_value);
              break;
            }
          }
          // current_touches_ doesn't include this touch, add it into
          // current_touches_
          if (type_ == EVENT_TOUCH_START && !in_current_touches) {
            current_touches->emplace_back(std::move(touch_value));
          }
        }
      }
    }

    detail.get()->SetValue(kX, detail_x);
    detail.get()->SetValue(kY, detail_y);
    dict.get()->SetValue(kDetail, std::move(detail));
    dict.get()->SetValue(kChangedTouches, std::move(changed_touches));

    // When the event type is touchend, the touches parameter needs to be
    // compatible.
    if (enable_multi_touch_params_compatible && type_ == EVENT_TOUCH_END) {
      dict.get()->SetValue(kTouches, std::move(touches_with_deleted));
    } else {
      dict.get()->SetValue(kTouches, lepus::Value::Clone(GetCurrentTouches()));
    }
  } else {
    detail.get()->SetValue(kX, page_x_ / layouts_unit_per_px);
    detail.get()->SetValue(kY, page_y_ / layouts_unit_per_px);
    dict.get()->SetValue(kDetail, std::move(detail));

    auto touch = lepus::Dictionary::Create();
    int64_t identifier = reinterpret_cast<int64_t>(&touch);
    touch.get()->SetValue(kIdentifier, identifier);
    touch.get()->SetValue(kX, x_ / layouts_unit_per_px);
    touch.get()->SetValue(kY, y_ / layouts_unit_per_px);
    touch.get()->SetValue(kPageX, page_x_ / layouts_unit_per_px);
    touch.get()->SetValue(kPageY, page_y_ / layouts_unit_per_px);
    touch.get()->SetValue(kClientX, client_x_ / layouts_unit_per_px);
    touch.get()->SetValue(kClientY, client_y_ / layouts_unit_per_px);

    auto touch_value = lepus_value(std::move(touch));
    auto touches = lepus::CArray::Create();
    touches.get()->push_back(touch_value);
    dict.get()->SetValue(kTouches, std::move(touches));

    auto changed_touches = lepus::CArray::Create();
    changed_touches.get()->emplace_back(std::move(touch_value));
    dict.get()->SetValue(kChangedTouches, std::move(changed_touches));
  }
}

bool TouchEvent::HandleEventConflictAndParam() {
  // TODO(hexionghui): Check whether it is the first finger.
  if (type_ == EVENT_TOUCH_START) {
    TouchEvent::long_press_consumed_ = false;
  }
  if (TouchEvent::long_press_consumed_ && type_ == EVENT_TAP) {
    return true;
  }
  if (type_ == EVENT_TOUCH_CANCEL) {
    GetCurrentTouches() = lepus::Value(lepus::CArray::Create());
  }
  return false;
}

lepus::Value& TouchEvent::GetCurrentTouches() {
  // Save the finger information on the screen before the event is distributed.
  static lepus::Value current_touches = lepus::Value(lepus::CArray::Create());
  return current_touches;
}

}  // namespace event
}  // namespace lynx
