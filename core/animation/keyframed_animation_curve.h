// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_KEYFRAMED_ANIMATION_CURVE_H_
#define CORE_ANIMATION_KEYFRAMED_ANIMATION_CURVE_H_

#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/animation_curve.h"
#include "core/renderer/css/css_property.h"
#include "gfx/animation/animation_keyframe.h"
#include "gfx/animation/animation_utils.h"
#include "gfx/animation/timing_function.h"

namespace lynx {
namespace starlight {
class NLength;
}
namespace animation {

tasm::CSSValue GetStyleInElement(tasm::CSSPropertyID id,
                                 tasm::Element* element);

tasm::CSSValue HandleCSSVariableValueIfNeed(tasm::CSSPropertyID id,
                                            const tasm::CSSValue& value,
                                            tasm::Element* element);

const std::unordered_set<AnimationCurve::CurveType>& GetOnXAxisCurveTypeSet();

//====Layout keyframe ====
class LayoutKeyframe : public gfx::LengthKeyframe {
 public:
  static std::pair<std::optional<gfx::LengthValue>, tasm::CSSValue>
  GetLayoutKeyframeValue(LayoutKeyframe* keyframe, tasm::CSSPropertyID id,
                         tasm::Element* element);
  static std::unique_ptr<LayoutKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);
  ~LayoutKeyframe() override = default;

  void SetLayout(const gfx::LengthValue& length) { SetResolvedValue(length); }

  void SetLayout(const starlight::NLength& length);

  bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                tasm::Element* element);

  void NotifyUnitValuesUpdated(uint32_t css_value_pattern);

  const tasm::CSSValue& CSSValue() const { return css_value_; }

  LayoutKeyframe(fml::TimeDelta time,
                 std::unique_ptr<gfx::TimingFunction> timing_function);

 private:
  tasm::CSSValue css_value_;
};
class KeyframedLayoutAnimationCurve : public LayoutAnimationCurve {
 public:
  static std::unique_ptr<KeyframedLayoutAnimationCurve> Create();
  ~KeyframedLayoutAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====Opacity keyframe ====
class OpacityKeyframe : public gfx::FloatKeyframe {
 public:
  constexpr static float kDefaultOpacity = 1.0f;
  static float GetOpacityKeyframeValue(OpacityKeyframe* keyframe,
                                       tasm::Element* element);

  static std::unique_ptr<OpacityKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);
  ~OpacityKeyframe() override = default;

  void SetOpacity(float opacity) { SetFloatValue(opacity); }

  bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                tasm::Element* element);

  OpacityKeyframe(fml::TimeDelta time,
                  std::unique_ptr<gfx::TimingFunction> timing_function);
};

class KeyframedOpacityAnimationCurve : public OpacityAnimationCurve {
 public:
  static std::unique_ptr<KeyframedOpacityAnimationCurve> Create();
  ~KeyframedOpacityAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====Color keyframe ====
class ColorKeyframe : public gfx::ColorKeyframe {
 public:
  constexpr static uint32_t kDefaultBackgroundColor = 0x0;
  constexpr static uint32_t kDefaultTextColor = 0xFF000000;
  static uint32_t GetColorKeyframeValue(ColorKeyframe*, tasm::CSSPropertyID id,
                                        tasm::Element*);
  static std::unique_ptr<ColorKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);
  ~ColorKeyframe() override = default;

  void SetColor(uint32_t color) { SetColorValue(color); }

  bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                tasm::Element* element);

  ColorKeyframe(fml::TimeDelta time,
                std::unique_ptr<gfx::TimingFunction> timing_function);
};
class KeyframedColorAnimationCurve : public ColorAnimationCurve {
 public:
  KeyframedColorAnimationCurve(starlight::XAnimationColorInterpolationType type)
      : interpolate_type_(type) {}
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
class FloatKeyframe : public gfx::FloatKeyframe {
 public:
  constexpr static float kDefaultFloatValue = 0.0f;
  static float GetFloatKeyframeValue(FloatKeyframe*, tasm::CSSPropertyID id,
                                     tasm::Element*);
  static std::unique_ptr<FloatKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);
  ~FloatKeyframe() override = default;

  void SetFloat(float value) { SetFloatValue(value); }

  bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                tasm::Element* element);

  FloatKeyframe(fml::TimeDelta time,
                std::unique_ptr<gfx::TimingFunction> timing_function);
};
class KeyframedFloatAnimationCurve : public FloatAnimationCurve {
 public:
  static std::unique_ptr<KeyframedFloatAnimationCurve> Create();
  ~KeyframedFloatAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====Filter keyframe ====
class FilterKeyframe : public gfx::FilterKeyframe {
 public:
  static tasm::CSSValue GetFilterKeyframeValue(FilterKeyframe* keyframe,
                                               tasm::CSSPropertyID id,
                                               tasm::Element* element);

  static std::unique_ptr<FilterKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);
  ~FilterKeyframe() override = default;

  void SetFilter(const tasm::CSSValue& filter) {
    filter_ = filter;
    ClearResolvedValue();
    MarkNonEmpty();
  }

  bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                tasm::Element* element);

  void NotifyUnitValuesUpdated(uint32_t css_value_pattern);

  FilterKeyframe(fml::TimeDelta time,
                 std::unique_ptr<gfx::TimingFunction> timing_function);

 private:
  tasm::CSSValue filter_;
};

class KeyframedFilterAnimationCurve : public FilterAnimationCurve {
 public:
  static std::unique_ptr<KeyframedFilterAnimationCurve> Create();
  ~KeyframedFilterAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====BackgroundPosition keyframe ====
class BackgroundPositionKeyframe : public gfx::Vec2Keyframe {
 public:
  static tasm::CSSValue GetBackgroundPositionKeyframeValue(
      BackgroundPositionKeyframe* keyframe, tasm::CSSPropertyID id,
      tasm::Element* element);

  static std::unique_ptr<BackgroundPositionKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);
  ~BackgroundPositionKeyframe() override = default;

  void SetBackgroundPosition(const tasm::CSSValue& background_position) {
    background_position_ = background_position;
    ClearResolvedValue();
    MarkNonEmpty();
  }

  tasm::CSSValue GetBackgroundPosition() const { return background_position_; }

  bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                tasm::Element* element);

  void NotifyUnitValuesUpdated(uint32_t css_value_pattern);

  BackgroundPositionKeyframe(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);

 private:
  tasm::CSSValue background_position_;
};

class KeyframedBackgroundPositionAnimationCurve
    : public BackgroundPositionAnimationCurve {
 public:
  static std::unique_ptr<KeyframedBackgroundPositionAnimationCurve> Create();
  ~KeyframedBackgroundPositionAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====transformOrigin keyframe ====
class TransformOriginKeyframe : public gfx::Vec2Keyframe {
 public:
  static tasm::CSSValue GetTransformOriginKeyframeValue(
      TransformOriginKeyframe* keyframe, tasm::CSSPropertyID id,
      tasm::Element* element);

  static std::unique_ptr<TransformOriginKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);
  ~TransformOriginKeyframe() override = default;

  tasm::CSSValue GetTransformOrigin() const { return transform_origin_; }

  bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                tasm::Element* element);

  void NotifyUnitValuesUpdated(uint32_t css_value_pattern);

  TransformOriginKeyframe(fml::TimeDelta time,
                          std::unique_ptr<gfx::TimingFunction> timing_function);

 private:
  tasm::CSSValue transform_origin_;
};

class KeyframedTransformOriginAnimationCurve
    : public TransformOriginAnimationCurve {
 public:
  static std::unique_ptr<KeyframedTransformOriginAnimationCurve> Create();
  ~KeyframedTransformOriginAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====visibility keyframe ====
class VisibilityKeyframe : public gfx::Keyframe {
 public:
  static starlight::VisibilityType GetVisibilityKeyframeValue(
      VisibilityKeyframe* keyframe, tasm::Element* element);

  static std::unique_ptr<VisibilityKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<gfx::TimingFunction> timing_function);
  ~VisibilityKeyframe() override = default;

  bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                tasm::Element* element);

  starlight::VisibilityType Visibility() const { return visibility_; }

  VisibilityKeyframe(fml::TimeDelta time,
                     std::unique_ptr<gfx::TimingFunction> timing_function);

 private:
  starlight::VisibilityType visibility_{starlight::VisibilityType::kVisible};
};

class KeyframedVisibilityAnimationCurve : public VisibilityAnimationCurve {
 public:
  static std::unique_ptr<KeyframedVisibilityAnimationCurve> Create();
  ~KeyframedVisibilityAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

}  // namespace animation
}  // namespace lynx
#endif  // CORE_ANIMATION_KEYFRAMED_ANIMATION_CURVE_H_
