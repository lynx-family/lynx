// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_EXTERNAL_VIEW_LAYER_H_
#define CLAY_FLOW_LAYERS_EXTERNAL_VIEW_LAYER_H_

#include <string>

#include "clay/common/element_id.h"
#include "clay/flow/layers/container_layer.h"

namespace clay {
class ExternalViewLayer : public ContainerLayer {
 public:
  ExternalViewLayer(const clay::ElementId& element_id, const skity::Vec2& size);

  void Preroll(PrerollContext* context) override;
  void Paint(PaintContext& context) const override;

#ifndef NDEBUG
  std::string DebugName() const override { return "ExternalViewLayer"; }
#endif

 private:
  clay::ElementId element_id_;
  skity::Vec2 size_;
};
}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_EXTERNAL_VIEW_LAYER_H_
