// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_BASE_VIEW_ANIMATION_MUTATOR_H_
#define CLAY_UI_COMPONENT_BASE_VIEW_ANIMATION_MUTATOR_H_

#include <string>

#include "clay/gfx/animation/animation_event_handler.h"
#include "clay/gfx/animation/animator_target.h"

namespace clay {
class BaseView;

// Implementing AnimatorTarget in BaseView would significantly increase the
// package size, so we use a proxy approach to avoid vtable bloat.
class BaseViewAnimationMutator : public AnimatorTarget,
                                 public AnimationEventHandler {
 public:
  explicit BaseViewAnimationMutator(BaseView* view);
  ~BaseViewAnimationMutator() override = default;

 private:
  void OnAnimationEvent(const AnimationParams& animation_params) override;
  void OnTransitionEvent(const AnimationParams& animation_params,
                         ClayAnimationPropertyType property_type) override;

  void GetProperty(ClayAnimationPropertyType type, float& value) override;
  void GetProperty(ClayAnimationPropertyType type, Color& value) override;
  void GetProperty(ClayAnimationPropertyType type,
                   TransformOperations& value) override;
  void GetProperty(ClayAnimationPropertyType type,
                   FilterOperations& value) override;

  void SetProperty(ClayAnimationPropertyType type, float value,
                   bool skip_update_for_raster_animation) override;
  void SetProperty(ClayAnimationPropertyType type, const Color& value,
                   bool skip_update_for_raster_animation) override;
  void SetProperty(ClayAnimationPropertyType type,
                   const TransformOperations& value,
                   bool skip_update_for_raster_animation) override;
  void SetProperty(ClayAnimationPropertyType type,
                   const FilterOperations& value) override;

  void SetProperty(ClayAnimationPropertyType type,
                   const BoxShadowOperations& value) override;
  void GetProperty(ClayAnimationPropertyType type,
                   BoxShadowOperations& value) override;

  AnimationHandler* GetAnimationHandler() override;

  FloatSize PercentageResolutionSize() override;

  const KeyframesMap* GetKeyframesMap(
      const std::string& animation_name) override;

  BaseView* view_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_BASE_VIEW_ANIMATION_MUTATOR_H_
