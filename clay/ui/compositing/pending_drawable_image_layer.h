// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_DRAWABLE_IMAGE_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_DRAWABLE_IMAGE_LAYER_H_

#include <string>

#include "clay/ui/compositing/pending_layer.h"

namespace clay {

class PendingDrawableImageLayer : public PendingLayer {
 public:
  PendingDrawableImageLayer(float dx, float dy, float width, float height,
                            int64_t image_id,
                            clay::DrawableImage::FitMode fit_mode);
  ~PendingDrawableImageLayer() override;

  std::string GetName() const override { return "PendingDrawableImageLayer"; }

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;

  float dx_ = 0.f;
  float dy_ = 0.f;
  float width_ = 0.f;
  float height_ = 0.f;
  int64_t image_id_;
  clay::DrawableImage::FitMode fit_mode_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_DRAWABLE_IMAGE_LAYER_H_
