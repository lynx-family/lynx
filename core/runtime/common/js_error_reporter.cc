// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/js_error_reporter.h"

#include <optional>
#include <sstream>
#include <string>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/include/vector.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/pointer.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace common {

namespace {
constexpr char kSourceMapReleaseErrorName[] = "LynxGetSourceMapReleaseError";
constexpr char kUnknownFunction[] = "?";
constexpr char kDefaultSourceMapUrl[] = "default";
constexpr char kFlagBacktrace[] = "backtrace:";
constexpr char kFlagTemplateDebug[] = "template_debug_url";
constexpr int kStackTraceLimit = 50;
}  // namespace

void FormatErrorUrl(base::LynxError& error, const std::string& url) {
  if (url.empty()) {
    return;
  }
  // decompose the URL into path and query for error aggregation
  size_t pos = url.find('?');
  error.AddContextInfo("component_url",
                       pos != std::string::npos ? url.substr(0, pos) : url);
  if (pos != std::string::npos) {
    error.AddContextInfo("component_url_query", url.substr(pos + 1));
  }
}

void ParseLineColumnUrl(std::string_view line, StackFrame& stack_frame,
                        std::string::size_type url_start, long column_end) {
  static constexpr char kNativeCode[] = "(native)";
  static constexpr char kAnonymous[] = "(<anonymous>)";
  if (line.find(kNativeCode) != std::string::npos) {
    stack_frame.filename = "native";
  } else if (line.find(kAnonymous) != std::string::npos) {
    stack_frame.filename = "<anonymous>";
  } else {
    // parse line and column
    // find last ':'
    auto column_start = line.rfind(':');
    // find second from the bottom ':'
    auto maybe_line_start = line.rfind(':', column_start - 1);
    std::string::size_type url_end;
    if (maybe_line_start != std::string::npos &&
        column_start != std::string::npos && column_start > maybe_line_start) {
      // substr "45" or "///app-service.js"
      auto maybe_line_str = line.substr(maybe_line_start + 1,
                                        column_start - maybe_line_start - 1);
      // is line number
      if (std::all_of(maybe_line_str.begin(), maybe_line_str.end(),
                      ::isdigit)) {
        stack_frame.lineno = std::atol(maybe_line_str.data());
        // substr '123'
        auto column_str =
            line.substr(column_start + 1, column_end - column_start - 1);
        stack_frame.colno = std::atol(column_str.data());
        url_end = maybe_line_start;
      } else {
        auto line_str =
            line.substr(column_start + 1, column_end - column_start);
        stack_frame.lineno = std::atol(line_str.data());
        url_end = column_start;
      }
    } else {
      // only have line.
      auto line_start = column_start;
      stack_frame.lineno = std::atol(
          line.substr(line_start + 1, column_end - line_start - 1).data());
      url_end = column_start;
    }

    // parse url 'file:///path/to/file.js'
    if (url_start != std::string::npos && url_end != std::string::npos &&
        url_end > url_start) {
      stack_frame.filename =
          line.substr(url_start + 1, url_end - url_start - 1);
    }
  }
}

std::string_view LimitStackString(std::string_view original_stack) {
  size_t pos = 0;
  int line_count = 0;
  for (size_t i = 0; i < original_stack.size(); i++) {
    if (original_stack[i] == '\n') {
      line_count++;
      if (line_count == kStackTraceLimit) {
        pos = i;
        break;
      }
    }
  }
  if (line_count < kStackTraceLimit) {
    return original_stack;
  }
  return original_stack.substr(0, pos);
}

/**
 * @params line maybe
 * call plain method like : "at myFunction (file:///path/to/file.js:123:45)"
 *
 * call native method v8 like: "at JSON.parse (<anonymous>)"
 * call native method quickjs like: "at <input>:0:0 \n at parse (native)"
 *
 * call eval v8 like: "at JSON.parse (<anonymous>)\n at eval (eval at
 * <anonymous> (foo.html:1), <anonymous>:1:6)" call eval quickjs like: "at
 * <input>:0:0\n at parse (native)\n at <eval> (<input>:1:15)"
 */
