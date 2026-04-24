// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/keyframed_animation_curve.h"

#include <cmath>
#include <limits>
#include <optional>

#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/animation_trace_event_def.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/types/nlength.h"
#include "gfx/animation/animation_utils.h"

namespace lynx {
namespace animation {

namespace {

std::optional<gfx::LengthValue> ToLengthValueFromNLength(
    const starlight::NLength& length, tasm::CSSPropertyID id,
    tasm::Element* element) {
  if (length.IsUnit()) {
    return gfx::LengthValue{length.GetRawValue(), gfx::LengthUnit::kNumber};
  }
  if (length.IsPercent()) {
    return gfx::LengthValue{length.GetRawValue(), gfx::LengthUnit::kPercent};
  }
  if (length.IsCalc()) {
    if (!element || !element->parent()) {
      return std::nullopt;
    }
    float parent_length = 0.0f;
    if (GetOnXAxisCurveTypeSet().count(
            static_cast<AnimationCurve::CurveType>(id)) != 0) {
      parent_length = element->parent()->width();
    } else {
      parent_length = element->parent()->height();
    }
    return gfx::LengthValue{starlight::NLengthToLayoutUnit(
                                length, starlight::LayoutUnit(parent_length))
                                .ToFloat(),
                            gfx::LengthUnit::kNumber};
  }
  return std::nullopt;
}

std::optional<gfx::LengthValue> ResolveLayoutLengthValue(
    tasm::CSSPropertyID id, const tasm::CSSValue& value,
    tasm::Element* element) {
  if (element == nullptr) {
    return std::nullopt;
  }
  auto keyframe_layout_value = HandleCSSVariableValueIfNeed(id, value, element);
  auto parse_result = starlight::CSSStyleUtils::ToLength(
      keyframe_layout_value, CSSKeyframeManager::GetLengthContext(element),
      element->element_manager()->GetCSSParserConfigs());
  if (!parse_result.second) {
    return std::nullopt;
  }
  return ToLengthValueFromNLength(parse_result.first, id, element);
}

std::optional<tasm::CSSValue> MakeNonCalcLengthCSSValue(
    const lepus::Value& value, tasm::CSSValuePattern css_pattern) {
  // A raw calc expression needs its original CSSValue string; this scalar path
  // only carries a value plus pattern.
  if (css_pattern == tasm::CSSValuePattern::CALC) {
    return std::nullopt;
  }
  return tasm::CSSValue(value, css_pattern);
}

struct ResolvedLengthComponent {
  double value{0.0};
  tasm::CSSValuePattern css_pattern{tasm::CSSValuePattern::NUMBER};
};

struct RawFilterValue {
  starlight::FilterType filter_type{starlight::FilterType::kNone};
  lepus::Value amount;
  tasm::CSSValuePattern amount_pattern{tasm::CSSValuePattern::EMPTY};
};

struct RawVec2Value {
  tasm::CSSValuePattern x_pattern{tasm::CSSValuePattern::EMPTY};
  lepus::Value x_value;
  tasm::CSSValuePattern y_pattern{tasm::CSSValuePattern::EMPTY};
  lepus::Value y_value;
};

enum class PositionAxis : uint8_t {
  kX,
  kY,
};

enum class Vec2CSSValueEncoding : uint8_t {
  kBackgroundPosition,
  kTransformOrigin,
};

std::optional<gfx::UnitTag> ToUnitTagFromCSSValuePattern(
    tasm::CSSValuePattern css_pattern);

std::optional<ResolvedLengthComponent> ResolveLengthComponent(
    const lepus::Value& value, tasm::CSSValuePattern pattern,
    tasm::Element* element) {
  if (element == nullptr) {
    return std::nullopt;
  }
  auto css_value = MakeNonCalcLengthCSSValue(value, pattern);
  if (!css_value) {
    return std::nullopt;
  }
  auto parse_result = starlight::CSSStyleUtils::ToLength(
      *css_value, CSSKeyframeManager::GetLengthContext(element),
      element->element_manager()->GetCSSParserConfigs());
  if (!parse_result.second) {
    return std::nullopt;
  }
  if (parse_result.first.IsPercent()) {
    return ResolvedLengthComponent{parse_result.first.GetRawValue(),
                                   tasm::CSSValuePattern::PERCENT};
  }
  if (parse_result.first.IsUnit()) {
    return ResolvedLengthComponent{parse_result.first.GetRawValue(),
                                   tasm::CSSValuePattern::NUMBER};
  }
  return std::nullopt;
}

std::optional<tasm::CSSValuePattern> ToCSSValuePatternFromUnitTag(
    gfx::UnitTag unit) {
  if (unit == gfx::UnitTag::kNumber) {
    return tasm::CSSValuePattern::NUMBER;
  }
  if (unit == gfx::UnitTag::kPercent) {
    return tasm::CSSValuePattern::PERCENT;
  }
  return std::nullopt;
}

std::optional<ResolvedLengthComponent> ResolveNumericFilterAmount(
    const lepus::Value& value, tasm::CSSValuePattern css_pattern) {
  if (css_pattern == tasm::CSSValuePattern::PERCENT) {
    return ResolvedLengthComponent{value.Number(),
                                   tasm::CSSValuePattern::PERCENT};
  }
  if (css_pattern == tasm::CSSValuePattern::NUMBER ||
      css_pattern == tasm::CSSValuePattern::PX) {
    return ResolvedLengthComponent{value.Number(),
                                   tasm::CSSValuePattern::NUMBER};
  }
  return std::nullopt;
}

std::optional<ResolvedLengthComponent> ResolveFilterAmount(
    starlight::FilterType filter_type, const lepus::Value& value,
    tasm::CSSValuePattern css_pattern, tasm::Element* element) {
  switch (filter_type) {
    case starlight::FilterType::kBlur:
      return ResolveLengthComponent(value, css_pattern, element);
    case starlight::FilterType::kGrayscale:
    case starlight::FilterType::kBrightness:
    case starlight::FilterType::kContrast:
    case starlight::FilterType::kSaturate:
      return ResolveNumericFilterAmount(value, css_pattern);
    case starlight::FilterType::kNone:
    case starlight::FilterType::kHueRotate:
      // kHueRotate exists in the enum table, but the current
      // filter parser and style decoder do not produce it as a supported value.
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<RawFilterValue> ParseRawFilterValue(
    const tasm::CSSValue& filter) {
  if (!filter.IsArray()) {
    return std::nullopt;
  }
  auto arr = filter.GetArray();
  if (arr->size() != 3) {
    return std::nullopt;
  }
  return RawFilterValue{
      static_cast<starlight::FilterType>(arr->get(0).UInt32()), arr->get(1),
      static_cast<tasm::CSSValuePattern>(arr->get(2).UInt32())};
}

std::optional<gfx::FilterValue> ResolveFilterValue(const RawFilterValue& raw,
                                                   tasm::Element* element) {
  auto amount = ResolveFilterAmount(raw.filter_type, raw.amount,
                                    raw.amount_pattern, element);
  if (!amount) {
    return std::nullopt;
  }
  auto unit = ToUnitTagFromCSSValuePattern(amount->css_pattern);
  if (!unit) {
    return std::nullopt;
  }
  return gfx::FilterValue{static_cast<uint32_t>(raw.filter_type), amount->value,
                          *unit};
}

std::optional<gfx::FilterValue> GetResolvedFilterValue(
    FilterKeyframe* keyframe, const RawFilterValue& raw,
    tasm::Element* element) {
  if (!keyframe->IsEmpty() && keyframe->HasResolvedValue()) {
    return keyframe->ResolvedValue();
  }
  auto resolved = ResolveFilterValue(raw, element);
  if (resolved && !keyframe->IsEmpty()) {
    keyframe->SetResolvedValue(*resolved);
  }
  return resolved;
}

std::pair<tasm::CSSValuePattern, lepus::Value>
NormalizeBackgroundPositionComponent(uint32_t position_or_pattern,
                                     const lepus::Value& position_value,
                                     PositionAxis axis) {
  auto make_percent = [](double value) {
    return std::make_pair(tasm::CSSValuePattern::PERCENT, lepus::Value(value));
  };

  auto position_type =
      static_cast<starlight::BackgroundPositionType>(position_or_pattern);
  if (position_type == starlight::BackgroundPositionType::kCenter) {
    return make_percent(50.0);
  }

  if (axis == PositionAxis::kX) {
    if (position_type == starlight::BackgroundPositionType::kLeft) {
      return make_percent(0.0);
    }
    if (position_type == starlight::BackgroundPositionType::kRight) {
      return make_percent(100.0);
    }
  } else {
    if (position_type == starlight::BackgroundPositionType::kTop) {
      return make_percent(0.0);
    }
    if (position_type == starlight::BackgroundPositionType::kBottom) {
      return make_percent(100.0);
    }
  }

  return std::make_pair(static_cast<tasm::CSSValuePattern>(position_or_pattern),
                        position_value);
}

std::optional<RawVec2Value> ParseRawBackgroundPositionValue(
    const tasm::CSSValue& background_position) {
  if (!background_position.IsArray()) {
    return std::nullopt;
  }
  auto outer = background_position.GetArray();
  if (outer->size() == 0 || !outer->get(0).IsArray()) {
    return std::nullopt;
  }
  auto arr = outer->get(0).Array();
  if (arr->size() < 4) {
    return std::nullopt;
  }
  auto [x_pattern, x_value] = NormalizeBackgroundPositionComponent(
      static_cast<uint32_t>(arr->get(0).Number()), arr->get(1),
      PositionAxis::kX);
  auto [y_pattern, y_value] = NormalizeBackgroundPositionComponent(
      static_cast<uint32_t>(arr->get(2).Number()), arr->get(3),
      PositionAxis::kY);
  return RawVec2Value{x_pattern, x_value, y_pattern, y_value};
}

std::optional<RawVec2Value> ParseRawTransformOriginValue(
    const tasm::CSSValue& transform_origin) {
  if (!transform_origin.IsArray()) {
    return std::nullopt;
  }
  auto arr = transform_origin.GetArray();
  if (arr->size() < 4) {
    return std::nullopt;
  }
  return RawVec2Value{
      static_cast<tasm::CSSValuePattern>(arr->get(1).UInt32()), arr->get(0),
      static_cast<tasm::CSSValuePattern>(arr->get(3).UInt32()), arr->get(2)};
}

std::optional<gfx::Vec2Tagged> ResolveVec2Value(const RawVec2Value& raw,
                                                tasm::Element* element) {
  auto x = ResolveLengthComponent(raw.x_value, raw.x_pattern, element);
  auto y = ResolveLengthComponent(raw.y_value, raw.y_pattern, element);
  if (!x || !y) {
    return std::nullopt;
  }
  auto x_tag = ToUnitTagFromCSSValuePattern(x->css_pattern);
  auto y_tag = ToUnitTagFromCSSValuePattern(y->css_pattern);
  if (!x_tag || !y_tag) {
    return std::nullopt;
  }
  return gfx::Vec2Tagged{gfx::TaggedNumber{x->value, *x_tag},
                         gfx::TaggedNumber{y->value, *y_tag}};
}

std::optional<gfx::UnitTag> ToUnitTagFromCSSValuePattern(
    tasm::CSSValuePattern css_pattern) {
  if (css_pattern == tasm::CSSValuePattern::NUMBER) {
    return gfx::UnitTag::kNumber;
  }
  if (css_pattern == tasm::CSSValuePattern::PX) {
    return gfx::UnitTag::kNumber;
  }
  if (css_pattern == tasm::CSSValuePattern::PERCENT) {
    return gfx::UnitTag::kPercent;
  }
  return std::nullopt;
}

std::optional<tasm::CSSValue> BuildVec2CSSValue(const gfx::Vec2Tagged& value,
                                                Vec2CSSValueEncoding encoding) {
  auto x_pattern = ToCSSValuePatternFromUnitTag(value.x.tag);
  auto y_pattern = ToCSSValuePatternFromUnitTag(value.y.tag);
  if (!x_pattern || !y_pattern) {
    return std::nullopt;
  }

  switch (encoding) {
    case Vec2CSSValueEncoding::kBackgroundPosition: {
      auto inner_array = lepus::CArray::Create();
      inner_array->emplace_back(static_cast<uint32_t>(*x_pattern));
      inner_array->emplace_back(value.x.value);
      inner_array->emplace_back(static_cast<uint32_t>(*y_pattern));
      inner_array->emplace_back(value.y.value);

      auto outer_array = lepus::CArray::Create();
      outer_array->emplace_back(std::move(inner_array));
      return tasm::CSSValue(std::move(outer_array));
    }
    case Vec2CSSValueEncoding::kTransformOrigin: {
      auto array = lepus::CArray::Create();
      array->emplace_back(value.x.value);
      array->emplace_back(static_cast<uint32_t>(*x_pattern));
      array->emplace_back(value.y.value);
      array->emplace_back(static_cast<uint32_t>(*y_pattern));
      return tasm::CSSValue(std::move(array));
    }
  }
  return std::nullopt;
}

template <typename KeyframeType>
std::optional<gfx::Vec2Tagged> GetResolvedVec2Value(KeyframeType* keyframe,
                                                    const RawVec2Value& raw,
                                                    tasm::Element* element) {
  if (!keyframe->IsEmpty() && keyframe->HasResolvedValue()) {
    return keyframe->ResolvedValue();
  }
  auto resolved = ResolveVec2Value(raw, element);
  if (resolved && !keyframe->IsEmpty()) {
    keyframe->SetResolvedValue(*resolved);
  }
  return resolved;
}

template <typename KeyframeType>
tasm::CSSValue InterpolateVec2CSSValue(
    const tasm::CSSValue& start_css_value, const tasm::CSSValue& end_css_value,
    const std::optional<RawVec2Value>& start_raw,
    const std::optional<RawVec2Value>& end_raw, KeyframeType* keyframe,
    KeyframeType* keyframe_next, double progress, tasm::Element* element,
    Vec2CSSValueEncoding encoding) {
  if (start_css_value == tasm::CSSValue() ||
      end_css_value == tasm::CSSValue()) {
    return start_css_value;
  }
  if (!start_raw || !end_raw) {
    return start_css_value;
  }

  if (std::fabs(progress - 0.0f) < std::numeric_limits<float>::epsilon()) {
    return start_css_value;
  }
  if (std::fabs(progress - 1.0f) < std::numeric_limits<float>::epsilon()) {
    return end_css_value;
  }

  auto start_value = GetResolvedVec2Value(keyframe, *start_raw, element);
  auto end_value = GetResolvedVec2Value(keyframe_next, *end_raw, element);
  if (!start_value || !end_value || start_value->x.tag != end_value->x.tag ||
      start_value->y.tag != end_value->y.tag) {
    return start_css_value;
  }

  auto out = gfx::InterpolateVec2Tagged(*start_value, *end_value, progress,
                                        gfx::DiscreteFallback::kUseStart);
  auto css_value = BuildVec2CSSValue(out, encoding);
  if (!css_value) {
    return start_css_value;
  }
  return std::move(*css_value);
}

}  // namespace

tasm::CSSValue GetStyleInElement(tasm::CSSPropertyID id,
                                 tasm::Element* element) {
  std::optional<tasm::CSSValue> value_opt = element->GetElementStyle(id);
  if (!value_opt) {
    return tasm::CSSValue();
  }
  return std::move(*value_opt);
}

tasm::CSSValue HandleCSSVariableValueIfNeed(tasm::CSSPropertyID id,
                                            const tasm::CSSValue& value,
                                            tasm::Element* element) {
  const auto& keyframe_value = value;
  bool is_variable = keyframe_value.IsVariable();
  if (is_variable) {
    tasm::StyleMap temp_var_map;
    temp_var_map.insert_or_assign(id, value);
    element->HandleCSSVariables(temp_var_map);
    if (temp_var_map.empty()) {
      return value;
    }
    return temp_var_map.front().second;
  }
  return keyframe_value;
}

const std::unordered_set<AnimationCurve::CurveType>& GetOnXAxisCurveTypeSet() {
  static const base::NoDestructor<std::unordered_set<AnimationCurve::CurveType>>
      onXAxisCurveTypeSet({ALL_X_AXIS_CURVE_TYPE});
  return *onXAxisCurveTypeSet;
}

//====== LayoutValueAnimator begin =======

std::unique_ptr<LayoutKeyframe> LayoutKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<LayoutKeyframe>(time, std::move(timing_function));
}

LayoutKeyframe::LayoutKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::LengthKeyframe(time, std::move(timing_function)) {}

void LayoutKeyframe::SetLayout(const starlight::NLength& length) {
  if (length.IsUnit()) {
    SetResolvedValue({length.GetRawValue(), gfx::LengthUnit::kNumber});
    return;
  }
  if (length.IsPercent()) {
    SetResolvedValue({length.GetRawValue(), gfx::LengthUnit::kPercent});
    return;
  }
  ClearResolvedValue();
}

void LayoutKeyframe::NotifyUnitValuesUpdated(uint32_t type) {
  if (css_value_.GetPattern() == static_cast<tasm::CSSValuePattern>(type)) {
    ClearResolvedValue();
  }
}

std::pair<std::optional<gfx::LengthValue>, tasm::CSSValue>
LayoutKeyframe::GetLayoutKeyframeValue(LayoutKeyframe* keyframe,
                                       tasm::CSSPropertyID id,
                                       tasm::Element* element) {
  std::optional<gfx::LengthValue> length;
  tasm::CSSValue css_value = tasm::CSSValue(starlight::LengthValueType::kAuto);
  if (keyframe->IsEmpty()) {
    std::optional<tasm::CSSValue> value_opt = element->GetElementStyle(id);
    if (!value_opt) {
      return std::make_pair(length, css_value);
    }
    auto resolved = ResolveLayoutLengthValue(id, *value_opt, element);
    if (resolved) {
      length = *resolved;
    }
    css_value = std::move(*value_opt);
  } else {
    css_value = keyframe->CSSValue();
    if (!keyframe->HasResolvedValue() && !css_value.IsEnum()) {
      auto resolved = ResolveLayoutLengthValue(id, css_value, element);
      if (resolved) {
        keyframe->SetLayout(*resolved);
        length = keyframe->ResolvedValue();
      }
    } else if (keyframe->HasResolvedValue()) {
      length = keyframe->ResolvedValue();
    }
  }
  return std::make_pair(length, css_value);
}

bool LayoutKeyframe::SetValue(tasm::CSSPropertyID id,
                              const tasm::CSSValue& value,
                              tasm::Element* element) {
  css_value_ = value;
  auto keyframe_layout_value = HandleCSSVariableValueIfNeed(id, value, element);
  auto parse_result = starlight::CSSStyleUtils::ToLength(
      keyframe_layout_value, CSSKeyframeManager::GetLengthContext(element),
      element->element_manager()->GetCSSParserConfigs());
  if (!parse_result.second) {
    return false;
  }
  if (!parse_result.first.IsUnit() && !parse_result.first.IsPercent() &&
      !parse_result.first.IsCalc() && !parse_result.first.IsAuto()) {
    return false;
  }
  MarkNonEmpty();
  // Keep layout keyframes in raw CSS form until sampling. Values such as em,
  // rem, rpx, vw/vh and calc depend on the element's current measure context;
  // resolving them when the keyframe is created can cache a stale value.
  ClearResolvedValue();
  return true;
}

std::unique_ptr<KeyframedLayoutAnimationCurve>
KeyframedLayoutAnimationCurve::Create() {
  return std::make_unique<KeyframedLayoutAnimationCurve>();
}

tasm::CSSValue KeyframedLayoutAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_LAYOUT_ANIMATION_CURVE_GET_VALUE,
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("LayoutAnimation");
              });

  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  DCHECK(sampling.valid);
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;

