// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_BUILTIN_FUNCTION_TABLE_H_
#define CORE_RUNTIME_LEPUS_BUILTIN_FUNCTION_TABLE_H_

#include <functional>
#include <utility>

#include "base/include/vector.h"
#include "core/runtime/lepus/restricted_value.h"

namespace lynx {
namespace lepus {

class VMContext;

class BuiltinFunctionTable {
 public:
  enum Type : uint8_t {
    ArrayPrototype,
    DatePrototype,
    NumberPrototype,
    RegexPrototype,
    StringPrototype,
    Console,
    Date,
    Json,
    LepusDate,
    Math,
    Object,
    String,
  };

  template <std::size_t N>
  BuiltinFunctionTable(
      Type type, const std::pair<const char*, CFunctionBuiltin> (&list)[N]) {
    static_assert(N <= 256,
                  "BuiltinFunctionTable supports registering functions max "
                  "count of 256.");
    map_.reserve(N);
    array_.reserve(N);
    for (size_t i = 0; i < N; i++) {
      map_[list[i].first] = i;
      array_.emplace_back(list[i].second);
    }
    // lower uint8_t of 1 as engaged flag, higher 8 bits is type.
    engaged_type_ = static_cast<uint16_t>(1 | (type << 8));
  }

  const RestrictedValue& GetFunction(const base::String& key) {
    base::RefCountedStringImpl* ref =
        base::String::Unsafe::GetUntaggedStringRawRef(key);
    if (ref->__get_padding_shorts__()[0] == engaged_type_) {
      return array_[ref->__get_padding_chars__()[2]];
    }

    // map search.
    auto it = map_.find(key);
    if (it == map_.end()) {
      static RestrictedValue kEmpty;
      return kEmpty;
    } else {
      if (!ref->__get_padding_chars__()[0]) {
        // padding not engaged, initialize as function cache of specific type
        ref->__get_padding_shorts__()[0] = engaged_type_;
        ref->__get_padding_chars__()[2] = static_cast<uint8_t>(it->second);
      }
      return array_[it->second];
    }
  }

 private:
  base::LinearFlatMap<base::String, size_t> map_;
  base::Vector<RestrictedValue> array_;
  uint16_t engaged_type_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BUILTIN_FUNCTION_TABLE_H_
