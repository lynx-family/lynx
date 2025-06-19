// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/value/table.h"

#include "base/include/log/logging.h"
#include "base/include/value/lepus_value.h"

namespace lynx {
namespace lepus {

Dictionary::Dictionary(HashMap map) : hash_map_(std::move(map)) {}

bool Dictionary::Contains(const base::String& key) const {
  return hash_map_.find(key) != hash_map_.end();
}

bool Dictionary::Erase(const base::String& key) {
  if (IsConstLog()) {
    return false;
  }
  hash_map_.erase(key);
  return true;
}

int32_t Dictionary::EraseKey(const base::String& key) {
  if (IsConstLog()) {
    return -1;
  }
  return static_cast<int32_t>(hash_map_.erase(key));
}

Dictionary::ValueWrapper Dictionary::GetValue(const base::String& key) const {
  auto iter = hash_map_.find(key);
  if (iter != hash_map_.end()) {
    return ValueWrapper(&iter->second);
  } else {
    static Value kNil;
    return ValueWrapper(&kNil);
  }
}

Dictionary::ValueWrapper Dictionary::GetValueOrUndefined(
    const base::String& key) const {
  auto iter = hash_map_.find(key);
  if (iter != hash_map_.end()) {
    return ValueWrapper(&iter->second);
  } else {
    static Value kUndefined(Value::kCreateAsUndefinedTag);
    return ValueWrapper(&kUndefined);
  }
}

Dictionary::ValueWrapper Dictionary::GetValueOrNull(
    const base::String& key) const {
  auto iter = hash_map_.find(key);
  if (iter != hash_map_.end()) {
    return ValueWrapper(&iter->second);
  } else {
    return ValueWrapper(nullptr);
  }
}

Dictionary::ValueWrapper Dictionary::GetValueOrInsert(const base::String& key) {
  if (IsConstLog()) {
    return ValueWrapper(nullptr);
  } else {
    return ValueWrapper(&hash_map_[key]);
  }
}

Dictionary::ValueWrapper Dictionary::GetValueOrInsert(base::String&& key) {
  if (IsConstLog()) {
    return ValueWrapper(nullptr);
  } else {
    return ValueWrapper(&hash_map_[std::move(key)]);
  }
}

void Dictionary::Dump() {
  LOGE("begin dump dict----------");
  auto it = begin();
  for (; it != end(); it++) {
    lepus::Value value = it->second;
    if (value.IsNumber()) {
      LOGE(it->first.str() << " : " << value.Number());
    }

    else if (value.IsString()) {
      LOGE(it->first.str() << " : " << value.StdString());
    } else if (value.IsTable()) {
      LOGE(it->first.str() << " : ===>");
      value.Table()->Dump();
    } else if (value.IsBool()) {
      LOGE(it->first.str() << " : "
                           << ((value.Bool() == true) ? "true" : "false"));
    } else if (value.IsArray()) {
      LOGE(it->first.str() << " : []");
    } else {
      LOGE(it->first.str() << " : type is " << value.Type());
    }
  }
  LOGE("end dump dict----------");
}

bool operator==(const Dictionary& left, const Dictionary& right) {
  return left.hash_map_ == right.hash_map_;
}

}  // namespace lepus
}  // namespace lynx