  auto* keyframe = static_cast<LayoutKeyframe*>(keyframes_[i].get());
  auto* keyframe_next = static_cast<LayoutKeyframe*>(keyframes_[i + 1].get());

  auto start_result = LayoutKeyframe::GetLayoutKeyframeValue(
      keyframe, static_cast<tasm::CSSPropertyID>(Type()), element_);
  auto end_result = LayoutKeyframe::GetLayoutKeyframeValue(
      keyframe_next, static_cast<tasm::CSSPropertyID>(Type()), element_);
  const auto& start_len = start_result.first;
  const auto& end_len = end_result.first;

  if (std::fabs(progress - 0.0f) < std::numeric_limits<float>::epsilon()) {
    return start_result.second;
  }
  if ((!start_len.has_value() || !end_len.has_value()) ||
      (std::fabs(progress - 1.0f) < std::numeric_limits<float>::epsilon())) {
    return end_result.second;
  }

  float start_value = start_len->value;
  float end_value = end_len->value;
  tasm::CSSValuePattern pattern = start_len->unit == gfx::LengthUnit::kPercent
                                      ? tasm::CSSValuePattern::PERCENT
                                      : tasm::CSSValuePattern::NUMBER;
  if (start_len->unit != end_len->unit) {
    if (!element_ || !element_->parent()) {
      return start_result.second;
    }
    float parent_length = 0.0f;
    if (GetOnXAxisCurveTypeSet().count(Type()) != 0) {
      parent_length = element_->parent()->width();
    } else {
      parent_length = element_->parent()->height();
    }
    if (start_len->unit == gfx::LengthUnit::kPercent) {
      start_value = parent_length * start_len->value / 100.0f;
    }
    if (end_len->unit == gfx::LengthUnit::kPercent) {
      end_value = parent_length * end_len->value / 100.0f;
    }
    pattern = tasm::CSSValuePattern::NUMBER;
  }
  float new_result = start_value + (end_value - start_value) * progress;
  return tasm::CSSValue(new_result, pattern);
}

