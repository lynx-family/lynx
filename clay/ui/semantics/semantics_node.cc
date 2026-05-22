// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/semantics/semantics_node.h"

#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/semantics/semantics_owner.h"

namespace clay {

SemanticsNode::SemanticsNode(SemanticsOwner* owner, BaseView* view, int id)
    : id_(id), owner_view_(view) {
  Attach(owner);
}

SemanticsNode::SemanticsNode(BaseView* view, int id)
    : id_(id), owner_view_(view) {}

void SemanticsNode::MarkDirty() {
  if (dirty_) {
    return;
  }
  dirty_ = true;
  if (Attached()) {
    fml::RefPtr<SemanticsNode> node = owner_->nodes_[id_];
    FML_DCHECK(node);
    FML_DCHECK(owner_->dirty_nodes_.find(node) == owner_->dirty_nodes_.end());
    owner_->dirty_nodes_.insert(node);
  }
}

SemanticsNode::~SemanticsNode() = default;

void SemanticsNode::Attach(SemanticsOwner* owner) {
  owner_ = owner;
  FML_DCHECK(owner_ && owner_->nodes_.find(id_) == owner_->nodes_.end());

  owner->nodes_[id_] = fml::RefPtr<SemanticsNode>(this);
  owner->detached_nodes_.erase(fml::RefPtr<SemanticsNode>(this));
  if (dirty_) {
    dirty_ = false;
    MarkDirty();
  }
}

void SemanticsNode::Detach() {
  FML_DCHECK(owner_ && owner_->nodes_.find(id_) != owner_->nodes_.end());
  fml::RefPtr<SemanticsNode> node = owner_->nodes_[id_];
  FML_DCHECK(owner_ && owner_->detached_nodes_.find(node) ==
                           owner_->detached_nodes_.end());
  owner_->nodes_.erase(id_);
  owner_->detached_nodes_.insert(node);
  owner_ = nullptr;

  if (!children_.empty()) {
    for (auto child : children_) {
      // The list of children may be stale and may contain nodes that have
      // been assigned to a different parent.
      if (child->Parent() == this) {
        child->Detach();
      }
    }
    children_.clear();
  }
}

void SemanticsNode::AddToUpdate(SemanticsUpdateBuilder* builder) {
  FML_DCHECK(dirty_);
  std::vector<int> children_in_hittest_order;
  if (!children_.empty()) {
    auto child_count = children_.size();
    children_in_hittest_order.resize(child_count);

    // children_ is in paint order, so we invert it to get the hit test
    // order.
    for (int i = child_count - 1; i >= 0; --i) {
      children_in_hittest_order[i] = children_[child_count - i - 1]->id_;
    }
  }

  builder->UpdateNode(id_, data_.actions, data_.flags, data_.scroll_children,
                      data_.semantics_bounds, data_.label, data_.id_selector,
                      data_.accessibility_elements, data_.transform,
                      children_in_hittest_order);

  dirty_ = false;
}

void SemanticsNode::RedepthChildren() {
  for (auto child : children_) {
    RedepthChild(child.get());
  }
}

void SemanticsNode::UpdateWith(
    const std::vector<fml::RefPtr<SemanticsNode>>& new_children,
    bool need_check_children, bool force_update) {
  FML_DCHECK(Attached());
  // First check if semantics data has changed during last update.
  if (old_data_ != data_ || force_update) {
    MarkDirty();
  }
  if (!need_check_children) {
    return;
  }

  for (auto node : children_) {
    node->dead_ = true;
  }
  for (auto node : new_children) {
    node->dead_ = false;
  }
  bool saw_change = false;
  for (auto child : children_) {
    if (child->dead_) {
      saw_change = true;
      DropChild(child.get());
      FML_DCHECK(child->Attached());
      child->Detach();
    }
  }
  for (auto child : new_children) {
    if (child->Parent()) {
      child->Parent()->DropChild(child.get());
    }
    AdoptChild(child.get());
    saw_change = true;
  }
  if (!saw_change) {
    FML_DCHECK(children_.size() == new_children.size());
    // Did the order change?
    for (size_t i = 0; i < children_.size(); ++i) {
      if (children_[i]->Id() != new_children[i]->Id()) {
        saw_change = true;
        break;
      }
    }
  }
  children_ = new_children;
  if (saw_change) {
    MarkDirty();
  }
}

std::u16string SemanticsNode::GetAccessibilityLabelWithChildren() const {
  if (data_.label.size() > 0) {
    return data_.label;
  }
  std::u16string label(u"");
  for (auto child : children_) {
    label += child->GetSemanticsData().label;
  }
  return label;
}

}  // namespace clay
