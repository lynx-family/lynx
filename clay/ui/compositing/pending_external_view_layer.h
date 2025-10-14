// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_EXTERNAL_VIEW_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_EXTERNAL_VIEW_LAYER_H_

#include <string>

#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_container_layer.h"

namespace clay {

class PendingExternalViewLayer : public PendingContainerLayer {
 public:
  PendingExternalViewLayer(const ElementId &element_id,
                           const skity::Vec2 &size);
  std::string GetName() const override { return "PendingExternalViewLayer"; }

 protected:
  void AddToFrame(FrameBuilder *builder, const FloatPoint &offset) override;

 private:
  ElementId element_id_;
  skity::Vec2 size_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_EXTERNAL_VIEW_LAYER_H_