//====== LayoutValueAnimator end =======

//====== OpacityValueAnimator begin =======
std::unique_ptr<OpacityKeyframe> OpacityKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<OpacityKeyframe>(time, std::move(timing_function));
}

OpacityKeyframe::OpacityKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::FloatKeyframe(time, std::move(timing_function)) {}

float OpacityKeyframe::GetOpacityKeyframeValue(OpacityKeyframe* keyframe,
                                               tasm::Element* element) {
  float value = OpacityKeyframe::kDefaultOpacity;
  if (keyframe->IsEmpty()) {
    tasm::CSSValue opacity =
        GetStyleInElement(tasm::kPropertyIDOpacity, element);
    if (opacity.IsNumber()) {
      value = static_cast<float>(opacity.AsNumber());
    }
  } else {
    value = keyframe->gfx::FloatKeyframe::Value();
  }
  return value;
}

bool OpacityKeyframe::SetValue(tasm::CSSPropertyID id,
                               const tasm::CSSValue& value,
                               tasm::Element* element) {
  auto keyframe_opacity_value =
      HandleCSSVariableValueIfNeed(id, value, element);
  if (!keyframe_opacity_value.IsNumber()) {
    return false;
  }
  SetOpacity(static_cast<float>(keyframe_opacity_value.GetNumber()));
  return true;
}

