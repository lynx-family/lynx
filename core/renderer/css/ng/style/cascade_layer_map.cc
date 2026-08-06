// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/style/cascade_layer_map.h"

#include <algorithm>

#include "base/include/log/logging.h"

namespace lynx {
namespace css {

void CascadeLayerMap::MergeLayerTree(CascadeLayer* fragment_root) {
  if (!fragment_root) return;
  if (!canonical_root_) {
    canonical_root_ = std::make_unique<CascadeLayer>();
  }
  local_to_canonical_[fragment_root] = canonical_root_.get();
  MergeSubLayers(fragment_root, canonical_root_.get());
}

void CascadeLayerMap::MergeSubLayers(CascadeLayer* local_node,
                                     CascadeLayer* canonical_node) {
  for (const auto& local_child : local_node->GetDirectSubLayers()) {
    CascadeLayer* canonical_child =
        canonical_node->GetOrAddDirectSubLayer(local_child->GetName());
    local_to_canonical_[local_child.get()] = canonical_child;
    canonical_parent_.try_emplace(canonical_child, canonical_node);
    MergeSubLayers(local_child.get(), canonical_child);
  }
}

void CascadeLayerMap::ComputeLayerOrder() {
  if (!canonical_root_) return;
  uint16_t counter = 0;
  AssignOrderPostOrder(canonical_root_.get(), counter);
  canonical_root_->SetOrder(CascadeLayer::kImplicitOuterLayerOrder);
}

void CascadeLayerMap::AssignOrderPostOrder(CascadeLayer* node,
                                           uint16_t& counter) {
  for (const auto& child : node->GetDirectSubLayers()) {
    AssignOrderPostOrder(child.get(), counter);
  }
  DCHECK(counter < CascadeLayer::kImplicitOuterLayerOrder);
  node->SetOrder(counter++);
}

uint16_t CascadeLayerMap::GetLayerOrder(CascadeLayer* layer) const {
  if (!layer) return CascadeLayer::kImplicitOuterLayerOrder;
  auto it = local_to_canonical_.find(layer);
  if (it == local_to_canonical_.end()) {
    return CascadeLayer::kImplicitOuterLayerOrder;
  }
  return static_cast<CascadeLayer*>(it->second)->GetOrder();
}

std::vector<std::string> CascadeLayerMap::GetLayerPath(
    const CascadeLayer* layer) const {
  std::vector<std::string> path;
  if (!layer || !canonical_root_) {
    return path;
  }
  auto it = local_to_canonical_.find(const_cast<CascadeLayer*>(layer));
  if (it == local_to_canonical_.end() || it->second == canonical_root_.get()) {
    return path;
  }

  auto* current = static_cast<CascadeLayer*>(it->second);
  while (current && current != canonical_root_.get()) {
    path.push_back(current->GetName());
    auto parent_it = canonical_parent_.find(current);
    current = parent_it == canonical_parent_.end()
                  ? nullptr
                  : static_cast<CascadeLayer*>(parent_it->second);
  }
  std::reverse(path.begin(), path.end());
  return path;
}

}  // namespace css
}  // namespace lynx
