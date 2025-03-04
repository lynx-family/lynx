// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_FOR_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_FOR_ELEMENT_H_

#include <vector>

#include "core/renderer/dom/fiber/block_element.h"

namespace lynx {
namespace tasm {

class ForElement : public BlockElement {
 public:
  ForElement(ElementManager* manager, const base::String& tag)
      : BlockElement(manager, tag) {}

  bool is_for() const override { return true; }

  void UpdateChildrenCount(uint32_t count);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FOR_ELEMENT_H_
