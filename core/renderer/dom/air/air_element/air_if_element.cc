// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/air_element/air_if_element.h"

namespace lynx {
namespace tasm {

AirIfElement::AirIfElement(ElementManager *manager, uint32_t lepus_id,
                           int32_t id)
    : AirBlockElement(manager, kAirIf, BASE_STATIC_STRING(kAirIfTag), lepus_id,
                      id) {}

void AirIfElement::UpdateIfIndex(int32_t ifIndex) {
  if (static_cast<uint32_t>(ifIndex) == active_index_ &&
      !air_children_.empty()) {
    return;
  }

  RemoveAllNodes();
  active_index_ = ifIndex;
}

uint32_t AirIfElement::NonVirtualNodeCountInParent() {
  uint32_t sum = 0;
  for (auto node : air_children_) {
    sum += node->NonVirtualNodeCountInParent();
  }
  return sum;
}

}  // namespace tasm
}  // namespace lynx
