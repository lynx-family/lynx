// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/native_module_invocation_context.h"

#include <atomic>
#include <utility>

namespace lynx {
namespace runtime {
namespace js {
namespace {

int64_t GenerateInvocationId() {
  static std::atomic<int64_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed) + 1;
}

}  // namespace

NativeModuleInvocationContext::NativeModuleInvocationContext(
    std::weak_ptr<NativeModuleRecordObserver> observer, std::string module_name,
    std::string method_name)
    : observer_(std::move(observer)),
      invocation_id_(GenerateInvocationId()),
      module_name_(std::move(module_name)),
      method_name_(std::move(method_name)),
      callback_argument_index_(-1) {}

NativeModuleInvocationContext::NativeModuleInvocationContext(
    std::weak_ptr<NativeModuleRecordObserver> observer, int64_t invocation_id,
    std::string module_name, std::string method_name,
    int32_t callback_argument_index)
    : observer_(std::move(observer)),
      invocation_id_(invocation_id),
      module_name_(std::move(module_name)),
      method_name_(std::move(method_name)),
      callback_argument_index_(callback_argument_index) {}

std::shared_ptr<NativeModuleInvocationContext>
NativeModuleInvocationContext::WithCallbackArgumentIndex(
    int32_t callback_argument_index) const {
  return std::shared_ptr<NativeModuleInvocationContext>(
      new NativeModuleInvocationContext(observer_, invocation_id_, module_name_,
                                        method_name_, callback_argument_index));
}

lepus::Value NativeModuleInvocationContext::BuildInvokeRecord(
    lepus::Value, bool, lepus::Value, int32_t, const std::string&) const {
  // TODO(liting.src): Implement protocol-safe value serialization.
  return lepus::Value();
}

lepus::Value NativeModuleInvocationContext::BuildCallbackRecord(
    lepus::Value) const {
  // TODO(liting.src): Implement protocol-safe value serialization.
  return lepus::Value();
}

void NativeModuleInvocationContext::EmitRecord(const lepus::Value&) const {
  // TODO(liting.src): Deliver the serialized record to the observer.
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
