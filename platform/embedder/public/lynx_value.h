// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_VALUE_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_VALUE_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "lynx_value_api.h"
namespace lynx {
namespace pub {

class LynxValue;
using LynxValueIterator =
    std::function<void(const LynxValue&, const LynxValue&)>;
using LynxValueIteratorCallback =
    std::function<void(lynx_api_env, const lynx_value&, const lynx_value&)>;

class LynxValue {
 public:
  ~LynxValue() { lynx_value_remove_reference(env_, value_, nullptr); }

  LynxValue(const LynxValue& val) {
    if (this == &val) return;
    lynx_value_add_reference(env_, val.value_, nullptr);
    value_ = val.value_;
  }

  LynxValue(LynxValue&& val) noexcept {
    value_ = val.value_;
    val.value_.type = lynx_value_null;
  }

  LynxValue& operator=(const LynxValue& val) {
    if (this != &val) {
      lynx_value_add_reference(env_, val.value_, nullptr);
      lynx_value_remove_reference(env_, value_, nullptr);
      value_ = val.value_;
    }
    return *this;
  }

  LynxValue& operator=(LynxValue&& val) noexcept {
    if (this != &val) {
      this->~LynxValue();
      new (this) LynxValue(std::move(val));
    }
    return *this;
  }

  explicit LynxValue(bool val) { lynx_value_create_bool(env_, val, &value_); }

  explicit LynxValue(int32_t val) {
    lynx_value_create_int32(env_, val, &value_);
  }

  explicit LynxValue(int64_t val) {
    lynx_value_create_int64(env_, val, &value_);
  }

  explicit LynxValue(uint32_t val) {
    lynx_value_create_uint32(env_, val, &value_);
  }

  explicit LynxValue(uint64_t val) {
    lynx_value_create_uint64(env_, val, &value_);
  }

  explicit LynxValue(double val) {
    lynx_value_create_double(env_, val, &value_);
  }

  explicit LynxValue(const char* str) {
    lynx_value_create_string_utf8(
        env_, str, str == nullptr ? 0 : std::strlen(str), &value_);
  }

  explicit LynxValue(const std::string& str) {
    lynx_value_create_string_utf8(env_, str.c_str(), str.size(), &value_);
  }

  enum CreateAsNullTag {
    kCreateAsNullTag,
  };
  explicit LynxValue(CreateAsNullTag) { lynx_value_create_null(env_, &value_); }

  enum CreateAsArrayTag {
    kCreateAsArrayTag,
  };
  explicit LynxValue(CreateAsArrayTag) {
    lynx_value_create_array(env_, &value_);
  }

  enum CreateAsMapTag {
    kCreateAsMapTag,
  };
  explicit LynxValue(CreateAsMapTag) { lynx_value_create_map(env_, &value_); }

  explicit LynxValue(const lynx_value& val) : value_(val) {
    lynx_value_add_reference(env_, value_, nullptr);
  }

  explicit LynxValue(lynx_value&& val) : value_(std::move(val)) {}

  lynx_value_type Type() const {
    lynx_value_type type;
    lynx_value_typeof(env_, value_, &type);
    return type;
  }

  lynx_value Value() const { return value_; }

  bool Bool() const {
    bool ret;
    lynx_api_status status = lynx_value_get_bool(env_, value_, &ret);
    if (status != lynx_api_ok) return false;
    return ret;
  }

  double Number() const {
    double ret;
    lynx_api_status status = lynx_value_get_number(env_, value_, &ret);
    if (status != lynx_api_ok) return 0.f;
    return ret;
  }

  double Double() const {
    double ret;
    lynx_api_status status = lynx_value_get_double(env_, value_, &ret);
    if (status != lynx_api_ok) return 0.f;
    return ret;
  }

  int32_t Int32() const {
    int32_t ret;
    lynx_api_status status = lynx_value_get_int32(env_, value_, &ret);
    if (status != lynx_api_ok) return 0;
    return ret;
  }

  uint32_t UInt32() const {
    uint32_t ret;
    lynx_api_status status = lynx_value_get_uint32(env_, value_, &ret);
    if (status != lynx_api_ok) return 0;
    return ret;
  }

