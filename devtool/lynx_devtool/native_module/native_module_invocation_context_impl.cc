// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/native_module/native_module_invocation_context_impl.h"

#include <atomic>
#include <utility>

#include "base/include/value/array.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "devtool/lynx_devtool/native_module/native_module_record_builder.h"

namespace lynx {
using runtime::js::NativeModuleRecordObserver;

namespace devtool {
namespace {

int64_t GenerateInvocationId() {
  static std::atomic<int64_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed) + 1;
}

}  // namespace

NativeModuleInvocationContextImpl::NativeModuleInvocationContextImpl(
    std::weak_ptr<NativeModuleRecordObserver> observer, std::string module_name,
    std::string method_name)
    : observer_(std::move(observer)),
      invocation_id_(GenerateInvocationId()),
      module_name_(std::move(module_name)),
      method_name_(std::move(method_name)),
      callback_argument_index_(-1) {}

NativeModuleInvocationContextImpl::NativeModuleInvocationContextImpl(
    std::weak_ptr<NativeModuleRecordObserver> observer, int64_t invocation_id,
    std::string module_name, std::string method_name,
    int32_t callback_argument_index)
    : observer_(std::move(observer)),
      invocation_id_(invocation_id),
      module_name_(std::move(module_name)),
      method_name_(std::move(method_name)),
      callback_argument_index_(callback_argument_index) {}

std::shared_ptr<runtime::js::NativeModuleInvocationContext>
NativeModuleInvocationContextImpl::WithCallbackArgumentIndex(
    int32_t callback_argument_index) const {
  return std::shared_ptr<runtime::js::NativeModuleInvocationContext>(
      new NativeModuleInvocationContextImpl(observer_, invocation_id_,
                                            module_name_, method_name_,
                                            callback_argument_index));
}

void NativeModuleInvocationContextImpl::OnInvoke(
    lepus::Value arguments, const runtime::CallbackMap& callbacks, bool success,
    std::optional<lepus::Value> result, int32_t error_code,
    const std::string& error_message) const {
  auto observer = observer_.lock();
  if (!observer) {
    return;
  }
  if (arguments.IsArray() && !callbacks.empty()) {
    auto arguments_array = arguments.Array();
    auto record_arguments = lepus::CArray::Create();
    record_arguments->reserve(arguments_array->size());

    for (size_t i = 0; i < arguments_array->size(); ++i) {
      record_arguments->push_back(arguments_array->get(i));
    }

    // Keep callback placeholders out of the actual invocation arguments.
    for (const auto& [index, _] : callbacks) {
      if (index < 0 || static_cast<size_t>(index) >= record_arguments->size()) {
        continue;
      }
      record_arguments->set(
          static_cast<size_t>(index),
          BuildCallbackPlaceholder(static_cast<int32_t>(index)));
    }
    arguments = lepus::Value(std::move(record_arguments));
  }
  observer->OnRecord(BuildInvokeRecord(
      invocation_id_, module_name_, method_name_, std::move(arguments), success,
      std::move(result), error_code, error_message));
}

void NativeModuleInvocationContextImpl::OnCallback(lepus::Value result) const {
  if (auto observer = observer_.lock()) {
    observer->OnRecord(
        BuildCallbackRecord(invocation_id_, module_name_, method_name_,
                            callback_argument_index_, std::move(result)));
  }
}

}  // namespace devtool
}  // namespace lynx
