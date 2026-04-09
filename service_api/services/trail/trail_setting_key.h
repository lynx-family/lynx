// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SERVICE_API_SERVICES_TRAIL_TRAIL_SETTING_KEY_H_
#define SERVICE_API_SERVICES_TRAIL_TRAIL_SETTING_KEY_H_

#include <service_api/service_api_utils.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>

namespace lynx {
namespace service {
namespace trail_service {

class LynxTrailService;

template <typename ValueType>
struct AlwaysFalse : std::false_type {};

inline bool ConvertToBool(const std::string& value) {
  if (value == "1") {
    return true;
  }
  if (value == "0") {
    return false;
  }

  static const std::string true_result = "true";
  return value.size() == true_result.size() &&
         std::equal(value.begin(), value.end(), true_result.begin(),
                    [](char a, char b) {
                      return std::tolower(static_cast<unsigned char>(a)) ==
                             std::tolower(static_cast<unsigned char>(b));
                    });
}

inline long ConvertToLong(const std::optional<std::string>& string_result,
                          long default_value) {
  if (!string_result.has_value() || string_result->empty()) {
    return default_value;
  }

  char* end_tag = nullptr;
  long result = std::strtol(string_result->c_str(), &end_tag, 10);
  if (*end_tag != '\0') {
    return default_value;
  }
  return result;
}

template <typename ValueType, bool HasDefault = false>
struct SettingValueTraits {
  using return_type = ValueType;

  static return_type GetDefaultValue() { return return_type{}; }

  static return_type ToReturnValue(const ValueType& value) { return value; }

  static return_type FromStringValue(const std::optional<std::string>&,
                                     const return_type&) {
    static_assert(AlwaysFalse<ValueType>::value,
                  "Unsupported trail setting type");
  }
};

template <bool HasDefault>
struct SettingValueTraits<bool, HasDefault> {
  using return_type = bool;

  static return_type GetDefaultValue() { return false; }

  static return_type ToReturnValue(bool value) { return value; }

  static return_type FromStringValue(
      const std::optional<std::string>& string_result,
      return_type default_value) {
    if (!string_result.has_value() || string_result->empty()) {
      return default_value;
    }
    return ConvertToBool(*string_result);
  }
};

template <bool HasDefault>
struct SettingValueTraits<long, HasDefault> {
  using return_type = long;

  static return_type GetDefaultValue() { return 0; }

  static return_type ToReturnValue(long value) { return value; }

  static return_type FromStringValue(
      const std::optional<std::string>& string_result,
      return_type default_value) {
    return ConvertToLong(string_result, default_value);
  }
};

template <>
struct SettingValueTraits<std::string, false> {
  using return_type = std::optional<std::string>;

  static return_type GetDefaultValue() { return std::nullopt; }

  static return_type ToReturnValue(const std::string& value) { return value; }

  static return_type ToReturnValue(const char* value) {
    if (value == nullptr) {
      return std::nullopt;
    }
    return std::string(value);
  }

  static return_type ToReturnValue(const return_type& value) { return value; }

  static return_type FromStringValue(const std::optional<std::string>& value,
                                     const return_type& default_value) {
    return value.has_value() ? value : default_value;
  }
};

template <>
struct SettingValueTraits<std::string, true> {
  using return_type = std::string;

  static return_type GetDefaultValue() { return ""; }

  static return_type ToReturnValue(const std::string& value) { return value; }

  static return_type ToReturnValue(const char* value) {
    return value == nullptr ? "" : value;
  }

  static return_type FromStringValue(const std::optional<std::string>& value,
                                     const return_type& default_value) {
    return value.has_value() ? *value : default_value;
  }
};

// Shared metadata for typed setting keys. The string key is stable, while the
// numeric key id is allocated lazily and used by TrailService implementations
// as a process-local cache key.
class SettingKeyBase {
 public:
  SettingKeyBase(const SettingKeyBase&) = default;
  SettingKeyBase(SettingKeyBase&&) = delete;
  SettingKeyBase& operator=(const SettingKeyBase&) = delete;
  SettingKeyBase& operator=(SettingKeyBase&&) = delete;

  const std::string& get_string_key() const {
    EnsureKeyIdInitialized();
    return key_str_;
  }