std::optional<StackFrame> JSErrorReporter::ParseChromiumBasedStack(
    std::string_view line) {
  std::optional<StackFrame> opt_stack_frame;
  static constexpr char kAt[] = "at ";
  static const auto kAtSize = std::strlen(kAt);
  static constexpr char kEvalAt[] = "(eval ";
  auto at_pos = line.find(kAt);
  if (at_pos == std::string::npos) {
    return std::nullopt;
  }
  opt_stack_frame = StackFrame{};
  if (line.find('(') == std::string::npos ||
      line.find(')') == std::string::npos) {
    // like "at foo.js:445"
    opt_stack_frame->function = "<anonymous>";
    ParseLineColumnUrl(line, *opt_stack_frame, line.find(" ", at_pos),
                       line.size());
  } else {
    auto maybe_eval_at_pos = line.find(kEvalAt);
    // at eval (eval at <anonymous> (file:///app-service.js:10),
    // <anonymous>:1:7)
    if (maybe_eval_at_pos != std::string::npos) {
      auto eval_start_pos = line.find(kAt, maybe_eval_at_pos + 1);
      auto eval_end_pos = line.find(")");
      if (eval_end_pos == std::string::npos) {
        return std::nullopt;
      }
      return ParseChromiumBasedStack(
          line.substr(eval_start_pos, eval_end_pos - eval_start_pos + 1));
    }
    // parse func name 'myFunction'
    auto func_end = line.find(' ', at_pos + kAtSize);
    if (func_end == std::string::npos) {
      // format is error.
      return std::nullopt;
    }
    if (at_pos + kAtSize < func_end) {
      opt_stack_frame->function =
          line.substr(at_pos + kAtSize, func_end - at_pos - kAtSize);
    } else {
      opt_stack_frame->function = kUnknownFunction;
    }
    ParseLineColumnUrl(line, *opt_stack_frame, line.find('(', func_end),
                       line.rfind(')'));
  }

  return opt_stack_frame;
}

/**
 * @params line maybe:
 * normal stack like: Foo@file:///bar.js:1:10
 * native stack like: parse@[native code]
 * eval stack like: eval code@ \n eval@[native code]
 *
 */
std::optional<StackFrame> JSErrorReporter::ParseDarwinStack(
    std::string_view line) {
  std::optional<StackFrame> opt_stack_frame;
  static constexpr char kAt = '@';
  static constexpr char kNativeCode[] = "[native code]";
  const auto trim_str =
      base::TrimString(line, " ", base::TrimPositions::TRIM_LEADING);
  auto at_pos = trim_str.find(kAt);
  if (at_pos == std::string::npos ||
      trim_str.find("eval code@") != std::string::npos) {
    return std::nullopt;
  }
  opt_stack_frame = StackFrame{};
  opt_stack_frame->function = trim_str.substr(0, at_pos);
  if (opt_stack_frame->function.empty()) {
    opt_stack_frame->function = kUnknownFunction;
  }
  if (trim_str.find(kNativeCode, at_pos) != std::string::npos) {
    opt_stack_frame->filename = kNativeCode;
  } else {
    ParseLineColumnUrl(trim_str, *opt_stack_frame, at_pos, trim_str.size());
  }

  return opt_stack_frame;
}

