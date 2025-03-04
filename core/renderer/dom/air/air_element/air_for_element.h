// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_FOR_ELEMENT_H_
#define CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_FOR_ELEMENT_H_

#include <memory>
#include <vector>

#include "core/renderer/dom/air/air_element/air_block_element.h"

namespace lynx {
namespace tasm {

class AirForElement : public AirBlockElement {
 public:
  AirForElement(ElementManager* manager, uint32_t lepus_id, int32_t id = -1);

  bool is_for() const override { return true; }
  void InsertNode(AirElement* child, bool from_virtual_child = false) override;
  void RemoveAllNodes(bool destroy = true) override;

  std::vector<fml::RefPtr<AirLepusRef>> GetForNodeChild(uint32_t lepus_id);
  void UpdateChildrenCount(uint32_t count);
  void UpdateActiveIndex(uint32_t index) { active_index_ = index; }

  AirElement* GetForNodeChildWithIndex(uint32_t index);
  uint32_t NonVirtualNodeCountInParent() override;

  uint32_t ActiveIndex() const { return active_index_; }

 private:
  uint32_t active_index_ = -1;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_FOR_ELEMENT_H_
