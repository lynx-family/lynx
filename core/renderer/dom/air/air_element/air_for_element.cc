// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/air_element/air_for_element.h"

#include <algorithm>

#include "core/renderer/dom/air/air_element/air_page_element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace tasm {

AirForElement::AirForElement(ElementManager* manager, uint32_t lepus_id,
                             int32_t id)
    : AirBlockElement(manager, kAirFor, BASE_STATIC_STRING(kAirForTag),
                      lepus_id, id) {}

std::vector<fml::RefPtr<AirLepusRef>> AirForElement::GetForNodeChild(
    uint32_t lepus_id) {
  return air_element_manager_->air_node_manager()->GetAllNodesForLepusId(
      lepus_id);
}

void AirForElement::InsertNode(AirElement* child, bool from_virtual_child) {
  if (!from_virtual_child) {
    InsertAirNode(child);
  }
  if (parent_) {
    parent_->InsertNode(child, true);
  }
}

void AirForElement::RemoveAllNodes(bool destroy) {
  if (parent_) {
    for (auto child : air_children_) {
      parent_->RemoveNode(child.get(), destroy);
    }
  }
  if (destroy) {
    air_children_.erase(air_children_.begin(), air_children_.end());
  }
}

void AirForElement::UpdateChildrenCount(uint32_t count) {
  if (!parent_) {
    return;
  }
  if (air_children_.size() < count) {
    return;
  }
  auto remove_range = std::vector<AirElement*>{};
  remove_range.reserve(air_children_.size() - count);
  std::transform(
      air_children_.begin() + count, air_children_.end(),
      std::back_inserter(remove_range),
      [](const std::shared_ptr<AirElement>& pChild) { return pChild.get(); });

  // The updated count may be zero, which means no items exists in tt:for.
  // But we should always keep node_ valid to ensure that if the count changed
  // again, AirForElement has valid child element to copy.
  for (auto* child : remove_range) {
    parent_->RemoveNode(child);
  }
  air_children_.erase(
      std::remove_if(
          air_children_.begin() + count, air_children_.end(),
          [&](const std::shared_ptr<AirElement>& child) { return true; }),
      air_children_.end());
}

AirElement* AirForElement::GetForNodeChildWithIndex(uint32_t index) {
  DCHECK(index < air_children_.size());
  return air_children_[index].get();
}

uint32_t AirForElement::NonVirtualNodeCountInParent() {
  uint32_t sum = 0;
  for (auto child : air_children_) {
    sum += child->NonVirtualNodeCountInParent();
  }
  return sum;
}

}  // namespace tasm
}  // namespace lynx
