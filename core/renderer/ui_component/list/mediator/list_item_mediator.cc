// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/mediator/list_item_mediator.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

ListItemMediator::ListItemMediator(Element* list_item_element)
    : list_item_element_(list_item_element) {}

int32_t ListItemMediator::GetImplId() const {
  return list_item_element_->impl_id();
}

std::string ListItemMediator::GetIdSelector() const {
  AttributeHolder* data_model = list_item_element_->data_model();
  return data_model ? data_model->idSelector().str() : "";
}

float ListItemMediator::GetWidth() const { return list_item_element_->width(); }

float ListItemMediator::GetHeight() const {
  return list_item_element_->height();
}

float ListItemMediator::GetLeft() const { return list_item_element_->left(); }

float ListItemMediator::GetTop() const { return list_item_element_->top(); }

const std::array<float, 4>& ListItemMediator::GetPaddings() const {
  return list_item_element_->paddings();
}

const std::array<float, 4>& ListItemMediator::GetMargins() const {
  return list_item_element_->margins();
}

const std::array<float, 4>& ListItemMediator::GetBorders() const {
  return list_item_element_->borders();
}

void ListItemMediator::UpdateLayoutToPlatform(float left, float top) {
  list_item_element_->UpdateLayout(left, top);
  list_item_element_->element_container()->UpdateLayout(left, top);
}

bool ListItemMediator::HasBoundEvent(const std::string& event_name) const {
  return list_item_element_->event_map().contains(event_name) ||
         list_item_element_->lepus_event_map().contains(event_name);
}

void ListItemMediator::SendCustomEvent(const std::string& event_name,
                                       const std::string& param_name,
                                       std::unique_ptr<pub::Value> param) {
  if (list_item_element_->element_manager() && param &&
      param->backend_type() == pub::ValueBackendType::ValueBackendTypeLepus) {
    list_item_element_->element_manager()->SendNativeCustomEvent(
        event_name, list_item_element_->impl_id(),
        pub::ValueUtils::ConvertValueToLepusValue(*param), param_name);
  }
}

void ListItemMediator::OnListItemWillAppear(const std::string& item_key) {
  list_item_element_->element_container()->ListCellWillAppear(item_key);
}

void ListItemMediator::OnListItemDisappear(bool is_exist,
                                           const std::string& item_key) {
  list_item_element_->element_container()->ListCellDisappear(is_exist,
                                                             item_key);
}

void ListItemMediator::FlushPatching() {
  list_item_element_->element_container()->UpdateLayoutPatching();
  list_item_element_->OnNodeReady();
  list_item_element_->element_container()->UpdateNodeReadyPatching();
  list_item_element_->element_container()->FlushImmediately();
}

void ListItemMediator::FlushAnimatedStyle(CSSPropertyID id, CSSValue value) {
  list_item_element_->FlushAnimatedStyle(id, value);
  list_item_element_->OnNodeReady();
  list_item_element_->element_container()->UpdateNodeReadyPatching();
  list_item_element_->element_container()->FlushImmediately();
}

}  // namespace tasm
}  // namespace lynx
