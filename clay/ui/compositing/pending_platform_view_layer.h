// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_PLATFORM_VIEW_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_PLATFORM_VIEW_LAYER_H_

#include <string>

#include "clay/ui/compositing/pending_layer.h"

namespace clay {

class PendingPlatformViewLayer : public PendingLayer {
 public:
  PendingPlatformViewLayer(float dx, float dy, float width, float height,
                           int64_t view_id);
  ~PendingPlatformViewLayer() override;

  std::string GetName() const override { return "PendingPlatformViewLayer"; }

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;

  float dx_ = 0.f;
  float dy_ = 0.f;
  float width_ = 0.f;
  float height_ = 0.f;
  int64_t view_id_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_PLATFORM_VIEW_LAYER_H_
