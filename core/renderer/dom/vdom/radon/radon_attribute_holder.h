// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_ATTRIBUTE_HOLDER_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_ATTRIBUTE_HOLDER_H_

#include "core/renderer/dom/attribute_holder.h"

namespace lynx {
namespace tasm {

class RadonNode;

class RadonAttributeHolder : public AttributeHolder {
 public:
  RadonAttributeHolder() = default;
  RadonAttributeHolder(const RadonAttributeHolder& holder)
      : AttributeHolder(holder) {}

  void OnStyleChange() override;
  void OnPseudoStateChanged(PseudoState, PseudoState) override;

  css::StyleNode* HolderParent() const override;
  css::StyleNode* NextSibling() const override;
  css::StyleNode* PreviousSibling() const override;

  CSSFragment* ParentStyleSheet() const override;

  bool GetRemoveCSSScopeEnabled() const override;
  bool GetCascadePseudoEnabled() const override;
  bool GetRemoveDescendantSelectorScope() const override;
  bool IsComponent() const override;

  RadonNode* radon_node_ptr() const override { return radon_node_ptr_; }
  void set_radon_node_ptr(RadonNode* radon_node_ptr) {
    radon_node_ptr_ = radon_node_ptr;
  }

 protected:
  RadonNode* radon_node_ptr_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_ATTRIBUTE_HOLDER_H_
