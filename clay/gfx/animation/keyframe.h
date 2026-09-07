// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_KEYFRAME_H_
#define CLAY_GFX_ANIMATION_KEYFRAME_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "clay/gfx/geometry/box_shadow_operations.h"
#include "clay/gfx/geometry/filter_operations.h"
#include "clay/gfx/geometry/transform_raw.h"
#include "gfx/animation/animation_keyframe.h"
#include "gfx/animation/timing_function.h"

namespace clay {

using FloatKeyframe = lynx::gfx::FloatKeyframe;
using ColorKeyframe = lynx::gfx::ColorKeyframe;
using TransformKeyframe = lynx::gfx::TransformKeyframe;

// See `RawTransformKeyframeSet` class
class RawTransformKeyframe : public lynx::gfx::Keyframe {
 public:
  static std::unique_ptr<RawTransformKeyframe> Create(
      fml::TimeDelta time, const std::vector<TransformRaw>& transform,
      std::unique_ptr<lynx::gfx::TimingFunction> timing_function = nullptr);

#ifndef NDEBUG
  std::string ToString() const;
#endif

  const std::vector<TransformRaw>& Operations() const { return operations_; }

 private:
  RawTransformKeyframe(
      fml::TimeDelta time, const std::vector<TransformRaw>& transform,
      std::unique_ptr<lynx::gfx::TimingFunction> timing_function);

  std::vector<TransformRaw> operations_;
};

class FilterKeyframe : public lynx::gfx::Keyframe {
 public:
  FilterKeyframe(const FilterKeyframe&) = delete;
  FilterKeyframe& operator=(const FilterKeyframe&) = delete;

  static std::unique_ptr<FilterKeyframe> Create(
      fml::TimeDelta time, const FilterOperations& value,
      std::unique_ptr<lynx::gfx::TimingFunction> timing_function = nullptr);
  ~FilterKeyframe() override;

  const FilterOperations& Value() const;

  std::unique_ptr<FilterKeyframe> Clone() const;

#ifndef NDEBUG
  std::string ToString() const;
#endif

 private:
  FilterKeyframe(fml::TimeDelta time, const FilterOperations& value,
                 std::unique_ptr<lynx::gfx::TimingFunction> timing_function);

  FilterOperations value_;
};

class BoxShadowKeyframe : public lynx::gfx::Keyframe {
 public:
  BoxShadowKeyframe(const BoxShadowKeyframe&) = delete;
  BoxShadowKeyframe& operator=(const BoxShadowKeyframe&) = delete;

  static std::unique_ptr<BoxShadowKeyframe> Create(
      fml::TimeDelta time, const BoxShadowOperations& value,
      std::unique_ptr<lynx::gfx::TimingFunction> timing_function = nullptr);
  ~BoxShadowKeyframe() override;

  const BoxShadowOperations& Value() const;

  std::unique_ptr<BoxShadowKeyframe> Clone() const;

#ifndef NDEBUG
  std::string ToString() const;
#endif

 private:
  BoxShadowKeyframe(fml::TimeDelta time, const BoxShadowOperations& value,
                    std::unique_ptr<lynx::gfx::TimingFunction> timing_function);

  BoxShadowOperations value_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_KEYFRAME_H_
