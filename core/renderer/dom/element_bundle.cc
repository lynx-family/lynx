// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_bundle.h"

#include <stack>
#include <utility>

#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

ElementBundle ElementBundle::DeepClone() const {
  ElementBundle bundle;
  if (!IsValid()) {
    return bundle;
  }
  auto new_page = TreeResolver::CloneElementRecursively(
      static_cast<FiberElement*>(page_node_.RefCounted().get()), true);
  bundle.page_node_ = lepus::Value(std::move(new_page));
  return bundle;
}

}  // namespace tasm
}  // namespace lynx
