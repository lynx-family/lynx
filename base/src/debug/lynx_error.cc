// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/debug/lynx_error.h"

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"

#if !defined(OS_WIN)
#include "base/include/debug/backtrace.h"
#endif

#include <cstdarg>

namespace lynx {

namespace base {

static constexpr const char* kLynxErrorKeyPrefixContext = "lynx_context_";

// some commonly used key
static constexpr const char* kLynxErrorKeyErrorStack = "error_stack";

namespace {

std::string AddBackTrace(std::string& error_message) {
#if OS_IOS
  return lynx::base::debug::GetBacktraceInfo(error_message);
#else
  return error_message;
#endif
}

}  // namespace

bool StoreError(int32_t error_code, std::string error_msg,
                std::string fix_suggestion, LynxErrorLevel level) {
  LynxError error{error_code, std::move(error_msg), std::move(fix_suggestion),
                  level};
  return ErrorStorage::GetInstance().SetError(std::move(error));
}

bool StoreErrorIfNot(bool expression, int32_t error_code, std::string error_msg,
                     std::string fix_suggestion, base::LynxErrorLevel level) {
  if (!expression) {
    LynxError error{error_code, std::move(error_msg), std::move(fix_suggestion),
                    level};
    return ErrorStorage::GetInstance().SetError(std::move(error));
  }
  return false;
}

LynxError::LynxError(int error_code, const char* format, ...)
    : error_code_(error_code), is_logbox_only_(false) {
  va_list args;
  va_start(args, format);
  error_message_ = FormatStringWithVaList(format, args);
  va_end(args);
  error_message_ = AddBackTrace(error_message_);
  error_level_ = LynxErrorLevel::Error;
  LOGI("LynxError occurs error_code:" << error_code
                                      << " error_message:" << error_message_);
}

LynxError::LynxError(int error_code, std::string error_msg,
                     std::string fix_suggestion, LynxErrorLevel level,
                     bool is_logbox_only)
    : error_level_(level),
      error_code_(error_code),
      error_message_(std::move(error_msg)),
      fix_suggestion_(std::move(fix_suggestion)),
      is_logbox_only_(is_logbox_only) {
  LOGI("LynxError occurs error_code:" << error_code_
                                      << " error_message:" << error_message_);
}

void LynxError::AddCallStack(const std::string& stack) {
  custom_info_[kLynxErrorKeyErrorStack] = stack;
}

void LynxError::AddContextInfo(const std::string& key,
                               const std::string& value) {
  std::string context_key(kLynxErrorKeyPrefixContext);
  context_key.append(key);
  custom_info_[context_key] = value;
}

std::string LynxError::GetLevelString(int32_t level) {
  LynxErrorLevel l_enum = LynxErrorLevel::Error;
  if (level >= static_cast<int32_t>(LynxErrorLevel::Fatal) &&
      level <= static_cast<int32_t>(LynxErrorLevel::Warn)) {
    l_enum = static_cast<LynxErrorLevel>(level);
  }
  switch (l_enum) {
    case LynxErrorLevel::Fatal:
      return "fatal";
    case LynxErrorLevel::Warn:
      return "warn";
    case LynxErrorLevel::Error:
    default:
      return "error";
  }
}

ErrorStorage& ErrorStorage::GetInstance() {
#if defined(OS_IOS) && defined(__i386__)
  // constructor is private, so must pass here
  static ThreadLocal<ErrorStorage> instance([] { return new ErrorStorage; });
#else
  static thread_local ErrorStorage instance;
#endif
  return instance;
}

void ErrorStorage::AddCustomInfoToError(
    const std::unordered_map<std::string, std::string>& custom_info) {
  if (!error_) {
    return;
  }
  for (const auto& [key, value] : custom_info) {
    error_->custom_info_[key] = value;
  }
}

void ErrorStorage::AddCustomInfoToError(const std::string& key,
                                        const std::string& value) {
  if (error_) {
    error_->custom_info_[key] = value;
  }
}

}  // namespace base
}  // namespace lynx