  uint64_t get_key_id() const { return EnsureKeyIdInitialized(); }

 protected:
  explicit SettingKeyBase(const std::string& key_str)
      : key_str_(key_str), key_id_state_(std::make_shared<KeyIdState>()) {}

 private:
  struct KeyIdState {
    std::once_flag init_flag;
    uint64_t key_id = 0;
  };

  static uint64_t GetNextKeyId() {
    static std::atomic<uint64_t> next_key_id{0};
    return next_key_id.fetch_add(1, std::memory_order_relaxed);
  }

  uint64_t EnsureKeyIdInitialized() const {
    std::call_once(key_id_state_->init_flag, [state = key_id_state_]() {
      state->key_id = SettingKeyBase::GetNextKeyId();
    });
    return key_id_state_->key_id;
  }

  const std::string key_str_;
  const std::shared_ptr<KeyIdState> key_id_state_;
};

// A typed trail setting key. The template parameter determines which
// LynxTrailService::get_value overload is selected and what value type is
// returned to callers.
template <typename ValueType, bool HasDefault = false>
class SettingKey : public SettingKeyBase {
 public:
  using value_type = ValueType;
  using return_type =
      typename SettingValueTraits<ValueType, HasDefault>::return_type;

  explicit SettingKey(const std::string& key_str)
      : SettingKeyBase(key_str),
        default_value_(
            SettingValueTraits<ValueType, HasDefault>::GetDefaultValue()) {}

  template <typename DefaultValueType>
  SettingKey(const std::string& key_str, const DefaultValueType& default_value)
      : SettingKeyBase(key_str),
        default_value_(SettingValueTraits<ValueType, HasDefault>::ToReturnValue(
            default_value)) {}

  // A shortcut for lynx::service::get_service<LynxTrailService>().get_value();
  return_type value() const;

 private:
  friend class LynxTrailService;
  const return_type& default_value() const { return default_value_; }
  const return_type default_value_;
};

}  // namespace trail_service
}  // namespace service
}  // namespace lynx

// Predefined typed setting keys exposed by TrailService.
#define _LYNX_SETTING_KEY_SELECTOR(_1, _2, _3, _4, NAME, ...) NAME

// Define this macro in exactly one translation unit before including this
// header to emit the SettingKey definitions. Other translation units only see
// extern declarations.
#ifdef LYNX_TRAIL_SETTING_KEYS_IMPLEMENTATION
#define _LYNX_SETTING_KEY_WITHOUT_DEFAULT(var, value_type, key_str)   \
  EXPORT_VAR extern const ::lynx::service::trail_service::SettingKey< \
      value_type, false>                                              \
  var(key_str)
#define _LYNX_SETTING_KEY_WITH_DEFAULT(var, value_type, key_str,      \
                                       default_value)                 \
  EXPORT_VAR extern const ::lynx::service::trail_service::SettingKey< \
      value_type, true>                                               \
  var(key_str, default_value)
#else
#define _LYNX_SETTING_KEY_WITHOUT_DEFAULT(var, value_type, key_str) \
  extern const ::lynx::service::trail_service::SettingKey<value_type, false> var
#define _LYNX_SETTING_KEY_WITH_DEFAULT(var, value_type, key_str, \
                                       default_value)            \
  extern const ::lynx::service::trail_service::SettingKey<value_type, true> var
#endif

// Declares or defines a typed trail setting key.
//
// Three-argument form declares a key without a default value:
//   LYNX_SETTING_KEY(RENDERER_TYPE, std::string, "renderer_type");
//
// Four-argument form declares a key with a default value:
//   LYNX_SETTING_KEY(ENABLE_CLAY_TRACE, bool, "enable_clay_trace", true);
#define LYNX_SETTING_KEY(...)                                             \
  _LYNX_SETTING_KEY_SELECTOR(__VA_ARGS__, _LYNX_SETTING_KEY_WITH_DEFAULT, \
                             _LYNX_SETTING_KEY_WITHOUT_DEFAULT)           \
  (__VA_ARGS__)

#endif  // SERVICE_API_SERVICES_TRAIL_TRAIL_SETTING_KEY_H_
