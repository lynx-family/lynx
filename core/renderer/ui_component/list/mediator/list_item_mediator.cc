// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/mediator/list_item_mediator.h"

#include "core/renderer/dom/element.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

ListItemMediator::ListItemMediator(Element* list_item_element)
    : list_item_element_(list_item_element) {}

int32_t ListItemMediator::GetImplId() const { return -1; }

std::string ListItemMediator::GetIdSelector() const { return ""; }

float ListItemMediator::GetWidth() const { return 0.f; }

float ListItemMediator::GetHeight() const { return 0.f; }

const std::array<float, 4>& ListItemMediator::GetPaddings() const {
  return list_item_element_->paddings();
}

const std::array<float, 4>& ListItemMediator::GetMargins() const {
  return list_item_element_->margins();
}

const std::array<float, 4>& ListItemMediator::GetBorders() const {
  return list_item_element_->borders();
}

void ListItemMediator::UpdateLayoutToPlatform(float left, float top) {}

bool ListItemMediator::HasBoundEvent(const std::string& event_name) const {
  return false;
}

void ListItemMediator::SendCustomEvent(const std::string& event_name,
                                       const std::string& param_name,
                                       std::unique_ptr<pub::Value> param) {}

}  // namespace tasm
}  // namespace lynx
