// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UTILS_MARKDOWN_LEPUS_VALUE_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UTILS_MARKDOWN_LEPUS_VALUE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "markdown/utils/markdown_value.h"

namespace lynx {
namespace tasm {
namespace harmony {

inline std::unique_ptr<serval::markdown::Value> LepusValueToMarkdownValue(
    const lepus::Value& value) {
  if (value.IsNil() || value.IsUndefined()) {
    return serval::markdown::Value::MakeNull();
  }
  if (value.IsBool()) {
    return serval::markdown::Value::MakeBool(value.Bool());
  }
  if (value.IsNumber()) {
    return serval::markdown::Value::MakeDouble(value.Number());
  }
  if (value.IsString()) {
    std::string content = value.StdString();
    return serval::markdown::Value::MakeString(std::move(content));
  }
  if (value.IsArray()) {
    serval::markdown::ValueArray array;
    auto lepus_array = value.Array();
    array.reserve(lepus_array->size());
    for (size_t i = 0; i < lepus_array->size(); ++i) {
      array.emplace_back(LepusValueToMarkdownValue(lepus_array->get(i)));
    }
    return serval::markdown::Value::MakeArray(std::move(array));
  }
  if (value.IsTable()) {
    serval::markdown::ValueMap map;
    auto lepus_map = value.Table();
    for (const auto& [key, item] : *lepus_map) {
      map.emplace(key.str(), LepusValueToMarkdownValue(item));
    }
    return serval::markdown::Value::MakeMap(std::move(map));
  }
  return serval::markdown::Value::MakeNull();
}

inline serval::markdown::ValueMap LepusValueToMarkdownValueMap(
    const lepus::Value& value) {
  auto markdown_value = LepusValueToMarkdownValue(value);
  if (markdown_value &&
      markdown_value->GetType() == serval::markdown::ValueType::kMap) {
    return std::move(markdown_value->AsMap());
  }
  return {};
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UTILS_MARKDOWN_LEPUS_VALUE_H_
