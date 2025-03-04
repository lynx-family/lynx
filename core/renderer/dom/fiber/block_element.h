// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_BLOCK_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_BLOCK_ELEMENT_H_

#include <vector>

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class BlockElement : public FiberElement {
 public:
  BlockElement(ElementManager* manager, const base::String& tag)
      : FiberElement(manager, tag) {}

  bool is_block() const override { return true; }

  virtual void InsertNode(const fml::RefPtr<Element>& child) override;
  virtual void RemoveNode(const fml::RefPtr<Element>& child,
                          bool destroy = true) override;

  void RemoveAllBlockNodes();

  size_t FindInsertIndex(const fml::RefPtr<FiberElement>& child);
  size_t GetAllNodeCountExcludeBlock();

  size_t FindBlockInsertIndex(const fml::RefPtr<FiberElement>& child);
  void AddBlockChildAt(const fml::RefPtr<FiberElement>& child, size_t index);

  size_t IndexOfBlockChild(const fml::RefPtr<FiberElement>& child);
  void RemoveBlockChildAt(size_t index);

 protected:
  base::InlineVector<fml::RefPtr<FiberElement>, kChildrenInlineVectorSize>
      block_children_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_BLOCK_ELEMENT_H_
