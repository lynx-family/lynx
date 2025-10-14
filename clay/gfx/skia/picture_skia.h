// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKIA_PICTURE_SKIA_H_
#define CLAY_GFX_SKIA_PICTURE_SKIA_H_

#include <list>
#include <memory>
#include <utility>

#include "clay/gfx/animation/picture_animation_type.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/style/color.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace clay {

// The wrapper class that contains a sequence of rendering operations for skia.
// The lifecycle of PictureSkia must be managered by the GPU thread.
class PictureSkia : public GPURefObject {
 public:
  explicit PictureSkia(sk_sp<SkPicture> picture, DynamicOps&& dynamic_ops)
      : picture_(std::move(picture)), dynamic_ops_(std::move(dynamic_ops)) {}

  ~PictureSkia() override = default;

  sk_sp<SkPicture> raw() const { return picture_; }

  uint32_t unique_id() const { return picture_->uniqueID(); }

  void DispatchToWorklet(ClayAnimationPropertyType type, const Color& value);

  Color ObtainWorkletValue(ClayAnimationPropertyType type) const;

  DynamicOps& GetDynamicOps() { return dynamic_ops_; }

 private:
  sk_sp<SkPicture> picture_;
  DynamicOps dynamic_ops_;
};

}  // namespace clay

#endif  // CLAY_GFX_SKIA_PICTURE_SKIA_H_
