// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/keyframed_animation_curve.h"

#include <limits>

#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace animation {

// keyframe
fml::TimeDelta Keyframe::Time() const { return time_; }

Keyframe::Keyframe(fml::TimeDelta time,
                   std::unique_ptr<TimingFunction> timing_function)
    : time_(time), timing_function_(std::move(timing_function)) {}

fml::TimeDelta TransformedAnimationTime(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    const std::unique_ptr<TimingFunction>& timing_function,
    double scaled_duration, fml::TimeDelta time) {
  if (timing_function) {
    fml::TimeDelta start_time = keyframes.front()->Time() * scaled_duration;
    fml::TimeDelta duration =
        (keyframes.back()->Time() - keyframes.front()->Time()) *
        scaled_duration;
    double progress = static_cast<double>(time.ToMicroseconds() -
                                          start_time.ToMicroseconds()) /
                      static_cast<double>(duration.ToMicroseconds());

    time = (duration * timing_function->GetValue(progress)) + start_time;
  }

  return time;
}

size_t GetActiveKeyframe(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    double scaled_duration, fml::TimeDelta time) {
  DCHECK(keyframes.size() >= 2);
  size_t i = 0;
  for (; i < keyframes.size() - 2; ++i) {  // Last keyframe is never active.
    if (time < (keyframes[i + 1]->Time() * scaled_duration)) break;
  }

  return i;
}

double TransformedKeyframeProgress(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    double scaled_duration, fml::TimeDelta time, size_t i) {
  double in_time = time.ToNanosecondsF();
  double time1 = keyframes[i]->Time().ToNanosecondsF() * scaled_duration;
  double time2 = keyframes[i + 1]->Time().ToNanosecondsF() * scaled_duration;

  // Corner case: If time1 is equal to time2, we should return 100% progress
  // here directly. Otherwise, we will get a progress value of NaN, because the
  // difference between time1 and time2 will be used as the divisor later.
  // FIXME(wujintian): Here is a bad case that if duration is 0 and delay is not
  // 0 and fill mode is backwards and phase is before now, it should return 0.0
  // instead of return 1.0.
  if (std::fabs(time2 - time1) < std::numeric_limits<double>::epsilon()) {
    return 1.0;
  }
  double progress = (in_time - time1) / (time2 - time1);

  if (keyframes[i]->timing_function()) {
    progress = keyframes[i]->timing_function()->GetValue(progress);
  }

  return progress;
}

tasm::CSSValue GetStyleInElement(tasm::CSSPropertyID id,
                                 tasm::Element* element) {
  std::optional<tasm::CSSValue> value_opt = element->GetElementStyle(id);
  if (!value_opt) {
    return tasm::CSSValue::Empty();
  }
  return std::move(*value_opt);
}

