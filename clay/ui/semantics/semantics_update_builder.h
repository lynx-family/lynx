// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SEMANTICS_SEMANTICS_UPDATE_BUILDER_H_
#define CLAY_UI_SEMANTICS_SEMANTICS_UPDATE_BUILDER_H_

#include <string>
#include <vector>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/transform.h"
#include "clay/ui/semantics/semantics_update_node.h"

namespace clay {

class SemanticsUpdateBuilder {
 public:
  SemanticsUpdateBuilder() = default;
  ~SemanticsUpdateBuilder() = default;

  // TODO(feiyue.1998): Determine more attributes that need update to platform
  // side.
  void UpdateNode(int32_t id, int32_t actions, int32_t flags,
                  int32_t scroll_children, float scroll_position,
                  float scroll_extent_max, float scroll_extent_min,
                  const FloatRect& rect, const std::u16string& label,
                  const std::string& id_selector,
                  const std::vector<std::string>& accessibility_elements,
                  const Transform& transform,
                  const std::vector<int>& children_in_hittest_order);

  const SemanticsUpdateNodes& GetSemanticsNodeUpdates() const { return nodes_; }

 private:
  SemanticsUpdateNodes nodes_;
};
}  // namespace clay

#endif  // CLAY_UI_SEMANTICS_SEMANTICS_UPDATE_BUILDER_H_
