// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/keyframe.h"

#include <memory>
#include <sstream>

#include "clay/gfx/geometry/filter_operations.h"
#include "clay/gfx/geometry/transform.h"

namespace clay {

Keyframe::Keyframe(float fraction, std::unique_ptr<Interpolator> interpolator)
    : fraction_(fraction), interpolator_(std::move(interpolator)) {}

Keyframe::~Keyframe() = default;

std::unique_ptr<RawTransformKeyframe> RawTransformKeyframe::Create(
    float fraction, const std::vector<TransformRaw>& transform,
    std::unique_ptr<Interpolator> interpolator) {
  return std::unique_ptr<RawTransformKeyframe>(
      new RawTransformKeyframe(fraction, transform, std::move(interpolator)));
}

RawTransformKeyframe::RawTransformKeyframe(
    float fraction, const std::vector<TransformRaw>& transform,
    std::unique_ptr<Interpolator> interpolator)
    : Keyframe(fraction, std::move(interpolator)), operations_(transform) {}

#ifndef NDEBUG
std::string RawTransformKeyframe::ToString() const {
  std::ostringstream os;
  os << "RawTransformKeyframe: fraction=" << GetFraction()
     << " operation_count=" << operations_.size();
  return os.str();
}
#endif

std::unique_ptr<TransformKeyframe> TransformKeyframe::Create(
    float fraction, const lynx::gfx::TransformOperations& value,
    std::unique_ptr<Interpolator> interpolator) {
  return std::unique_ptr<TransformKeyframe>(
      new TransformKeyframe(fraction, value, std::move(interpolator)));
}

TransformKeyframe::TransformKeyframe(
    float fraction, const lynx::gfx::TransformOperations& value,
    std::unique_ptr<Interpolator> interpolator)
    : Keyframe(fraction, std::move(interpolator)), value_(value) {}

TransformKeyframe::~TransformKeyframe() = default;

const lynx::gfx::TransformOperations& TransformKeyframe::Value() const {
  return value_;
}

std::unique_ptr<TransformKeyframe> TransformKeyframe::Clone() const {
  std::unique_ptr<Interpolator> func;
  if (GetInterpolator()) {
    func = GetInterpolator()->Clone();
  }
  return TransformKeyframe::Create(GetFraction(), Value(), std::move(func));
}

#ifndef NDEBUG
std::string TransformKeyframe::ToString() const {
  std::ostringstream os;
  os << "TransformKeyframe: fraction=" << GetFraction() << " value="
     << Transform(ApplyTransform(Value(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f))
            .ToString();
  return os.str();
}
#endif

FilterKeyframe::FilterKeyframe(float fraction, const FilterOperations& value,
                               std::unique_ptr<Interpolator> interpolator)
    : Keyframe(fraction, std::move(interpolator)), value_(value) {}

std::unique_ptr<FilterKeyframe> FilterKeyframe::Create(
    float fraction, const FilterOperations& value,
    std::unique_ptr<Interpolator> interpolator) {
  return std::unique_ptr<FilterKeyframe>(
      new FilterKeyframe(fraction, value, std::move(interpolator)));
}

FilterKeyframe::~FilterKeyframe() = default;

#ifndef NDEBUG
std::string FilterKeyframe::ToString() const { return "FilterKeyframe"; }
#endif

std::unique_ptr<FilterKeyframe> FilterKeyframe::Clone() const {
  std::unique_ptr<Interpolator> func;
  if (GetInterpolator()) {
    func = GetInterpolator()->Clone();
  }
  return FilterKeyframe::Create(GetFraction(), Value(), std::move(func));
}

const FilterOperations& FilterKeyframe::Value() const { return value_; }

std::unique_ptr<BoxShadowKeyframe> BoxShadowKeyframe::Create(
    float fraction, const BoxShadowOperations& value,
    std::unique_ptr<Interpolator> interpolator) {
  return std::unique_ptr<BoxShadowKeyframe>(
      new BoxShadowKeyframe(fraction, value, std::move(interpolator)));
}
BoxShadowKeyframe::~BoxShadowKeyframe() {}

const BoxShadowOperations& BoxShadowKeyframe::Value() const { return value_; }

std::unique_ptr<BoxShadowKeyframe> BoxShadowKeyframe::Clone() const {
  std::unique_ptr<Interpolator> func;
  if (GetInterpolator()) {
    func = GetInterpolator()->Clone();
  }
  return BoxShadowKeyframe::Create(GetFraction(), Value(), std::move(func));
}

#ifndef NDEBUG
std::string BoxShadowKeyframe::ToString() const { return "BoxShadowKeyframe"; }
#endif

BoxShadowKeyframe::BoxShadowKeyframe(float fraction,
                                     const BoxShadowOperations& value,
                                     std::unique_ptr<Interpolator> interpolator)
    : Keyframe(fraction, std::move(interpolator)), value_(value) {}

}  // namespace clay