  int64_t Int64() const {
    int64_t ret;
    lynx_api_status status = lynx_value_get_int64(env_, value_, &ret);
    if (status != lynx_api_ok) return 0;
    return ret;
  }

  uint64_t UInt64() const {
    uint64_t ret;
    lynx_api_status status = lynx_value_get_uint64(env_, value_, &ret);
    if (status != lynx_api_ok) return 0;
    return ret;
  }

  std::string StdString() const {
    size_t str_size;
    lynx_api_status status =
        lynx_value_get_string_utf8(env_, value_, nullptr, 0, &str_size);
    if (status != lynx_api_ok) return "";
    auto buf = std::make_unique<char[]>(str_size + 1);
    status = lynx_value_get_string_utf8(env_, value_, buf.get(), str_size + 1,
                                        &str_size);
    if (status != lynx_api_ok) return "";
    return std::string(buf.get(), str_size);
  }

  uint32_t ArrayLength() const {
    uint32_t ret;
    lynx_value_get_array_length(env_, value_, &ret);
    return ret;
  }

  bool InsertValue(uint32_t idx, const LynxValue& val) {
    lynx_api_status status =
        lynx_value_set_element(env_, value_, idx, val.value_);
    if (status != lynx_api_ok) return false;
    return true;
  }

  LynxValue GetValueAt(uint32_t idx) const {
    lynx_value ret;
    lynx_api_status status = lynx_value_get_element(env_, value_, idx, &ret);
    if (status != lynx_api_ok) return LynxValue(kCreateAsNullTag);
    return LynxValue(std::move(ret));
  }

  bool HasProperty(const char* key) const {
    bool ret;
    lynx_api_status status = lynx_value_has_property(env_, value_, key, &ret);
    if (status != lynx_api_ok) return false;
    return ret;
  }

  bool SetProperty(const char* key, const LynxValue& val) {
    lynx_api_status status =
        lynx_value_set_named_property(env_, value_, key, val.value_);
    if (status != lynx_api_ok) return false;
    return true;
  }

  LynxValue GetProperty(const char* key) const {
    lynx_value ret;
    lynx_api_status status =
        lynx_value_get_named_property(env_, value_, key, &ret);
    if (status != lynx_api_ok) return LynxValue(kCreateAsNullTag);
    return LynxValue(std::move(ret));
  }

  void IteratorValue(const LynxValueIterator& callback) const {
    LynxValueIteratorCallback callback_wrap =
        [&callback](lynx_api_env env, const lynx_value& key,
                    const lynx_value& value) {
          LynxValue keyWrap(key);
          lynx_value v = value;
          LynxValue valueWrap(std::move(v));
          callback(keyWrap, valueWrap);
        };
    lynx_value_iterate_value(env_, value_, InternalLynxValueIteratorCallback,
                             reinterpret_cast<void*>(&callback_wrap), nullptr);
  }

  static void ForEachLynxValue(const LynxValue& val, LynxValueIterator func) {
    lynx_api_env env = val.env_;
    lynx_value property_names;
    lynx_api_status status =
        lynx_value_get_property_names(env, val.value_, &property_names);
    if (status != lynx_api_ok) return;
    uint32_t len;
    lynx_value_get_array_length(env, property_names, &len);
    for (uint32_t i = 0; i < len; i++) {
      lynx_value element;
      lynx_value_get_element(env, property_names, i, &element);
      auto key = LynxValue(std::move(element));
      lynx_value property;
      lynx_value_get_named_property(env, val.value_, key.StdString().c_str(),
                                    &property);
      auto value = LynxValue(std::move(property));
      func(key, value);
    }
    lynx_value_remove_reference(env, property_names, nullptr);
  }

 private:
  static inline void InternalLynxValueIteratorCallback(lynx_api_env env,
                                                       lynx_value key,
                                                       lynx_value value,
                                                       void* pfunc,
                                                       void* raw_data) {
    reinterpret_cast<LynxValueIteratorCallback*>(pfunc)->operator()(env, key,
                                                                    value);
  }

  lynx_value value_;
  lynx_api_env env_ = nullptr;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_VALUE_H_
