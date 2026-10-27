// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/base_view_animation_mutator.h"

#include <string>

#include "clay/ui/component/base_view.h"

namespace clay {

BaseViewAnimationMutator::BaseViewAnimationMutator(BaseView* view)
    : view_(view) {}

void BaseViewAnimationMutator::OnAnimationEvent(
    const AnimationParams& animation_params) {
  view_->OnAnimationEvent(animation_params);
}

void BaseViewAnimationMutator::OnTransitionEvent(
    const AnimationParams& animation_params,
    ClayAnimationPropertyType property_type) {
  view_->OnTransitionEvent(animation_params, property_type);
}

void BaseViewAnimationMutator::GetProperty(ClayAnimationPropertyType type,
                                           float& value) {
  view_->GetProperty(type, value);
}

void BaseViewAnimationMutator::GetProperty(ClayAnimationPropertyType type,
                                           Color& value) {
  view_->GetProperty(type, value);
}

void BaseViewAnimationMutator::GetProperty(ClayAnimationPropertyType type,
                                           TransformOperations& value) {
  view_->GetProperty(type, value);
}

void BaseViewAnimationMutator::GetProperty(ClayAnimationPropertyType type,
                                           FilterOperations& value) {
  view_->GetProperty(type, value);
}

void BaseViewAnimationMutator::SetProperty(
    ClayAnimationPropertyType type, float value,
    bool skip_update_for_raster_animation) {
  view_->SetProperty(type, value, skip_update_for_raster_animation);
}

void BaseViewAnimationMutator::SetProperty(
    ClayAnimationPropertyType type, const Color& value,
    bool skip_update_for_raster_animation) {
  view_->SetProperty(type, value, skip_update_for_raster_animation);
}

void BaseViewAnimationMutator::SetProperty(
    ClayAnimationPropertyType type, const TransformOperations& value,
    bool skip_update_for_raster_animation) {
  view_->SetProperty(type, value, skip_update_for_raster_animation);
}

void BaseViewAnimationMutator::SetProperty(ClayAnimationPropertyType type,
                                           const FilterOperations& value) {
  view_->SetProperty(type, value);
}

AnimationHandler* BaseViewAnimationMutator::GetAnimationHandler() {
  return view_->GetAnimationHandler();
}

FloatSize BaseViewAnimationMutator::PercentageResolutionSize() {
  return view_->PercentageResolutionSize();
}

const KeyframesMap* BaseViewAnimationMutator::GetKeyframesMap(
    const std::string& animation_name) {
  return view_->GetKeyframesMap(animation_name);
}

void BaseViewAnimationMutator::SetProperty(ClayAnimationPropertyType type,
                                           const BoxShadowOperations& value) {
  if (type != ClayAnimationPropertyType::kBoxShadow) {
    return;
  }
  view_->SetBoxShadowOperations(value);
}

void BaseViewAnimationMutator::GetProperty(ClayAnimationPropertyType type,
                                           BoxShadowOperations& value) {
  if (type != ClayAnimationPropertyType::kBoxShadow) {
    return;
  }
  value = view_->box_shadow_ops_;
}

}  // namespace clay
