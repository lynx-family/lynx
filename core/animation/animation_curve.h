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
#include "core/renderer/css/css_property.h"
#include "core/renderer/starlight/style/css_type.h"
#include "gfx/animation/animation_keyframe.h"
#include "gfx/animation/animation_keyframe_curve.h"
#include "gfx/animation/timing_function.h"

namespace lynx {

namespace starlight {
struct AnimationData;
struct TimingFunctionData;
}  // namespace starlight

namespace tasm {
class Element;
}

namespace animation {

class OpacityAnimationCurve;
class LayoutAnimationCurve;
class ColorAnimationCurve;
class FloatAnimationCurve;
class FilterAnimationCurve;
class LayoutKeyframe;
class FilterKeyframe;
class BackgroundPositionKeyframe;
class TransformOriginKeyframe;
class TransformKeyframe;

struct KeyframeCallbacks {
  using NotifyElementSizeUpdatedCallback = void (*)(gfx::Keyframe*);
  using NotifyUnitValuesUpdatedCallback = void (*)(gfx::Keyframe*, uint32_t);

  gfx::Keyframe* keyframe{nullptr};
  NotifyElementSizeUpdatedCallback notify_element_size_updated{nullptr};
  NotifyUnitValuesUpdatedCallback notify_unit_values_updated{nullptr};
};

KeyframeCallbacks MakeKeyframeCallbacks(gfx::Keyframe* keyframe);
KeyframeCallbacks MakeKeyframeCallbacks(LayoutKeyframe* keyframe);
KeyframeCallbacks MakeKeyframeCallbacks(FilterKeyframe* keyframe);
KeyframeCallbacks MakeKeyframeCallbacks(BackgroundPositionKeyframe* keyframe);
KeyframeCallbacks MakeKeyframeCallbacks(TransformOriginKeyframe* keyframe);
KeyframeCallbacks MakeKeyframeCallbacks(TransformKeyframe* keyframe);

gfx::TimingFunctionData ToGfxTimingFunctionData(
    const starlight::TimingFunctionData& data);
gfx::AnimationData ToGfxAnimationData(const starlight::AnimationData& data);

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

class AnimationCurve : public gfx::AnimationCurve {
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
    OFFSET_DISTANCE = tasm::kPropertyIDOffsetDistance,
    BACKGROUND_POSITION = tasm::kPropertyIDBackgroundPosition,
    TRANSFORM_ORIGIN = tasm::kPropertyIDTransformOrigin,
    VISIBILITY = tasm::kPropertyIDVisibility,
  };

  virtual ~AnimationCurve() = default;
  CurveType Type() const { return type_; }

  AnimationCurve::CurveType type_;

  void SetElement(tasm::Element* element) { element_ = element; }

  template <typename T>
  void AddKeyframe(std::unique_ptr<T> keyframe) {
    auto callbacks = MakeKeyframeCallbacks(keyframe.get());
    AddKeyframe(std::unique_ptr<gfx::Keyframe>(std::move(keyframe)), callbacks);
  }

  void AddKeyframe(std::unique_ptr<gfx::Keyframe> keyframe,
                   KeyframeCallbacks callbacks = {});

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdated(tasm::CSSValuePattern);

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override = 0;

  virtual tasm::CSSValue GetValue(fml::TimeDelta& t) const = 0;

 protected:
  tasm::Element* element_{nullptr};
  std::vector<KeyframeCallbacks> keyframe_callbacks_;
};

class LayoutAnimationCurve : public AnimationCurve {
 public:
  ~LayoutAnimationCurve() override = default;

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class OpacityAnimationCurve : public AnimationCurve {
 public:
  ~OpacityAnimationCurve() override = default;

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class ColorAnimationCurve : public AnimationCurve {
 public:
  ~ColorAnimationCurve() override = default;

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class FloatAnimationCurve : public AnimationCurve {
 public:
  ~FloatAnimationCurve() override = default;

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class FilterAnimationCurve : public AnimationCurve {
 public:
  ~FilterAnimationCurve() override = default;

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class BackgroundPositionAnimationCurve : public AnimationCurve {
 public:
  ~BackgroundPositionAnimationCurve() override = default;

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class TransformOriginAnimationCurve : public AnimationCurve {
 public:
  ~TransformOriginAnimationCurve() override = default;

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class VisibilityAnimationCurve : public AnimationCurve {
 public:
  ~VisibilityAnimationCurve() override = default;

  std::unique_ptr<gfx::Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

}  // namespace animation
}  // namespace lynx
#endif  // CORE_ANIMATION_ANIMATION_CURVE_H_
