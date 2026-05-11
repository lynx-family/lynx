// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/media_query/media_query_set.h"

#include <sstream>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace css {

std::string MediaQuerySet::Serialize() const {
  if (queries_.empty()) return std::string();

  std::ostringstream os;
  bool first = true;
  for (const auto& query : queries_) {
    if (!query) continue;
    if (!first) os << ", ";
    os << query->Serialize();
    first = false;
  }
  return os.str();
}

lepus_value MediaQuerySet::ToLepus() const {
  auto arr = lepus::CArray::Create();
  for (const auto& query : queries_) {
    if (!query) continue;
    arr->emplace_back(query->ToLepus());
  }
  return lepus_value(std::move(arr));
}

// static
fml::RefPtr<const MediaQuerySet> MediaQuerySet::FromLepus(
    const lepus_value& value) {
  if (!value.IsArray()) return nullptr;
  const auto& arr = value.Array();
  std::vector<fml::RefPtr<const MediaQuery>> queries;
  queries.reserve(arr->size());
  for (size_t i = 0; i < arr->size(); ++i) {
    auto query = MediaQuery::FromLepus(arr->get(i));
    if (!query) continue;
    queries.push_back(std::move(query));
  }
  return fml::MakeRefCounted<MediaQuerySet>(std::move(queries));
}

}  // namespace css
}  // namespace lynx
