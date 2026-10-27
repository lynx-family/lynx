// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/ui/compositing/pending_layer.h"

#include <sstream>

#include "clay/fml/logging.h"
#include "clay/ui/rendering/render_object.h"

namespace clay {

PendingLayer::PendingLayer() = default;

PendingLayer::~PendingLayer() = default;

void PendingLayer::AddChild(PendingLayer* child) {
  MarkNeedsAddToFrame();
  AdoptChild(child);
}

void PendingLayer::RemoveChild(PendingLayer* child) {
  MarkNeedsAddToFrame();
  DropChild(child);
}

void PendingLayer::Attach(RenderObject* owner) {
  FML_DCHECK(!owner_);
  owner_ = owner;
}

void PendingLayer::Detach() {
  FML_DCHECK(owner_);
  Remove();
  owner_->ResetLayer();
  owner_ = nullptr;
}

void PendingLayer::RedepthChildren() {
  PendingLayer* child = FirstChild();
  while (child) {
    RedepthChild(child);
    child = child->NextSibling();
  }
}

void PendingLayer::SetCacheStrategyRecursively(CacheStrategy strategy) {
  strategy_ = strategy;
  auto child_layer = FirstChild();
  while (child_layer) {
    child_layer->SetCacheStrategyRecursively(strategy);
    child_layer = child_layer->NextSibling();
  }
}

void PendingLayer::UpdateSubtreeNeedsAddToFrame() {}

void PendingLayer::MarkNeedsAddToFrame() { need_add_to_frame_ = true; }

#ifndef NDEBUG
void PendingLayer::DumpPendingLayerTree() const {
  std::string intent;
  intent.append(2 * Depth(), ' ');
  FML_LOG(ERROR) << intent << "[" << GetName() << "] " << this << ToString();

  PendingLayer* child = FirstChild();
  while (child) {
    child->DumpPendingLayerTree();
    child = child->NextSibling();
  }
}

std::string PendingLayer::ToString() const {
  std::stringstream ss;
  if (Attached()) {
    ss << " Owner=" << owner_->GetName() << "(" << owner_
       << " Owner id=" << owner_->ID() << ")";
  }
  return ss.str();
}
#endif

}  // namespace clay