std::unique_ptr<KeyframedOpacityAnimationCurve>
KeyframedOpacityAnimationCurve::Create() {
  return std::make_unique<KeyframedOpacityAnimationCurve>();
}

tasm::CSSValue KeyframedOpacityAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_OPACITY_ANIMATION_CURVE_GET_VALUE,
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("OpacityAnimation");
              });

  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  if (!sampling.valid) {
    return tasm::CSSValue(OpacityKeyframe::kDefaultOpacity,
                          tasm::CSSValuePattern::NUMBER);
  }
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;

  auto* keyframe = static_cast<OpacityKeyframe*>(keyframes_[i].get());
  auto* keyframe_next = static_cast<OpacityKeyframe*>(keyframes_[i + 1].get());

  float start_opacity =
      OpacityKeyframe::GetOpacityKeyframeValue(keyframe, element_);
  float end_opacity =
      OpacityKeyframe::GetOpacityKeyframeValue(keyframe_next, element_);

  float result_value = static_cast<float>(
      gfx::InterpolateNumber(static_cast<double>(start_opacity),
                             static_cast<double>(end_opacity), progress));

  if (start_opacity > end_opacity && result_value > 0.0f &&
      base::FloatsEqual(result_value, 0.0f)) {
    result_value = 0.0f;
  } else if (start_opacity < end_opacity && result_value < 1.0f &&
             base::FloatsEqual(result_value, 1.0f)) {
    result_value = 1.0f;
  }

  return tasm::CSSValue(result_value, tasm::CSSValuePattern::NUMBER);
}

