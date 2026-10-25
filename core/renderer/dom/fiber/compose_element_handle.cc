// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_element_handle.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

namespace {

constexpr char kOverlayTag[] = "overlay";
constexpr char kSvgTag[] = "svg";
constexpr char kAnimaXTag[] = "animax-view";

fml::RefPtr<Element> CreateContentElement(ElementManager* manager,
                                          ComposeElementKind kind,
                                          const base::String& tag,
                                          TemplateAssembler* tasm) {
  switch (kind) {
    case ComposeElementKind::kView:
      return manager->CreateFiberView();
    case ComposeElementKind::kText:
      return manager->CreateFiberText(BASE_STATIC_STRING(kElementTextTag));
    case ComposeElementKind::kImage:
      return manager->CreateFiberImage(BASE_STATIC_STRING(kElementImageTag));
    case ComposeElementKind::kList:
      return manager->CreateFiberList(tasm, BASE_STATIC_STRING(kElementListTag),
                                      {}, {}, {});
    case ComposeElementKind::kSvg:
      return manager->CreateFiberElement(BASE_STATIC_STRING(kSvgTag));
    case ComposeElementKind::kOverlay:
      return manager->CreateFiberElement(BASE_STATIC_STRING(kOverlayTag));
    case ComposeElementKind::kAnimaX:
      return manager->CreateFiberElement(BASE_STATIC_STRING(kAnimaXTag));
    case ComposeElementKind::kCustom:
      return manager->CreateFiberElement(tag);
  }
  return nullptr;
}

}  // namespace

ComposeElementHandle::ComposeElementHandle(ElementManager* manager,
                                           ComposeElementKind kind,
                                           const base::String& tag,
                                           TemplateAssembler* tasm)
    : content_element_(CreateContentElement(manager, kind, tag, tasm)),
      mount_root_(content_element_) {}

}  // namespace tasm
}  // namespace lynx
