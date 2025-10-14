// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANIMATOR_TARGET_H_
#define CLAY_GFX_ANIMATION_ANIMATOR_TARGET_H_

#include <string>

#include "clay/gfx/animation/keyframe_set.h"
#include "clay/gfx/geometry/box_shadow_operations.h"
#include "clay/gfx/geometry/filter_operations.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/style/color.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"

namespace clay {

class AnimationHandler;

class AnimatorTarget {
 public:
  virtual ~AnimatorTarget() = default;

  virtual void GetProperty(ClayAnimationPropertyType type, float& value) {}
  virtual void GetProperty(ClayAnimationPropertyType type, Color& value) {}
  virtual void GetProperty(ClayAnimationPropertyType type,
                           TransformOperations& value) {}
  virtual void GetProperty(ClayAnimationPropertyType type,
                           FilterOperations& value) {}
  virtual void GetProperty(ClayAnimationPropertyType type,
                           BoxShadowOperations& value) {}
  // SetProperty will modify value without triggering transition animation
  virtual void SetProperty(ClayAnimationPropertyType type, float value,
                           bool skip_update_for_raster_animation) {}
  virtual void SetProperty(ClayAnimationPropertyType type, const Color& value,
                           bool skip_update_for_raster_animation) {}
  virtual void SetProperty(ClayAnimationPropertyType type,
                           const TransformOperations& value,
                           bool skip_update_for_raster_animation) {}

  virtual void SetProperty(ClayAnimationPropertyType type,
                           const FilterOperations& value) {}
  virtual void SetProperty(ClayAnimationPropertyType type,
                           const BoxShadowOperations& value) {}

  virtual AnimationHandler* GetAnimationHandler() { return nullptr; }
  virtual const KeyframesMap* GetKeyframesMap(
      const std::string& animation_name) {
    return nullptr;
  }
  virtual FloatSize PercentageResolutionSize() = 0;
};

}  // namespace clay
#endif  // CLAY_GFX_ANIMATION_ANIMATOR_TARGET_H_
