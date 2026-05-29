// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/semantics/semantics_update_builder.h"

#include <string>
#include <vector>

#include "base/include/string/string_utils.h"

namespace clay {

std::vector<std::u16string> ConvertFromUtf8Vector(
    const std::vector<std::string>& strings) {
  std::vector<std::u16string> result(strings.size());
  for (auto i = 0u; i < strings.size(); ++i) {
    result[i] = lynx::base::U8StringToU16(strings[i]);
  }
  return result;
}

void SemanticsUpdateBuilder::UpdateNode(
    int32_t id, int32_t actions, int32_t flags, int32_t scroll_children,
    float scroll_position, float scroll_extent_max, float scroll_extent_min,
    const FloatRect& rect, const std::u16string& label,
    const std::string& id_selector,
    const std::vector<std::string>& accessibility_elements,
    const Transform& transform,
    const std::vector<int>& children_in_hittest_order) {
  SemanticsUpdateNode node;
  node.id = id;
  node.actions = actions;
  node.flags = flags;
  node.scroll_children = scroll_children;
  node.scroll_position = scroll_position;
  node.scroll_extent_max = scroll_extent_max;
  node.scroll_extent_min = scroll_extent_min;
  node.rect = rect;
  node.label = label;
  node.id_selector = lynx::base::U8StringToU16(id_selector);
  node.accessibility_elements = ConvertFromUtf8Vector(accessibility_elements);
  node.transform = transform.matrix();
  node.children_in_hittest_order = children_in_hittest_order;

  nodes_[id] = node;
}

}  // namespace clay
