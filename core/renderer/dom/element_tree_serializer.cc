// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_tree_serializer.h"

#include <utility>

#include "core/renderer/dom/element.h"
#include "core/runtime/lepus/json_parser.h"

namespace lynx {
namespace tasm {

lepus::Value ElementTreeSerializer::ToLepusValue(Element* root) {
  if (root == nullptr) {
    return lepus::Value();
  }

  auto result = lepus::Dictionary::Create();
  // TODO(songshourui.null): Include layout position fields relative to
  // LynxView and screen when the cross-platform field contract is finalized.
  BASE_STATIC_STRING_DECL(kSign, "sign");
  BASE_STATIC_STRING_DECL(kTagName, "tagName");
  BASE_STATIC_STRING_DECL(kId, "id");
  BASE_STATIC_STRING_DECL(kNodeIndex, "nodeIndex");
  BASE_STATIC_STRING_DECL(kChildren, "children");
  result->SetValue(kSign, root->impl_id());
  result->SetValue(kTagName, root->GetTag());
  result->SetValue(kId, root->GetIdSelector());
  result->SetValue(kNodeIndex, root->NodeIndex());

  auto children = lepus::CArray::Create();
  for (const auto& child : root->children()) {
    children->emplace_back(ToLepusValue(child.get()));
  }
  result->SetValue(kChildren, std::move(children));
  return lepus::Value(std::move(result));
}

std::string ElementTreeSerializer::ToJSONString(Element* root) {
  if (root == nullptr) {
    return "";
  }
  return lepus::lepusValueToJSONString(ToLepusValue(root), true);
}

}  // namespace tasm
}  // namespace lynx
