// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/for_element.h"

namespace lynx {
namespace tasm {

void ForElement::UpdateChildrenCount(uint32_t count) {
  if (!parent_) {
    return;
  }
  if (block_children_.size() < count) {
    return;
  }
  auto remove_range = std::vector<fml::RefPtr<FiberElement>>{};
  remove_range.reserve(block_children_.size() - count);
  std::transform(
      block_children_.begin() + count, block_children_.end(),
      std::back_inserter(remove_range),
      [](const fml::RefPtr<FiberElement>& pChild) { return pChild; });

  for (auto& child : remove_range) {
    RemoveNode(child);
  }
}

}  // namespace tasm
}  // namespace lynx
