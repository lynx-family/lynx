// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_LAYER_H_

#include <memory>
#include <string>

#include "clay/flow/layers/container_layer.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/rendering/abstract_node.h"

namespace clay {

class RenderObject;
using CacheStrategy = clay::CacheStrategy;

// A pending layer is a collection of paint ops that will end up in the same
// engine layer.
// During painting, the render tree generates a tree of composited layers that
// are uploaded into the engine and displayed by the compositor.
// This class is the base class for all composited layers.
class PendingLayer : public AbstractNode {
 public:
  PendingLayer();
  virtual ~PendingLayer();

  virtual std::string GetName() const { return "PendingLayer"; }

  virtual PendingLayer* FirstChild() const { return nullptr; }
  virtual PendingLayer* LastChild() const { return nullptr; }

  PendingLayer* PreviousSibling() const { return previous_; }
  PendingLayer* NextSibling() const { return next_; }

  virtual void AddChild(PendingLayer* child);
  virtual void RemoveChild(PendingLayer* child);

  void Remove() {
    if (Parent()) static_cast<PendingLayer*>(Parent())->RemoveChild(this);
  }

  void SetPreviousSibling(PendingLayer* previous) { previous_ = previous; }
  void SetNextSibling(PendingLayer* next) { next_ = next; }

  bool Attached() const { return owner_ != nullptr; }
  void Attach(RenderObject* owner);
  void Detach();

  RenderObject* Owner() const { return owner_; }

  void RetainLayer(std::shared_ptr<clay::ContainerLayer> layer) {
    engine_layer_ = layer;
    PendingLayer* parent = static_cast<PendingLayer*>(Parent());
    if (parent) {
      parent->MarkNeedsAddToFrame();
    }
  }

  std::shared_ptr<clay::ContainerLayer> ReuseLayer() const {
    return engine_layer_;
  }

  // Upload this pending layer to the engine.
  virtual void AddToFrame(FrameBuilder* builder,
                          const FloatPoint& offset = FloatPoint()) = 0;

  void SetCacheStrategy(CacheStrategy strategy) { strategy_ = strategy; }
  void SetCacheStrategyRecursively(CacheStrategy strategy);
  CacheStrategy GetCacheStrategy() const { return strategy_; }

  virtual void UpdateSubtreeNeedsAddToFrame();

  void MarkNeedsAddToFrame();

#ifndef NDEBUG
  void DumpPendingLayerTree() const;
  virtual std::string ToString() const;
#endif

 protected:
  CacheStrategy strategy_ = CacheStrategy::None;

 private:
  void RedepthChildren() override;

  // The owner_ owns this pending layer.
  RenderObject* owner_ = nullptr;
  // This layer's previous sibling in the parent layer's child list.
  PendingLayer* previous_ = nullptr;
  // This layer's next sibling in the parent layer's child list.
  PendingLayer* next_ = nullptr;

  // For reuse.
  std::shared_ptr<clay::ContainerLayer> engine_layer_;

  bool need_add_to_frame_ = true;

  friend class PendingContainerLayer;
  friend class FrameBuilder;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_LAYER_H_
