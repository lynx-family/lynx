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

#if OS_WIN || OS_MAC
namespace {
bool GetLynxValueString(lynx_api_env env, lynx_value val, std::string& str) {
  size_t length = 0;
  if (lynx_api_ok ==
      lynx_value_get_string_utf8(env, val, nullptr, 0, &length)) {
    str.resize(length);
    return lynx_api_ok ==
           lynx_value_get_string_utf8(env, val, &str[0], length + 1, &length);
  }
  return false;
}
}  // namespace

clay::Value ValueConverter::CreateClayValue(lynx_api_env env, lynx_value val) {
  switch (val.type) {
    case lynx_value_null:
      return clay::Value::Null();
    case lynx_value_undefined:
      return clay::Value();
    case lynx_value_bool:
      return clay::Value(val.val_bool);
    case lynx_value_double:
      return clay::Value(val.val_double);
    case lynx_value_int32:
      return clay::Value(val.val_int32);
    case lynx_value_uint32:
      return clay::Value(val.val_uint32);
    case lynx_value_int64:
      return clay::Value(val.val_int64);
    case lynx_value_uint64:
      return clay::Value(int64_t(val.val_uint64));
    case lynx_value_string: {
      std::string str;
      if (GetLynxValueString(env, val, str)) {
        return clay::Value(std::move(str));
      }
      break;
    }
    case lynx_value_array: {
      clay::Value::Array array;
      uint32_t length;
      lynx_value_get_array_length(env, val, &length);
      for (uint32_t i = 0; i < length; i++) {
        lynx_value element;
        lynx_value_get_element(env, val, i, &element);
        clay::Value copy = lynx::ValueConverter::CreateClayValue(env, element);
        lynx_value_remove_reference(env, element, nullptr);
        array.push_back(std::move(copy));
      }
      return clay::Value(std::move(array));
    }
    case lynx_value_map: {
      clay::Value::Map map;
      lynx_value val_names;
      lynx_value_get_property_names(env, val, &val_names);
      uint32_t length;
      lynx_value_get_array_length(env, val_names, &length);
      for (uint32_t i = 0; i < length; i++) {
        std::string name;
        lynx_value val_name;
        lynx_value_get_element(env, val_names, i, &val_name);
        GetLynxValueString(env, val_name, name);
        lynx_value_remove_reference(env, val_name, nullptr);

        lynx_value property;
        lynx_value_get_named_property(env, val, name.c_str(), &property);
        clay::Value copy = lynx::ValueConverter::CreateClayValue(env, property);
        lynx_value_remove_reference(env, property, nullptr);

        map[name] = std::move(copy);
      }
      lynx_value_remove_reference(env, val_names, nullptr);
      return clay::Value(std::move(map));
    }
    default:
      break;
  }

  FML_LOG(ERROR) << "unsupported lynx_value type: " << val.type;
  return clay::Value();
}

lynx_value ValueConverter::CreateLynxValue(lynx_api_env env,
                                           const clay::Value& val) {
  switch (val.type()) {
    case clay::Value::kNone:
      return {.val_ptr = 0, .type = lynx_value_undefined};
    case clay::Value::kPointer:
      return {.val_ptr = 0, .type = lynx_value_null};
    case clay::Value::kBool:
      return {.val_bool = val.GetBool(), .type = lynx_value_bool};
    case clay::Value::kInt:
      return {.val_int32 = val.GetInt(), .type = lynx_value_int32};
    case clay::Value::kUInt:
      return {.val_uint32 = val.GetUint(), .type = lynx_value_uint32};
    case clay::Value::kLong:
      return {.val_int64 = val.GetLong(), .type = lynx_value_int64};
    case clay::Value::kFloat:
      return {.val_double = val.GetFloat(), .type = lynx_value_double};
    case clay::Value::kDouble:
      return {.val_double = val.GetDouble(), .type = lynx_value_double};
    case clay::Value::kString: {
      lynx_value result;
      const auto& str = val.GetString();
      lynx_value_create_string_utf8(env, str.c_str(), str.size(), &result);
      return result;
    }
    case clay::Value::kArray:
      return CreateLynxValue(env, val.GetArray());
    case clay::Value::kMap:
      return CreateLynxValue(env, val.GetMap());
    default:
      break;
  }

  FML_LOG(ERROR) << "unsupported clay::Value type: " << val.type();
  return {.val_ptr = 0, .type = lynx_value_undefined};
}

lynx_value ValueConverter::CreateLynxValue(lynx_api_env env,
                                           const clay::Value::Map& map) {
  lynx_value val_map;
  lynx_value_create_map(env, &val_map);
  for (const auto& [key, val] : map) {
    lynx_value val_property = CreateLynxValue(env, val);
    lynx_value_set_named_property(env, val_map, key.c_str(), val_property);
    lynx_value_remove_reference(env, val_property, nullptr);
  }
  return val_map;
}

lynx_value ValueConverter::CreateLynxValue(lynx_api_env env,
                                           const clay::Value::Array& array) {
  lynx_value val_array;
  lynx_value_create_array(env, &val_array);
  uint32_t i = 0;
  for (const auto& element : array) {
    lynx_value val_element = CreateLynxValue(env, element);
    lynx_value_set_element(env, val_array, i++, val_element);
    lynx_value_remove_reference(env, val_element, nullptr);
  }
  return val_array;
}
#endif

}  // namespace lynx