//====== OpacityValueAnimator end =======

//====== ColorValueAnimator begin =======
std::unique_ptr<ColorKeyframe> ColorKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<ColorKeyframe>(time, std::move(timing_function));
}

ColorKeyframe::ColorKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::ColorKeyframe(time, std::move(timing_function)) {}

uint32_t ColorKeyframe::GetColorKeyframeValue(ColorKeyframe* keyframe,
                                              tasm::CSSPropertyID id,
                                              tasm::Element* element) {
  uint32_t value = (id == tasm::kPropertyIDColor)
                       ? ColorKeyframe::kDefaultTextColor
                       : ColorKeyframe::kDefaultBackgroundColor;
  if (keyframe->IsEmpty()) {
    tasm::CSSValue color = GetStyleInElement(id, element);
    if (color.IsNumber()) {
      value = static_cast<uint32_t>(color.AsNumber());
    }
  } else {
    value = keyframe->gfx::ColorKeyframe::Value();
  }
  return value;
}

bool ColorKeyframe::SetValue(tasm::CSSPropertyID id,
                             const tasm::CSSValue& value,
                             tasm::Element* element) {
  auto keyframe_color_value = HandleCSSVariableValueIfNeed(id, value, element);
  if (!keyframe_color_value.IsNumber()) {
    return false;
  }
  SetColor(static_cast<uint32_t>(keyframe_color_value.GetNumber()));
  return true;
}

std::unique_ptr<KeyframedColorAnimationCurve>
KeyframedColorAnimationCurve::Create(
    starlight::XAnimationColorInterpolationType type) {
  return std::make_unique<KeyframedColorAnimationCurve>(type);
}

tasm::CSSValue KeyframedColorAnimationCurve::GetValue(fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_COLOR_ANIMATION_CURVE_GET_VALUE,
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("ColorAnimation");
              });
  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  if (!sampling.valid) {
    return tasm::CSSValue(ColorKeyframe::kDefaultBackgroundColor,
                          tasm::CSSValuePattern::NUMBER);
  }
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;

  auto* keyframe = static_cast<ColorKeyframe*>(keyframes_[i].get());
  auto* keyframe_next = static_cast<ColorKeyframe*>(keyframes_[i + 1].get());

  uint32_t start_color = ColorKeyframe::GetColorKeyframeValue(
      keyframe, static_cast<tasm::CSSPropertyID>(Type()), element_);
  uint32_t end_color = ColorKeyframe::GetColorKeyframeValue(
      keyframe_next, static_cast<tasm::CSSPropertyID>(Type()), element_);

  gfx::ColorInterpolation mode = gfx::ColorInterpolation::kAuto;
  if (interpolate_type_ == starlight::XAnimationColorInterpolationType::kAuto) {
#if OS_IOS
    mode = gfx::ColorInterpolation::kLinearRGB;
#else
    mode = gfx::ColorInterpolation::kSRGB;
#endif
  } else if (interpolate_type_ ==
             starlight::XAnimationColorInterpolationType::kLinearRGB) {
    mode = gfx::ColorInterpolation::kLinearRGB;
  } else {
    mode = gfx::ColorInterpolation::kSRGB;
  }

  uint32_t result_value = gfx::InterpolateColorARGB32(
      static_cast<gfx::ColorARGB32>(start_color),
      static_cast<gfx::ColorARGB32>(end_color), progress, mode);
  return tasm::CSSValue(result_value, tasm::CSSValuePattern::NUMBER);
}
//====== ColorValueAnimator end =======

