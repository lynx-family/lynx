// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/table.h"

#include "base/include/log/logging.h"
#include "core/runtime/vm/lepus/lepus_value.h"

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

const Value& Dictionary::GetValue(const base::String& key, bool forUndef) {
  HashMap::iterator iter = hash_map_.find(key);
  if (iter != hash_map_.end()) {
    return iter->second;
  }

  if (forUndef) {
    static Value kUndefined;
    kUndefined.SetUndefined();
    return kUndefined;
  } else {
    static Value kEmpty;
    kEmpty.SetNil();
    return kEmpty;
  }
}

Value* Dictionary::At(const base::String& key) {
  if (IsConstLog()) {
    return nullptr;
  } else {
    return &hash_map_[key];
  }
}

Value* Dictionary::At(base::String&& key) {
  if (IsConstLog()) {
    return nullptr;
  } else {
    return &hash_map_[std::move(key)];
  }
}

void Dictionary::dump() {
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
      value.Table()->dump();
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

void Dictionary::ReleaseSelf() const { delete this; }
}  // namespace lepus
}  // namespace lynx
