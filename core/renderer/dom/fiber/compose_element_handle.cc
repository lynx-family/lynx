// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_element_handle.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

namespace {

fml::RefPtr<Element> CreateContentElement(ElementManager* manager,
                                          ComposeElementKind kind) {
  switch (kind) {
    case ComposeElementKind::kView:
      return manager->CreateFiberView();
    case ComposeElementKind::kText:
      return manager->CreateFiberText(BASE_STATIC_STRING(kElementTextTag));
    case ComposeElementKind::kImage:
      return manager->CreateFiberImage(BASE_STATIC_STRING(kElementImageTag));
  }
  return nullptr;
}

}  // namespace

ComposeElementHandle::ComposeElementHandle(ElementManager* manager,
                                           ComposeElementKind kind)
    : content_element_(CreateContentElement(manager, kind)),
      mount_root_(content_element_) {}

}  // namespace tasm
}  // namespace lynx
