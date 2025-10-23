// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SEMANTICS_SEMANTICS_UPDATE_NODE_H_
#define CLAY_UI_SEMANTICS_SEMANTICS_UPDATE_NODE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "skity/geometry/matrix.hpp"
#include "skity/geometry/rect.hpp"

namespace clay {

struct SemanticsUpdateNode {
  SemanticsUpdateNode() = default;
  SemanticsUpdateNode(const SemanticsUpdateNode& other) = default;
  ~SemanticsUpdateNode() = default;

  int32_t id;
  int32_t actions;
  int32_t flags;
  int32_t scroll_children;
  std::u16string label;
  std::u16string id_selector;
  std::vector<std::u16string> accessibility_elements;
  skity::Rect rect =
      skity::Rect::MakeEmpty();  // Local space, relative to parent.
  skity::Matrix transform;       // Identity
  std::vector<int> children_in_hittest_order;
};

// Contains semantic nodes that need to be updated.
//
// The keys in the map are stable node IDd, and the values contain
// semantic information for the node corresponding to the ID.
using SemanticsUpdateNodes = std::unordered_map<int, SemanticsUpdateNode>;

}  // namespace clay

#endif  // CLAY_UI_SEMANTICS_SEMANTICS_UPDATE_NODE_H_
