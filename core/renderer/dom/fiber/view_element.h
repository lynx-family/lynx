// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_VIEW_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_VIEW_ELEMENT_H_

#include <memory>

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class ElementManager;

class ViewElement : public FiberElement {
 public:
  ViewElement(ElementManager* manager);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new ViewElement(*this, clone_resolved_props));
  }

  bool is_view() const override { return true; }

  void ConvertToInlineElement() override;

  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

 protected:
  void OnNodeAdded(FiberElement* child) override;

  ViewElement(const ViewElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {
    MarkCanBeLayoutOnly(true);
  }
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_VIEW_ELEMENT_H_
