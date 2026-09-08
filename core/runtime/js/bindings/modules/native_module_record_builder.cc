// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/native_module_record_builder.h"

#include <cmath>
#include <string>
#include <utility>

#include "base/include/timer/time_utils.h"
#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/base/js_constants.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {

// Record-level keys.
static constexpr char kInvocationIdKey[] = "invocationId";
static constexpr char kTimestampKey[] = "timestamp";
static constexpr char kTypeKey[] = "type";
static constexpr char kPhaseKey[] = "phase";
static constexpr char kMethodKey[] = "method";
static constexpr char kCallbackArgumentIndexKey[] = "callbackArgumentIndex";
static constexpr char kArgumentsKey[] = "arguments";
static constexpr char kResultKey[] = "result";
static constexpr char kTruncatedKey[] = "truncated";

// OperationResult keys.
static constexpr char kSuccessKey[] = "success";
static constexpr char kValueKey[] = "value";
static constexpr char kCodeKey[] = "code";
static constexpr char kErrMsgKey[] = "errMsg";

// Type values.
static constexpr char kTypeCall[] = "call";
static constexpr char kTypeEvent[] = "event";
static constexpr char kPhaseInvoke[] = "invoke";
static constexpr char kPhaseCallback[] = "callback";

// Placeholder keys.
static constexpr char kPlaceholderTypeKey[] = "$type";
static constexpr char kPlaceholderValueKey[] = "value";
static constexpr char kPlaceholderByteLengthKey[] = "byteLength";
static constexpr char kPlaceholderArgumentIndexKey[] = "argumentIndex";

lepus::DictionaryPtr MakePlaceholder(const char* type) {
  auto placeholder = lepus::Dictionary::Create();
  placeholder->SetValue(BASE_STATIC_STRING(kPlaceholderTypeKey), type);
  return placeholder;
}

lepus::Value Sanitize(const lepus::Value& in, int depth,
                      const ValueSnapshotCaps& caps, bool& truncated) {
  switch (in.Type()) {
    case lepus::Value_Nil:
    case lepus::Value_Bool:
    case lepus::Value_Int32:
    case lepus::Value_UInt32:
      return in;
    case lepus::Value_Double: {
      double value = in.Number();
      if (std::isfinite(value)) {
        return in;
      }
      auto placeholder = MakePlaceholder("number");
      placeholder->SetValue(
          BASE_STATIC_STRING(kPlaceholderValueKey),
          std::isnan(value) ? "NaN" : (value > 0 ? "Infinity" : "-Infinity"));
      return lepus::Value(std::move(placeholder));
    }
    case lepus::Value_Int64: {
      int64_t value = in.Int64();
      if (value < kMinJavaScriptNumber || value > kMaxJavaScriptNumber) {
        auto placeholder = MakePlaceholder("bigint");
        placeholder->SetValue(BASE_STATIC_STRING(kPlaceholderValueKey),
                              std::to_string(value));
        return lepus::Value(std::move(placeholder));
      }
      return in;
    }
    case lepus::Value_UInt64: {
      uint64_t value = in.UInt64();
      if (value > static_cast<uint64_t>(kMaxJavaScriptNumber)) {
        auto placeholder = MakePlaceholder("bigint");
        placeholder->SetValue(BASE_STATIC_STRING(kPlaceholderValueKey),
                              std::to_string(value));
        return lepus::Value(std::move(placeholder));
      }
      return in;
    }
    case lepus::Value_String: {
      const std::string& str = in.StdString();
      if (str.size() > caps.max_string_length) {
        truncated = true;
        size_t length = caps.max_string_length;
        // The limit is in bytes. If it splits a UTF-8 code point, exclude
        // that entire code point instead of returning an incomplete prefix.
        while (length > 0 &&
               (static_cast<unsigned char>(str[length]) & 0xC0) == 0x80) {
          --length;
        }
        return lepus::Value(str.substr(0, length));
      }
      return in;
    }
    case lepus::Value_Undefined:
      return lepus::Value(MakePlaceholder("undefined"));
    case lepus::Value_ByteArray: {
      auto placeholder = MakePlaceholder("arraybuffer");
      auto byte_array = in.ByteArray();
      placeholder->SetValue(
          BASE_STATIC_STRING(kPlaceholderByteLengthKey),
          static_cast<int64_t>(byte_array ? byte_array->GetLength() : 0));
      return lepus::Value(std::move(placeholder));
    }
    case lepus::Value_Array: {
      if (depth >= caps.max_depth) {
        truncated = true;
        return lepus::Value(MakePlaceholder("unsupported"));
      }
      auto src = in.Array();
      auto out = lepus::CArray::Create();
      size_t count = src ? src->size() : 0;
      if (count > caps.max_container_size) {
        count = caps.max_container_size;
        truncated = true;
      }
      out->reserve(count);
      for (size_t i = 0; i < count; ++i) {
        out->emplace_back(Sanitize(src->get(i), depth + 1, caps, truncated));
      }
      return lepus::Value(std::move(out));
    }
    case lepus::Value_Table: {
      if (depth >= caps.max_depth) {
        truncated = true;
        return lepus::Value(MakePlaceholder("unsupported"));
      }
      auto src = in.Table();
      auto out = lepus::Dictionary::Create();
      size_t remaining = caps.max_container_size;
      if (src) {
        for (const auto& pair : *src) {
          if (remaining == 0) {
            truncated = true;
            break;
          }
          --remaining;
          out->SetValue(pair.first,
                        Sanitize(pair.second, depth + 1, caps, truncated));
        }
      }
      return lepus::Value(std::move(out));
    }
    default:
      truncated = true;
      return lepus::Value(MakePlaceholder("unsupported"));
  }
}

