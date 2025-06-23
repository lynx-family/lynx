// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/harmony/logging_harmony.h"

#include <hilog/log.h>

#include <memory>
#include <string>

#include "base/include/log/alog_wrapper.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace base {
namespace logging {
namespace {

static alog_write_func_ptr s_alog_write = nullptr;
static bool s_use_sys_log = false;

alog_write_func_ptr GetLynxLogWriteFunction() { return s_alog_write; }

const unsigned int LOG_PRINT_DOMAIN = 0xFF00;
void PrintLogMessageByLogDelegate(LogMessage *msg, const char *tag) {
  LogLevel priority = LogLevel::LOG_DEBUG;
  switch (msg->severity()) {
    case logging::LOG_VERBOSE:
    case logging::LOG_DEBUG:
      priority = LogLevel::LOG_DEBUG;
      break;
    case logging::LOG_INFO:
      priority = LogLevel::LOG_INFO;
      break;
    case logging::LOG_WARNING:
      priority = LogLevel::LOG_WARN;
      break;
    case logging::LOG_ERROR:
      priority = LogLevel::LOG_ERROR;
      break;
    case logging::LOG_FATAL:
      priority = LogLevel::LOG_FATAL;
      break;
  }
  OH_LOG_Print(LOG_APP, priority, LOG_PRINT_DOMAIN, tag, "%{public}s",
               msg->stream().c_str());
}

}  // namespace

napi_value LynxLog::Init(napi_env env, napi_value exports) {
  NAPI_CREATE_FUNCTION(env, exports, "nativeInitLynxLogWriteFunction",
                       NativeInitLynxLogWriteFunction);
  NAPI_CREATE_FUNCTION(env, exports, "nativeInitLynxLog", NativeInitLynxLog);
  NAPI_CREATE_FUNCTION(env, exports, "nativeUseSysLog", NativeUseSysLog);
  NAPI_CREATE_FUNCTION(env, exports, "nativeInternalLog", nativeInternalLog);
  NAPI_CREATE_FUNCTION(env, exports, "nativeSetMinLogLevel",
                       nativeSetMinLogLevel);
  return exports;
}

napi_value LynxLog::NativeInitLynxLogWriteFunction(napi_env env,
                                                   napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int64_t log_write_function_address = NapiUtil::ConvertToInt64(env, args[0]);
  if (!HasInitedLynxLogWriteFunction() && log_write_function_address != 0) {
    s_alog_write =
        reinterpret_cast<alog_write_func_ptr>(log_write_function_address);
  }
  return nullptr;
}

napi_value LynxLog::NativeInitLynxLog(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  InitLynxLogging(
      GetLynxLogWriteFunction, PrintLogMessageByLogDelegate,
      tasm::LynxEnv::GetInstance().IsDevToolEnabled() || s_use_sys_log);
  return nullptr;
}

napi_value LynxLog::NativeUseSysLog(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  s_use_sys_log = NapiUtil::ConvertToBoolean(env, args[0]);
  return nullptr;
}

napi_value LynxLog::nativeInternalLog(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int32_t level = NapiUtil::ConvertToInt32(env, args[0]);
  std::string tag = NapiUtil::ConvertToString(env, args[1]);
  std::string message = NapiUtil::ConvertToString(env, args[2]);
  PrintLogToLynxLogging(static_cast<int>(level), tag.c_str(), message.c_str());
  return nullptr;
}

napi_value LynxLog::nativeSetMinLogLevel(napi_env env,
                                         napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int32_t level = NapiUtil::ConvertToInt32(env, args[0]);
  SetMinLogLevel(static_cast<int>(level));
  return nullptr;
}
}  // namespace logging
}  // namespace base
}  // namespace lynx
