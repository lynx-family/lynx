// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_DRAWABLE_IMAGE_LAYER_H_
#define CLAY_FLOW_LAYERS_DRAWABLE_IMAGE_LAYER_H_

#include <string>

#include "clay/common/graphics/drawable_image.h"
#include "clay/flow/layers/layer.h"

namespace clay {

class DrawableImageLayer : public Layer {
 public:
  DrawableImageLayer(
      const skity::Vec2& offset, const skity::Vec2& size, int64_t image_id,
      bool freeze, clay::ImageSampling sampling,
      DrawableImage::FitMode fit_mode = DrawableImage::FitMode::kScaleToFill);

  bool IsReplacing(DiffContext* context, const Layer* layer) const override {
    return layer->as_drawable_image_layer() != nullptr;
  }

  void Diff(DiffContext* context, const Layer* old_layer) override;

  const DrawableImageLayer* as_drawable_image_layer() const override {
    return this;
  }

  void Preroll(PrerollContext* context) override;
  void Paint(PaintContext& context) const override;

#ifndef NDEBUG
  std::string DebugName() const override { return "DrawableImageLayer"; }
#endif

 private:
  skity::Vec2 offset_;
  skity::Vec2 size_;
  int64_t image_id_;
  bool freeze_;
  clay::ImageSampling sampling_;
  DrawableImage::FitMode fit_mode_;

  BASE_DISALLOW_COPY_AND_ASSIGN(DrawableImageLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_DRAWABLE_IMAGE_LAYER_H_
