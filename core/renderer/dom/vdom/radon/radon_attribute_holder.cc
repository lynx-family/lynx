// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_attribute_holder.h"

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"

namespace lynx {
namespace tasm {

void RadonAttributeHolder::OnStyleChange() {
  if (radon_node_ptr_) {
    radon_node_ptr_->OnStyleChange();
  }
}

void RadonAttributeHolder::OnPseudoStateChanged(PseudoState prev,
                                                PseudoState curr) {
  if (radon_node_ptr_) {
    radon_node_ptr_->OnPseudoStateChanged(prev, curr);
  }
}

css::StyleNode* RadonAttributeHolder::HolderParent() const {
  if (radon_node_ptr_) {
    return radon_node_ptr_->HolderParent();
  } else {
    return AttributeHolder::HolderParent();
  }
}

css::StyleNode* RadonAttributeHolder::NextSibling() const {
  if (radon_node_ptr_) {
    return radon_node_ptr_->NextSibling();
  } else {
    return AttributeHolder::NextSibling();
  }
}

css::StyleNode* RadonAttributeHolder::PreviousSibling() const {
  if (radon_node_ptr_) {
    return radon_node_ptr_->PreviousSibling();
  } else {
    return AttributeHolder::PreviousSibling();
  }
}

CSSFragment* RadonAttributeHolder::ParentStyleSheet() const {
  if (radon_node_ptr_) {
    return radon_node_ptr_->ParentStyleSheet();
  }
  return nullptr;
}

bool RadonAttributeHolder::GetRemoveCSSScopeEnabled() const {
  if (radon_node_ptr_) {
    radon_node_ptr_->GetRemoveCSSScopeEnabled();
  }
  return false;
}

bool RadonAttributeHolder::GetCascadePseudoEnabled() const {
  if (radon_node_ptr_) {
    radon_node_ptr_->GetCascadePseudoEnabled();
  }
  return false;
}

bool RadonAttributeHolder::GetRemoveDescendantSelectorScope() const {
  if (radon_node_ptr_) {
    radon_node_ptr_->GetRemoveDescendantSelectorScope();
  }
  return true;
}

bool RadonAttributeHolder::IsComponent() const {
  if (radon_node_ptr_) {
    radon_node_ptr_->IsComponent();
  }
  return false;
}

}  // namespace tasm
}  // namespace lynx
