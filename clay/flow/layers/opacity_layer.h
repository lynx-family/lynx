// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_OPACITY_LAYER_H_
#define CLAY_FLOW_LAYERS_OPACITY_LAYER_H_

#include <memory>
#include <string>
#include <utility>

#include "clay/flow/layers/cacheable_layer.h"
#include "clay/flow/layers/layer.h"
#include "clay/gfx/geometry/float_size.h"

namespace clay {

// Don't add an OpacityLayer with no children to the layer tree. Painting an
// OpacityLayer is very costly due to the saveLayer call. If there's no child,
// having the OpacityLayer or not has the same effect. In debug_unopt build,
// |Preroll| will assert if there are no children.
class OpacityLayer : public CacheableContainerLayer {
 public:
  // An offset is provided here because OpacityLayer.addToScene method in the
  // Flutter framework can take an optional offset argument.
  //
  // By default, that offset is always zero, and all the offsets are handled by
  // some parent TransformLayers. But we allow the offset to be non-zero for
  // backward compatibility. If it's non-zero, the old behavior is to propagate
  // that offset to all the leaf layers (e.g., DisplayListLayer). That will make
  // the retained rendering inefficient as a small offset change could propagate
  // to many leaf layers. Therefore we try to capture that offset here to stop
  // the propagation as repainting the OpacityLayer is expensive.
  OpacityLayer(uint8_t alpha, const skity::Vec2& offset);

  void Diff(DiffContext* context, const Layer* old_layer) override;

  void Preroll(PrerollContext* context) override;

  void Paint(PaintContext& context) const override;

  // Returns whether the children are capable of inheriting an opacity value
  // and modifying their rendering accordingly. This value is only guaranteed
  // to be valid after the local |Preroll| method is called.
  bool children_can_accept_opacity() const {
    return children_can_accept_opacity_;
  }
  void set_children_can_accept_opacity(bool value) {
    children_can_accept_opacity_ = value;
  }

  float opacity() const { return alpha() * 1.0 / 0xFF; }
#ifndef NDEBUG
  std::string DebugName() const override { return "OpacityLayer"; }
#endif

 private:
  float alpha() const;

  uint8_t alpha_;
  skity::Vec2 offset_;
  bool children_can_accept_opacity_;

  BASE_DISALLOW_COPY_AND_ASSIGN(OpacityLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_OPACITY_LAYER_H_
