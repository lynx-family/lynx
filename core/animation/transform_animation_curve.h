// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TRANSFORM_ANIMATION_CURVE_H_
#define CORE_ANIMATION_TRANSFORM_ANIMATION_CURVE_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/animation_curve.h"
#include "core/animation/transforms/transform_operations.h"
#include "core/animation/utils/timing_function.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {

namespace tasm {
class Element;
}

namespace animation {

class TransformAnimationCurve : public AnimationCurve {
 public:
  ~TransformAnimationCurve() override = default;

  std::unique_ptr<Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) override;
};

class KeyframedTransformAnimationCurve : public TransformAnimationCurve {
 public:
  static std::unique_ptr<KeyframedTransformAnimationCurve> Create();
  ~KeyframedTransformAnimationCurve() override = default;

  tasm::CSSValue GetValue(fml::TimeDelta& t) const override;
};

//====Transform keyframe ====
class TransformKeyframe : public Keyframe {
 public:
  static transforms::TransformOperations GetTransformKeyframeValueInElement(
      tasm::Element*);
  static std::unique_ptr<TransformKeyframe> Create(
      fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function);
  ~TransformKeyframe() override = default;

  void SetTransform(
      std::unique_ptr<transforms::TransformOperations> transform) {
    value_ = std::move(transform);
    is_empty_ = false;
  }

  bool SetValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair,
      tasm::Element* element) override;

  const std::unique_ptr<transforms::TransformOperations>& Value() const {
    return value_;
  };

  void NotifyElementSizeUpdated() override;

  void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern) override;

  TransformKeyframe(fml::TimeDelta time,
                    std::unique_ptr<TimingFunction> timing_function);

  tasm::CSSValue CSSValue() { return css_value_; }

 private:
  std::unique_ptr<transforms::TransformOperations> value_;
  tasm::CSSValue css_value_;
};

}  // namespace animation
}  // namespace lynx
#endif  // CORE_ANIMATION_TRANSFORM_ANIMATION_CURVE_H_
