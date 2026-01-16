// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/animation/animation_info.h"

#include "base/include/value/array.h"

namespace lynx {
namespace tasm {
namespace harmony {

size_t AnimationInfo::SetTimingFunction(lepus::CArray* array, size_t start) {
  if (array->size() < 6) {
    SetTimingType(0);
    SetStepsType(0);
    SetX1(0);
    SetY1(0);
    SetX2(0);
    SetY2(0);
    return start;
  }
  SetTimingType(array->get(start).Number());
  SetStepsType(array->get(start + 1).Number());
  SetX1(array->get(start + 2).Number());
  SetY1(array->get(start + 3).Number());
  SetX2(array->get(start + 4).Number());
  SetY2(array->get(start + 5).Number());
  return start + 6;
}

AnimationInfo AnimationInfo::ToAnimationInfo(const lepus::Value& v) {
  AnimationInfo info;
  const auto& arr = v.Array();
  size_t idx = 0;
  info.SetName(arr->get(idx++).StdString());
  info.SetDuration(arr->get(idx++).Number());
  idx = info.SetTimingFunction(arr.get(), idx);
  info.SetDelay(arr->get(idx++).Number());
  info.SetIterationCount(arr->get(idx++).Number());
  info.SetDirection(arr->get(idx++).Number());
  info.SetFillMode(arr->get(idx++).Number());
  info.SetPlayState(arr->get(idx++).Number());
  return info;
}

bool AnimationInfo::IsEqualTo(const AnimationInfo& info) const {
  return IsEqualExceptPlayState(info) && play_state_ == info.play_state_;
}

bool AnimationInfo::IsOnlyPlayStateChanged(const AnimationInfo& info) const {
  return IsEqualExceptPlayState(info) && play_state_ != info.play_state_;
}
ArkUI_AnimationFillMode AnimationInfo::GetArkUIAnimationFillMode() const {
  switch (fill_mode_) {
    case 1:
      return ARKUI_ANIMATION_FILL_MODE_FORWARDS;
    case 2:
      return ARKUI_ANIMATION_FILL_MODE_BACKWARDS;
    case 3:
      return ARKUI_ANIMATION_FILL_MODE_BOTH;
    default:
    case 0:
      return ARKUI_ANIMATION_FILL_MODE_NONE;
  }
}

ArkUI_AnimationDirection AnimationInfo::GetArkUIAnimationDirection() const {
  switch (direction_) {
    case 1:
      return ARKUI_ANIMATION_DIRECTION_REVERSE;
    case 2:
      return ARKUI_ANIMATION_DIRECTION_ALTERNATE;
    case 3:
      return ARKUI_ANIMATION_DIRECTION_ALTERNATE_REVERSE;
    default:
    case 0:
      return ARKUI_ANIMATION_DIRECTION_NORMAL;
  }
}

bool AnimationInfo::IsEqualExceptPlayState(const AnimationInfo& info) const {
  return name_ == info.name_ && duration_ == info.duration_ &&
         delay_ == info.delay_ && timing_type_ == info.timing_type_ &&
         x1_ == info.x1_ && y1_ == info.y1_ && x2_ == info.x2_ &&
         y2_ == info.y2_ && steps_type_ == info.steps_type_ &&
         iteration_count_ == info.iteration_count_ &&
         fill_mode_ == info.fill_mode_ && direction_ == info.direction_;
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