//====== FloatValueAnimator begin =======
std::unique_ptr<FloatKeyframe> FloatKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<FloatKeyframe>(time, std::move(timing_function));
}

FloatKeyframe::FloatKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::FloatKeyframe(time, std::move(timing_function)) {}

float FloatKeyframe::GetFloatKeyframeValue(FloatKeyframe* keyframe,
                                           tasm::CSSPropertyID id,
                                           tasm::Element* element) {
  float value = FloatKeyframe::kDefaultFloatValue;
  if (keyframe->IsEmpty()) {
    tasm::CSSValue float_value =
        GetStyleInElement(tasm::kPropertyIDFlexGrow, element);
    if (float_value.IsNumber()) {
      value = static_cast<float>(float_value.AsNumber());
    }
  } else {
    value = keyframe->gfx::FloatKeyframe::Value();
  }
  return value;
}

bool FloatKeyframe::SetValue(tasm::CSSPropertyID id,
                             const tasm::CSSValue& value,
                             tasm::Element* element) {
  auto keyframe_float_value = HandleCSSVariableValueIfNeed(id, value, element);
  if (!keyframe_float_value.IsNumber()) {
    return false;
  }
  SetFloat(static_cast<float>(keyframe_float_value.GetNumber()));
  return true;
}

std::unique_ptr<KeyframedFloatAnimationCurve>
KeyframedFloatAnimationCurve::Create() {
  return std::make_unique<KeyframedFloatAnimationCurve>();
}

tasm::CSSValue KeyframedFloatAnimationCurve::GetValue(fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_FONT_ANIMATION_CURVE_GET_VALUE,
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("FloatAnimation");
              });

  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  if (!sampling.valid) {
    return tasm::CSSValue(FloatKeyframe::kDefaultFloatValue,
                          tasm::CSSValuePattern::NUMBER);
  }
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;

  auto* keyframe = static_cast<FloatKeyframe*>(keyframes_[i].get());
  auto* keyframe_next = static_cast<FloatKeyframe*>(keyframes_[i + 1].get());

  float start_float = FloatKeyframe::GetFloatKeyframeValue(
      keyframe, tasm::kPropertyIDFlexGrow, element_);
  float end_float = FloatKeyframe::GetFloatKeyframeValue(
      keyframe_next, tasm::kPropertyIDFlexGrow, element_);

  float result_value = static_cast<float>(
      gfx::InterpolateNumber(static_cast<double>(start_float),
                             static_cast<double>(end_float), progress));
  return tasm::CSSValue(result_value, tasm::CSSValuePattern::NUMBER);
}

//====== FloatValueAnimator end =======

//====== FilterValueAnimator begin =======

std::unique_ptr<FilterKeyframe> FilterKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<FilterKeyframe>(time, std::move(timing_function));
}

FilterKeyframe::FilterKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::FilterKeyframe(time, std::move(timing_function)) {}

tasm::CSSValue FilterKeyframe::GetFilterKeyframeValue(FilterKeyframe* keyframe,
                                                      tasm::CSSPropertyID id,
                                                      tasm::Element* element) {
  tasm::CSSValue filter = tasm::CSSValue();
  if (keyframe->IsEmpty()) {
    filter = GetStyleInElement(id, element);
  } else {
    filter = keyframe->filter_;
  }
  return filter;
}

bool FilterKeyframe::SetValue(tasm::CSSPropertyID id,
                              const tasm::CSSValue& value,
                              tasm::Element* element) {
  auto keyframe_filter_value = HandleCSSVariableValueIfNeed(id, value, element);
  filter_ = keyframe_filter_value;
  ClearResolvedValue();
  MarkNonEmpty();
  return true;
}

void FilterKeyframe::NotifyUnitValuesUpdated(uint32_t type) {
  auto raw = ParseRawFilterValue(filter_);
  auto updated_pattern = static_cast<tasm::CSSValuePattern>(type);
  if (raw && raw->amount_pattern == updated_pattern) {
    ClearResolvedValue();
  }
}

std::unique_ptr<KeyframedFilterAnimationCurve>
KeyframedFilterAnimationCurve::Create() {
  return std::make_unique<KeyframedFilterAnimationCurve>();
}

