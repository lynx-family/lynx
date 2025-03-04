// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_IF_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_IF_ELEMENT_H_

#include "core/renderer/dom/fiber/block_element.h"

namespace lynx {
namespace tasm {

class IfElement : public BlockElement {
 public:
  IfElement(ElementManager* manager, const base::String& tag)
      : BlockElement(manager, tag) {}

  bool is_if() const override { return true; }

  void UpdateIfIndex(int32_t ifIndex);

 private:
  uint32_t active_index_ = -1;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_IF_ELEMENT_H_
