// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_container_layer.h"

#include "clay/fml/logging.h"
#include "clay/ui/rendering/render_object.h"

namespace clay {

PendingContainerLayer::PendingContainerLayer() = default;

PendingContainerLayer::~PendingContainerLayer() { RemoveAllChildren(); }

void PendingContainerLayer::AppendChild(PendingLayer* child) {
  FML_DCHECK(child != this);
  FML_DCHECK(child != first_child_);
  FML_DCHECK(child != last_child_);
  FML_DCHECK(child->Parent() == nullptr);
  FML_DCHECK(child->NextSibling() == nullptr);
  FML_DCHECK(child->PreviousSibling() == nullptr);

  AddChild(child);

  // Add child to the children list.
  child->SetPreviousSibling(last_child_);
  if (last_child_) {
    last_child_->SetNextSibling(child);
  }

  last_child_ = child;

  if (!first_child_) {
    first_child_ = child;
  }
}

void PendingContainerLayer::RemoveChild(PendingLayer* child) {
  FML_DCHECK(child->Parent() == this);

  if (!child->PreviousSibling()) {
    FML_DCHECK(child == first_child_);
    first_child_ = child->NextSibling();
  } else {
    child->PreviousSibling()->SetNextSibling(child->NextSibling());
  }

  if (!child->NextSibling()) {
    FML_DCHECK(last_child_ == child);
    last_child_ = child->PreviousSibling();
  } else {
    child->NextSibling()->SetPreviousSibling(child->PreviousSibling());
  }
  child->SetPreviousSibling(nullptr);
  child->SetNextSibling(nullptr);

  PendingLayer::RemoveChild(child);

  // The owner[RenderObject] is responsible for deleting the subtree. If the
  // owner is not dirty, then we can reuse this subtree or contents.
  if (!child->Attached()) {
    delete child;
    child = nullptr;
  }
}

void PendingContainerLayer::RemoveAllChildren() {
  PendingLayer* child = first_child_;
  while (child) {
    PendingLayer* next = child->NextSibling();
    child->SetPreviousSibling(nullptr);
    child->SetNextSibling(nullptr);

    PendingLayer::RemoveChild(child);

    if (!child->Attached()) {
      delete child;
    }
    child = next;
  }
  first_child_ = nullptr;
  last_child_ = nullptr;
}

void PendingContainerLayer::AddToFrame(FrameBuilder* builder,
                                       const FloatPoint& offset) {
  AddChildrenToFrame(builder, offset);
}

void PendingContainerLayer::AddChildrenToFrame(FrameBuilder* builder,
                                               const FloatPoint& offset) {
  PendingLayer* child = first_child_;
  while (child) {
    if (!child->need_add_to_frame_ && child->ReuseLayer()) {
      builder->AddRetained(child->ReuseLayer());
    } else {
      child->AddToFrame(builder, offset);
      child->need_add_to_frame_ = false;
    }

    child = child->NextSibling();
  }
}

void PendingContainerLayer::UpdateSubtreeNeedsAddToFrame() {
  PendingLayer::UpdateSubtreeNeedsAddToFrame();
  PendingLayer* child = first_child_;
  while (child) {
    child->UpdateSubtreeNeedsAddToFrame();
    need_add_to_frame_ = need_add_to_frame_ || child->need_add_to_frame_;

    child = child->NextSibling();
  }
}

}  // namespace clay