tasm::CSSValue KeyframedFilterAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_FILTER_ANIMATION_CURVE_GET_VALUE,
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("FilterAnimation");
              });
  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  if (!sampling.valid) {
    return tasm::CSSValue();
  }
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;
  auto* keyframe = static_cast<FilterKeyframe*>(keyframes_[i].get());
  auto* keyframe_next = static_cast<FilterKeyframe*>(keyframes_[i + 1].get());

  tasm::CSSValue start_filter = FilterKeyframe::GetFilterKeyframeValue(
      keyframe, tasm::kPropertyIDFilter, element_);
  tasm::CSSValue end_filter = FilterKeyframe::GetFilterKeyframeValue(
      keyframe_next, tasm::kPropertyIDFilter, element_);
  if (start_filter == tasm::CSSValue() || end_filter == tasm::CSSValue()) {
    return start_filter;
  }
  auto start_raw = ParseRawFilterValue(start_filter);
  auto end_raw = ParseRawFilterValue(end_filter);
  if (!start_raw || !end_raw ||
      start_raw->filter_type != end_raw->filter_type) {
    return start_filter;
  }

  if (std::fabs(progress - 0.0f) < std::numeric_limits<float>::epsilon()) {
    return start_filter;
  }
  if (std::fabs(progress - 1.0f) < std::numeric_limits<float>::epsilon()) {
    return end_filter;
  }

  auto start_value = GetResolvedFilterValue(keyframe, *start_raw, element_);
  auto end_value = GetResolvedFilterValue(keyframe_next, *end_raw, element_);
  if (!start_value || !end_value || start_value->unit != end_value->unit) {
    return start_filter;
  }

  auto out = gfx::InterpolateFilterValue(*start_value, *end_value, progress,
                                         gfx::DiscreteFallback::kUseStart);
  auto out_pattern = ToCSSValuePatternFromUnitTag(out.unit);
  if (!out_pattern) {
    return start_filter;
  }
  auto res_arr = lepus::CArray::Create();
  res_arr->emplace_back(out.function);
  res_arr->emplace_back(out.value);
  res_arr->emplace_back(static_cast<uint32_t>(*out_pattern));
  return tasm::CSSValue(std::move(res_arr));
}

//====== FilterValueAnimator end =======

//====== BackgroundPositionAnimator begin =======

BackgroundPositionKeyframe::BackgroundPositionKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::Vec2Keyframe(time, std::move(timing_function)) {}

tasm::CSSValue BackgroundPositionKeyframe::GetBackgroundPositionKeyframeValue(
    BackgroundPositionKeyframe* keyframe, tasm::CSSPropertyID id,
    tasm::Element* element) {
  if (keyframe && !keyframe->IsEmpty()) {
    return keyframe->GetBackgroundPosition();
  }
  return tasm::CSSValue();
}

std::unique_ptr<BackgroundPositionKeyframe> BackgroundPositionKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<BackgroundPositionKeyframe>(
      time, std::move(timing_function));
}

bool BackgroundPositionKeyframe::SetValue(tasm::CSSPropertyID id,
                                          const tasm::CSSValue& value,
                                          tasm::Element* element) {
  auto keyframe_background_position_value =
      HandleCSSVariableValueIfNeed(id, value, element);
  if (!keyframe_background_position_value.IsArray()) {
    return false;
  }
  background_position_ = keyframe_background_position_value;
  ClearResolvedValue();
  MarkNonEmpty();
  return true;
}

void BackgroundPositionKeyframe::NotifyUnitValuesUpdated(uint32_t type) {
  auto raw = ParseRawBackgroundPositionValue(background_position_);
  auto updated_pattern = static_cast<tasm::CSSValuePattern>(type);
  if (raw && (raw->x_pattern == updated_pattern ||
              raw->y_pattern == updated_pattern)) {
    ClearResolvedValue();
  }
}

std::unique_ptr<KeyframedBackgroundPositionAnimationCurve>
KeyframedBackgroundPositionAnimationCurve::Create() {
  return std::make_unique<KeyframedBackgroundPositionAnimationCurve>();
}

tasm::CSSValue KeyframedBackgroundPositionAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              KEYFRAME_BACKGROUND_POSITION_ANIMATION_CURVE_GET_VALUE,
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("BackgroundPositionAnimation");
              });

  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  if (!sampling.valid) {
    return tasm::CSSValue();
  }
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;

  BackgroundPositionKeyframe* keyframe =
      static_cast<BackgroundPositionKeyframe*>(keyframes_[i].get());
  BackgroundPositionKeyframe* keyframe_next =
      static_cast<BackgroundPositionKeyframe*>(keyframes_[i + 1].get());

  tasm::CSSValue start_background_position =
      BackgroundPositionKeyframe::GetBackgroundPositionKeyframeValue(
          keyframe, tasm::kPropertyIDBackgroundPosition, element_);
  tasm::CSSValue end_background_position =
      BackgroundPositionKeyframe::GetBackgroundPositionKeyframeValue(
          keyframe_next, tasm::kPropertyIDBackgroundPosition, element_);

  auto start_raw = ParseRawBackgroundPositionValue(start_background_position);
  auto end_raw = ParseRawBackgroundPositionValue(end_background_position);
  return InterpolateVec2CSSValue(start_background_position,
                                 end_background_position, start_raw, end_raw,
                                 keyframe, keyframe_next, progress, element_,
                                 Vec2CSSValueEncoding::kBackgroundPosition);
}

//====== BackgroundPositionAnimator end =======

//====== TransformOriginAnimator start =======
TransformOriginKeyframe::TransformOriginKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::Vec2Keyframe(time, std::move(timing_function)) {}

tasm::CSSValue TransformOriginKeyframe::GetTransformOriginKeyframeValue(
    TransformOriginKeyframe* keyframe, tasm::CSSPropertyID id,
    tasm::Element* element) {
  if (keyframe && !keyframe->IsEmpty()) {
    return keyframe->GetTransformOrigin();
  }
  return tasm::CSSValue();
}

