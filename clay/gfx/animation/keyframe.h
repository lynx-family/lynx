// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_KEYFRAME_H_
#define CLAY_GFX_ANIMATION_KEYFRAME_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/macros.h"
#include "clay/gfx/animation/interpolator.h"
#include "clay/gfx/geometry/box_shadow_operations.h"
#include "clay/gfx/geometry/filter_operations.h"
#include "clay/gfx/geometry/transform_operations.h"
#include "clay/gfx/style/color.h"

namespace clay {

class Keyframe {
 public:
  Keyframe(const Keyframe&) = delete;
  Keyframe& operator=(const Keyframe&) = delete;

  /**
   * Gets the time for this keyframe, as a fraction of the overall animation
   * duration.
   *
   * @return The time associated with this keyframe, as a fraction of the
   * overall animation duration. This should be a value between 0 and 1.
   */
  float GetFraction() const { return fraction_; }

  /**
   * Gets the optional interpolator for this Keyframe. A null pointer
   * indicates that there is no interpolation, which is the same as linear
   * interpolation.
   *
   * @return The optional interpolator for this Keyframe.
   */
  Interpolator* GetInterpolator() const { return interpolator_.get(); }

#ifndef NDEBUG
  virtual std::string ToString() const = 0;
#endif

 protected:
  Keyframe(float fraction, std::unique_ptr<Interpolator> interpolator);
  virtual ~Keyframe();

 private:
  /**
   * The time at which value_ will hold true.
   */
  float fraction_ = 0.f;

  /**
   * The optional time interpolator for the interval preceding this keyframe.
   * A null interpolator (the default) results in linear interpolation
   * over the interval.
   */
  std::unique_ptr<Interpolator> interpolator_;
};

class FloatKeyframe : public Keyframe {
 public:
  typedef float ValueType;

  FloatKeyframe(const FloatKeyframe&) = delete;
  FloatKeyframe& operator=(const FloatKeyframe&) = delete;

  static std::unique_ptr<FloatKeyframe> Create(
      float fraction, float value, std::unique_ptr<Interpolator> interpolator);
  ~FloatKeyframe() override;

  float Value() const;

  std::unique_ptr<FloatKeyframe> Clone() const;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  FloatKeyframe(float fraction, float value,
                std::unique_ptr<Interpolator> interpolator);

  /**
   * The value of the animation at the time fraction_.
   */
  float value_;
};

class ColorKeyframe : public Keyframe {
 public:
  typedef Color ValueType;

  ColorKeyframe(const ColorKeyframe&) = delete;
  ColorKeyframe& operator=(const ColorKeyframe&) = delete;

  static std::unique_ptr<ColorKeyframe> Create(
      float fraction, Color value, std::unique_ptr<Interpolator> interpolator);
  ~ColorKeyframe() override;

  Color Value() const;

  std::unique_ptr<ColorKeyframe> Clone() const;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  ColorKeyframe(float fraction, Color value,
                std::unique_ptr<Interpolator> interpolator);

  /**
   * The value of the animation at the time fraction_.
   */
  Color value_;
};

// See `RawTransformKeyframeSet` class
class RawTransformKeyframe : public Keyframe {
 public:
  static std::unique_ptr<RawTransformKeyframe> Create(
      float fraction, const ClayTransform& transform,
      std::unique_ptr<Interpolator> interpolator);
  static std::unique_ptr<RawTransformKeyframe> Create(
      float fraction, const std::vector<ClayTransformOP>& transform,
      std::unique_ptr<Interpolator> interpolator);

#ifndef NDEBUG
  std::string ToString() const override;
#endif

  const std::vector<ClayTransformOP>& Operations() const { return operations_; }

 private:
  RawTransformKeyframe(float fraction, const ClayTransform& transform,
                       std::unique_ptr<Interpolator> interpolator);
  RawTransformKeyframe(float fraction,
                       const std::vector<ClayTransformOP>& transform,
                       std::unique_ptr<Interpolator> interpolator);

  std::vector<ClayTransformOP> operations_;
};

class TransformKeyframe : public Keyframe {
 public:
  typedef TransformOperations ValueType;

  TransformKeyframe(const TransformKeyframe&) = delete;
  TransformKeyframe& operator=(const TransformKeyframe&) = delete;

  static std::unique_ptr<TransformKeyframe> Create(
      float fraction, const TransformOperations& value,
      std::unique_ptr<Interpolator> interpolator);
  ~TransformKeyframe() override;

  const TransformOperations& Value() const;

  std::unique_ptr<TransformKeyframe> Clone() const;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  TransformKeyframe(float fraction, const TransformOperations& value,
                    std::unique_ptr<Interpolator> interpolator);

  /**
   * The value of the animation at the time fraction_.
   */
  TransformOperations value_;
};

class FilterKeyframe : public Keyframe {
 public:
  typedef FilterOperations ValueType;
  FilterKeyframe(const FilterKeyframe&) = delete;
  FilterKeyframe& operator=(const FilterKeyframe&) = delete;

  static std::unique_ptr<FilterKeyframe> Create(
      float fraction, const FilterOperations& value,
      std::unique_ptr<Interpolator> interpolator);
  ~FilterKeyframe() override;

  const FilterOperations& Value() const;

  std::unique_ptr<FilterKeyframe> Clone() const;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  FilterKeyframe(float fraction, const FilterOperations& value,
                 std::unique_ptr<Interpolator> interpolator);

  /**
   * The value of the animation at the time fraction_.
   */
  FilterOperations value_;

  friend class FilterKeyframeSet;
};

class BoxShadowKeyframe : public Keyframe {
 public:
  typedef BoxShadowOperations ValueType;
  BoxShadowKeyframe(const BoxShadowKeyframe&) = delete;
  BoxShadowKeyframe& operator=(const BoxShadowKeyframe&) = delete;

  static std::unique_ptr<BoxShadowKeyframe> Create(
      float fraction, const BoxShadowOperations& value,
      std::unique_ptr<Interpolator> interpolator);
  ~BoxShadowKeyframe() override;

  const BoxShadowOperations& Value() const;

  std::unique_ptr<BoxShadowKeyframe> Clone() const;

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  BoxShadowKeyframe(float fraction, const BoxShadowOperations& value,
                    std::unique_ptr<Interpolator> interpolator);

  /**
   * The value of the animation at the time fraction_.
   */
  BoxShadowOperations value_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_KEYFRAME_H_
