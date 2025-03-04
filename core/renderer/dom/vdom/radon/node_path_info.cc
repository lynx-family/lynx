// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/node_path_info.h"

#include <utility>

#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {

lepus::Value RadonPathInfo::GetNodesInfo(
    const std::vector<RadonNode *> &nodes) {
  auto ret = lepus::CArray::Create();
  for (auto node : nodes) {
    ret->emplace_back(
        GetNodeInfo(node, {"tag", "id", "dataSet", "index", "class"}));
  }
  return lepus::Value(std::move(ret));
}

lepus::Value RadonPathInfo::GetNodeInfo(
    RadonNode *node, const std::vector<std::string> &fields) {
  auto ret = lepus::Dictionary::Create();
  if (node == nullptr) {
    return lepus::Value(std::move(ret));
  }

  for (auto &field : fields) {
    if (field == "id") {
      BASE_STATIC_STRING_DECL(kId, "id");
      ret->SetValue(kId, node->id_selector());
    } else if (field == "dataset" || field == "dataSet") {
      auto dataset_value = lepus::Dictionary::Create();
      for (const auto &[key, value] : node->data_set()) {
        dataset_value->SetValue(key, value);
      }
      ret->SetValue(field, std::move(dataset_value));
    } else if (field == "tag") {
      BASE_STATIC_STRING_DECL(kTag, "tag");
      ret->SetValue(kTag, node->tag());
    } else if (field == "unique_id") {
      BASE_STATIC_STRING_DECL(kUniqueId, "unique_id");
      ret->SetValue(kUniqueId, node->ImplId());
    } else if (field == "name") {
      BASE_STATIC_STRING_DECL(kName, "name");
      auto iter = node->attributes().find(kName);
      if (iter == node->attributes().end()) {
        ret->SetValue(kName, base::String());
      } else {
        ret->SetValue(kName, iter->second);
      }
    } else if (field == "index") {
      BASE_STATIC_STRING_DECL(kIndex, "index");
      ret->SetValue(kIndex, node->IndexInSiblings());
    } else if (field == "class") {
      auto classes_value = lepus::CArray::Create();
      for (const auto &v : node->classes()) {
        classes_value->emplace_back(v);
      }
      BASE_STATIC_STRING_DECL(kClass, "class");
      ret->SetValue(kClass, std::move(classes_value));
    } else if (field == "attribute") {
      auto attributes_value = lepus::Dictionary::Create();

      for (const auto &[key, value_pair] : node->attributes()) {
        // Id is not returned here. It can be acquired by option "id".
        // Classes, styles, dataset & attributes having value of function type,
        // undefined or null are not returned either.
        if (key == AttributeHolder::kIdSelectorAttrName ||
            value_pair.IsJSFunction() || value_pair.IsNil() ||
            value_pair.IsUndefined()) {
          continue;
        }
        attributes_value->SetValue(key, value_pair);
      }
      BASE_STATIC_STRING_DECL(kAttribute, "attribute");
      ret->SetValue(kAttribute, std::move(attributes_value));
    }
  }
  return lepus::Value(std::move(ret));
}

std::vector<RadonNode *> RadonPathInfo::PathToRoot(RadonBase *base) {
  std::vector<RadonNode *> path;
  while (base) {
    if (base->IsRadonNode()) {
      path.push_back(static_cast<RadonNode *>(base));
    }
    base = base->Parent();
  }
  return path;
}

}  // namespace tasm
}  // namespace lynx
