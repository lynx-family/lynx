// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/fiber_node_info.h"

#include <utility>

namespace lynx {
namespace tasm {
lepus::Value FiberNodeInfo::GetNodesInfo(
    const std::vector<FiberElement *> &nodes,
    const std::vector<std::string> &fields) {
  auto ret = lepus::CArray::Create();
  for (auto node : nodes) {
    ret->emplace_back(GetNodeInfo(node, fields));
  }
  return lepus::Value(std::move(ret));
}

lepus::Value FiberNodeInfo::GetNodeInfo(
    FiberElement *node, const std::vector<std::string> &fields) {
  auto ret = lepus::Dictionary::Create();
  if (node == nullptr) {
    return lepus::Value(std::move(ret));
  }

  for (auto &field : fields) {
    if (field == "id") {
      BASE_STATIC_STRING_DECL(kId, "id");
      ret->SetValue(kId, node->GetIdSelector());
    } else if (field == "dataset" || field == "dataSet") {
      auto dataset_value = lepus::Dictionary::Create();
      for (const auto &[key, value] : node->dataset()) {
        dataset_value->SetValue(key, value);
      }
      ret->SetValue(field, std::move(dataset_value));
    } else if (field == "tag") {
      BASE_STATIC_STRING_DECL(kTag, "tag");
      ret->SetValue(kTag, node->GetTag());
    } else if (field == "unique_id") {
      BASE_STATIC_STRING_DECL(kUniqueId, "unique_id");
      ret->SetValue(kUniqueId, node->impl_id());
    } else if (field == "name") {
      BASE_STATIC_STRING_DECL(kName, "name");
      auto iter = node->data_model()->attributes().find(kName);
      if (iter == node->data_model()->attributes().end()) {
        ret->SetValue(kName, base::String());
      } else {
        ret->SetValue(kName, iter->second);
      }
    } else if (field == "index") {
      auto index =
          node->parent()
              ? static_cast<FiberElement *>(node->parent())->IndexOf(node)
              : 0;
      BASE_STATIC_STRING_DECL(kIndex, "index");
      ret->SetValue(kIndex, index);
    } else if (field == "class") {
      auto classes_value = lepus::CArray::Create();
      for (const auto &v : node->classes()) {
        classes_value->emplace_back(v);
      }
      BASE_STATIC_STRING_DECL(kClass, "class");
      ret->SetValue(kClass, std::move(classes_value));
    } else if (field == "attribute") {
      auto attributes_value = lepus::Dictionary::Create();

      for (const auto &[key, value_pair] : node->data_model()->attributes()) {
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

std::vector<FiberElement *> FiberNodeInfo::PathToRoot(FiberElement *base) {
  std::vector<FiberElement *> path;
  while (base) {
    path.push_back(base);
    base = static_cast<FiberElement *>(base->parent());
  }
  return path;
}
}  // namespace tasm
}  // namespace lynx