tasm::CSSValue HandleCSSVariableValueIfNeed(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
    tasm::Element* element) {
  const auto& keyframe_value = css_value_pair.second;
  bool is_variable = keyframe_value.IsVariable();
  if (is_variable) {
    tasm::StyleMap temp_var_map;
    temp_var_map.insert_or_assign(css_value_pair.first, css_value_pair.second);
    element->HandleCSSVariables(temp_var_map);
    if (temp_var_map.empty()) {
      return css_value_pair.second;
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
    fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function) {
  return std::make_unique<LayoutKeyframe>(time, std::move(timing_function));
}

LayoutKeyframe::LayoutKeyframe(fml::TimeDelta time,
                               std::unique_ptr<TimingFunction> timing_function)
    : Keyframe(time, std::move(timing_function)),
      value_(starlight::NLength::MakeAutoNLength()) {}

// When view or font size has changed, mark the value 'AutoNLength'.
void LayoutKeyframe::NotifyUnitValuesUpdatedToAnimation(
    tasm::CSSValuePattern type) {
  if (css_value_.GetPattern() == type) {
    value_ = starlight::NLength::MakeAutoNLength();
  }
}

std::pair<starlight::NLength, tasm::CSSValue>
LayoutKeyframe::GetLayoutKeyframeValue(LayoutKeyframe* keyframe,
                                       tasm::CSSPropertyID id,
                                       tasm::Element* element) {
  // Layout length default value : auto
  starlight::NLength length = starlight::NLength::MakeAutoNLength();
  tasm::CSSValue css_value = tasm::CSSValue(
      lepus::Value(static_cast<int>(starlight::LengthValueType::kAuto)),
      tasm::CSSValuePattern::ENUM);
  if (keyframe->IsEmpty()) {
    std::optional<tasm::CSSValue> value_opt = element->GetElementStyle(id);
    if (!value_opt) {
      // return default value
      return std::make_pair(length, css_value);
    }
    const auto& configs = element->element_manager()->GetCSSParserConfigs();
    auto parse_result = starlight::CSSStyleUtils::ToLength(
        *value_opt, CSSKeyframeManager::GetLengthContext(element), configs);
    length = parse_result.first;
    css_value = std::move(*value_opt);
  } else {
    length = keyframe->Value();
    css_value = keyframe->CSSValue();
  }
  return std::make_pair(length, css_value);
}

bool LayoutKeyframe::SetValue(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
    tasm::Element* element) {
  auto keyframe_layout_value =
      HandleCSSVariableValueIfNeed(css_value_pair, element);
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
  value_ = parse_result.first;
  css_value_ = css_value_pair.second;
  is_empty_ = false;
  return true;
}

std::unique_ptr<KeyframedLayoutAnimationCurve>
KeyframedLayoutAnimationCurve::Create() {
  return std::make_unique<KeyframedLayoutAnimationCurve>();
}

tasm::CSSValue KeyframedLayoutAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframedLayoutAnimationCurve::GetValue",
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("LayoutAnimation");
              });

  t = TransformedAnimationTime(keyframes_, timing_function_, scaled_duration(),
                               t);
  size_t i = GetActiveKeyframe(keyframes_, scaled_duration(), t);
  double progress =
      TransformedKeyframeProgress(keyframes_, scaled_duration(), t, i);

  LayoutKeyframe* keyframe =
      reinterpret_cast<LayoutKeyframe*>(keyframes_[i].get());
  LayoutKeyframe* keyframe_next =
      reinterpret_cast<LayoutKeyframe*>(keyframes_[i + 1].get());

  auto start_len = keyframe->Value();
  auto end_len = keyframe_next->Value();
  // When view or font size has changed, let start_len and end_len be
  // 'AutoNLength', and then get The actual Nlength based on the updated size.
  if (start_len.IsAuto() && !keyframe->CSSValue().IsEnum()) {
    auto prev_temp_pair = std::make_pair(
        static_cast<tasm::CSSPropertyID>(Type()), keyframe->CSSValue());
    keyframe->SetValue(prev_temp_pair, element_);
  }
  if (end_len.IsAuto() && !keyframe_next->CSSValue().IsEnum()) {
    auto next_temp_pair = std::make_pair(
        static_cast<tasm::CSSPropertyID>(Type()), keyframe_next->CSSValue());
    keyframe_next->SetValue(next_temp_pair, element_);
  }

  auto start_result = LayoutKeyframe::GetLayoutKeyframeValue(
      keyframe, static_cast<tasm::CSSPropertyID>(Type()), element_);
  start_len = start_result.first;
  auto end_result = LayoutKeyframe::GetLayoutKeyframeValue(
      keyframe_next, static_cast<tasm::CSSPropertyID>(Type()), element_);
  end_len = end_result.first;

  if (((!start_len.IsUnit() && !start_len.IsPercent() && !start_len.IsCalc()) ||
       (!end_len.IsUnit() && !end_len.IsPercent() && !end_len.IsCalc())) ||
      (std::fabs(progress - 1.0f) < std::numeric_limits<float>::epsilon())) {
    return end_result.second;
  }
  if (std::fabs(progress - 0.0f) < std::numeric_limits<float>::epsilon()) {
    return start_result.second;
  }

  float start_value = 0.0f;
  float end_value = 0.0f;
  tasm::CSSValuePattern pattern = tasm::CSSValuePattern::NUMBER;
  if ((start_len.IsUnit() && end_len.IsPercent()) ||
      (start_len.IsPercent() && end_len.IsUnit()) ||
      (start_len.IsCalc() || end_len.IsCalc())) {
    if (!element_ || !element_->parent()) {
      return tasm::CSSValue(lepus::Value(start_len.GetRawValue()),
                            start_len.IsCalc() ? tasm::CSSValuePattern::CALC
                            : start_len.IsUnit()
                                ? tasm::CSSValuePattern::NUMBER
                                : tasm::CSSValuePattern::PERCENT);
    }
    float parent_length = 0;
    if (GetOnXAxisCurveTypeSet().count(Type()) != 0) {
      parent_length = element_->parent()->width();
    } else {
      parent_length = element_->parent()->height();
    }
    start_value = starlight::NLengthToLayoutUnit(
                      start_len, starlight::LayoutUnit(parent_length))
                      .ToFloat();
    end_value = starlight::NLengthToLayoutUnit(
                    end_len, starlight::LayoutUnit(parent_length))
                    .ToFloat();
    pattern = tasm::CSSValuePattern::NUMBER;
  } else {
    start_value = start_len.GetRawValue();
    end_value = end_len.GetRawValue();
    pattern = start_len.IsUnit() ? tasm::CSSValuePattern::NUMBER
                                 : tasm::CSSValuePattern::PERCENT;
  }
  float new_result = start_value + (end_value - start_value) * progress;
  return tasm::CSSValue(lepus::Value(new_result), pattern);
}

