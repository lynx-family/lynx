// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_DEBUG_LYNX_ERROR_H_
#define BASE_INCLUDE_DEBUG_LYNX_ERROR_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/base_export.h"

// Store the error into ErrorStorage, so that it can be retrieved and reported
// from ErrorStorage after a processing is completed. e.g. report error when the
// destructor of class Scope is called.
#define LYNX_WARN(code, msg, suggestion)        \
  lynx::base::StoreError(code, msg, suggestion, \
                         lynx::base::LynxErrorLevel::Warn)
#define LYNX_ERROR(code, msg, suggestion)       \
  lynx::base::StoreError(code, msg, suggestion, \
                         lynx::base::LynxErrorLevel::Error)

// Store the error into ErrorStorage if expression is not satisfied.
#define LYNX_WARN_CHECK(expression, code, msg, suggestion)       \
  lynx::base::StoreErrorIfNot(expression, code, msg, suggestion, \
                              lynx::base::LynxErrorLevel::Warn)
#define LYNX_ERROR_CHECK(expression, code, msg, suggestion)      \
  lynx::base::StoreErrorIfNot(expression, code, msg, suggestion, \
                              lynx::base::LynxErrorLevel::Error)

namespace lynx {
namespace base {

/** Some commonly used suggestions */
// Suggestion for errors with a complex cause that require a detailed
// explanation on the official site.
static constexpr const char* kLynxErrorSuggestionRefOfficialSite =
    "Please refer to the solution in Doc 'LynxError FAQ' on the official "
    "website.";

enum class LynxErrorLevel : int32_t { Fatal = 0, Error, Warn };

// Store the error into ErrorStorage
bool StoreError(int32_t error_code, std::string error_msg,
                std::string fix_suggestion,
                LynxErrorLevel level = LynxErrorLevel::Error);
// Store the error into ErrorStorage if expression is not satisfied.
bool StoreErrorIfNot(bool expression, int32_t error_code, std::string error_msg,
                     std::string fix_suggestion,
                     LynxErrorLevel level = LynxErrorLevel::Error);

struct LynxError {
  BASE_EXPORT LynxError(int error_code, const char* format, ...);
  BASE_EXPORT LynxError(int error_code, std::string error_msg,
                        std::string fix_suggestion, LynxErrorLevel level,
                        bool is_logbox_only = false);

  LynxError(int error_code, std::string error_message)
      : LynxError(error_code, error_message, "", LynxErrorLevel::Error) {}

  BASE_EXPORT LynxError(LynxError&& other) = default;
  BASE_EXPORT LynxError& operator=(LynxError&& other) = default;

  LynxError(const LynxError& other) = delete;
  LynxError& operator=(const LynxError& other) = delete;

  BASE_EXPORT void AddCallStack(const std::string& stack);
  void AddContextInfo(const std::string& key, const std::string& value);

  static std::string GetLevelString(int32_t level);

  // required fields
  LynxErrorLevel error_level_;
  int error_code_;
  std::string error_message_;
  // optional fields
  std::string fix_suggestion_;
  // custom fields
  std::unordered_map<std::string, std::string> custom_info_;

  // Indicates whether the error only needs to be displayed
  // using LogBox and does not require reporting.
  bool is_logbox_only_;
  // Indicates whether the error should be abort
  bool should_abort_ = false;
};

class ErrorStorage {
 public:
  BASE_EXPORT static ErrorStorage& GetInstance();

  template <typename... T>
  bool SetError(T&&... args) {
    if (error_ == nullptr) {
      error_ = std::make_unique<LynxError>(std::forward<T>(args)...);
      return true;
    }
    return false;
  }

  void Reset() { error_ = nullptr; }

  const std::unique_ptr<LynxError>& GetError() const { return error_; }

  void AddCustomInfoToError(
      const std::unordered_map<std::string, std::string>& custom_info);

  void AddCustomInfoToError(const std::string& key, const std::string& value);

 private:
  ErrorStorage() = default;

  std::unique_ptr<LynxError> error_;
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_DEBUG_LYNX_ERROR_H_
