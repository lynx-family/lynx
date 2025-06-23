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

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
