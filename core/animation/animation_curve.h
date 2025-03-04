// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_ANIMATION_CURVE_H_
#define CORE_ANIMATION_ANIMATION_CURVE_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/utils/timing_function.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {

namespace tasm {
class Element;
}

namespace animation {

class OpacityAnimationCurve;
class LayoutAnimationCurve;
class ColorAnimationCurve;
class FloatAnimationCurve;
class FilterAnimationCurve;

#define ALL_X_AXIS_CURVE_TYPE                                                 \
  AnimationCurve::CurveType::LEFT, AnimationCurve::CurveType::RIGHT,          \
      AnimationCurve::CurveType::WIDTH, AnimationCurve::CurveType::MAX_WIDTH, \
      AnimationCurve::CurveType::MIN_WIDTH,                                   \
      AnimationCurve::CurveType::MARGIN_LEFT,                                 \
      AnimationCurve::CurveType::MARGIN_RIGHT,                                \
      AnimationCurve::CurveType::PADDING_LEFT,                                \
      AnimationCurve::CurveType::PADDING_RIGHT,                               \
      AnimationCurve::CurveType::BORDER_LEFT_WIDTH,                           \
      AnimationCurve::CurveType::BORDER_RIGHT_WIDTH

#define ALL_LAYOUT_CURVE_TYPE                                               \
  ALL_X_AXIS_CURVE_TYPE, AnimationCurve::CurveType::TOP,                    \
      AnimationCurve::CurveType::BOTTOM, AnimationCurve::CurveType::HEIGHT, \
      AnimationCurve::CurveType::MAX_HEIGHT,                                \
      AnimationCurve::CurveType::MIN_HEIGHT,                                \
      AnimationCurve::CurveType::PADDING_TOP,                               \
      AnimationCurve::CurveType::PADDING_BOTTOM,                            \
      AnimationCurve::CurveType::MARGIN_TOP,                                \
      AnimationCurve::CurveType::MARGIN_BOTTOM,                             \
      AnimationCurve::CurveType::BORDER_TOP_WIDTH,                          \
      AnimationCurve::CurveType::BORDER_BOTTOM_WIDTH,                       \
      AnimationCurve::CurveType::FLEX_BASIS

class Keyframe {
 public:
  Keyframe(const Keyframe&) = delete;
  Keyframe& operator=(const Keyframe&) = delete;

  fml::TimeDelta Time() const;
  const TimingFunction* timing_function() const {
    return timing_function_.get();
  }

  bool IsEmpty() { return is_empty_; }

  virtual ~Keyframe() = default;

  virtual void NotifyElementSizeUpdated(){};

  virtual void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern){};

  virtual bool SetValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
      tasm::Element* element) = 0;

 protected:
  bool is_empty_{true};

  Keyframe(fml::TimeDelta time,
           std::unique_ptr<TimingFunction> timing_function);

 private:
  fml::TimeDelta time_;
  std::unique_ptr<TimingFunction> timing_function_;
};

class AnimationCurve {
 public:
  enum class CurveType {
    UNSUPPORT = 0,
    LEFT = tasm::kPropertyIDLeft,
    RIGHT = tasm::kPropertyIDRight,
    TOP = tasm::kPropertyIDTop,
    BOTTOM = tasm::kPropertyIDBottom,
    WIDTH = tasm::kPropertyIDWidth,
    HEIGHT = tasm::kPropertyIDHeight,
    OPACITY = tasm::kPropertyIDOpacity,
    BGCOLOR = tasm::kPropertyIDBackgroundColor,
    TEXTCOLOR = tasm::kPropertyIDColor,
    TRANSFORM = tasm::kPropertyIDTransform,
    MAX_WIDTH = tasm::kPropertyIDMaxWidth,
    MIN_WIDTH = tasm::kPropertyIDMinWidth,
    MAX_HEIGHT = tasm::kPropertyIDMaxHeight,
    MIN_HEIGHT = tasm::kPropertyIDMinHeight,
    PADDING_LEFT = tasm::kPropertyIDPaddingLeft,
    PADDING_RIGHT = tasm::kPropertyIDPaddingRight,
    PADDING_TOP = tasm::kPropertyIDPaddingTop,
    PADDING_BOTTOM = tasm::kPropertyIDPaddingBottom,
    MARGIN_LEFT = tasm::kPropertyIDMarginLeft,
    MARGIN_RIGHT = tasm::kPropertyIDMarginRight,
    MARGIN_TOP = tasm::kPropertyIDMarginTop,
    MARGIN_BOTTOM = tasm::kPropertyIDMarginBottom,
    BORDER_LEFT_WIDTH = tasm::kPropertyIDBorderLeftWidth,
    BORDER_RIGHT_WIDTH = tasm::kPropertyIDBorderRightWidth,
    BORDER_TOP_WIDTH = tasm::kPropertyIDBorderTopWidth,
    BORDER_BOTTOM_WIDTH = tasm::kPropertyIDBorderBottomWidth,
    BORDER_LEFT_COLOR = tasm::kPropertyIDBorderLeftColor,
    BORDER_RIGHT_COLOR = tasm::kPropertyIDBorderRightColor,
    BORDER_TOP_COLOR = tasm::kPropertyIDBorderTopColor,
    BORDER_BOTTOM_COLOR = tasm::kPropertyIDBorderBottomColor,
    FLEX_BASIS = tasm::kPropertyIDFlexBasis,
    FLEX_GROW = tasm::kPropertyIDFlexGrow,
    FILTER = tasm::kPropertyIDFilter,
  };

  virtual ~AnimationCurve() = default;
  CurveType Type() const { return type_; }
  fml::TimeDelta Duration() const;

  AnimationCurve::CurveType type_;
  TimingFunction* timing_function() { return timing_function_.get(); }
  void SetTimingFunction(std::unique_ptr<TimingFunction> timing_function) {
    timing_function_ = std::move(timing_function);
  }
  double scaled_duration() const { return scaled_duration_; }
  void set_scaled_duration(double scaled_duration) {
    scaled_duration_ = scaled_duration;
  }

  size_t get_keyframes_size() { return keyframes_.size(); }
  void AddKeyframe(std::unique_ptr<Keyframe> keyframe);

  void SetElement(tasm::Element* element) { element_ = element; }

  void EnsureFromAndToKeyframe();

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern);

  virtual std::unique_ptr<Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) = 0;

  virtual tasm::CSSValue GetValue(fml::TimeDelta& t) const = 0;

 protected:
  std::unique_ptr<TimingFunction> timing_function_;
  double scaled_duration_{1.0};
  std::vector<std::unique_ptr<Keyframe>> keyframes_;
  tasm::Element* element_{nullptr};
};

class LayoutAnimationCurve : public AnimationCurve {
 public:
  ~LayoutAnimationCurve() override = default;

  std::unique_ptr<Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class OpacityAnimationCurve : public AnimationCurve {
 public:
  ~OpacityAnimationCurve() override = default;

  std::unique_ptr<Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class ColorAnimationCurve : public AnimationCurve {
 public:
  ~ColorAnimationCurve() override = default;

  std::unique_ptr<Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class FloatAnimationCurve : public AnimationCurve {
 public:
  ~FloatAnimationCurve() override = default;

  std::unique_ptr<Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class FilterAnimationCurve : public AnimationCurve {
 public:
  ~FilterAnimationCurve() override = default;

  std::unique_ptr<Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

}  // namespace animation
}  // namespace lynx
#endif  // CORE_ANIMATION_ANIMATION_CURVE_H_
