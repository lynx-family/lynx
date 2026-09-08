// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_RECORD_BUILDER_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_RECORD_BUILDER_H_

#include <cstddef>
#include <optional>
#include <string>

#include "base/include/value/base_value.h"

namespace lynx {
namespace runtime {
namespace js {

// Bounds applied while turning a raw runtime snapshot into a protocol value.
// Hitting any bound trims the value and flags the record as truncated.
struct ValueSnapshotCaps {
  int max_depth = 8;
  size_t max_container_size = 100;
  // Byte limit; a valid UTF-8 input is truncated at a code point boundary.
  size_t max_string_length = 8 * 1024;
};

// Converts a raw lepus snapshot into a protocol-safe value. JSON-native scalars
// and containers pass through; non-JSON leaves become typed placeholders.
lepus::Value SanitizeToProtocolValue(const lepus::Value& in, bool& truncated,
                                     const ValueSnapshotCaps& caps = {});

// Builds a typed placeholder for a callback argument slot. Runtime replaces
// registered callback ids with this so the record shows a callback, not an
// opaque id.
lepus::Value BuildCallbackPlaceholder(int32_t argument_index);

// Builds an invoke-phase NativeModule record. |result| == nullopt means no
// result was captured; an engaged Nil represents an explicit null.
lepus::Value BuildInvokeRecord(int64_t invocation_id,
                               const std::string& module_name,
                               const std::string& method_name,
                               lepus::Value arguments, bool success,
                               std::optional<lepus::Value> result,
                               int32_t error_code,
                               const std::string& error_message);

// Builds a callback-phase NativeModule record for a delivered callback.
lepus::Value BuildCallbackRecord(int64_t invocation_id,
                                 const std::string& module_name,
                                 const std::string& method_name,
                                 int32_t callback_argument_index,
                                 lepus::Value result);

// Builds a standalone type=event GlobalEvent record.
lepus::Value BuildGlobalEventRecord(const std::string& name,
                                    const lepus::Value& arguments);

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_RECORD_BUILDER_H_
