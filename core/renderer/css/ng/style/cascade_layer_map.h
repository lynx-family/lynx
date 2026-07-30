// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_STYLE_CASCADE_LAYER_MAP_H_
#define CORE_RENDERER_CSS_NG_STYLE_CASCADE_LAYER_MAP_H_

#include <memory>
#include <unordered_map>

#include "core/renderer/css/ng/style/cascade_layer.h"

namespace lynx {
namespace css {

// Merges per-fragment layer trees into a single canonical tree and computes
// global layer order via post-order DFS. Used to resolve cascade priority
// for rules belonging to different @layer declarations.
class CascadeLayerMap {
 public:
  CascadeLayerMap() = default;

  // Merge a fragment's local layer tree into the canonical tree.
  // Call once per fragment in document order. |fragment_root| may be null
  // (fragment has no @layer), in which case this is a no-op.
  void MergeLayerTree(CascadeLayer* fragment_root);

  // After all trees are merged, compute the global layer order.
  void ComputeLayerOrder();

  // Returns the global order for |layer|. nullptr means the rule is in the
  // implicit outer layer (highest normal priority).
  uint16_t GetLayerOrder(CascadeLayer* layer) const;

  bool IsEmpty() const { return canonical_root_ == nullptr; }

 private:
  void MergeSubLayers(CascadeLayer* local_node, CascadeLayer* canonical_node);
  void AssignOrderPostOrder(CascadeLayer* node, uint16_t& counter);

  std::unique_ptr<CascadeLayer> canonical_root_;
  // Maps each local CascadeLayer* (from any fragment) to its canonical node.
  std::unordered_map<void*, void*> local_to_canonical_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_STYLE_CASCADE_LAYER_MAP_H_
