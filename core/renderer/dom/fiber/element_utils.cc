// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/element_utils.h"

#include "core/renderer/dom/fiber/compose_element_handle.h"

namespace lynx {
namespace tasm {

fml::RefPtr<Element> GetComposeContentOrFiberElementFromValue(
    const lepus::Value& value) {
  auto reference = value.RefCounted();
  if (reference->GetRefType() == lepus::RefType::kComposeElementHandle) {
    return fml::static_ref_ptr_cast<ComposeElementHandle>(reference)
        ->content_element();
  }
  return fml::static_ref_ptr_cast<Element>(reference);
}

fml::RefPtr<Element> GetComposeMountRootOrFiberElementFromValue(
    const lepus::Value& value) {
  auto reference = value.RefCounted();
  if (reference->GetRefType() == lepus::RefType::kComposeElementHandle) {
    return fml::static_ref_ptr_cast<ComposeElementHandle>(reference)
        ->mount_root();
  }
  return fml::static_ref_ptr_cast<Element>(reference);
}

}  // namespace tasm
}  // namespace lynx
