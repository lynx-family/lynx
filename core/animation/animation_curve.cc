// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/animation_curve.h"

#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/transform_animation_curve.h"
#include "core/style/animation_data.h"

namespace lynx {
namespace animation {

namespace {

template <typename KeyframeType>
void NotifyElementSizeUpdated(gfx::Keyframe* keyframe) {
  static_cast<KeyframeType*>(keyframe)->NotifyElementSizeUpdated();
}

template <typename KeyframeType>
void NotifyUnitValuesUpdated(gfx::Keyframe* keyframe, uint32_t type) {
  static_cast<KeyframeType*>(keyframe)->NotifyUnitValuesUpdated(type);
}

}  // namespace

gfx::TimingFunctionData ToGfxTimingFunctionData(
    const starlight::TimingFunctionData& data) {
  gfx::TimingFunctionData result;
  result.x1 = data.x1;
  result.y1 = data.y1;
  result.x2 = data.x2;
  result.y2 = data.y2;
  result.timing_func = static_cast<gfx::TimingFunctionType>(data.timing_func);
  result.steps_type = static_cast<gfx::StepsType>(data.steps_type);
  return result;
}

gfx::AnimationData ToGfxAnimationData(const starlight::AnimationData& data) {
  gfx::AnimationData result;
  result.duration = data.duration;
  result.delay = data.delay;
  result.timing_func = ToGfxTimingFunctionData(data.timing_func);
  result.iteration_count = data.iteration_count;
  result.fill_mode = static_cast<gfx::AnimationFillModeType>(data.fill_mode);
  result.direction = static_cast<gfx::AnimationDirectionType>(data.direction);
  result.play_state = static_cast<gfx::AnimationPlayStateType>(data.play_state);
  return result;
}

KeyframeCallbacks MakeKeyframeCallbacks(gfx::Keyframe*) { return {}; }

KeyframeCallbacks MakeKeyframeCallbacks(LayoutKeyframe* keyframe) {
  return {keyframe, nullptr, NotifyUnitValuesUpdated<LayoutKeyframe>};
}

KeyframeCallbacks MakeKeyframeCallbacks(FilterKeyframe* keyframe) {
  return {keyframe, nullptr, NotifyUnitValuesUpdated<FilterKeyframe>};
}

KeyframeCallbacks MakeKeyframeCallbacks(BackgroundPositionKeyframe* keyframe) {
  return {keyframe, nullptr,
          NotifyUnitValuesUpdated<BackgroundPositionKeyframe>};
}

KeyframeCallbacks MakeKeyframeCallbacks(TransformOriginKeyframe* keyframe) {
  return {keyframe, nullptr, NotifyUnitValuesUpdated<TransformOriginKeyframe>};
}

KeyframeCallbacks MakeKeyframeCallbacks(TransformKeyframe* keyframe) {
  return {keyframe, NotifyElementSizeUpdated<TransformKeyframe>,
          NotifyUnitValuesUpdated<TransformKeyframe>};
}

void AnimationCurve::AddKeyframe(std::unique_ptr<gfx::Keyframe> keyframe,
                                 KeyframeCallbacks callbacks) {
  if (callbacks.keyframe && (callbacks.notify_element_size_updated ||
                             callbacks.notify_unit_values_updated)) {
    keyframe_callbacks_.push_back(callbacks);
  }
  gfx::AnimationCurve::AddKeyframe(std::move(keyframe));
}

void AnimationCurve::NotifyElementSizeUpdated() {
  for (auto& callbacks : keyframe_callbacks_) {
    if (callbacks.keyframe && callbacks.notify_element_size_updated) {
      callbacks.notify_element_size_updated(callbacks.keyframe);
    }
  }
}

void AnimationCurve::NotifyUnitValuesUpdated(tasm::CSSValuePattern type) {
  for (auto& callbacks : keyframe_callbacks_) {
    if (callbacks.keyframe && callbacks.notify_unit_values_updated) {
      callbacks.notify_unit_values_updated(callbacks.keyframe,
                                           static_cast<uint32_t>(type));
    }
  }
}

std::unique_ptr<gfx::Keyframe> LayoutAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return LayoutKeyframe::Create(offset, nullptr);
}

std::unique_ptr<gfx::Keyframe> OpacityAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return OpacityKeyframe::Create(offset, nullptr);
}

std::unique_ptr<gfx::Keyframe> ColorAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return ColorKeyframe::Create(offset, nullptr);
}

std::unique_ptr<gfx::Keyframe> FloatAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return FloatKeyframe::Create(offset, nullptr);
}

std::unique_ptr<gfx::Keyframe> FilterAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return FilterKeyframe::Create(offset, nullptr);
}

std::unique_ptr<gfx::Keyframe>
BackgroundPositionAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return BackgroundPositionKeyframe::Create(offset, nullptr);
}

std::unique_ptr<gfx::Keyframe> TransformOriginAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return TransformOriginKeyframe::Create(offset, nullptr);
}

std::unique_ptr<gfx::Keyframe> VisibilityAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return VisibilityKeyframe::Create(offset, nullptr);
}

}  // namespace animation
}  // namespace lynx