StackTrace JSErrorReporter::ComputeStackTrace(const JSErrorInfo& error,
                                              bool find_file_name_only,
                                              std::string* top_file_name) {
  StackTrace stack_trace;
  std::vector<StackFrame> frames;
  if (!error.stack.empty()) {
    base::SplitString(
        error.stack, '\n', false,
        [this, &frames, &error, &stack_trace, find_file_name_only,
         top_file_name](const char* str, size_t len, int index) {
          if (index >= kStackTraceLimit) {
            return false;
          }

          std::string_view line(str, len);

          auto opt_sf = ParseChromiumBasedStack(line);
          if (!opt_sf) {
            opt_sf = ParseDarwinStack(line);
          }
          if (opt_sf) {
            // If stack starts with one of our API calls, remove it (starts,
            // meaning it's the top of the stack - aka last call)
            if (frames.size() == 0 &&
                (opt_sf->function.find("captureMessage") != std::string::npos ||
                 opt_sf->function.find("captureException") !=
                     std::string::npos)) {
              return true;
            }
            // If stack ends with one of our internal API calls, remove it
            // (ends, meaning it's the bottom of the stack - aka top-most call)
            if (opt_sf->function.find("sentryWrapped") != std::string::npos) {
              return false;
            }
            // If stack match (/dynamic-component\/(.*?)\/\/app-service.js/),
            // mark it is the file name of dynamic component
            if (stack_trace.dynamic_component_path.empty()) {
              stack_trace.dynamic_component_path =
                  GetFileNameFromStack(opt_sf->filename);
            }
            if (opt_sf->filename.find("file://") == std::string::npos &&
                !opt_sf->filename.empty() && opt_sf->filename != "native" &&
                opt_sf->filename != "[native code]" &&
                opt_sf->filename != "<input>" &&
                opt_sf->filename != "<anonymous>" &&
                opt_sf->filename != kDefaultSourceMapUrl) {
              opt_sf->filename = "file://" + opt_sf->filename;
            }
            if (opt_sf->filename.find(error.file_name) != std::string::npos) {
              std::string suffix;
              if (error.release.empty()) {
                suffix = ".js";
              } else {
                suffix = "." + error.release + ".js";
              }
              opt_sf->filename = error.file_name + suffix;
              opt_sf->release = error.release;
            } else {
              opt_sf->release = GetSourceMapRelease(opt_sf->filename);
            }
            if (find_file_name_only) {
              if (!opt_sf->filename.empty()) {
                if (top_file_name != nullptr) {
                  // If top_file_name is not null, returns the file name by it.
                  *top_file_name = opt_sf->filename;
                } else {
                  // Or the caller consumes the single stack frame for file
                  // name.
                  frames.push_back(std::move(*opt_sf));
                }
                // Anyway, just break when find first.
                return false;
              }
            } else {
              // Only push stack frame if not find_file_name_only
              frames.push_back(std::move(*opt_sf));
            }
          }
          return true;
        });
  }

  stack_trace.name = error.name;
  stack_trace.message = error.message;
  if (find_file_name_only && top_file_name != nullptr) {
    stack_trace.failed = top_file_name->empty();
  } else {
    stack_trace.failed = frames.empty();
  }
  // reverse stack.
  std::reverse(frames.begin(), frames.end());
  stack_trace.frames = std::move(frames);
  return stack_trace;
}

void JSErrorReporter::SetSourceMapRelease(JSErrorInfo error) {
  if (error.message.empty() || error.stack.empty()) {
    auto trace = error.stack;
    LOGE("JSErrorReporter: setSourceMapRelease failed, stack_trace is"
         << trace << ", message is " << error.message);
    return;
  }
  error.name = kSourceMapReleaseErrorName;
  std::string file_name;
  ComputeStackTrace(error, true, &file_name);
  if (!file_name.empty()) {
    source_maps_[file_name] = error.message;
    LOGI("setSourceMapRelease success with url:" << file_name << ", release:"
                                                 << error.message);
  } else {
    LOGE("setSourceMapRelease failed with error.message:"
         << error.message << ", error.stack:" << error.stack
         << ", error.name:" << error.name);
  }
}

std::string JSErrorReporter::GetSourceMapRelease(const std::string& url) {
  if (source_maps_.find(url) != source_maps_.end()) {
    return source_maps_[url];
  }
  if (source_maps_.find(kDefaultSourceMapUrl) != source_maps_.end()) {
    return source_maps_[kDefaultSourceMapUrl];
  }
  return "";
}

std::optional<base::LynxError> JSErrorReporter::SendMTError(
    std::string_view original_error, int error_code, int error_level) {
  if (original_error.empty()) {
    LOGE("JSErrorReporter: originError is not string or empty string.");
    return std::nullopt;
  }
  LOGI("JSErrorReporter.sendError:" << original_error);
  auto separator_pos = original_error.find(kFlagBacktrace);
  auto message = separator_pos == std::string::npos
                     ? original_error
                     : original_error.substr(0, separator_pos);
  auto stack = separator_pos == std::string::npos
                   ? original_error
                   : original_error.substr(separator_pos);
  auto debug_info_separator_pos = stack.find(kFlagTemplateDebug);
  stack = debug_info_separator_pos == std::string::npos
              ? stack
              : stack.substr(0, debug_info_separator_pos);
  // limit stack in 50 lines to avoid OOM.
  stack = LimitStackString(stack);

  JSErrorInfo error;
  error.name = "Error";
  error.message = message;
  error.stack = stack;
  error.cause = original_error;

  auto msg_formatted = FormatError(error);
  // to json string.
  auto json_str = ErrorEventToJsonString(error, msg_formatted);

  auto error_level_enum = static_cast<base::LynxErrorLevel>(error_level);

  return ReportException(json_str, stack, error_code, error_level_enum, "");
}

std::optional<base::LynxError> JSErrorReporter::SendBTError(JSErrorInfo& info) {
  // limit stack in 50 lines to avoid OOM.
  info.stack = LimitStackString(info.stack);
  auto msg_formatted = FormatError(info);
  // to json string.
  auto json_str = ErrorEventToJsonString(info, msg_formatted);
  return ReportException(json_str, info.stack, info.error_code,
                         info.error_level, info.dynamic_component_path);
}

