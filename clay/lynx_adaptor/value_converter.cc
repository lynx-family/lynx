// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/value_converter.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/lynx_adaptor/base_def.h"

namespace lynx {

clay::Value ValueConverter::CreateClayValue(
    const pub::Value& source,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  if (source.IsNil()) {
    return clay::Value::Null();
  } else if (source.IsString()) {
    const auto& s = source.str();
    return clay::Value(s);
  } else if (source.IsInt32()) {
    return clay::Value{source.Int32()};
  } else if (source.IsInt64()) {
    return clay::Value{source.Int64()};
  } else if (source.IsUInt32()) {
    return clay::Value{source.UInt32()};
  } else if (source.IsUInt64()) {
    return clay::Value{static_cast<int64_t>(source.UInt64())};
  } else if (source.IsNumber()) {
    return clay::Value{source.Number()};
  } else if (source.IsArrayBuffer()) {
    size_t length = source.Length();
    uint8_t* buffer = source.ArrayBuffer();
    clay::Value::ArrayBuffer ab(length);
    memcpy(ab.data(), buffer, length);
    return clay::Value{std::move(ab)};
  } else if (source.IsArray()) {
    pub::ScopedCircleChecker scoped_circle_checker;
    if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector,
                                                       source, depth)) {
      clay::Value::Array array;
      source.ForeachArray([&](int64_t i, const pub::Value& v) {
        array.push_back(CreateClayValue(v, prev_value_vector, depth + 1));
      });
      return clay::Value(std::move(array));
    }
  } else if (source.IsMap()) {
    if (source.backend_type() == pub::ValueBackendType::ValueBackendTypePiper &&
        source.Contains(lynx::BIG_INT_VAL)) {
      auto val_long = source.GetValueForKey(lynx::BIG_INT_VAL);
      if (val_long) {
        auto val_str = val_long->str();
        if (!val_str.empty()) {
          auto v = std::stoll(val_str.c_str(), nullptr, 0);
          return clay::Value{static_cast<int64_t>(v)};
        }
      }
      return clay::Value{};
    }
    pub::ScopedCircleChecker scoped_circle_checker;
    if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector,
                                                       source, depth)) {
      clay::Value::Map map;
      source.ForeachMap([&](const pub::Value& k, const pub::Value& v) {
        auto rk_value = CreateClayValue(v, prev_value_vector, depth + 1);
        map.emplace(k.str(), std::move(rk_value));
      });
      return clay::Value{std::move(map)};
    }
  } else if (source.IsBool()) {
    return clay::Value{source.Bool()};
  } else if (source.IsUndefined()) {
    return clay::Value{};
  } else {
    FML_DLOG(ERROR) << "CreateClayValue unknown type " << source.Type();
  }
  return clay::Value::Null();
}

clay::Value ValueConverter::CreateClayValue(const uint8_t* data, size_t size) {
  clay::Value::Array array;
  for (size_t i = 0; i < size; ++i) {
    array.push_back(clay::Value{data[i]});
  }
  return clay::Value(std::move(array));
}

clay::Value ValueConverter::CreateClayValue(const uint32_t* data, size_t size) {
  clay::Value::Array array;
  for (size_t i = 0; i < size; ++i) {
    array.push_back(clay::Value{data[i]});
  }
  return clay::Value(std::move(array));
}

}  // namespace lynx
