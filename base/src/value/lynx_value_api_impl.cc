// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>

#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/lynx_value_api.h"
#include "base/include/value/table.h"

lynx_api_status lynx_value_typeof(lynx_api_env env, lynx_value value,
                                  lynx_value_type* result) {
  *result = value.type;
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_null(lynx_api_env env, lynx_value* result) {
  (*result).type = lynx_value_null;
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_bool(lynx_api_env env, bool value,
                                       lynx_value* result) {
  *result = {.val_bool = value, .type = lynx_value_bool};
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_double(lynx_api_env env, double value,
                                         lynx_value* result) {
  *result = {.val_double = value, .type = lynx_value_double};
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_int32(lynx_api_env env, int32_t value,
                                        lynx_value* result) {
  *result = {.val_int32 = value, .type = lynx_value_int32};
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_uint32(lynx_api_env env, uint32_t value,
                                         lynx_value* result) {
  *result = {.val_uint32 = value, .type = lynx_value_uint32};
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_int64(lynx_api_env env, int64_t value,
                                        lynx_value* result) {
  *result = {.val_int64 = value, .type = lynx_value_int64};
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_uint64(lynx_api_env env, uint64_t value,
                                         lynx_value* result) {
  *result = {.val_uint64 = value, .type = lynx_value_uint64};
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_string_utf8(lynx_api_env env, const char* str,
                                              size_t length,
                                              lynx_value* result) {
  *result = {.val_ptr = reinterpret_cast<lynx_value_ptr>(
                 lynx::base::RefCountedStringImpl::Unsafe::RawCreate(str)),
             .type = lynx_value_string};
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_map(lynx_api_env env, lynx_value* result) {
  *result = {.val_ptr = reinterpret_cast<lynx_value_ptr>(
                 lynx::lepus::Dictionary::Unsafe::RawCreate()),
             .type = lynx_value_map};
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_array(lynx_api_env env, lynx_value* result) {
  *result = {.val_ptr = reinterpret_cast<lynx_value_ptr>(
                 lynx::lepus::CArray::Unsafe::RawCreate()),
             .type = lynx_value_array};
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_number(lynx_api_env env, lynx_value value,
                                      double* result) {
  switch (value.type) {
    case lynx_value_double:
      *result = value.val_double;
      break;
    case lynx_value_int32:
      *result = value.val_int32;
      break;
    case lynx_value_uint32:
      *result = value.val_uint32;
      break;
    case lynx_value_int64:
      *result = value.val_int64;
      break;
    case lynx_value_uint64:
      *result = value.val_uint64;
      break;
    case lynx_value_bool:
      *result = value.val_bool;
      break;
    case lynx_value_string: {
      auto* base_string =
          reinterpret_cast<lynx::base::RefCountedStringImpl*>(value.val_ptr);
      if (base_string) {
        auto* str = base_string->c_str();
        *result = strtod(str, nullptr);
      }
    } break;
    default:
      *result = 0;
      break;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_double(lynx_api_env env, lynx_value value,
                                      double* result) {
  if (unlikely(value.type != lynx_value_double)) {
    return lynx_api_double_expected;
  }
  *result = value.val_double;

  return lynx_api_ok;
}

lynx_api_status lynx_value_get_int32(lynx_api_env env, lynx_value value,
                                     int32_t* result) {
  if (unlikely(value.type != lynx_value_int32)) {
    return lynx_api_int32_expected;
  }
  *result = value.val_int32;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_uint32(lynx_api_env env, lynx_value value,
                                      uint32_t* result) {
  if (unlikely(value.type != lynx_value_uint32)) {
    return lynx_api_uint32_expected;
  }
  *result = value.val_uint32;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_int64(lynx_api_env env, lynx_value value,
                                     int64_t* result) {
  if (unlikely(value.type != lynx_value_int64)) {
    return lynx_api_int64_expected;
  }
  *result = value.val_int64;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_uint64(lynx_api_env env, lynx_value value,
                                      uint64_t* result) {
  if (unlikely(value.type != lynx_value_uint64)) {
    return lynx_api_uint64_expected;
  }
  *result = value.val_uint64;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_bool(lynx_api_env env, lynx_value value,
                                    bool* result) {
  if (unlikely(value.type != lynx_value_bool)) {
    return lynx_api_bool_expected;
  }
  *result = value.val_bool;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_string_utf8(lynx_api_env env, lynx_value value,
                                           char* buf, size_t bufsize,
                                           size_t* result) {
  if (unlikely(value.type != lynx_value_string || value.val_ptr == nullptr)) {
    *result = 0;
    return lynx_api_string_expected;
  }
  auto* base_string =
      reinterpret_cast<lynx::base::RefCountedStringImpl*>(value.val_ptr);
  if (buf == nullptr) {
    *result = base_string->length();
  } else {
    auto* str = base_string->c_str();
    size_t size{std::min(base_string->length(), bufsize - 1)};
    std::copy(str, str + size, buf);
    buf[size] = '\0';
    if (result != nullptr) {
      *result = size;
    }
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_array_length(lynx_api_env env, lynx_value value,
                                            uint32_t* result) {
  if (unlikely(value.type != lynx_value_array || value.val_ptr == nullptr)) {
    *result = 0;
    return lynx_api_array_expected;
  }
  *result = static_cast<uint32_t>(
      reinterpret_cast<lynx::lepus::CArray*>(value.val_ptr)->size());
  return lynx_api_ok;
}

lynx_api_status lynx_value_set_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, lynx_value value) {
  if (unlikely(object.type != lynx_value_array || object.val_ptr == nullptr)) {
    return lynx_api_array_expected;
  }
  auto* array = reinterpret_cast<lynx::lepus::CArray*>(object.val_ptr);
  array->set(index, lynx::lepus::Value(env, value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, lynx_value* result) {
  if (unlikely(object.type != lynx_value_array || object.val_ptr == nullptr)) {
    (*result).type = lynx_value_null;
    return lynx_api_array_expected;
  }
  auto* array = reinterpret_cast<lynx::lepus::CArray*>(object.val_ptr);
  array->get(index).DupValue();
  *result = array->get(index).value();
  return lynx_api_ok;
}

lynx_api_status lynx_value_has_property(lynx_api_env env, lynx_value object,
                                        const char* utf8name, bool* result) {
  if (unlikely(object.type != lynx_value_map || object.val_ptr == nullptr)) {
    return lynx_api_map_expected;
  }
  auto* map = reinterpret_cast<lynx::lepus::Dictionary*>(object.val_ptr);
  auto key = lynx::base::String(utf8name);
  *result = map->Contains(key);
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_property_names(lynx_api_env env,
                                              lynx_value object,
                                              lynx_value* result) {
  if (unlikely(object.type != lynx_value_map || object.val_ptr == nullptr)) {
    return lynx_api_map_expected;
  }
  auto* array = lynx::lepus::CArray::Unsafe::RawCreate();
  auto* map = reinterpret_cast<lynx::lepus::Dictionary*>(object.val_ptr);
  map->for_each([&](const auto& key, lynx::lepus::Value& value) {
    array->emplace_back(key);
  });
  *result = {.val_ptr = reinterpret_cast<lynx_value_ptr>(array),
             .type = lynx_value_array};
  return lynx_api_ok;
}

lynx_api_status lynx_value_set_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              lynx_value value) {
  if (unlikely(object.type != lynx_value_map || object.val_ptr == nullptr)) {
    return lynx_api_map_expected;
  }
  auto* map = reinterpret_cast<lynx::lepus::Dictionary*>(object.val_ptr);
  map->SetValue(utf8name, lynx::lepus::Value(env, value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              lynx_value* result) {
  if (unlikely(object.type != lynx_value_map || object.val_ptr == nullptr)) {
    (*result).type = lynx_value_null;
    return lynx_api_invalid_arg;
  }
  auto* map = reinterpret_cast<lynx::lepus::Dictionary*>(object.val_ptr);
  auto key = lynx::base::String(utf8name);
  map->GetValue(key).value().DupValue();
  *result = map->GetValue(key).value().value();
  return lynx_api_ok;
}

lynx_api_status lynx_value_iterate_value(lynx_api_env env, lynx_value object,
                                         lynx_value_iterator_callback callback,
                                         void* pfunc, void* raw_data) {
  if (object.val_ptr == nullptr) {
    return lynx_api_invalid_arg;
  }
  if (object.type == lynx_value_map) {
    auto* map = reinterpret_cast<lynx::lepus::Dictionary*>(object.val_ptr);
    map->for_each([&](const auto& key, lynx::lepus::Value& value) {
      auto* ptr = lynx::base::String::Unsafe::GetStringRawRef(key);
      lynx_value k;
      k.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
      k.type = lynx_value_string;
      value.DupValue();
      callback(env, k, value.value(), pfunc, raw_data);
    });
  } else if (object.type == lynx_value_array) {
    auto* array = reinterpret_cast<lynx::lepus::CArray*>(object.val_ptr);
    for (std::size_t i = 0; i < array->size(); ++i) {
      lynx_value k{.val_uint32 = static_cast<uint32_t>(i),
                   .type = lynx_value_uint32};
      array->get(i).DupValue();
      callback(env, k, array->get(i).value(), pfunc, raw_data);
    }
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_add_reference(lynx_api_env env, lynx_value value,
                                         lynx_value_ref* result) {
  if (value.type >= lynx_value_string && value.type <= lynx_value_object &&
      value.val_ptr != nullptr) {
    reinterpret_cast<lynx::fml::RefCountedThreadSafeStorage*>(value.val_ptr)
        ->AddRef();
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_remove_reference(lynx_api_env env, lynx_value value,
                                            lynx_value_ref ref) {
  if (value.type >= lynx_value_string && value.type <= lynx_value_object &&
      value.val_ptr != nullptr) {
    reinterpret_cast<lynx::fml::RefCountedThreadSafeStorage*>(value.val_ptr)
        ->Release();
  }
  return lynx_api_ok;
}
