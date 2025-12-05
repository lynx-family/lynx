// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_PATH_EFFECT_H_
#define CLAY_GFX_STYLE_PATH_EFFECT_H_

#include <cstring>
#include <memory>
#include <optional>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/attributes.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class DashPathEffect;

enum class PathEffectType {
  kDash,
  kUnknown,
};

class PathEffect : public Attribute<PathEffect, GrPathEffect, PathEffectType> {
 public:
  virtual ~PathEffect() = default;

  std::shared_ptr<PathEffect> MakeDash(const float intervals[], int count,
                                       float phase);

  virtual const DashPathEffect* asDash() const { return nullptr; }

  virtual std::optional<GrRect> effect_bounds(GrRect&) const = 0;

 protected:
  PathEffect() = default;

 private:
  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(PathEffect);
};

/// The DashPathEffect which breaks a path up into dash segments, and it
/// only affects stroked paths.
/// intervals: array containing an even number of entries (>=2), with
/// the even indices specifying the length of "on" intervals, and the odd
/// indices specifying the length of "off" intervals. This array will be
/// copied in Make, and can be disposed of freely after.
/// count: number of elements in the intervals array.
/// phase: initial distance into the intervals at which to start the dashing
/// effect for the path.
///
/// For example: if intervals[] = {10, 20}, count = 2, and phase = 25,
/// this will set up a dashed path like so:
/// 5 pixels off
/// 10 pixels on
/// 20 pixels off
/// 10 pixels on
/// 20 pixels off
/// ...
/// A phase of -5, 25, 55, 85, etc. would all result in the same path,
/// because the sum of all the intervals is 30.
///
class DashPathEffect final : public PathEffect {
 public:
  static std::shared_ptr<PathEffect> Make(const float intervals[], int count,
                                          float phase);

  PathEffectType type() const override { return PathEffectType::kDash; }
  size_t size() const override {
    return sizeof(*this) + sizeof(float) * count_;
  }

  std::shared_ptr<PathEffect> shared() const override {
    return Make(intervals(), count_, phase_);
  }

  const DashPathEffect* asDash() const override { return this; }

  GrPathEffectPtr gr_object() const override {
    return PATH_EFFECT_MAKE_DASH(intervals(), count_, phase_);
  }

  const float* intervals() const {
    return reinterpret_cast<const float*>(this + 1);
  }

  std::optional<GrRect> effect_bounds(GrRect& rect) const override;

 protected:
  bool equals_(PathEffect const& other) const override {
    FML_DCHECK(other.type() == PathEffectType::kDash);
    auto that = static_cast<DashPathEffect const*>(&other);
    return count_ == that->count_ && phase_ == that->phase_ &&
           memcmp(intervals(), that->intervals(), sizeof(float) * count_) == 0;
  }

 private:
  // DashPathEffect constructor assumes the caller has prealloced storage for
  // the intervals. If the intervals is nullptr the intervals will
  // uninitialized.
  DashPathEffect(const float intervals[], int count, float phase)
      : count_(count), phase_(phase) {
    if (intervals != nullptr) {
      float* intervals_ = reinterpret_cast<float*>(this + 1);
      memcpy(intervals_, intervals, sizeof(float) * count);
    }
  }

  explicit DashPathEffect(const DashPathEffect* dash_effect)
      : DashPathEffect(dash_effect->intervals(), dash_effect->count_,
                       dash_effect->phase_) {}

  float* intervals_unsafe() { return reinterpret_cast<float*>(this + 1); }

  int count_;
  float phase_;

  friend class PathEffect;
  friend class RenderingMaterial;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(DashPathEffect);
};

class UnknownPathEffect final : public PathEffect {
 public:
  UnknownPathEffect(GrPathEffectPtr effect)
      : sk_path_effect_(std::move(effect)) {}
  UnknownPathEffect(const UnknownPathEffect& effect)
      : UnknownPathEffect(effect.sk_path_effect_) {}
  UnknownPathEffect(const UnknownPathEffect* effect)
      : UnknownPathEffect(effect->sk_path_effect_) {}

  PathEffectType type() const override { return PathEffectType::kUnknown; }
  size_t size() const override { return sizeof(*this); }

  std::shared_ptr<PathEffect> shared() const override {
    return std::make_shared<UnknownPathEffect>(this);
  }

  GrPathEffectPtr gr_object() const override { return sk_path_effect_; }

  virtual ~UnknownPathEffect() = default;

  std::optional<GrRect> effect_bounds(GrRect& rect) const override;

 protected:
  bool equals_(const PathEffect& other) const override {
    FML_DCHECK(other.type() == PathEffectType::kUnknown);
    auto that = static_cast<UnknownPathEffect const*>(&other);
    return sk_path_effect_ == that->sk_path_effect_;
  }

 private:
  GrPathEffectPtr sk_path_effect_;
};

}  // namespace clay

#endif  // CLAY_GFX_STYLE_PATH_EFFECT_H_
