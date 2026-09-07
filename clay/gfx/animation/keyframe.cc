// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/keyframe.h"

#include <memory>
#include <sstream>
#include <utility>

#include "clay/gfx/geometry/filter_operations.h"

namespace clay {

namespace {

std::unique_ptr<lynx::gfx::TimingFunction> CloneKeyframeTimingFunction(
    const lynx::gfx::Keyframe& keyframe) {
  return keyframe.timing_function() ? keyframe.timing_function()->Clone()
                                    : nullptr;
}

}  // namespace

std::unique_ptr<RawTransformKeyframe> RawTransformKeyframe::Create(
    fml::TimeDelta time, const std::vector<TransformRaw>& transform,
    std::unique_ptr<lynx::gfx::TimingFunction> timing_function) {
  return std::unique_ptr<RawTransformKeyframe>(
      new RawTransformKeyframe(time, transform, std::move(timing_function)));
}

RawTransformKeyframe::RawTransformKeyframe(
    fml::TimeDelta time, const std::vector<TransformRaw>& transform,
    std::unique_ptr<lynx::gfx::TimingFunction> timing_function)
    : lynx::gfx::Keyframe(time, std::move(timing_function)),
      operations_(transform) {
  MarkNonEmpty();
}

#ifndef NDEBUG
std::string RawTransformKeyframe::ToString() const {
  std::ostringstream os;
  os << "RawTransformKeyframe: fraction=" << Time().ToSecondsF()
     << " operation_count=" << operations_.size();
  return os.str();
}
#endif

FilterKeyframe::FilterKeyframe(
    fml::TimeDelta time, const FilterOperations& value,
    std::unique_ptr<lynx::gfx::TimingFunction> timing_function)
    : lynx::gfx::Keyframe(time, std::move(timing_function)), value_(value) {
  MarkNonEmpty();
}

std::unique_ptr<FilterKeyframe> FilterKeyframe::Create(
    fml::TimeDelta time, const FilterOperations& value,
    std::unique_ptr<lynx::gfx::TimingFunction> timing_function) {
  return std::unique_ptr<FilterKeyframe>(
      new FilterKeyframe(time, value, std::move(timing_function)));
}

FilterKeyframe::~FilterKeyframe() = default;

#ifndef NDEBUG
std::string FilterKeyframe::ToString() const { return "FilterKeyframe"; }
#endif

std::unique_ptr<FilterKeyframe> FilterKeyframe::Clone() const {
  return FilterKeyframe::Create(Time(), Value(),
                                CloneKeyframeTimingFunction(*this));
}

const FilterOperations& FilterKeyframe::Value() const { return value_; }

std::unique_ptr<BoxShadowKeyframe> BoxShadowKeyframe::Create(
    fml::TimeDelta time, const BoxShadowOperations& value,
    std::unique_ptr<lynx::gfx::TimingFunction> timing_function) {
  return std::unique_ptr<BoxShadowKeyframe>(
      new BoxShadowKeyframe(time, value, std::move(timing_function)));
}
BoxShadowKeyframe::~BoxShadowKeyframe() = default;

const BoxShadowOperations& BoxShadowKeyframe::Value() const { return value_; }

std::unique_ptr<BoxShadowKeyframe> BoxShadowKeyframe::Clone() const {
  return BoxShadowKeyframe::Create(Time(), Value(),
                                   CloneKeyframeTimingFunction(*this));
}

#ifndef NDEBUG
std::string BoxShadowKeyframe::ToString() const { return "BoxShadowKeyframe"; }
#endif

BoxShadowKeyframe::BoxShadowKeyframe(
    fml::TimeDelta time, const BoxShadowOperations& value,
    std::unique_ptr<lynx::gfx::TimingFunction> timing_function)
    : lynx::gfx::Keyframe(time, std::move(timing_function)), value_(value) {
  MarkNonEmpty();
}

}  // namespace clay
