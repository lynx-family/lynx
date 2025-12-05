// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_JS_ERROR_REPORTER_H_
#define CORE_RUNTIME_COMMON_JS_ERROR_REPORTER_H_

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "core/build/gen/lynx_sub_error_code.h"

namespace lynx {
namespace common {

struct JSErrorInfo {
  std::string name{"Error"};
  std::string stack;
  std::string message;
  std::string cause;
  std::string build_version;
  std::string version_code;
  std::string kind;

  std::string file_name{"lynx_core"};
  std::string dynamic_component_path;
  // sourcemap release for kernel
  std::string release;
  int32_t error_code = error::E_BTS_RUNTIME_ERROR;
  base::LynxErrorLevel error_level{base::LynxErrorLevel::Error};
};

struct StackFrame {
  long colno{0};
  std::string filename;
  std::string function;
  long lineno{0};
  std::string release;
};

struct StackTrace {
  std::string name;
  std::string message;
  std::vector<StackFrame> frames;
  bool failed{false};
  std::string dynamic_component_path;
};

struct Exception {
  std::string type;
  std::string value;
  StackTrace stack_trace;
};

struct ErrorEvent {
  std::string pid;
  std::string dynamic_component_path;
  std::string url;
  Exception exception;
  std::string level;
  std::string platform;
};

namespace testing {
class TestJSErrorReporter;
}

// append url and query for dynamic component error
void FormatErrorUrl(base::LynxError& error, const std::string& url);

class JSErrorReporter {
 public:
  explicit JSErrorReporter();
  ~JSErrorReporter() = default;

  void SetSourceMapRelease(JSErrorInfo error);
  std::string GetSourceMapRelease(const std::string& url);

  std::optional<base::LynxError> SendMTError(std::string_view original_error,
                                             int error_code, int error_level);

  std::optional<base::LynxError> SendBTError(JSErrorInfo& info);
  void AddCustomInfoToError(
      const std::unordered_map<std::string, std::string>& info);
  void AppendCustomInfo(base::LynxError& error);

 private:
  ErrorEvent FormatError(JSErrorInfo& error_info);
  StackTrace ComputeStackTrace(const JSErrorInfo& error,
                               bool find_file_name_only,
                               std::string* top_file_name = nullptr);
  std::optional<StackFrame> ParseChromiumBasedStack(std::string_view line);
  std::optional<StackFrame> ParseDarwinStack(std::string_view line);
  std::string GetFileNameFromStack(std::string_view line);
  std::string ErrorEventToJsonString(JSErrorInfo& info,
                                     ErrorEvent& error_event);

  base::LynxError ReportException(std::string_view msg, std::string_view stack,
                                  int32_t error_code,
                                  base::LynxErrorLevel error_level,
                                  const std::string& dynamic_component_path);

  std::unordered_map<std::string, std::string> source_maps_;
  std::unordered_map<std::string, std::string> custom_info_;

  friend class lynx::common::testing::TestJSErrorReporter;
};
}  // namespace common
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_JS_ERROR_REPORTER_H_
