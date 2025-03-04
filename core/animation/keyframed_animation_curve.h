// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_KEYFRAMED_ANIMATION_CURVE_H_
#define CORE_ANIMATION_KEYFRAMED_ANIMATION_CURVE_H_

#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/animation_curve.h"
#include "core/animation/utils/timing_function.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace animation {

fml::TimeDelta TransformedAnimationTime(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    const std::unique_ptr<TimingFunction>& timing_function,
    double scaled_duration, fml::TimeDelta time);

size_t GetActiveKeyframe(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    double scaled_duration, fml::TimeDelta time);

double TransformedKeyframeProgress(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    double scaled_duration, fml::TimeDelta time, size_t i);

tasm::CSSValue GetStyleInElement(tasm::CSSPropertyID id,
                                 tasm::Element* element);

tasm::CSSValue HandleCSSVariableValueIfNeed(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
    tasm::Element* element);

const std::unordered_set<AnimationCurve::CurveType>& GetOnXAxisCurveTypeSet();

//====Layout keyframe ====
class LayoutKeyframe : public Keyframe {
 public:
  static std::pair<starlight::NLength, tasm::CSSValue> GetLayoutKeyframeValue(
      LayoutKeyframe* keyframe, tasm::CSSPropertyID id, tasm::Element* element);
  static std::unique_ptr<LayoutKeyframe> Create(
      fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function);
  ~LayoutKeyframe() override = default;

  void SetLayout(starlight::NLength length) {
    value_ = length;
    is_empty_ = false;
  }

  bool SetValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
      tasm::Element* element) override;

  void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern) override;

  const starlight::NLength& Value() const { return value_; }

  const tasm::CSSValue CSSValue() const { return css_value_; }

  LayoutKeyframe(fml::TimeDelta time,
                 std::unique_ptr<TimingFunction> timing_function);

 private:
  starlight::NLength value_;
  tasm::CSSValue css_value_;
};
class KeyframedLayoutAnimationCurve : public LayoutAnimationCurve {
 public:
  static std::unique_ptr<KeyframedLayoutAnimationCurve> Create();
  ~KeyframedLayoutAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====Opacity keyframe ====
class OpacityKeyframe : public Keyframe {
 public:
  constexpr static float kDefaultOpacity = 1.0f;
  static float GetOpacityKeyframeValue(OpacityKeyframe* keyframe,
                                       tasm::Element* element);

  static std::unique_ptr<OpacityKeyframe> Create(
      fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function);
  ~OpacityKeyframe() override = default;

  void SetOpacity(float opacity) {
    value_ = opacity;
    is_empty_ = false;
  }

  bool SetValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
      tasm::Element* element) override;

  float Value() const { return value_; }

  OpacityKeyframe(fml::TimeDelta time,
                  std::unique_ptr<TimingFunction> timing_function);

 private:
  float value_{kDefaultOpacity};
};

class KeyframedOpacityAnimationCurve : public OpacityAnimationCurve {
 public:
  static std::unique_ptr<KeyframedOpacityAnimationCurve> Create();
  ~KeyframedOpacityAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====Color keyframe ====
class ColorKeyframe : public Keyframe {
 public:
  constexpr static uint32_t kDefaultBackgroundColor = 0x0;
  constexpr static uint32_t kDefaultTextColor = 0xFF000000;
  static uint32_t GetColorKeyframeValue(ColorKeyframe*, tasm::CSSPropertyID id,
                                        tasm::Element*);
  static std::unique_ptr<ColorKeyframe> Create(
      fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function);
  ~ColorKeyframe() override = default;

  void SetColor(uint32_t color) {
    value_ = color;
    is_empty_ = false;
  }

  bool SetValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
      tasm::Element* element) override;

  uint32_t Value() const { return value_; }

  ColorKeyframe(fml::TimeDelta time,
                std::unique_ptr<TimingFunction> timing_function);

 private:
  uint32_t value_{kDefaultBackgroundColor};
};
class KeyframedColorAnimationCurve : public ColorAnimationCurve {
 public:
  KeyframedColorAnimationCurve(
      starlight::XAnimationColorInterpolationType type) {}
  static std::unique_ptr<KeyframedColorAnimationCurve> Create(
      starlight::XAnimationColorInterpolationType type);
  ~KeyframedColorAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;

  starlight::XAnimationColorInterpolationType get_color_interpolate_type() {
    return interpolate_type_;
  }

  void set_color_interpolate_type(
      starlight::XAnimationColorInterpolationType type) {
    interpolate_type_ = type;
  }

 private:
  starlight::XAnimationColorInterpolationType interpolate_type_ =
      starlight::XAnimationColorInterpolationType::kAuto;
};

//====Float keyframe ====
class FloatKeyframe : public Keyframe {
 public:
  constexpr static float kDefaultFloatValue = 1.0f;
  static float GetFloatKeyframeValue(FloatKeyframe*, tasm::CSSPropertyID id,
                                     tasm::Element*);
  static std::unique_ptr<FloatKeyframe> Create(
      fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function);
  ~FloatKeyframe() override = default;

  void SetFloat(float value) { value_ = value; }

  bool SetValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
      tasm::Element* element) override;

  uint32_t Value() const { return value_; }

  FloatKeyframe(fml::TimeDelta time,
                std::unique_ptr<TimingFunction> timing_function);

 private:
  float value_{kDefaultFloatValue};
};
class KeyframedFloatAnimationCurve : public FloatAnimationCurve {
 public:
  static std::unique_ptr<KeyframedFloatAnimationCurve> Create();
  ~KeyframedFloatAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====Filter keyframe ====
class FilterKeyframe : public Keyframe {
 public:
  static tasm::CSSValue GetFilterKeyframeValue(FilterKeyframe* keyframe,
                                               tasm::CSSPropertyID id,
                                               tasm::Element* element);

  static std::unique_ptr<FilterKeyframe> Create(
      fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function);
  ~FilterKeyframe() override = default;

  void SetFilter(const tasm::CSSValue& filter) { filter_ = filter; }

  bool SetValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
      tasm::Element* element) override;

  FilterKeyframe(fml::TimeDelta time,
                 std::unique_ptr<TimingFunction> timing_function);

 private:
  tasm::CSSValue filter_;
};

class KeyframedFilterAnimationCurve : public FilterAnimationCurve {
 public:
  static std::unique_ptr<KeyframedFilterAnimationCurve> Create();
  ~KeyframedFilterAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

}  // namespace animation
}  // namespace lynx
#endif  // CORE_ANIMATION_KEYFRAMED_ANIMATION_CURVE_H_
