// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transform_animation_curve.h"

#include <cmath>
#include <limits>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/animation_trace_event_def.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/renderer/css/transforms/transform_operations.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "gfx/animation/animation_utils.h"

namespace lynx {
namespace animation {

//====== TransformValueAnimator begin =======
std::unique_ptr<TransformKeyframe> TransformKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<TransformKeyframe>(time, std::move(timing_function));
}

TransformKeyframe::TransformKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::Keyframe(time, std::move(timing_function)) {}

void TransformKeyframe::NotifyElementSizeUpdated() {
  if (value_) {
    value_->NotifyElementSizeUpdated();
  }
}

// When view or font size has changed, mark the value 'AutoNLength'.
void TransformKeyframe::NotifyUnitValuesUpdated(uint32_t type) {
  if (value_) {
    value_->NotifyUnitValuesUpdatedToAnimation(
        static_cast<tasm::CSSValuePattern>(type));
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

bool TransformKeyframe::SetValue(tasm::CSSPropertyID id,
                                 const tasm::CSSValue& value,
                                 tasm::Element* element) {
  auto keyframe_transform_value =
      HandleCSSVariableValueIfNeed(id, value, element);
  if (!keyframe_transform_value.IsArray()) {
    return false;
  }
  css_value_ = value;
  value_.reset();
  MarkNonEmpty();
  return true;
}

bool TransformKeyframe::EnsureResolvedValue(tasm::CSSPropertyID id,
                                            tasm::Element* element) {
  if (value_ && !value_->GetOperations().empty()) {
    return true;
  }
  auto keyframe_transform_value =
      HandleCSSVariableValueIfNeed(id, css_value_, element);
  if (!keyframe_transform_value.IsArray()) {
    return false;
  }
  value_ = std::make_unique<transforms::TransformOperations>(
      element, keyframe_transform_value);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_TRANSFORM_ANIMATION_CURVE_GET_VALUE,
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("TransformAnimation");
              });
  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  DCHECK(sampling.valid);
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;
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

  // Keep transform keyframes in raw CSS form until sampling, then parse them
  // with the current element context before blending.
  if (!keyframe->IsEmpty() &&
      !keyframe->EnsureResolvedValue(static_cast<tasm::CSSPropertyID>(Type()),
                                     element_)) {
    return keyframe->CSSValue();
  }
  if (!keyframe_next->IsEmpty() &&
      !keyframe_next->EnsureResolvedValue(
          static_cast<tasm::CSSPropertyID>(Type()), element_)) {
    return keyframe_next->CSSValue();
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

std::unique_ptr<gfx::Keyframe> TransformAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return TransformKeyframe::Create(offset, nullptr);
}

}  // namespace animation
}  // namespace lynx