//====== LayoutValueAnimator end =======

//====== OpacityValueAnimator begin =======
std::unique_ptr<OpacityKeyframe> OpacityKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function) {
  return std::make_unique<OpacityKeyframe>(time, std::move(timing_function));
}

OpacityKeyframe::OpacityKeyframe(
    fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function)
    : Keyframe(time, std::move(timing_function)) {}

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
    value = static_cast<float>(keyframe->Value());
  }
  return value;
}

bool OpacityKeyframe::SetValue(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
    tasm::Element* element) {
  auto keyframe_opacity_value =
      HandleCSSVariableValueIfNeed(css_value_pair, element);
  if (!keyframe_opacity_value.IsNumber()) {
    return false;
  }
  value_ = keyframe_opacity_value.GetValue().Number();
  is_empty_ = false;
  return true;
}

std::unique_ptr<KeyframedOpacityAnimationCurve>
KeyframedOpacityAnimationCurve::Create() {
  return std::make_unique<KeyframedOpacityAnimationCurve>();
}

tasm::CSSValue KeyframedOpacityAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframedOpacityAnimationCurve::GetValue",
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("OpacityAnimation");
              });

  t = TransformedAnimationTime(keyframes_, timing_function_, scaled_duration(),
                               t);
  size_t i = GetActiveKeyframe(keyframes_, scaled_duration(), t);
  double progress =
      TransformedKeyframeProgress(keyframes_, scaled_duration(), t, i);

  OpacityKeyframe* keyframe =
      reinterpret_cast<OpacityKeyframe*>(keyframes_[i].get());
  OpacityKeyframe* keyframe_next =
      reinterpret_cast<OpacityKeyframe*>(keyframes_[i + 1].get());

  float start_opacity =
      OpacityKeyframe::GetOpacityKeyframeValue(keyframe, element_);
  float end_opacity =
      OpacityKeyframe::GetOpacityKeyframeValue(keyframe_next, element_);
  float result_value = start_opacity + (end_opacity - start_opacity) * progress;

  if (start_opacity > end_opacity && result_value > 0.0f &&
      base::FloatsEqual(result_value, 0.0f)) {
    result_value = 0.0f;
  } else if (start_opacity < end_opacity && result_value < 1.0f &&
             base::FloatsEqual(result_value, 1.0f)) {
    result_value = 1.0f;
  }

  return tasm::CSSValue(lepus_value(result_value),
                        tasm::CSSValuePattern::NUMBER);
}

//====== OpacityValueAnimator end =======

//====== ColorValueAnimator begin =======
std::unique_ptr<ColorKeyframe> ColorKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function) {
  return std::make_unique<ColorKeyframe>(time, std::move(timing_function));
}

ColorKeyframe::ColorKeyframe(fml::TimeDelta time,
                             std::unique_ptr<TimingFunction> timing_function)
    : Keyframe(time, std::move(timing_function)) {}

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
    value = static_cast<uint32_t>(keyframe->Value());
  }
  return value;
}

bool ColorKeyframe::SetValue(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
    tasm::Element* element) {
  auto keyframe_color_value =
      HandleCSSVariableValueIfNeed(css_value_pair, element);
  if (!keyframe_color_value.IsNumber()) {
    return false;
  }
  value_ = keyframe_color_value.GetValue().Number();
  is_empty_ = false;
  return true;
}

std::unique_ptr<KeyframedColorAnimationCurve>
KeyframedColorAnimationCurve::Create(
    starlight::XAnimationColorInterpolationType type) {
  return std::make_unique<KeyframedColorAnimationCurve>(type);
}

