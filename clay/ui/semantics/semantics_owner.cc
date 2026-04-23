// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/semantics/semantics_owner.h"

#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"

namespace clay {

void SemanticsOwner::SendSemanticsUpdate() {
  if (dirty_nodes_.empty()) {
    return;
  }
  std::vector<fml::RefPtr<SemanticsNode>> local_dirty_nodes;
  // local_dirty_nodes should hava all dirty nodes which are not detached.
  std::copy_if(dirty_nodes_.begin(), dirty_nodes_.end(),
               std::back_inserter(local_dirty_nodes),
               [this](fml::RefPtr<SemanticsNode> node) {
                 return std::none_of(
                     detached_nodes_.begin(), detached_nodes_.end(),
                     [&node](fml::RefPtr<SemanticsNode> detached_node) {
                       return node == detached_node;
                     });
               });
  dirty_nodes_.clear();
  detached_nodes_.clear();
  std::stable_sort(
      local_dirty_nodes.begin(), local_dirty_nodes.end(),
      [](fml::RefPtr<SemanticsNode> a, fml::RefPtr<SemanticsNode> b) {
        return a->Depth() < b->Depth();
      });

  SemanticsUpdateBuilder builder;
  for (auto node : local_dirty_nodes) {
    // All nodes that have been visited will be marked as not dirty.
    // And parent nodes will be visited prior to children nodes.
    FML_DCHECK(!node->Parent() ||
               !static_cast<SemanticsNode*>(node->Parent())->IsDirty());
    if (node->IsDirty() && node->Attached()) {
      node->AddToUpdate(&builder);
    }
  }
  if (on_semantics_update_) {
    on_semantics_update_(builder.GetSemanticsNodeUpdates());
  }
}

void SemanticsOwner::AddDirtySemanticsForDescendants(BaseView* node) {
  if (!IsSemanticsEnabled()) {
    return;
  }
  semantics_nodes_to_update_descendants_.insert(node);
}
void SemanticsOwner::RemoveDirtySemanticsForDescendants(BaseView* node) {
  if (!IsSemanticsEnabled() || !node) {
    return;
  }
  node->VisitChildren([this](BaseView* child_view) {
    RemoveDirtySemanticsForDescendants(child_view);
  });
  semantics_nodes_to_update_descendants_.erase(node);
}

BaseView* SemanticsOwner::GetViewFromId(int id) const {
  auto it = nodes_.find(id);
  if (it != nodes_.end()) {
    return it->second->OwnerView();
  }
  return nullptr;
}

void SemanticsOwner::Reset() {
  if (nodes_.size() != 0) {
    // Detach root node to detach the whole semantics tree.
    nodes_[0]->Detach();
  }
  FML_DCHECK(nodes_.size() == 0);
  dirty_nodes_.clear();
  detached_nodes_.clear();
  semantics_nodes_to_update_descendants_.clear();
  need_rebuild_semantics_tree_ = true;
}
}  // namespace clay