void JSErrorReporter::AddCustomInfoToError(
    const std::unordered_map<std::string, std::string>& info) {
  for (const auto& [key, value] : info) {
    custom_info_[key] = value;
  }
}

void JSErrorReporter::AppendCustomInfo(base::LynxError& error) {
  for (const auto& [key, value] : custom_info_) {
    error.custom_info_[key] = value;
  }
}

base::LynxError JSErrorReporter::ReportException(
    std::string_view msg, std::string_view stack, int32_t error_code,
    base::LynxErrorLevel error_level,
    const std::string& dynamic_component_path) {
  std::stringstream ss;
  ss << "JSErrorReporter::reportException " << this << ", error code is "
     << error_code << ", dynamic component path is " << dynamic_component_path
     << ", message is  " << msg << "\n"
     << stack;
  auto log_message = ss.str();
  switch (error_level) {
    case base::LynxErrorLevel::Warn:
      LOGW(log_message);
      break;
    case base::LynxErrorLevel::Error:
      LOGE(log_message);
      break;
    default:
      LOGE(log_message);
      break;
  }
  auto error = base::LynxError(error_code, std::string(msg), "", error_level);
  AppendCustomInfo(error);
  common::FormatErrorUrl(error, dynamic_component_path);
  return error;
}

std::string JSErrorReporter::GetFileNameFromStack(std::string_view line) {
  constexpr static char kDynamicComponentType[] = "dynamic-component/";
  static auto kDynamicComponentTypeLength = std::strlen(kDynamicComponentType);
  constexpr static char kAppService[] = "//app-service.js";
  auto start_pos = line.find(kDynamicComponentType);
  if (start_pos != std::string::npos) {
    auto end_pos = line.find(kAppService);
    if (end_pos != std::string::npos) {
      return std::string(
          line.substr(start_pos + kDynamicComponentTypeLength,
                      end_pos - start_pos - kDynamicComponentTypeLength));
    }
  }
  return "";
}

