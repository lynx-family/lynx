// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/wrapper_element.h"

#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace tasm {

constexpr const static char kWrapperElementTag[] = "wrapper";

WrapperElement::WrapperElement(ElementManager* manager, const base::String& tag)
    : FiberElement(manager, tag) {
  is_layout_only_ = true;
}

WrapperElement::WrapperElement(ElementManager* manager)
    : WrapperElement(manager, BASE_STATIC_STRING(kWrapperElementTag)) {}

double WrapperElement::GetFontSize() { return GetParentFontSize(); }

const FiberElement::InheritedProperty& WrapperElement::GetInheritedProperty() {
  return GetParentInheritedProperty();
}

ParallelFlushReturn WrapperElement::PrepareForCreateOrUpdate() {
  // do nothing for Wrapper Element CreateOrUpdate action, only
  // createElementContainer
  if (!has_painting_node_) {
    CreateElementContainer(false);
    has_painting_node_ = true;
  }

  dirty_ &= kDirtyTree;
  this->parallel_flush_ = false;
  return [this]() { this->UpdateResolveStatus(AsyncResolveStatus::kUpdated); };
}

void WrapperElement::MarkDirtyLite(const uint32_t flag) {
  dirty_ |= flag;
  MarkRequireFlush();
  for (const auto& child : children()) {
    child->MarkDirtyLite(flag);
  }
}

void WrapperElement::MarkAsListItem() {
  Element::MarkAsListItem();
  for (auto& child : scoped_children_) {
    child->MarkAsListItem();
  }
}

void WrapperElement::SetWrapperType(Type type) { type_ |= type; }

void WrapperElement::SetAttribute(const base::String& key,
                                  const lepus::Value& value,
                                  bool need_update_data_model) {
  FiberElement::SetAttribute(key, value, need_update_data_model);

  if (type_ & kTouchable) {
    for (auto& child : scoped_children_) {
      child->SetAttribute(key, value, need_update_data_model);
    }
  }
}

void WrapperElement::OnNodeAdded(FiberElement* child) {
  FiberElement::OnNodeAdded(child);
  if (is_list_item()) {
    child->MarkAsListItem();
  }

  if (type_ & kTouchable) {
    for (auto& it : data_model_->attributes()) {
      child->SetAttribute(it.first, it.second);
    }
  }
}

void WrapperElement::OnNodeRemoved(FiberElement* child) {
  FiberElement::OnNodeRemoved(child);

  if (type_ & kTouchable) {
    for (auto& it : data_model_->attributes()) {
      child->SetAttribute(it.first, lepus::Value());
    }
  }
}

}  // namespace tasm
}  // namespace lynx
