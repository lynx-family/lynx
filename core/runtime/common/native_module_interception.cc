// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/common/native_module_interception.h"

#include <atomic>
#include <utility>

#include "base/include/value/table.h"
namespace lynx::runtime {
using Kind = shell::InterceptKind;
using lepus::Value;
namespace {
int64_t GenerateInvocationId() {
  static std::atomic<int64_t> next{1};
  return next.fetch_add(1, std::memory_order_relaxed);
}
}  // namespace

NativeModuleInvocationInfo::NativeModuleInvocationInfo(std::string module,
                                                       std::string method,
                                                       base::LynxEntityId view)
    : id(GenerateInvocationId()),
      module(std::move(module)),
      method(std::move(method)),
      view(view) {}

std::shared_ptr<NativeModuleInterception> NativeModuleInterception::Create(
    const std::shared_ptr<shell::Interceptor>& provider,
    const std::string& module, const std::string& method,
    base::LynxEntityId view) {
  if (!provider || !provider->IsAttached()) return nullptr;
  return Create(provider, std::make_shared<NativeModuleInvocationInfo>(
                              module, method, view));
}
std::shared_ptr<NativeModuleInterception> NativeModuleInterception::Create(
    const std::shared_ptr<shell::Interceptor>& provider,
    std::shared_ptr<const NativeModuleInvocationInfo> info) {
  // Retain environment identity even before any JSB handlers are registered.
  if (!provider || !provider->IsAttached()) return nullptr;
  if (!shell::Interceptor::IsViewAlive(info->view)) {
    shell::Interceptor::ReportCoverageGap(
        Kind::kCall, "NativeModule has no registered view");
    return nullptr;
  }
  auto result = std::make_shared<NativeModuleInterception>();
  result->owner_ = provider;
  result->info_ = std::move(info);
  return result;
}
Value NativeModuleInterception::BuildEvent() const {
  auto event = Value(lepus::Dictionary::Create());
  event.SetProperty("module", Value(info_->module));
  event.SetProperty("method", Value(info_->method));
  event.SetProperty("viewId", Value(std::to_string(info_->view)));
  event.SetProperty("invocationId", Value(std::to_string(info_->id)));
  return event;
}
bool NativeModuleInterception::IsAlive() const {
  auto provider = owner_.lock();
  return provider && provider->IsAttached() &&
         shell::Interceptor::IsViewAlive(info_->view);
}
std::shared_ptr<shell::Interceptor> NativeModuleInterception::Provider(
    Kind kind) const {
  auto provider = owner_.lock();
  return IsAlive() && provider->HasHandlers(kind) ? provider : nullptr;
}
shell::InterceptResult NativeModuleInterception::Call(const Value& args,
                                                      const Value& callbacks,
                                                      const Value& opaque) {
  auto provider = Provider(Kind::kCall);
  if (!provider) return {};
  auto event = BuildEvent();
  event.SetProperty("args", args);
  event.SetProperty("callbackIndices", callbacks);
  event.SetProperty("opaqueIndices", opaque);
  return provider->Dispatch(Kind::kCall, event);
}
Value NativeModuleInterception::Result(const Value& value) {
  auto provider = Provider(Kind::kResult);
  if (!provider) return value;
  auto event = BuildEvent();
  event.SetProperty("value", value);
  auto decision = provider->Dispatch(Kind::kResult, event);
  return !decision.failed && decision.patch.IsTable() &&
                 decision.patch.Contains("value")
             ? decision.patch.GetProperty("value")
             : value;
}
Value NativeModuleInterception::Callback(int argument_index,
                                         const Value& args) {
  auto provider = Provider(Kind::kCallback);
  if (!provider) return args;
  auto event = BuildEvent();
  event.SetProperty("argumentIndex", Value(argument_index));
  event.SetProperty("args", args);
  auto decision = provider->Dispatch(Kind::kCallback, event);
  return !decision.failed && decision.patch.IsTable() &&
                 decision.patch.Contains("args")
             ? decision.patch.GetProperty("args")
             : args;
}
}  // namespace lynx::runtime
