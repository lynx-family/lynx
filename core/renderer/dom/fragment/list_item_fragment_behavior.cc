// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/list_item_fragment_behavior.h"

#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx {
namespace tasm {

namespace {

bool IsInCUIList(Element* element) {
  for (Element* parent = element ? element->parent() : nullptr; parent;
       parent = parent->parent()) {
    if (parent->is_list()) {
      return parent->DisableListPlatformImplementation();
    }
  }
  return false;
}

}  // namespace

ListItemFragmentBehavior::ListItemFragmentBehavior(Fragment* fragment)
    : FragmentBehavior(fragment) {}

void ListItemFragmentBehavior::CreatePlatformRenderer(
    const fml::RefPtr<PropBundle>& attributes) {
  if (painting_context() && fragment()) {
    painting_context()->CreatePlatformRenderer(
        fragment()->id(), PlatformRendererType::kListItem, attributes);
  }
}

void ListItemFragmentBehavior::OnUpdateLayout(const LayoutInfoForDraw&) {
  Element* element = fragment() ? fragment()->element() : nullptr;
  if (IsInCUIList(element)) {
    fragment()->InvalidateForRedraw();
  }
}

}  // namespace tasm
}  // namespace lynx
