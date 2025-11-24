// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/view_element.h"

#include <memory>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/view_fragment_behavior.h"

namespace lynx {
namespace tasm {

ViewElement::ViewElement(ElementManager* manager)
    : FiberElement(manager, BASE_STATIC_STRING(kElementViewTag)) {
  MarkCanBeLayoutOnly(true);
  if (element_manager_ == nullptr) {
    return;
  }
  SetDefaultOverflow(element_manager_->GetDefaultOverflowVisible());
  element_manager_->IncreaseViewElementCount();
}

void ViewElement::ConvertToInlineElement() { MarkAsInline(); }

void ViewElement::OnNodeAdded(FiberElement* child) {
  UpdateRenderRootElementIfNecessary(child);
}

void ViewElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  SetDefaultOverflow(element_manager_->GetDefaultOverflowVisible());
}

void ViewElement::SetupFragmentBehavior(Fragment* fragment) {
  fragment->SetBehavior(std::make_unique<ViewFragmentBehavior>(fragment));
}

}  // namespace tasm
}  // namespace lynx
