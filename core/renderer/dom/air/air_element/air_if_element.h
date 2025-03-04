// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_IF_ELEMENT_H_
#define CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_IF_ELEMENT_H_

#include <memory>
#include <vector>

#include "core/renderer/dom/air/air_element/air_block_element.h"

namespace lynx {
namespace tasm {

class AirIfElement : public AirBlockElement {
 public:
  AirIfElement(ElementManager* manager, uint32_t lepus_id, int32_t id = -1);

  bool is_if() const override { return true; }
  void UpdateIfIndex(int32_t ifIndex);

  uint32_t NonVirtualNodeCountInParent() override;

 private:
  uint32_t active_index_ = -1;
  std::vector<AirElement*> active_nodes_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_IF_ELEMENT_H_
