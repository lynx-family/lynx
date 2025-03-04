// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_PAGE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_PAGE_ELEMENT_H_

#include <memory>

#include "core/renderer/dom/fiber/component_element.h"

namespace lynx {
namespace tasm {

class ElementManager;

class PageElement : public ComponentElement {
 public:
  PageElement(ElementManager* manager, const base::String& component_id,
              int32_t css_id);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new PageElement(*this, clone_resolved_props));
  }

  bool is_page() const override { return true; }

  bool IsPageForBaseComponent() const override { return true; }

  virtual void FlushActionsAsRoot() override;

  void PostResolveTaskToThreadPool(
      bool is_engine_thread, ParallelReduceTaskQueue& task_queue) override;

  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

  void SetCSSID(int32_t id) override;

 protected:
  PageElement(const PageElement& element, bool clone_resolved_props);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_PAGE_ELEMENT_H_
