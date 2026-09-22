// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/bindings/modules/host_script_module_interceptor.h"

#include <atomic>
#include <unordered_set>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_impl_piper.h"
namespace lynx::runtime::js {
namespace {
using Kind = shell::InterceptKind;
bool Copyable(Runtime& rt, const Value& value, std::vector<Object>& parents) {
  if (!value.isObject()) return !value.isSymbol();
  auto object = value.getObject(rt);
  if (object.isFunction(rt) || object.isHostObject(rt) ||
      object.hasProperty(rt, "then"))
    return false;
  if (!object.isArray(rt) && !object.isArrayBuffer(rt)) {
    auto constructor = rt.global().getPropertyAsObject(rt, "Object");
    if (!constructor) return false;
    auto prototype_of =
        constructor->getPropertyAsFunction(rt, "getPrototypeOf");
    if (!prototype_of) return false;
    auto prototype = prototype_of->call(rt, {Value(rt, value)});
    auto plain = constructor->getProperty(rt, "prototype");
    if (!prototype || !plain ||
        (!prototype->isNull() && !Value::strictEquals(rt, *prototype, *plain)))
      return false;
  }
  if (object.isArrayBuffer(rt)) return true;
  if (parents.size() >= 64) return false;
  for (const auto& parent : parents)
    if (Object::strictEquals(rt, parent, object)) return false;
  parents.push_back(value.getObject(rt));
  auto keys = object.getPropertyNames(rt);
  if (!keys) return false;
  auto count = keys->size(rt);
  if (!count) return false;
  for (size_t i = 0; i < *count; ++i) {
    auto key = keys->getValueAtIndex(rt, i);
    if (!key || !key->isString()) return false;
    auto child = object.getProperty(rt, key->getString(rt));
    if (!child || !Copyable(rt, *child, parents)) return false;
  }
  parents.pop_back();
  return true;
}
lepus::Value Copy(Runtime& rt, const Value& value) {
  return pub::ValueUtils::ConvertValueToLepusValue(
      pub::ValueImplPiper(rt, value));
}
Value Restore(Runtime& rt, const lepus::Value& value) {
  return pub::ValueUtils::ConvertValueToPiperValue(rt,
                                                   pub::ValueImplLepus(value));
}
}  // namespace
std::shared_ptr<ModuleInterceptor>
HostScriptModuleInterceptor::CreateInvocation(const std::string& module,
                                              const std::string& method,
                                              base::LynxEntityId view) {
#if ENABLE_INSPECTOR
  if (!shell::Interceptor::IsEnabled()) return nullptr;
  for (auto kind : {Kind::kCall, Kind::kResult, Kind::kCallback}) {
    auto provider =
        shell::Interceptor::Current(kind, shell::Interceptor::Domain::kBTS);
    if (!provider) continue;
    if (!shell::Interceptor::IsViewAlive(view)) {
      shell::Interceptor::ReportCoverageGap(
          Kind::kCall, "NativeModule has no registered view");
      return nullptr;
    }
    static std::atomic<int64_t> next{1};
    auto call = std::make_shared<HostScriptModuleInterceptor>();
    call->owner_ = provider;
    call->id_ = next.fetch_add(1, std::memory_order_relaxed);
    call->module_ = module;
    call->method_ = method;
    call->view_ = view;
    return call;
  }
#endif
  return nullptr;
}
std::shared_ptr<shell::Interceptor> HostScriptModuleInterceptor::Provider(
    Kind kind) const {
  auto provider = owner_.lock();
  return provider && provider->IsAttached() &&
                 shell::Interceptor::IsViewAlive(view_) &&
                 provider->HasHandlers(kind)
             ? provider
             : nullptr;
}
lepus::Value HostScriptModuleInterceptor::BuildEvent() const {
  auto event = lepus::Value(lepus::Dictionary::Create());
  event.SetProperty("module", lepus::Value(module_));
  event.SetProperty("method", lepus::Value(method_));
  event.SetProperty("viewId", lepus::Value(std::to_string(view_)));
  event.SetProperty("invocationId", lepus::Value(std::to_string(id_)));
  return event;
}
void HostScriptModuleInterceptor::RewriteArguments(
    Runtime& rt, const Value* args, size_t count,
    std::vector<Value>& rewritten) {
  auto provider = Provider(Kind::kCall);
  if (!provider) return;
  auto values = lepus::CArray::Create();
  auto callbacks = lepus::CArray::Create();
  auto opaque = lepus::CArray::Create();
  std::vector<bool> immutable(count, false);
  for (size_t i = 0; i < count; ++i) {
    bool callback = args[i].isObject() && args[i].getObject(rt).isFunction(rt);
    std::vector<Object> parents;
    if (callback || !Copyable(rt, args[i], parents)) {
      auto marker = lepus::Value(lepus::Dictionary::Create());
      marker.SetProperty(callback ? "callback" : "opaque",
                         lepus::Value(static_cast<double>(i)));
      values->push_back(marker);
      (callback ? callbacks : opaque)
          ->push_back(lepus::Value(static_cast<double>(i)));
      immutable[i] = true;
    } else
      values->push_back(Copy(rt, args[i]));
  }
  auto event = BuildEvent();
  event.SetProperty("args", lepus::Value(values));
  event.SetProperty("callbackIndices", lepus::Value(callbacks));
  event.SetProperty("opaqueIndices", lepus::Value(opaque));
  decision_ = provider->Dispatch(Kind::kCall, event);
  const auto& decision = decision_;
  if (!decision.failed && decision.patch.IsTable() &&
      decision.patch.Contains("args")) {
    auto replacement = decision.patch.GetProperty("args");
    rewritten.reserve(count);
    for (size_t i = 0; i < count; ++i)
      rewritten.push_back(
          immutable[i]
              ? Value(rt, args[i])
              : Restore(rt, replacement.GetProperty(static_cast<uint32_t>(i))));
  }
}

bool HostScriptModuleInterceptor::HandlesCall() const {
  return !decision_.failed && decision_.mock.IsTable();
}
bool HostScriptModuleInterceptor::RewriteResult(Runtime& rt, Value& result) {
  const bool mock_result =
      HandlesCall() && decision_.mock.Contains("returnValue");
  auto provider = Provider(Kind::kResult);
  if (!provider) return mock_result;
  std::vector<Object> parents;
  if (!Copyable(rt, result, parents)) return mock_result;
  auto event = BuildEvent();
  event.SetProperty("value", Copy(rt, result));
  auto decision = provider->Dispatch(Kind::kResult, event);
  if (!decision.failed && decision.patch.IsTable() &&
      decision.patch.Contains("value")) {
    result = Restore(rt, decision.patch.GetProperty("value"));
  }
  return true;
}
void HostScriptModuleInterceptor::BeforeCallback(
    int argument_index, std::unique_ptr<pub::Value>& args) {
  if (!args || !args->IsArray()) return;
  auto provider = Provider(Kind::kCallback);
  if (!provider) return;
  auto event = BuildEvent();
  event.SetProperty("argumentIndex", lepus::Value(argument_index));
  event.SetProperty("args", pub::ValueUtils::ConvertValueToLepusValue(*args));
  auto decision = provider->Dispatch(Kind::kCallback, event);
  if (!decision.failed && decision.patch.IsTable() &&
      decision.patch.Contains("args")) {
    args = std::make_unique<pub::ValueImplLepus>(
        decision.patch.GetProperty("args"));
  }
}
ModuleInterceptorResult HostScriptModuleInterceptor::InterceptModuleMethod(
    const std::shared_ptr<LynxModule>& module,
    const LynxModule::MethodMetadata& method, Runtime* rt,
    const std::shared_ptr<ModuleDelegate>& delegate, const Value* args,
    size_t count, const std::unique_ptr<pub::Value>& pub_args,
    const CallbackMap& callbacks,
    NativeModuleInfoCollectorPtr timing_collector) const {
  if (!HandlesCall()) return {false, Value::null()};
  auto result = decision_.mock.Contains("returnValue")
                    ? Restore(*rt, decision_.mock.GetProperty("returnValue"))
                    : Value::undefined();
  auto deliveries = decision_.mock.GetProperty("callbacks");
  std::unordered_set<int> delivered_indices;
  for (int i = 0; deliveries.IsArray() && i < deliveries.GetLength(); ++i) {
    auto item = deliveries.GetProperty(i);
    int index = static_cast<int>(item.GetProperty("argumentIndex").Number());
    auto entry = callbacks.find(index);
    if (index < 0 || static_cast<size_t>(index) >= count ||
        entry == callbacks.end())
      continue;
    auto original = std::static_pointer_cast<ModuleCallback>(entry->second);
    auto id = delivered_indices.insert(index).second
                  ? original->CallbackId()
                  : delegate->RegisterJSCallbackFunction(
                        args[index].getObject(*rt).getFunction(*rt));
    auto callback = original->CloneForMockDelivery(id);
    callback->SetArgs(
        std::make_unique<pub::ValueImplLepus>(item.GetProperty("args")));
    std::static_pointer_cast<LynxJSIModule>(module)->InvokeCallback(callback);
  }
  return {true, std::move(result)};
}
}  // namespace lynx::runtime::js
