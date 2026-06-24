// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_LAYOUT_MANAGER_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_LAYOUT_MANAGER_HARMONY_H_

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "base/include/fml/memory/ref_counted.h"
#include "core/public/layout_node_value.h"

namespace lynx {
namespace tasm {

class Element;

namespace harmony {
class ShadowNode;
class ShadowNodeOwner;
class TextShadowNode;
}  // namespace harmony

class TextLayoutManagerHarmony final {
 public:
  explicit TextLayoutManagerHarmony(harmony::ShadowNodeOwner* node_owner)
      : node_owner_(node_owner) {}
  ~TextLayoutManagerHarmony() = default;

  void DispatchLayoutBefore(Element* element);
  LayoutResult Measure(Element* element, float width, int width_mode,
                       float height, int height_mode);
  void Align(Element* element);
  void Destroy(Element* element);

  fml::RefPtr<fml::RefCountedThreadSafeStorage> GetTextBundle(int32_t id);

 private:
  using NodeIdSet = std::unordered_set<int32_t>;

  harmony::ShadowNode* GetOrCreateShadowNode(Element* element,
                                             NodeIdSet& current_node_ids);
  harmony::TextShadowNode* GetOrCreateTextShadowNode(
      Element* element, NodeIdSet& current_node_ids);
  void SyncTextSubtree(Element* element, harmony::ShadowNode* shadow_node,
                       NodeIdSet& current_node_ids);
  void DestroyNodes(const NodeIdSet& node_ids, int32_t root_id);
  void RemoveNodeIdsFromTextTrees(const NodeIdSet& node_ids);

  harmony::ShadowNodeOwner* node_owner_{nullptr};
  std::unordered_set<int32_t> owned_node_ids_;
  std::unordered_map<int32_t, NodeIdSet> text_tree_node_ids_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_LAYOUT_MANAGER_HARMONY_H_
