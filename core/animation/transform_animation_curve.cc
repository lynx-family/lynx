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

std::optional<transforms::ResolvedTransformOperations>
ResolveTransformForElement(const tasm::CSSValue& value,
                           tasm::Element* element) {
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
    : gfx::TransformKeyframe(time, std::move(timing_function)) {}

void TransformKeyframe::NotifyElementSizeUpdated() {
  if (HasResolvedValue() && depends_on_element_size_) {
    ClearResolvedValue();
  }
}

void TransformKeyframe::NotifyUnitValuesUpdated(uint32_t type) {
  if (HasResolvedValue() &&
      type < static_cast<uint32_t>(tasm::CSSValuePattern::COUNT) &&
      (unit_dependencies_ & (1u << type)) != 0) {
    ClearResolvedValue();
  }
}

bool TransformKeyframe::HasDynamicDependencies() const {
  constexpr uint32_t kStableUnitDependencies =
      (1u << static_cast<uint32_t>(tasm::CSSValuePattern::EMPTY)) |
      (1u << static_cast<uint32_t>(tasm::CSSValuePattern::NUMBER)) |
      (1u << static_cast<uint32_t>(tasm::CSSValuePattern::PX)) |
      (1u << static_cast<uint32_t>(tasm::CSSValuePattern::PPX)) |
      // A pure percentage remains typed in gfx::TransformOperations. Whether
      // the target can resolve it is decided by backend capability.
      (1u << static_cast<uint32_t>(tasm::CSSValuePattern::PERCENT));
  return depends_on_element_size_ ||
         (unit_dependencies_ & ~kStableUnitDependencies) != 0;
}

bool TransformKeyframe::SetValue(tasm::CSSPropertyID id,
                                 const tasm::CSSValue& value,
                                 tasm::Element* element) {
  css_value_ = value;
  ClearResolvedValue();
  depends_on_element_size_ = false;
  unit_dependencies_ = 0;
  auto keyframe_transform_value = value;
  if (element != nullptr) {
    keyframe_transform_value = HandleCSSVariableValueIfNeed(id, value, element);
  }
  if (!keyframe_transform_value.IsArray()) {
    return false;
  }
  if (element != nullptr) {
    auto resolved =
        ResolveTransformForElement(keyframe_transform_value, element);
    if (resolved) {
      depends_on_element_size_ = resolved->depends_on_element_size;
      unit_dependencies_ = resolved->unit_dependencies;
      SetResolvedValue(std::move(resolved->operations));
    }
  }
  MarkNonEmpty();
  return true;
}

bool TransformKeyframe::EnsureResolvedValue(tasm::CSSPropertyID id,
                                            tasm::Element* element) {
  if (HasResolvedValue()) {
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
  depends_on_element_size_ = resolved->depends_on_element_size;
  unit_dependencies_ = resolved->unit_dependencies;
  SetResolvedValue(std::move(resolved->operations));
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
  // A synthesized endpoint must use the animation's underlying transform,
  // rather than the final computed style that may contain the previous
  // animation sample. The new styling pipeline keeps a separate live base
  // style; the legacy pipeline uses the value captured when the curve is
  // created.
  tasm::CSSValue underlying_value;
  gfx::TransformOperations underlying_transform;
  if (keyframe->IsEmpty() || keyframe_next->IsEmpty()) {
    underlying_value = GetUnderlyingValue();
    auto resolved = ResolveTransformForElement(underlying_value, element_);
    if (resolved) {
      underlying_transform = std::move(resolved->operations);
    }
  }

  if (std::fabs(progress - 0.0f) < std::numeric_limits<float>::epsilon()) {
    return keyframe->IsEmpty() ? underlying_value : keyframe->CSSValue();
  }
  if (std::fabs(progress - 1.0f) < std::numeric_limits<float>::epsilon()) {
    return keyframe_next->IsEmpty() ? underlying_value
                                    : keyframe_next->CSSValue();
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

  const gfx::TransformOperations& start_transform =
      keyframe->IsEmpty() ? underlying_transform : keyframe->ResolvedValue();
  const gfx::TransformOperations& end_transform =
      keyframe_next->IsEmpty() ? underlying_transform
                               : keyframe_next->ResolvedValue();

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
