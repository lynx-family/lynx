// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/keyframe_model.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <limits>
#include <utility>

#include "base/include/log/logging.h"
#include "core/animation/animation_curve.h"

namespace lynx {
namespace animation {

std::unique_ptr<KeyframeModel> KeyframeModel::Create(
    std::unique_ptr<AnimationCurve> curve) {
  return std::make_unique<KeyframeModel>(std::move(curve));
}

KeyframeModel::KeyframeModel(std::unique_ptr<AnimationCurve> curve)
    : gfx_model_(lynx::gfx::KeyframeModel::Create(std::move(curve))) {}

bool KeyframeModel::InEffect(fml::TimePoint monotonic_time) const {
  return gfx_model_->InEffect(monotonic_time);
}

void KeyframeModel::UpdateAnimationData(starlight::AnimationData* data) {
  if (data == nullptr) {
    animation_data_ = nullptr;
    gfx_model_->SetAnimationData(nullptr);
    return;
  }
  animation_data_ = data;
  gfx_animation_data_ = ToGfxAnimationData(*animation_data_);
  gfx_model_->SetAnimationData(&gfx_animation_data_);
}

void KeyframeModel::EnsureFromAndToKeyframe() {
  gfx_model_->EnsureFromAndToKeyframe();
}

void KeyframeModel::NotifyElementSizeUpdated() {
  if (auto* c = curve()) {
    c->NotifyElementSizeUpdated();
  }
}

void KeyframeModel::NotifyUnitValuesUpdated(tasm::CSSValuePattern type) {
  if (auto* c = curve()) {
    c->NotifyUnitValuesUpdated(type);
  }
}

bool KeyframeModel::ShouldPersistFillStyle() const {
  if (!animation_data_) {
    return false;
  }
  return animation_data_->fill_mode ==
             starlight::AnimationFillModeType::kForwards ||
         animation_data_->fill_mode == starlight::AnimationFillModeType::kBoth;
}

}  // namespace animation
}  // namespace lynx
