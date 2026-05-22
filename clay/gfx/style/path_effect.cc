// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/path_effect.h"

#include <memory>
#include <optional>
#include <utility>

#include "clay/gfx/skity_to_skia_utils.h"
#include "skity/geometry/rect.hpp"

namespace clay {

static void PathEffectDeleter(void* p) {
  // Some of our target environments would prefer a sized delete,
  // but other target environments do not have that operator.
  // Use an unsized delete until we get better agreement in the
  // environments.
  // See https://github.com/flutter/flutter/issues/100327
  ::operator delete(p);
}

std::shared_ptr<PathEffect> PathEffect::From(GrPathEffect* sk_path_effect) {
  if (sk_path_effect == nullptr) {
    return nullptr;
  }
#ifndef ENABLE_SKITY
  SkPathEffect::DashInfo info;
  if (SkPathEffect::DashType::kDash_DashType ==
      sk_path_effect->asADash(&info)) {
    auto dash_path_effect =
        DashPathEffect::Make(nullptr, info.fCount, info.fPhase);
    info.fIntervals = reinterpret_cast<DashPathEffect*>(dash_path_effect.get())
                          ->intervals_unsafe();
    sk_path_effect->asADash(&info);
    return dash_path_effect;
  }
  // If not dash path effect, we will use UnknownPathEffect to wrap it.
  return std::make_shared<UnknownPathEffect>(sk_ref_sp(sk_path_effect));
#else
  FML_UNIMPLEMENTED();
  return nullptr;
#endif  // ENABLE_SKITY
}

std::shared_ptr<PathEffect> DashPathEffect::Make(const float* intervals,
                                                 int count, float phase) {
  size_t needed = sizeof(DashPathEffect) + sizeof(float) * count;
  void* storage = ::operator new(needed);

  std::shared_ptr<DashPathEffect> ret;
  ret.reset(new (storage) DashPathEffect(intervals, count, phase),
            PathEffectDeleter);
  return std::move(ret);
}

std::optional<GrRect> DashPathEffect::effect_bounds(GrRect& rect) const {
  // SkDashPathEffect returns the original bounds as the bounds of the effect
  // since the dashed path will always be a subset of the original.
  return rect;
}

std::optional<GrRect> UnknownPathEffect::effect_bounds(GrRect& rect) const {
  if (!RECT_IS_SORTED(rect)) {
    return std::nullopt;
  }
  GrPaint p;
  PAINT_SET_PATH_EFFECT(p, sk_path_effect_);
  if (!PAINT_CAN_COMPUTE_FAST_BOUNDS(p)) {
    return std::nullopt;
  }
  rect = PAINT_COMPUTE_FAST_BOUNDS(p, rect, rect);
  return rect;
}

}  // namespace clay
