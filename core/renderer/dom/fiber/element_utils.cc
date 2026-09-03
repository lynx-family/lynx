// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/element_utils.h"

#include <string>

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

base::String ConvertTextContent(const lepus::Value& value) {
  auto result = value.String();
  if (!result.empty()) {
    return result;
  }
  if (value.IsInt32()) {
    result = base::String(std::to_string(value.Int32()));
  } else if (value.IsUInt32()) {
    result = base::String(std::to_string(value.UInt32()));
  } else if (value.IsInt64()) {
    result = base::String(std::to_string(value.Int64()));
  } else if (value.IsUInt64()) {
    result = base::String(std::to_string(value.UInt64()));
  } else if (value.IsNumber()) {
    result =
        base::String(base::StringConvertHelper::DoubleToString(value.Number()));
  } else if (value.IsNaN()) {
    BASE_STATIC_STRING_DECL(kNaN, "NaN");
    result = kNaN;
  } else if (value.IsNil()) {
    BASE_STATIC_STRING_DECL(kNull, "null");
    result = kNull;
  } else if (value.IsUndefined()) {
    BASE_STATIC_STRING_DECL(kUndefined, "undefined");
    result = kUndefined;
  }
  return result;
}

}  // namespace tasm
}  // namespace lynx
