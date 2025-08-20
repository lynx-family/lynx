// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/value/table.h"

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace lepus {

bool Dictionary::Erase(const base::String& key) {
  if (IsConstLog()) {
    return false;
  }
  map_.erase(key);
  return true;
}

int32_t Dictionary::EraseKey(const base::String& key) {
  if (IsConstLog()) {
    return -1;
  }
  return static_cast<int32_t>(map_.erase(key));
}

Dictionary::ValueWrapper Dictionary::GetValue(const base::String& key) const {
  const auto* ptr = map_.find(key);
  if (ptr != nullptr) {
    return ValueWrapper(ptr);
  } else {
    static Value kNil;
    return ValueWrapper(&kNil);
  }
}

Dictionary::ValueWrapper Dictionary::GetValueOrUndefined(
    const base::String& key) const {
  const auto* ptr = map_.find(key);
  if (ptr != nullptr) {
    return ValueWrapper(ptr);
  } else {
    static Value kUndefined(Value::kCreateAsUndefinedTag);
    return ValueWrapper(&kUndefined);
  }
}

Dictionary::ValueWrapper Dictionary::GetValueOrNull(
    const base::String& key) const {
  return ValueWrapper(map_.find(key));
}

bool operator==(const Dictionary& left, const Dictionary& right) {
  if (left.size() != right.size()) {
    return false;
  }
  if (left.using_small_map()) {
    for (const auto& [key, value] : left.map_.small_map()) {
      auto it = right.find(key);
      if (it == right.end()) {
        return false;
      }
      if (value != it->second) {
        return false;
      }
    }
  } else {
    for (const auto& [key, value] : left.map_.big_map()) {
      auto it = right.find(key);
      if (it == right.end()) {
        return false;
      }
      if (value != it->second) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace lepus
}  // namespace lynx
