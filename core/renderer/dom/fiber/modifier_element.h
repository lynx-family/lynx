// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_MODIFIER_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_MODIFIER_ELEMENT_H_

#include "core/renderer/dom/element.h"

namespace lynx {
namespace tasm {

class ElementManager;

// A physical box boundary introduced by a Compose LayoutModifier. It uses the
// ordinary Element behavior and participates in the normal Fiber child tree.
class ModifierElement : public Element {
 public:
  explicit ModifierElement(ElementManager* manager);
  ~ModifierElement() override = default;

  bool is_modifier() const override { return true; }

  fml::RefPtr<Element> CloneElement(bool clone_resolved_props) const override {
    return fml::AdoptRef<Element>(
        new ModifierElement(*this, clone_resolved_props));
  }

 private:
  ModifierElement(const ModifierElement& element, bool clone_resolved_props)
      : Element(element, clone_resolved_props) {}
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_MODIFIER_ELEMENT_H_
