// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/native_module_record_builder.h"

#include <string>
#include <utility>

#include "base/include/timer/time_utils.h"
#include "base/include/value/table.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {

// Type / phase values.
static constexpr char kTypeCall[] = "call";
static constexpr char kTypeEvent[] = "event";
static constexpr char kPhaseInvoke[] = "invoke";
static constexpr char kPhaseCallback[] = "callback";

lepus::DictionaryPtr BuildCallRecordBase(int64_t invocation_id,
                                         const std::string& method,
                                         const char* phase) {
  BASE_STATIC_STRING_DECL(kInvocationId, "invocationId");
  BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kPhase, "phase");
  BASE_STATIC_STRING_DECL(kMethod, "method");
  auto record = lepus::Dictionary::Create();
  record->SetValue(kInvocationId, std::to_string(invocation_id));
  record->SetValue(kTimestamp,
                   static_cast<int64_t>(base::CurrentSystemTimeMilliseconds()));
  record->SetValue(kType, kTypeCall);
  record->SetValue(kPhase, phase);
  record->SetValue(kMethod, method);
  return record;
}

lepus::Value BuildOperationResult(bool success,
                                  std::optional<lepus::Value> value,
                                  int32_t error_code,
                                  const std::string& error_message) {
  BASE_STATIC_STRING_DECL(kSuccess, "success");
  BASE_STATIC_STRING_DECL(kValue, "value");
  BASE_STATIC_STRING_DECL(kCode, "code");
  BASE_STATIC_STRING_DECL(kErrMsg, "errMsg");
  auto operation_result = lepus::Dictionary::Create();
  operation_result->SetValue(kSuccess, success);
  if (success) {
    if (value.has_value()) {
      operation_result->SetValue(kValue, std::move(*value));
    }
  } else {
    operation_result->SetValue(kCode, error_code);
    operation_result->SetValue(kErrMsg, error_message);
  }
  return lepus::Value(std::move(operation_result));
}

}  // namespace

lepus::Value BuildInvokeRecord(int64_t invocation_id,
                               const std::string& module_name,
                               const std::string& method_name,
                               lepus::Value arguments, bool success,
                               std::optional<lepus::Value> result,
                               int32_t error_code,
                               const std::string& error_message) {
  BASE_STATIC_STRING_DECL(kArguments, "arguments");
  BASE_STATIC_STRING_DECL(kResult, "result");
  BASE_STATIC_STRING_DECL(kTruncated, "truncated");
  auto record = BuildCallRecordBase(
      invocation_id, module_name + "." + method_name, kPhaseInvoke);
  record->SetValue(kArguments, std::move(arguments));
  record->SetValue(kResult, BuildOperationResult(success, std::move(result),
                                                 error_code, error_message));
  record->SetValue(kTruncated, false);
  return lepus::Value(std::move(record));
}

lepus::Value BuildCallbackRecord(int64_t invocation_id,
                                 const std::string& module_name,
                                 const std::string& method_name,
                                 int32_t callback_argument_index,
                                 lepus::Value result) {
  BASE_STATIC_STRING_DECL(kResult, "result");
  BASE_STATIC_STRING_DECL(kTruncated, "truncated");
  BASE_STATIC_STRING_DECL(kCallbackArgumentIndex, "callbackArgumentIndex");
  auto record = BuildCallRecordBase(
      invocation_id, module_name + "." + method_name, kPhaseCallback);
  record->SetValue(kCallbackArgumentIndex, callback_argument_index);
  record->SetValue(kResult,
                   BuildOperationResult(/*success=*/true, std::move(result),
                                        /*error_code=*/0, std::string()));
  record->SetValue(kTruncated, false);
  return lepus::Value(std::move(record));
}

lepus::Value BuildGlobalEventRecord(const std::string& name,
                                    const lepus::Value& arguments) {
  BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kMethod, "method");
  BASE_STATIC_STRING_DECL(kArguments, "arguments");
  BASE_STATIC_STRING_DECL(kTruncated, "truncated");
  auto record = lepus::Dictionary::Create();
  record->SetValue(kTimestamp,
                   static_cast<int64_t>(base::CurrentSystemTimeMilliseconds()));
  record->SetValue(kType, kTypeEvent);
  record->SetValue(kMethod, name);
  // |arguments| is the emitter's listener-argument list, always an array.
  record->SetValue(kArguments, arguments);
  record->SetValue(kTruncated, false);
  return lepus::Value(std::move(record));
}

lepus::Value BuildCallbackPlaceholder(int32_t argument_index) {
  BASE_STATIC_STRING_DECL(kPlaceholderType, "$type");
  BASE_STATIC_STRING_DECL(kPlaceholderArgumentIndex, "argumentIndex");

  auto placeholder = lepus::Dictionary::Create();
  placeholder->SetValue(kPlaceholderType, "callback");
  placeholder->SetValue(kPlaceholderArgumentIndex, argument_index);
  return lepus::Value(std::move(placeholder));
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
