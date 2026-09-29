// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_OBSERVER_NATIVE_MODULE_RECORD_OBSERVER_H_
#define CORE_INSPECTOR_OBSERVER_NATIVE_MODULE_RECORD_OBSERVER_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/include/value/base_value.h"
#include "core/public/jsb/lynx_module_callback.h"

namespace lynx {
namespace runtime {
namespace js {

// Records invocations and callbacks on the JS thread.
class NativeModuleInvocationContext {
 public:
  virtual ~NativeModuleInvocationContext() = default;

  virtual std::shared_ptr<NativeModuleInvocationContext>
  WithCallbackArgumentIndex(int32_t callback_argument_index) const = 0;

  // Do not mutate shared arguments. nullopt differs from an explicit null.
  virtual void OnInvoke(lepus::Value arguments, const CallbackMap& callbacks,
                        bool success, std::optional<lepus::Value> result,
                        int32_t error_code,
                        const std::string& error_message) const = 0;
  virtual void OnCallback(lepus::Value result) const = 0;
};

// JS-thread hooks for NativeModule and global event recording.
class NativeModuleRecordObserver {
 public:
  virtual ~NativeModuleRecordObserver() = default;

  virtual void OnRecord(const lepus::Value& record) = 0;

  virtual std::shared_ptr<NativeModuleInvocationContext> CreateInvocation(
      const std::string& module_name, const std::string& method_name) = 0;
  virtual void OnGlobalEvent(const std::string& name,
                             const lepus::Value& arguments) = 0;
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_INSPECTOR_OBSERVER_NATIVE_MODULE_RECORD_OBSERVER_H_
