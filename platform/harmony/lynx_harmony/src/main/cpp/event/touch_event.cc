// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"

namespace lynx {
namespace tasm {
namespace harmony {

void TouchEvent::SetTargetPoint(float target_point[2]) {
  target_point_[0] = target_point[0];
  target_point_[1] = target_point[1];
}

void TouchEvent::GetTargetPoint(float target_point[2]) const {
  target_point[0] = target_point_[0];
  target_point[1] = target_point_[1];
}

void TouchEvent::SetPagePoint(float page_point[2]) {
  page_point_[0] = page_point[0];
  page_point_[1] = page_point[1];
}

void TouchEvent::GetPagePoint(float page_point[2]) const {
  page_point[0] = page_point_[0];
  page_point[1] = page_point_[1];
}

void TouchEvent::SetClientPoint(float client_point[2]) {
  client_point_[0] = client_point[0];
  client_point_[1] = client_point[1];
}

void TouchEvent::GetClientPoint(float client_point[2]) const {
  client_point[0] = client_point_[0];
  client_point[1] = client_point_[1];
}

lepus::Value TouchEvent::EventParams() const {
  auto event_params = lepus::CArray::Create();
  event_params->emplace_back(Name());
  event_params->emplace_back(static_cast<int>(EventType()));
  event_params->emplace_back(ID());
  event_params->emplace_back(TimeStamp());
  event_params->emplace_back(EventID());

  auto event_detail = lepus::CArray::Create();
  event_detail->emplace_back(IsMultiTouch());
  if (IsMultiTouch()) {
    event_detail->emplace_back(lepus::Value::Clone(UITouchMap()));
  } else {
    event_detail->emplace_back(client_point_[0]);
    event_detail->emplace_back(client_point_[1]);
    event_detail->emplace_back(page_point_[0]);
    event_detail->emplace_back(page_point_[1]);
    event_detail->emplace_back(target_point_[0]);
    event_detail->emplace_back(target_point_[1]);
    if (!current_target_points_.empty()) {
      auto current_target_points = lepus::CArray::Create();
      for (const auto& point : current_target_points_) {
        auto entry = lepus::CArray::Create();
        entry->emplace_back(point.element_id);
        entry->emplace_back(point.x);
        entry->emplace_back(point.y);
        current_target_points->emplace_back(std::move(entry));
      }
      event_detail->emplace_back(std::move(current_target_points));
    }
  }
  event_params->emplace_back(std::move(event_detail));
  return lepus::Value(event_params);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