std::string JSErrorReporter::ErrorEventToJsonString(JSErrorInfo& info,
                                                    ErrorEvent& error_event) {
  rapidjson::Document document;
  document.SetObject();
  auto& allocator = document.GetAllocator();

  // make raw_error
  rapidjson::Value raw_error(rapidjson::kObjectType);
  rapidjson::Value stack_str;
  stack_str.SetString(info.stack.c_str(), allocator);
  raw_error.AddMember("stack", stack_str, allocator);
  raw_error.AddMember("message", info.message, allocator);
  rapidjson::Value raw_error_cause(rapidjson::kObjectType);
  raw_error_cause.AddMember("cause", info.cause, allocator);
  raw_error.AddMember("cause", raw_error_cause, allocator);
  document.AddMember("rawError", raw_error, allocator);

  // pid
  document.AddMember("pid", error_event.pid, allocator);

  // url
  document.AddMember("url", error_event.url, allocator);

  // dynamic_component_path
  document.AddMember("dynamicComponentPath", error_event.dynamic_component_path,
                     allocator);

  // sentry
  rapidjson::Value sentry(rapidjson::kObjectType);
  // sentry.platform
  sentry.AddMember("platform", error_event.platform, allocator);
  // sentry.sdk
  {
    rapidjson::Value sentry_sdk(rapidjson::kObjectType);
    sentry_sdk.AddMember("name", "sentry.javascript.browser", allocator);
    sentry_sdk.AddMember("version", "5.15.5", allocator);
    rapidjson::Value sentry_sdk_packages(rapidjson::kArrayType);
    rapidjson::Value sentry_sdk_packages_item(rapidjson::kObjectType);
    sentry_sdk_packages_item.AddMember("name", "npm:@sentry/browser",
                                       allocator);
    sentry_sdk_packages_item.AddMember("version", "5.15.5", allocator);
    sentry_sdk_packages.PushBack(sentry_sdk_packages_item, allocator);
    sentry_sdk.AddMember("packages", sentry_sdk_packages, allocator);
    rapidjson::Value sentry_sdk_integrations(rapidjson::kArrayType);
    sentry_sdk_integrations.PushBack("InboundFilters", allocator);
    sentry_sdk_integrations.PushBack("FunctionToString", allocator);
    sentry_sdk_integrations.PushBack("Breadcrumbs", allocator);
    sentry_sdk_integrations.PushBack("GlobalHandlers", allocator);
    sentry_sdk_integrations.PushBack("LinkedErrors", allocator);
    sentry_sdk_integrations.PushBack("UserAgent", allocator);
    sentry_sdk.AddMember("integrations", sentry_sdk_integrations, allocator);
    sentry.AddMember("sdk", sentry_sdk, allocator);
  }
  // sentry.level
  sentry.AddMember("level", error_event.level, allocator);
  // sentry.exception
  rapidjson::Value sentry_exception(rapidjson::kObjectType);
  {
    // sentry.exception.values
    rapidjson::Value sentry_exception_values(rapidjson::kArrayType);

    rapidjson::Value exception(rapidjson::kObjectType);
    exception.AddMember("type", error_event.exception.type, allocator);
    exception.AddMember("value", error_event.exception.value, allocator);
    rapidjson::Value exception_stacktrace(rapidjson::kObjectType);

    rapidjson::Value exception_stacktrace_frame(rapidjson::kArrayType);
    for (auto& item : error_event.exception.stack_trace.frames) {
      rapidjson::Value exception_stacktrace_frame_item(rapidjson::kObjectType);
      rapidjson::Value exception_stacktrace_frame_colno(rapidjson::kNumberType);
      exception_stacktrace_frame_colno.SetInt64(item.colno);
      exception_stacktrace_frame_item.AddMember(
          "colno", exception_stacktrace_frame_colno, allocator);
      exception_stacktrace_frame_item.AddMember("filename", item.filename,
                                                allocator);
      exception_stacktrace_frame_item.AddMember("function", item.function,
                                                allocator);
      exception_stacktrace_frame_item.AddMember("in_app", true, allocator);
      exception_stacktrace_frame_item.AddMember("release", item.release,
                                                allocator);
      rapidjson::Value exception_stacktrace_frame_lineno(
          rapidjson::kNumberType);
      exception_stacktrace_frame_lineno.SetInt64(item.lineno);
      exception_stacktrace_frame_item.AddMember(
          "lineno", exception_stacktrace_frame_lineno, allocator);
      exception_stacktrace_frame.PushBack(exception_stacktrace_frame_item,
                                          allocator);
    }

    exception_stacktrace.AddMember("frames", exception_stacktrace_frame,
                                   allocator);
    exception.AddMember("stacktrace", exception_stacktrace, allocator);

    // mechainsm
    rapidjson::Value exception_mechanism(rapidjson::kObjectType);
    exception_mechanism.AddMember("handled", true, allocator);
    exception_mechanism.AddMember("type", "generic", allocator);
    exception.AddMember("mechanism", exception_mechanism, allocator);

    sentry_exception_values.PushBack(exception, allocator);
    sentry_exception.AddMember("values", sentry_exception_values, allocator);
  }
  sentry.AddMember("exception", sentry_exception, allocator);

  // tags
  rapidjson::Value tags(rapidjson::kObjectType);
  tags.AddMember("error_type", info.name, allocator);
  tags.AddMember("extra", info.message, allocator);
  tags.AddMember("lib_version", info.build_version, allocator);
  tags.AddMember("run_type", info.file_name, allocator);
  tags.AddMember("version_code", info.version_code, allocator);
  sentry.AddMember("tags", tags, allocator);

  document.AddMember("sentry", sentry, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  document.Accept(writer);
  return buffer.GetString();
}

JSErrorReporter::JSErrorReporter() = default;

ErrorEvent JSErrorReporter::FormatError(JSErrorInfo& error_info) {
  const auto original_stack = error_info.stack;

  ErrorEvent error_event;
  auto stack_trace = ComputeStackTrace(error_info, false);

  error_event.exception.type = stack_trace.name;
  error_event.exception.value = stack_trace.message;
  if (!stack_trace.frames.empty()) {
    error_event.exception.stack_trace.frames = stack_trace.frames;
  }
  if (error_event.exception.type.empty() &&
      error_event.exception.value.empty()) {
    error_event.exception.value = "Unrecoverable error caught";
  }
  error_event.level = "error";
  error_event.platform = "javascript";
  error_event.pid =
      error_info.kind == "USER_ERROR" ? "USER_ERROR" : "INTERNAL_ERROR";
  error_info.dynamic_component_path = stack_trace.dynamic_component_path;
  error_event.dynamic_component_path = stack_trace.dynamic_component_path;
  error_event.url = "file://" + error_info.file_name + ".js";

  return error_event;
}

}  // namespace common
}  // namespace lynx
