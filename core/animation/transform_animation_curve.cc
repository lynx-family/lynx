// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transform_animation_curve.h"

#include <cmath>
#include <limits>
#include <optional>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/animation_trace_event_def.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/renderer/css/transforms/transform_operations_helper.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "gfx/animation/animation_utils.h"

namespace lynx {
namespace animation {

namespace {

std::optional<gfx::TransformOperations> ResolveTransformForElement(
    const tasm::CSSValue& value, tasm::Element* element) {
  const auto layout_result = element->layout_result();
  return transforms::ResolveTransformOperations(
      value, CSSKeyframeManager::GetLengthContext(element),
      element->element_manager()->GetCSSParserConfigs(),
      layout_result.size_.width_, layout_result.size_.height_);
}

}  // namespace

//====== TransformValueAnimator begin =======
std::unique_ptr<TransformKeyframe> TransformKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<TransformKeyframe>(time, std::move(timing_function));
}

TransformKeyframe::TransformKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::Keyframe(time, std::move(timing_function)) {}

void TransformKeyframe::NotifyElementSizeUpdated() {
  // Resolved calc() translations depend on the element's current size.
  value_.reset();
}

void TransformKeyframe::NotifyUnitValuesUpdated(uint32_t) {
  // CSS unit dependency tracking belongs to the Lynx resolver. Re-resolve the
  // raw CSS value on the next sample instead of leaking CSSValuePattern to gfx.
  value_.reset();
}

gfx::TransformOperations TransformKeyframe::GetTransformKeyframeValueInElement(
    tasm::Element* element) {
  tasm::CSSValue transform =
      GetStyleInElement(tasm::kPropertyIDTransform, element);
  if (transform.IsArray()) {
    auto resolved = ResolveTransformForElement(transform, element);
    if (resolved) {
      return std::move(*resolved);
    }
  }
  return gfx::TransformOperations();
}

bool TransformKeyframe::SetValue(tasm::CSSPropertyID id,
                                 const tasm::CSSValue& value,
                                 tasm::Element* element) {
  css_value_ = value;
  auto keyframe_transform_value = value;
  if (element != nullptr) {
    keyframe_transform_value = HandleCSSVariableValueIfNeed(id, value, element);
  }
  if (!keyframe_transform_value.IsArray()) {
    value_.reset();
    return false;
  }
  if (element != nullptr) {
    auto resolved =
        ResolveTransformForElement(keyframe_transform_value, element);
    value_ =
        resolved
            ? std::make_unique<gfx::TransformOperations>(std::move(*resolved))
            : nullptr;
  } else {
    value_.reset();
  }
  MarkNonEmpty();
  return true;
}

bool TransformKeyframe::EnsureResolvedValue(tasm::CSSPropertyID id,
                                            tasm::Element* element) {
  if (value_) {
    return true;
  }
  auto keyframe_transform_value =
      HandleCSSVariableValueIfNeed(id, css_value_, element);
  if (!keyframe_transform_value.IsArray()) {
    return false;
  }
  auto resolved = ResolveTransformForElement(keyframe_transform_value, element);
  if (!resolved) {
    return false;
  }
  value_ = std::make_unique<gfx::TransformOperations>(std::move(*resolved));
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
  auto transform_in_element = gfx::TransformOperations();

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

  gfx::TransformOperations& start_transform =
      keyframe->IsEmpty() ? transform_in_element : *keyframe->Value();
  gfx::TransformOperations& end_transform =
      keyframe_next->IsEmpty() ? transform_in_element : *keyframe_next->Value();

  const auto layout_result = element_->layout_result();
  gfx::TransformOperations blended_result =
      end_transform.Blend(start_transform, progress, layout_result.size_.width_,
                          layout_result.size_.height_);
  const auto& measure_context = CSSKeyframeManager::GetLengthContext(element_);
  return transforms::ConvertToCSSValue(blended_result,
                                       measure_context.layouts_unit_per_px_);
}

//====== TransformValueAnimator end =======

std::unique_ptr<gfx::Keyframe> TransformAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return TransformKeyframe::Create(offset, nullptr);
}

}  // namespace animation
}  // namespace lynx
