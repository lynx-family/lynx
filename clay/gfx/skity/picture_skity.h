// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_PICTURE_SKITY_H_
#define CLAY_GFX_SKITY_PICTURE_SKITY_H_

#include <memory>
#include <utility>

#include "clay/gfx/animation/picture_animation_type.h"
#include "clay/gfx/gpu_ref_object.h"
#include "clay/gfx/style/color.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "skity/recorder/display_list.hpp"

namespace clay {

// The wrapper class that contains a sequence of rendering operations for skity.
// The lifecycle of DisplayListSkity must be managered by the GPU thread.
class PictureSkity : public GPURefObject {
 public:
  PictureSkity(DynamicOps dynamic_ops,
               std::unique_ptr<skity::DisplayList> picture)
      : dynamic_ops_(dynamic_ops),
        picture_(std::move(picture)),
        unique_id_(NextUniqueID()) {}

  ~PictureSkity() = default;

  std::shared_ptr<skity::DisplayList> raw() const { return picture_; }

  uint32_t unique_id() const { return unique_id_; }

  DynamicOps& GetDynamicOps() { return dynamic_ops_; }

  void DispatchToWorklet(ClayAnimationPropertyType type,
                         const clay::Color& value);
  clay::Color ObtainWorkletValue(ClayAnimationPropertyType type) const;

 private:
  static uint32_t NextUniqueID();

  DynamicOps dynamic_ops_;
  std::shared_ptr<skity::DisplayList> picture_;

  const uint32_t unique_id_;
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_PICTURE_SKITY_H_
