// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/air_element/air_block_element.h"

namespace lynx {
namespace tasm {

AirBlockElement::AirBlockElement(ElementManager* manager, uint32_t lepus_id,
                                 int32_t id)
    : AirElement(kAirBlock, manager, BASE_STATIC_STRING(kAirBlockTag), lepus_id,
                 id) {}

AirBlockElement::AirBlockElement(ElementManager* manager, AirElementType type,
                                 const base::String& tag, uint32_t lepus_id,
                                 int32_t id)
    : AirElement(type, manager, tag, lepus_id, id) {}

void AirBlockElement::InsertNode(AirElement* child, bool from_virtual_child) {
  if (!from_virtual_child) {
    InsertAirNode(child);
  }
  if (parent_) {
    parent_->InsertNode(child, true);
  }
}

void AirBlockElement::RemoveNode(AirElement* child, bool destroy) {
  if (destroy) {
    RemoveAirNode(child);
  }
  if (parent_) {
    parent_->RemoveNode(child, destroy);
  }
}

void AirBlockElement::RemoveAllNodes(bool destroy) {
  if (parent_) {
    for (auto const& child : SharedAirElementVector(air_children_)) {
      parent_->RemoveNode(child.get(), destroy);
    }
  }

  if (destroy) {
    air_children_.erase(air_children_.begin(), air_children_.end());
  }
}

uint32_t AirBlockElement::NonVirtualNodeCountInParent() {
  uint32_t sum = 0;
  for (auto child : air_children_) {
    sum += child->NonVirtualNodeCountInParent();
  }
  return sum;
}

}  // namespace tasm
}  // namespace lynx
