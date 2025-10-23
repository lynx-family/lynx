// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/renderer.h"

#include <algorithm>

#include "clay/fml/logging.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_object.h"

namespace clay {

Renderer::Renderer(RendererClient* client,
                   fml::RefPtr<GPUUnrefQueue> unref_queue)
    : client_(client), unref_queue_(unref_queue) {}

Renderer::~Renderer() = default;

void Renderer::SetRoot(RenderObject* root) {
  FML_DCHECK(root);
  root_ = root;
  root_->SetRenderer(this);
}

void Renderer::AddOverlayChild(RenderObject* node) {
  FML_DCHECK(node && node->IsOverlay() && node->Visible());
  // Overlays are ordered by level with descent order. For overlays with the
  // same level, newer is at the end.
  auto iter =
      std::find(overlay_children_.begin(), overlay_children_.end(), node);
  if (iter != overlay_children_.end()) {
    overlay_children_.erase(iter);
  }
  auto it = overlay_children_.begin();
  for (; it != overlay_children_.end(); it++) {
    if ((*it)->OverlayLevel() < node->OverlayLevel()) {
      break;
    }
  }
  overlay_children_.insert(it, node);
}

PendingContainerLayer* Renderer::GetLayer() const {
  return root_ ? root_->GetLayer() : nullptr;
}

void Renderer::RemoveDirtyNode(RenderObject* node) {
  nodes_needing_paint_.remove(node);
  if (node->IsOverlay()) {
    auto iter =
        std::find(overlay_children_.begin(), overlay_children_.end(), node);
    if (iter != overlay_children_.end()) {
      overlay_children_.erase(iter);
    }
    // Remove overlay's layer from layer tree.
    if (node->GetLayer()) {
      node->GetLayer()->Remove();
    }
  }
}

bool Renderer::Contains(RenderObject* node) const {
  auto it =
      std::find(nodes_needing_paint_.begin(), nodes_needing_paint_.end(), node);
  return it != nodes_needing_paint_.end();
}

void Renderer::Paint() {
  if (nodes_needing_paint_.empty()) {
    return;
  }

  root_->ValidateForPaint(true);  // root is always visible
  root_->WillPaint();
  std::list<RenderObject*> dirty_nodes;
  dirty_nodes.swap(nodes_needing_paint_);
  nodes_needing_paint_.clear();
  dirty_nodes.unique();
  // Sort the dirty nodes in reverse order (deepest first).
  dirty_nodes.sort([](RenderObject* node1, RenderObject* node2) {
    return node1->Depth() > node2->Depth();
  });

  for (auto* node : dirty_nodes) {
    if (!node->IsRepaintBoundary()) {
      FML_DLOG(ERROR) << "find dirty node: " << node->ID()
                      << " which is not IsRepaintBoundary";
      node->MarkNeedsPaint(true);
      continue;
    }
    if (node->NeedsPaint() || node->NeedsEffect()) {
      PaintingContext::RepaintCompositedChild(node, unref_queue_);
    }
  }

  // Append all layers of overlay children to root layer.
  AddOverlayToRootLayer();
}

void Renderer::RequestPaint() {
  // If we are in the layout phase, there is no need to request a new frame
  if (client_ && client_->GetRenderPhase() != RenderPhase::kLayout) {
    client_->RequestNewFrame();
  }
}

void Renderer::AddOverlayToRootLayer() {
  if (overlay_children_.empty()) {
    return;
  }

  for (auto* child : overlay_children_) {
    PendingContainerLayer* overlay_layer = child->GetLayer();
    if (!overlay_layer) {
      continue;
    }
    if (!overlay_layer->Parent()) {
      root_->GetLayer()->AppendChild(overlay_layer);
    } else {
      FML_DCHECK(overlay_layer->Parent() == root_->GetLayer());
    }
  }
}

}  // namespace clay
