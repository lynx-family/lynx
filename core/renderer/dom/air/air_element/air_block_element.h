// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_BLOCK_ELEMENT_H_
#define CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_BLOCK_ELEMENT_H_

#include "core/renderer/dom/air/air_element/air_element.h"

namespace lynx {
namespace tasm {

class AirBlockElement : public AirElement {
 public:
  AirBlockElement(ElementManager* manager, uint32_t lepus_id, int32_t id = -1);
  AirBlockElement(ElementManager* manager, AirElementType type,
                  const base::String& tag, uint32_t lepus_id, int32_t id = -1);

  bool is_block() const override { return true; }
  void InsertNode(AirElement* child, bool from_virtual_child = false) override;
  void RemoveNode(AirElement* child, bool destroy = true) override;
  void RemoveAllNodes(bool destroy = true) override;

  uint32_t NonVirtualNodeCountInParent() override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_BLOCK_ELEMENT_H_