std::unique_ptr<TransformOriginKeyframe> TransformOriginKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<TransformOriginKeyframe>(time,
                                                   std::move(timing_function));
}

bool TransformOriginKeyframe::SetValue(tasm::CSSPropertyID id,
                                       const tasm::CSSValue& value,
                                       tasm::Element* element) {
  auto keyframe_transform_origin_value =
      HandleCSSVariableValueIfNeed(id, value, element);
  if (!keyframe_transform_origin_value.IsArray()) {
    return false;
  }
  transform_origin_ = keyframe_transform_origin_value;
  ClearResolvedValue();
  MarkNonEmpty();
  return true;
}

void TransformOriginKeyframe::NotifyUnitValuesUpdated(uint32_t type) {
  auto raw = ParseRawTransformOriginValue(transform_origin_);
  auto updated_pattern = static_cast<tasm::CSSValuePattern>(type);
  if (raw && (raw->x_pattern == updated_pattern ||
              raw->y_pattern == updated_pattern)) {
    ClearResolvedValue();
  }
}

std::unique_ptr<KeyframedTransformOriginAnimationCurve>
KeyframedTransformOriginAnimationCurve::Create() {
  return std::make_unique<KeyframedTransformOriginAnimationCurve>();
}

tasm::CSSValue KeyframedTransformOriginAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  if (!sampling.valid) {
    return tasm::CSSValue();
  }
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;

  TransformOriginKeyframe* keyframe =
      static_cast<TransformOriginKeyframe*>(keyframes_[i].get());
  TransformOriginKeyframe* keyframe_next =
      static_cast<TransformOriginKeyframe*>(keyframes_[i + 1].get());

  tasm::CSSValue start_transform_origin =
      TransformOriginKeyframe::GetTransformOriginKeyframeValue(
          keyframe, tasm::kPropertyIDTransformOrigin, element_);
  tasm::CSSValue end_transform_origin =
      TransformOriginKeyframe::GetTransformOriginKeyframeValue(
          keyframe_next, tasm::kPropertyIDTransformOrigin, element_);

  auto start_raw = ParseRawTransformOriginValue(start_transform_origin);
  auto end_raw = ParseRawTransformOriginValue(end_transform_origin);
  return InterpolateVec2CSSValue(start_transform_origin, end_transform_origin,
                                 start_raw, end_raw, keyframe, keyframe_next,
                                 progress, element_,
                                 Vec2CSSValueEncoding::kTransformOrigin);
}
//====== TransformOriginAnimator end =======

//====== VisibilityAnimator start =======
VisibilityKeyframe::VisibilityKeyframe(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function)
    : gfx::Keyframe(time, std::move(timing_function)) {}

starlight::VisibilityType VisibilityKeyframe::GetVisibilityKeyframeValue(
    VisibilityKeyframe* keyframe, tasm::Element* element) {
  if (keyframe->IsEmpty()) {
    const auto& value = GetStyleInElement(tasm::kPropertyIDVisibility, element);
    if (value.IsEnum()) {
      return static_cast<starlight::VisibilityType>(
          static_cast<int>(value.AsNumber()));
    }
    return starlight::VisibilityType::kVisible;
  }
  return keyframe->Visibility();
}

std::unique_ptr<VisibilityKeyframe> VisibilityKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<gfx::TimingFunction> timing_function) {
  return std::make_unique<VisibilityKeyframe>(time, std::move(timing_function));
}

bool VisibilityKeyframe::SetValue(tasm::CSSPropertyID id,
                                  const tasm::CSSValue& value,
                                  tasm::Element* element) {
  auto visibility_value = HandleCSSVariableValueIfNeed(id, value, element);
  if (!visibility_value.IsEnum()) {
    return false;
  }
  visibility_ =
      static_cast<starlight::VisibilityType>(visibility_value.AsNumber());
  MarkNonEmpty();
  return true;
}

std::unique_ptr<KeyframedVisibilityAnimationCurve>
KeyframedVisibilityAnimationCurve::Create() {
  return std::make_unique<KeyframedVisibilityAnimationCurve>();
}

tasm::CSSValue KeyframedVisibilityAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  auto sampling = gfx::ComputeKeyframedProgress(keyframes_, timing_function(),
                                                scaled_duration(), t);
  DCHECK(sampling.valid);
  t = sampling.effective_time;
  size_t i = sampling.index;
  double progress = sampling.progress;

  auto* keyframe = static_cast<VisibilityKeyframe*>(keyframes_[i].get());
  auto* keyframe_next =
      static_cast<VisibilityKeyframe*>(keyframes_[i + 1].get());

  starlight::VisibilityType start_vis =
      VisibilityKeyframe::GetVisibilityKeyframeValue(keyframe, element_);
  starlight::VisibilityType end_vis =
      VisibilityKeyframe::GetVisibilityKeyframeValue(keyframe_next, element_);

  starlight::VisibilityType result;
  if (start_vis == starlight::VisibilityType::kVisible ||
      end_vis == starlight::VisibilityType::kVisible) {
    if (progress <= 0.0) {
      result = start_vis;
    } else if (base::FloatsLargerOrEqual(progress, 1.0f)) {
      result = end_vis;
    } else {
      result = starlight::VisibilityType::kVisible;
    }
  } else {
    result = progress < 0.5 ? start_vis : end_vis;
  }

  return tasm::CSSValue(static_cast<int>(result), tasm::CSSValuePattern::ENUM);
}
//====== VisibilityAnimator end =======
}  // namespace animation
}  // namespace lynx
