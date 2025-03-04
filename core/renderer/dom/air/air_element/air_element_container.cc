// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/air_element/air_element_container.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/air/air_element/air_element.h"

namespace lynx {
namespace tasm {

namespace {
constexpr static int32_t kInvalidIndex = -1;
}  // namespace

AirElementContainer::AirElementContainer(AirElement* air_element)
    : air_element_(air_element) {}

int AirElementContainer::id() const { return air_element_->impl_id(); }

void AirElementContainer::AddChild(AirElementContainer* child, int index) {
  if (child->parent()) {
    child->RemoveFromParent(true);
  }
  children_.push_back(child);

  child->parent_ = this;
  if (!child->air_element()->IsLayoutOnly()) {
    painting_context()->InsertPaintingNode(id(), child->id(), index);
    none_layout_only_children_size_++;
  }
}

void AirElementContainer::RemoveChild(AirElementContainer* child) {
  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end()) {
    children_.erase(it);
    if (!child->air_element_->IsLayoutOnly()) {
      none_layout_only_children_size_--;
    }
  }
  child->parent_ = nullptr;
}

void AirElementContainer::RemoveFromParent(bool is_move) {
  if (!parent_) return;
  if (!air_element()->IsLayoutOnly()) {
    painting_context()->RemovePaintingNode(parent_->id(), id(), 0, is_move);
  } else {
    // Layout only node remove children from parent recursively.
    for (int i = static_cast<int>(air_element_->GetChildCount()) - 1; i >= 0;
         --i) {
      AirElement* child = air_element_->GetChildAt(i);
      if (child && child->element_container()) {
        child->element_container()->RemoveFromParent(is_move);
      }
    }
  }
  parent_->RemoveChild(this);
}

void AirElementContainer::Destroy() {
  // Layout only destroy recursively
  if (!air_element()->IsLayoutOnly()) {
    painting_context()->DestroyPaintingNode(
        parent() ? parent()->id() : kInvalidIndex, id(), 0);
  } else {
    for (int i = static_cast<int>(air_element_->GetChildCount()) - 1; i >= 0;
         --i) {
      air_element_->GetChildAt(i)->element_container()->Destroy();
    }
  }
  if (parent()) {
    parent()->RemoveChild(this);
  }
}

void AirElementContainer::RemoveSelf(bool destroy) {
  if (!parent_) return;

  if (destroy) {
    Destroy();
  } else {
    RemoveFromParent(false);
  }
}

void AirElementContainer::InsertSelf() {
  if (!parent_ && air_element()->parent()) {
    air_element()->parent()->element_container()->AttachChildToTargetContainer(
        air_element());
  }
}

PaintingContext* AirElementContainer::painting_context() {
  return air_element()->painting_context();
}

std::pair<AirElementContainer*, int> AirElementContainer::FindParentForChild(
    AirElement* child) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElementContainer::FindParentForChild");
  AirElement* node = air_element_;
  size_t ui_index = air_element_->GetUIIndexForChild(child);
  while (node->IsLayoutOnly()) {
    AirElement* parent = node->parent();
    if (!parent) {
      return {nullptr, kInvalidIndex};
    }
    ui_index += static_cast<int>(parent->GetUIIndexForChild(node));
    node = parent;
  }
  return {node->element_container(), ui_index};
}

void AttachChildToTargetContainerRecursive(AirElementContainer* parent,
                                           AirElement* child, int& index) {
  parent->AddChild(child->element_container(), index);
  if (!child->IsLayoutOnly()) {
    ++index;
    return;
  }
  if (child->has_been_removed()) {
    // Layout only node should add subtree to parent recursively.
    for (size_t i = 0; i < child->GetChildCount(); ++i) {
      AirElement* grand_child = child->GetChildAt(i);
      AttachChildToTargetContainerRecursive(parent, grand_child, index);
    }
  }
}

void AirElementContainer::AttachChildToTargetContainer(AirElement* child) {
  auto result = FindParentForChild(child);
  if (result.first) {
    int index = result.second;
    AttachChildToTargetContainerRecursive(result.first, child, index);
  }
}

// Calculate position for AirElement and update it to impl layer.
void AirElementContainer::UpdateLayout(float left, float top,
                                       bool transition_view) {
  // Self is updated or self position is changed because of parent's frame
  // changing.
  bool need_update_impl =
      (!transition_view || is_layouted_) &&
      (air_element_->frame_changed() || left != last_left_ || top != last_top_);

  last_left_ = left;
  last_top_ = top;

  // The offset of child's position in its real parent's coordinator.
  float dx = left, dy = top;

  if (!air_element_->IsLayoutOnly()) {
    dx = 0;
    dy = 0;

    if (need_update_impl) {  // Update to impl layer
      air_element_->painting_context()->UpdateLayout(
          air_element_->impl_id(), left, top, air_element_->width(),
          air_element_->height(), air_element_->paddings().data(),
          air_element_->margins().data(), air_element_->borders().data(),
          nullptr, nullptr, air_element_->max_height(),
          air_element_->GetLepusId());
    }
    if (need_update_impl || props_changed_) {
      air_element_->painting_context()->OnNodeReady(air_element_->impl_id());
      props_changed_ = false;
    }
  }

  // Layout children
  for (size_t i = 0; i < air_element_->GetChildCount(); ++i) {
    AirElement* child = air_element_->GetChildAt(i);
    child->element_container()->UpdateLayout(
        child->left() + dx, child->top() + dy, transition_view);
  }
  air_element_->MarkUpdated();

  is_layouted_ = true;
}

void AirElementContainer::UpdateLayoutWithoutChange() {
  if (props_changed_) {
    air_element_->painting_context()->OnNodeReady(air_element_->impl_id());
    props_changed_ = false;
  }
  for (size_t i = 0; i < air_element_->GetChildCount(); ++i) {
    AirElement* child = air_element_->GetChildAt(i);
    if (child->element_container()) {
      child->element_container()->UpdateLayoutWithoutChange();
    }
  }
}

ElementManager* AirElementContainer::element_manager() {
  return air_element()->element_manager();
}

}  // namespace tasm
}  // namespace lynx