lepus::DictionaryPtr BuildCallRecordBase(int64_t invocation_id,
                                         const std::string& method,
                                         const char* phase,
                                         int32_t callback_argument_index) {
  auto record = lepus::Dictionary::Create();
  record->SetValue(BASE_STATIC_STRING(kInvocationIdKey),
                   std::to_string(invocation_id));
  record->SetValue(BASE_STATIC_STRING(kTimestampKey),
                   static_cast<int64_t>(base::CurrentSystemTimeMilliseconds()));
  record->SetValue(BASE_STATIC_STRING(kTypeKey), kTypeCall);
  record->SetValue(BASE_STATIC_STRING(kPhaseKey), phase);
  record->SetValue(BASE_STATIC_STRING(kMethodKey), method);
  if (callback_argument_index >= 0) {
    record->SetValue(BASE_STATIC_STRING(kCallbackArgumentIndexKey),
                     callback_argument_index);
  }
  return record;
}

lepus::Value BuildOperationResult(bool success,
                                  std::optional<lepus::Value> value,
                                  int32_t error_code,
                                  const std::string& error_message,
                                  bool& truncated) {
  auto operation_result = lepus::Dictionary::Create();
  operation_result->SetValue(BASE_STATIC_STRING(kSuccessKey), success);
  if (success) {
    if (value.has_value()) {
      operation_result->SetValue(BASE_STATIC_STRING(kValueKey),
                                 SanitizeToProtocolValue(*value, truncated));
    }
  } else {
    operation_result->SetValue(BASE_STATIC_STRING(kCodeKey), error_code);
    operation_result->SetValue(BASE_STATIC_STRING(kErrMsgKey), error_message);
  }
  return lepus::Value(std::move(operation_result));
}

}  // namespace

lepus::Value SanitizeToProtocolValue(const lepus::Value& in, bool& truncated,
                                     const ValueSnapshotCaps& caps) {
  return Sanitize(in, /*depth=*/0, caps, truncated);
}

lepus::Value BuildCallbackPlaceholder(int32_t argument_index) {
  auto placeholder = MakePlaceholder("callback");
  placeholder->SetValue(BASE_STATIC_STRING(kPlaceholderArgumentIndexKey),
                        argument_index);
  return lepus::Value(std::move(placeholder));
}

lepus::Value BuildInvokeRecord(int64_t invocation_id,
                               const std::string& module_name,
                               const std::string& method_name,
                               lepus::Value arguments, bool success,
                               std::optional<lepus::Value> result,
                               int32_t error_code,
                               const std::string& error_message) {
  auto record = BuildCallRecordBase(
      invocation_id, module_name + "." + method_name, kPhaseInvoke,
      /*callback_argument_index=*/-1);
  bool truncated = false;
  if (!arguments.IsNil()) {
    record->SetValue(BASE_STATIC_STRING(kArgumentsKey),
                     SanitizeToProtocolValue(arguments, truncated));
  }
  record->SetValue(BASE_STATIC_STRING(kResultKey),
                   BuildOperationResult(success, std::move(result), error_code,
                                        error_message, truncated));
  record->SetValue(BASE_STATIC_STRING(kTruncatedKey), truncated);
  return lepus::Value(std::move(record));
}

lepus::Value BuildCallbackRecord(int64_t invocation_id,
                                 const std::string& module_name,
                                 const std::string& method_name,
                                 int32_t callback_argument_index,
                                 lepus::Value result) {
  auto record =
      BuildCallRecordBase(invocation_id, module_name + "." + method_name,
                          kPhaseCallback, callback_argument_index);
  bool truncated = false;
  record->SetValue(BASE_STATIC_STRING(kResultKey),
                   BuildOperationResult(
                       /*success=*/true, std::move(result),
                       /*error_code=*/0, std::string(), truncated));
  record->SetValue(BASE_STATIC_STRING(kTruncatedKey), truncated);
  return lepus::Value(std::move(record));
}

lepus::Value BuildGlobalEventRecord(const std::string& name,
                                    const lepus::Value& arguments) {
  auto record = lepus::Dictionary::Create();
  record->SetValue(BASE_STATIC_STRING(kTimestampKey),
                   static_cast<int64_t>(base::CurrentSystemTimeMilliseconds()));
  record->SetValue(BASE_STATIC_STRING(kTypeKey), kTypeEvent);
  record->SetValue(BASE_STATIC_STRING(kMethodKey), name);
  bool truncated = false;
  auto event_arguments = lepus::CArray::Create();
  event_arguments->push_back(SanitizeToProtocolValue(arguments, truncated));
  record->SetValue(BASE_STATIC_STRING(kArgumentsKey),
                   std::move(event_arguments));
  record->SetValue(BASE_STATIC_STRING(kTruncatedKey), truncated);
  return lepus::Value(std::move(record));
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
