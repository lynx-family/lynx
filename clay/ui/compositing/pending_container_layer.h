// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_CONTAINER_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_CONTAINER_LAYER_H_

#include <string>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_layer.h"

namespace clay {

// A pending container layer that has a list of children.
//
// A [PendingContainerLayer] instance merely takes a list of children and
// inserts them into the composited rendering in order. There are subclasses of
// [PendingContainerLayer] which apply more elaborate effects in the process.
class PendingContainerLayer : public PendingLayer {
 public:
  PendingContainerLayer();
  ~PendingContainerLayer() override;

  std::string GetName() const override { return "PendingContainerLayer"; }

  PendingLayer* FirstChild() const override { return first_child_; }
  PendingLayer* LastChild() const override { return last_child_; }

  void SetFirstChild(PendingLayer* child) { first_child_ = child; }
  void SetLastChild(PendingLayer* child) { last_child_ = child; }

  // Adds the given layer to the end of this layer's child list.
  void AppendChild(PendingLayer* child);
  void RemoveChild(PendingLayer* child) override;
  void RemoveAllChildren();

  bool HasChildren() const { return first_child_; }

  void UpdateSubtreeNeedsAddToFrame() override;

 protected:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;
  // Uploads all of this layer's children to the engine.
  void AddChildrenToFrame(FrameBuilder* builder,
                          const FloatPoint& offset = FloatPoint());

 private:
  PendingLayer* first_child_ = nullptr;
  PendingLayer* last_child_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_CONTAINER_LAYER_H_
