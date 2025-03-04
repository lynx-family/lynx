// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transform_animation_curve.h"

#include <limits>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/transforms/transform_operations.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace animation {

//====== TransformValueAnimator begin =======
std::unique_ptr<TransformKeyframe> TransformKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function) {
  return std::make_unique<TransformKeyframe>(time, std::move(timing_function));
}

TransformKeyframe::TransformKeyframe(
    fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function)
    : Keyframe(time, std::move(timing_function)) {}

void TransformKeyframe::NotifyElementSizeUpdated() {
  if (value_) {
    value_->NotifyElementSizeUpdated();
  }
}

// When view or font size has changed, mark the value 'AutoNLength'.
void TransformKeyframe::NotifyUnitValuesUpdatedToAnimation(
    tasm::CSSValuePattern type) {
  if (value_) {
    value_->NotifyUnitValuesUpdatedToAnimation(type);
  }
}

transforms::TransformOperations
TransformKeyframe::GetTransformKeyframeValueInElement(tasm::Element* element) {
  tasm::CSSValue transform =
      GetStyleInElement(tasm::kPropertyIDTransform, element);
  if (transform.IsArray()) {
    return transforms::TransformOperations(element, transform);
  } else {
    return transforms::TransformOperations(element);
  }
}

bool TransformKeyframe::SetValue(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
    tasm::Element* element) {
  auto keyframe_transform_value =
      HandleCSSVariableValueIfNeed(css_value_pair, element);
  if (!keyframe_transform_value.IsArray()) {
    return false;
  }
  auto transform = std::make_unique<transforms::TransformOperations>(
      element, keyframe_transform_value);
  value_ = std::move(transform);
  css_value_ = css_value_pair.second;
  is_empty_ = false;
  return true;
}

std::unique_ptr<KeyframedTransformAnimationCurve>
KeyframedTransformAnimationCurve::Create() {
  return std::make_unique<KeyframedTransformAnimationCurve>();
}

// Using for getting the corresponding transform style value based on the local
// time passed in. The local time is converted from monotonic time of VSYNC.
//
// Details: This method get the active keyframe based on the local time passed
// in firstly. Then it gets the progress between the active keyframe and the
// next one. It gets the start transform value from the active keyframe and the
// end transform value from the keyframe next to the active keyframe. If the
// keyframe is empty, use the transform value in element instead. Finally, blend
// the start transform and end transform based on the progress, and return the
// blend result as the real time style of animation.
tasm::CSSValue KeyframedTransformAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframedTransformAnimationCurve::GetValue",
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("TransformAnimation");
              });
  t = TransformedAnimationTime(keyframes_, timing_function_, scaled_duration(),
                               t);
  size_t i = GetActiveKeyframe(keyframes_, scaled_duration(), t);
  double progress =
      TransformedKeyframeProgress(keyframes_, scaled_duration(), t, i);
  TransformKeyframe* keyframe =
      static_cast<TransformKeyframe*>(keyframes_[i].get());
  TransformKeyframe* keyframe_next =
      static_cast<TransformKeyframe*>(keyframes_[i + 1].get());
  auto transform_in_element = transforms::TransformOperations(nullptr);

  if (std::fabs(progress - 0.0f) < std::numeric_limits<float>::epsilon()) {
    return keyframe->IsEmpty()
               ? GetStyleInElement(tasm::kPropertyIDTransform, element_)
               : keyframe->CSSValue();
  }
  if (std::fabs(progress - 1.0f) < std::numeric_limits<float>::epsilon()) {
    return keyframe_next->IsEmpty()
               ? GetStyleInElement(tasm::kPropertyIDTransform, element_)
               : keyframe_next->CSSValue();
  }

  if (keyframe->IsEmpty() || keyframe_next->IsEmpty()) {
    transform_in_element =
        TransformKeyframe::GetTransformKeyframeValueInElement(element_);
  }

  // When view or font size has changed, let start_len and end_len be
  // 'AutoNLength', and then get The actual Nlength based on the updated size.
  if (keyframe->Value() && keyframe->Value()->GetOperations().empty()) {
    auto prev_temp_pair = std::make_pair(
        static_cast<tasm::CSSPropertyID>(Type()), keyframe->CSSValue());
    keyframe->SetValue(prev_temp_pair, element_);
  }
  if (keyframe_next->Value() &&
      keyframe_next->Value()->GetOperations().empty()) {
    auto next_temp_pair = std::make_pair(
        static_cast<tasm::CSSPropertyID>(Type()), keyframe_next->CSSValue());
    keyframe_next->SetValue(next_temp_pair, element_);
  }

  transforms::TransformOperations& start_transform =
      keyframe->IsEmpty() ? transform_in_element : *keyframe->Value();
  transforms::TransformOperations& end_transform =
      keyframe_next->IsEmpty() ? transform_in_element : *keyframe_next->Value();

  transforms::TransformOperations blended_result =
      end_transform.Blend(start_transform, progress);
  return blended_result.ToTransformRawValue();
}

//====== TransformValueAnimator end =======

std::unique_ptr<Keyframe> TransformAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return TransformKeyframe::Create(offset, nullptr);
}

}  // namespace animation
}  // namespace lynx