tasm::CSSValue KeyframedColorAnimationCurve::GetValue(fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframedColorAnimationCurve::GetValue",
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("ColorAnimation");
              });
  t = TransformedAnimationTime(keyframes_, timing_function_, scaled_duration(),
                               t);
  size_t i = GetActiveKeyframe(keyframes_, scaled_duration(), t);
  double progress =
      TransformedKeyframeProgress(keyframes_, scaled_duration(), t, i);

  ColorKeyframe* keyframe =
      reinterpret_cast<ColorKeyframe*>(keyframes_[i].get());
  ColorKeyframe* keyframe_next =
      reinterpret_cast<ColorKeyframe*>(keyframes_[i + 1].get());

  uint32_t start_color = ColorKeyframe::GetColorKeyframeValue(
      keyframe, static_cast<tasm::CSSPropertyID>(Type()), element_);
  uint32_t end_color = ColorKeyframe::GetColorKeyframeValue(
      keyframe_next, static_cast<tasm::CSSPropertyID>(Type()), element_);

  double color_space_constant = 1.0;
  if (interpolate_type_ == starlight::XAnimationColorInterpolationType::kAuto) {
#if !OS_IOS
    color_space_constant = 2.2;
#endif
  } else {
    color_space_constant =
        interpolate_type_ ==
                starlight::XAnimationColorInterpolationType::kLinearRGB
            ? 1.0
            : 2.2;
  }

  float startA = ((start_color >> 24) & 0xff) / 255.0f;
  float startR = ((start_color >> 16) & 0xff) / 255.0f;
  float startG = ((start_color >> 8) & 0xff) / 255.0f;
  float startB = ((start_color) & 0xff) / 255.0f;

  float endA = ((end_color >> 24) & 0xff) / 255.0f;
  float endR = ((end_color >> 16) & 0xff) / 255.0f;
  float endG = ((end_color >> 8) & 0xff) / 255.0f;
  float endB = ((end_color) & 0xff) / 255.0f;

  // convert RGB to linear
  startR = static_cast<float>(pow(startR, color_space_constant));
  startG = static_cast<float>(pow(startG, color_space_constant));
  startB = static_cast<float>(pow(startB, color_space_constant));

  endR = static_cast<float>(pow(endR, color_space_constant));
  endG = static_cast<float>(pow(endG, color_space_constant));
  endB = static_cast<float>(pow(endB, color_space_constant));

  // compute the interpolated color in linear space
  float a = startA + progress * (endA - startA);
  float b = startB + progress * (endB - startB);
  float r = startR + progress * (endR - startR);
  float g = startG + progress * (endG - startG);

  // convert back to RGB to [0,255] range
  a = a * 255.0f;
  r = static_cast<float>(pow(r, 1.0 / color_space_constant)) * 255.0f;
  g = static_cast<float>(pow(g, 1.0 / color_space_constant)) * 255.0f;
  b = static_cast<float>(pow(b, 1.0 / color_space_constant)) * 255.0f;
  uint32_t result_value = static_cast<uint32_t>(round(a)) << 24 |
                          static_cast<uint32_t>(round(r)) << 16 |
                          static_cast<uint32_t>(round(g)) << 8 |
                          static_cast<uint32_t>(round(b));
  return tasm::CSSValue(lepus_value(result_value),
                        tasm::CSSValuePattern::NUMBER);
}
//====== ColorValueAnimator end =======

//====== FloatValueAnimator begin =======
std::unique_ptr<FloatKeyframe> FloatKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function) {
  return std::make_unique<FloatKeyframe>(time, std::move(timing_function));
}

FloatKeyframe::FloatKeyframe(fml::TimeDelta time,
                             std::unique_ptr<TimingFunction> timing_function)
    : Keyframe(time, std::move(timing_function)) {}

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
    value = static_cast<float>(keyframe->Value());
  }
  return value;
}

bool FloatKeyframe::SetValue(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
    tasm::Element* element) {
  auto keyframe_float_value =
      HandleCSSVariableValueIfNeed(css_value_pair, element);
  if (!keyframe_float_value.IsNumber()) {
    return false;
  }
  value_ = keyframe_float_value.GetValue().Number();
  is_empty_ = false;
  return true;
}

std::unique_ptr<KeyframedFloatAnimationCurve>
KeyframedFloatAnimationCurve::Create() {
  return std::make_unique<KeyframedFloatAnimationCurve>();
}

