// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_SCROLL_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_SCROLL_ELEMENT_H_

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class ScrollElement : public FiberElement {
 public:
  ScrollElement(ElementManager* manager, const base::String& tag)
      : FiberElement(manager, tag) {
    can_has_layout_only_children_ = false;
  }

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new ScrollElement(*this, clone_resolved_props));
  }

  bool is_scroll_view() const override { return true; }

 protected:
  void OnNodeAdded(FiberElement* child) override;
  void SetAttributeInternal(const base::String& key,
                            const lepus::Value& value) override;

  ScrollElement(const ScrollElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {}

 private:
  void HandleLayoutNodeAttributeUpdate();
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_SCROLL_ELEMENT_H_
