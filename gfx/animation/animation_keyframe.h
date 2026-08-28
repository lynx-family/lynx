// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_ANIMATION_KEYFRAME_H_
#define GFX_ANIMATION_ANIMATION_KEYFRAME_H_

#include <cstdint>
#include <memory>
#include <utility>

#include "base/include/fml/time/time_delta.h"
#include "gfx/animation/timing_function.h"
#include "gfx/geometry/length.h"
#include "gfx/geometry/transform_operations.h"

namespace lynx {
namespace gfx {

enum class UnitTag : uint8_t {
  kNumber = 0,
  kPercent,
};

struct TaggedNumber {
  double value{0.0};
  UnitTag tag{UnitTag::kNumber};
};

struct Vec2Tagged {
  TaggedNumber x;
  TaggedNumber y;
};

struct FilterValue {
  uint32_t function{0};
  double value{0.0};
  UnitTag unit{UnitTag::kNumber};
};

class Keyframe {
 public:
  Keyframe(const Keyframe&) = delete;
  Keyframe& operator=(const Keyframe&) = delete;
  virtual ~Keyframe() = default;

  fml::TimeDelta Time() const { return time_; }
  double Offset() const { return time_.ToSecondsF(); }
  const TimingFunction* timing_function() const {
    return timing_function_.get();
  }

  bool IsEmpty() const { return is_empty_; }
  virtual KeyframeValueType ValueType() const {
    return KeyframeValueType::kUnknown;
  }

 protected:
  Keyframe(fml::TimeDelta time, std::unique_ptr<TimingFunction> timing_function)
      : time_(time), timing_function_(std::move(timing_function)) {}

  void MarkNonEmpty() { is_empty_ = false; }

  bool is_empty_{true};

 private:
  fml::TimeDelta time_;
  std::unique_ptr<TimingFunction> timing_function_;
};

class FloatKeyframe : public Keyframe {
 public:
  static std::unique_ptr<FloatKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<TimingFunction> timing_function = nullptr) {
    return std::make_unique<FloatKeyframe>(time, std::move(timing_function));
  }

  KeyframeValueType ValueType() const override {
    return KeyframeValueType::kFloat;
  }

  FloatKeyframe(fml::TimeDelta time,
                std::unique_ptr<TimingFunction> timing_function)
      : Keyframe(time, std::move(timing_function)) {}

  float Value() const { return value_; }

  void SetValue(float value) { SetFloatValue(value); }

 protected:
  void SetFloatValue(float value) {
    value_ = value;
    MarkNonEmpty();
  }

 private:
  float value_{0.0f};
};

class ColorKeyframe : public Keyframe {
 public:
  static std::unique_ptr<ColorKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<TimingFunction> timing_function = nullptr) {
    return std::make_unique<ColorKeyframe>(time, std::move(timing_function));
  }

  KeyframeValueType ValueType() const override {
    return KeyframeValueType::kColor;
  }

  ColorKeyframe(fml::TimeDelta time,
                std::unique_ptr<TimingFunction> timing_function)
      : Keyframe(time, std::move(timing_function)) {}

  uint32_t Value() const { return value_; }

  void SetValue(uint32_t value) { SetColorValue(value); }

 protected:
  void SetColorValue(uint32_t value) {
    value_ = value;
    MarkNonEmpty();
  }

 private:
  uint32_t value_{0};
};

class IntKeyframe : public Keyframe {
 public:
  static std::unique_ptr<IntKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<TimingFunction> timing_function = nullptr) {
    return std::make_unique<IntKeyframe>(time, std::move(timing_function));
  }

  IntKeyframe(fml::TimeDelta time,
              std::unique_ptr<TimingFunction> timing_function)
      : Keyframe(time, std::move(timing_function)) {}

  int32_t Value() const { return value_; }

  void SetValue(int32_t value) {
    value_ = value;
    MarkNonEmpty();
  }

 private:
  int32_t value_{0};
};

class LengthKeyframe : public Keyframe {
 public:
  KeyframeValueType ValueType() const override {
    return KeyframeValueType::kLength;
  }
  bool HasResolvedValue() const { return has_resolved_value_; }
  const LengthValue& ResolvedValue() const { return resolved_value_; }

  void SetResolvedValue(const LengthValue& value) {
    resolved_value_ = value;
    has_resolved_value_ = true;
    MarkNonEmpty();
  }

 protected:
  LengthKeyframe(fml::TimeDelta time,
                 std::unique_ptr<TimingFunction> timing_function)
      : Keyframe(time, std::move(timing_function)) {}

  void ClearResolvedValue() { has_resolved_value_ = false; }

 private:
  bool has_resolved_value_{false};
  LengthValue resolved_value_{};
};

class Vec2Keyframe : public Keyframe {
 public:
  KeyframeValueType ValueType() const override {
    return KeyframeValueType::kVec2;
  }
  bool HasResolvedValue() const { return has_resolved_value_; }
  const Vec2Tagged& ResolvedValue() const { return resolved_value_; }

  void SetResolvedValue(const Vec2Tagged& value) {
    resolved_value_ = value;
    has_resolved_value_ = true;
    MarkNonEmpty();
  }

 protected:
  Vec2Keyframe(fml::TimeDelta time,
               std::unique_ptr<TimingFunction> timing_function)
      : Keyframe(time, std::move(timing_function)) {}

  void ClearResolvedValue() { has_resolved_value_ = false; }

 private:
  bool has_resolved_value_{false};
  Vec2Tagged resolved_value_{};
};

class TransformKeyframe : public Keyframe {
 public:
  static std::unique_ptr<TransformKeyframe> Create(
      fml::TimeDelta time,
      std::unique_ptr<TimingFunction> timing_function = nullptr) {
    return std::make_unique<TransformKeyframe>(time,
                                               std::move(timing_function));
  }

  TransformKeyframe(fml::TimeDelta time,
                    std::unique_ptr<TimingFunction> timing_function)
      : Keyframe(time, std::move(timing_function)) {}

  KeyframeValueType ValueType() const override {
    return KeyframeValueType::kTransform;
  }

  bool HasResolvedValue() const { return has_resolved_value_; }
  const TransformOperations& ResolvedValue() const { return resolved_value_; }

  void SetResolvedValue(const TransformOperations& value) {
    resolved_value_ = value;
    has_resolved_value_ = true;
    MarkNonEmpty();
  }

  void SetResolvedValue(TransformOperations&& value) {
    resolved_value_ = std::move(value);
    has_resolved_value_ = true;
    MarkNonEmpty();
  }

 protected:
  void ClearResolvedValue() { has_resolved_value_ = false; }

 private:
  bool has_resolved_value_{false};
  TransformOperations resolved_value_{};
};

class FilterKeyframe : public Keyframe {
 public:
  KeyframeValueType ValueType() const override {
    return KeyframeValueType::kFilter;
  }
  bool HasResolvedValue() const { return has_resolved_value_; }
  const FilterValue& ResolvedValue() const { return resolved_value_; }

  void SetResolvedValue(const FilterValue& value) {
    resolved_value_ = value;
    has_resolved_value_ = true;
    MarkNonEmpty();
  }

 protected:
  FilterKeyframe(fml::TimeDelta time,
                 std::unique_ptr<TimingFunction> timing_function)
      : Keyframe(time, std::move(timing_function)) {}

  void ClearResolvedValue() { has_resolved_value_ = false; }

 private:
  bool has_resolved_value_{false};
  FilterValue resolved_value_{};
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_ANIMATION_KEYFRAME_H_