tasm::CSSValue KeyframedFloatAnimationCurve::GetValue(fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframedFloatAnimationCurve::GetValue",
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("FloatAnimation");
              });

  t = TransformedAnimationTime(keyframes_, timing_function_, scaled_duration(),
                               t);
  size_t i = GetActiveKeyframe(keyframes_, scaled_duration(), t);
  double progress =
      TransformedKeyframeProgress(keyframes_, scaled_duration(), t, i);

  FloatKeyframe* keyframe =
      reinterpret_cast<FloatKeyframe*>(keyframes_[i].get());
  FloatKeyframe* keyframe_next =
      reinterpret_cast<FloatKeyframe*>(keyframes_[i + 1].get());

  float start_float = FloatKeyframe::GetFloatKeyframeValue(
      keyframe, tasm::kPropertyIDFlexGrow, element_);
  float end_float = FloatKeyframe::GetFloatKeyframeValue(
      keyframe_next, tasm::kPropertyIDFlexGrow, element_);
  float result_value = start_float + (end_float - start_float) * progress;
  return tasm::CSSValue(lepus_value(result_value),
                        tasm::CSSValuePattern::NUMBER);
}

//====== FloatValueAnimator end =======

//====== FilterValueAnimator begin =======

std::unique_ptr<FilterKeyframe> FilterKeyframe::Create(
    fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function) {
  return std::make_unique<FilterKeyframe>(time, std::move(timing_function));
}

FilterKeyframe::FilterKeyframe(fml::TimeDelta time,
                               std::unique_ptr<TimingFunction> timing_function)
    : Keyframe(time, std::move(timing_function)) {}

tasm::CSSValue FilterKeyframe::GetFilterKeyframeValue(FilterKeyframe* keyframe,
                                                      tasm::CSSPropertyID id,
                                                      tasm::Element* element) {
  tasm::CSSValue filter = tasm::CSSValue::Empty();
  if (keyframe->IsEmpty()) {
    filter = GetStyleInElement(id, element);
  } else {
    filter = keyframe->filter_;
  }
  return filter;
}

bool FilterKeyframe::SetValue(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
    tasm::Element* element) {
  auto keyframe_filter_value =
      HandleCSSVariableValueIfNeed(css_value_pair, element);
  filter_ = keyframe_filter_value;
  is_empty_ = false;
  return true;
}

std::unique_ptr<KeyframedFilterAnimationCurve>
KeyframedFilterAnimationCurve::Create() {
  return std::make_unique<KeyframedFilterAnimationCurve>();
}

tasm::CSSValue KeyframedFilterAnimationCurve::GetValue(
    fml::TimeDelta& t) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframedFilterAnimationCurve::GetValue",
              [](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("curveType");
                curveTypeInfo->set_string_value("FilterAnimation");
              });
  t = TransformedAnimationTime(keyframes_, timing_function_, scaled_duration(),
                               t);
  size_t i = GetActiveKeyframe(keyframes_, scaled_duration(), t);
  double progress =
      TransformedKeyframeProgress(keyframes_, scaled_duration(), t, i);
  FilterKeyframe* keyframe =
      reinterpret_cast<FilterKeyframe*>(keyframes_[i].get());
  FilterKeyframe* keyframe_next =
      reinterpret_cast<FilterKeyframe*>(keyframes_[i + 1].get());

  tasm::CSSValue start_filter = FilterKeyframe::GetFilterKeyframeValue(
      keyframe, tasm::kPropertyIDFilter, element_);
  tasm::CSSValue end_filter = FilterKeyframe::GetFilterKeyframeValue(
      keyframe_next, tasm::kPropertyIDFilter, element_);
  if (start_filter == tasm::CSSValue::Empty() ||
      end_filter == tasm::CSSValue::Empty()) {
    return start_filter;
  }
  double start_filter_value =
      start_filter.GetValue().Array().get()->get(1).Double();
  uint32_t function_type_1 =
      start_filter.GetValue().Array().get()->get(0).UInt32();
  uint32_t pattern_1 = start_filter.GetValue().Array().get()->get(2).UInt32();
  double end_filter_value =
      end_filter.GetValue().Array().get()->get(1).Double();
  uint32_t function_type_2 =
      end_filter.GetValue().Array().get()->get(0).UInt32();
  uint32_t pattern_2 = end_filter.GetValue().Array().get()->get(2).UInt32();
  if (function_type_1 != function_type_2 || pattern_1 != pattern_2) {
    return start_filter;
  }
  double result_filter_value =
      start_filter_value + (end_filter_value - start_filter_value) * progress;
  auto res_arr = lepus::CArray::Create();
  res_arr->emplace_back(function_type_1);
  res_arr->emplace_back(result_filter_value);
  res_arr->emplace_back(pattern_1);
  return tasm::CSSValue(std::move(res_arr));
}

//====== FilterValueAnimator end =======

}  // namespace animation
}  // namespace lynx
