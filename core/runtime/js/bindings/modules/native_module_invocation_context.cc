// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/native_module_invocation_context.h"

#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_impl_piper.h"
#if ENABLE_INSPECTOR || defined(ENABLE_UNITTESTS)
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/runtime/js/bindings/modules/native_module_record_builder.h"
#endif

namespace lynx::runtime::js {
namespace {
using Kind = shell::InterceptKind;
thread_local std::shared_ptr<NativeModuleInvocationContext> active_invocation;
// Promise, functions, host objects and cyclic data stay in the page VM. A
// non-copyable argument is represented by an immutable opaque marker as a
// whole.
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

NativeModuleInvocationContext::Scope::Scope(
    std::shared_ptr<NativeModuleInvocationContext> current)
    : previous_(std::move(active_invocation)) {
  active_invocation = std::move(current);
}
NativeModuleInvocationContext::Scope::~Scope() {
  active_invocation = std::move(previous_);
}
std::shared_ptr<NativeModuleInvocationContext>
NativeModuleInvocationContext::Current() {
  return active_invocation;
}

NativeModuleInvocationContext::NativeModuleInvocationContext(
    std::weak_ptr<NativeModuleRecordObserver> observer, std::string module,
    std::string method, base::LynxEntityId view)
    : info_(std::make_shared<NativeModuleInvocationInfo>(
          std::move(module), std::move(method), view)),
      observer_(std::move(observer)) {}

std::shared_ptr<NativeModuleInvocationContext>
NativeModuleInvocationContext::Create(
    std::weak_ptr<NativeModuleRecordObserver> observer,
    const std::string& module, const std::string& method,
    base::LynxEntityId view) {
#if ENABLE_INSPECTOR
  std::shared_ptr<shell::Interceptor> provider;
  if (shell::Interceptor::IsEnabled()) {
    for (auto kind : {Kind::kCall, Kind::kResult, Kind::kCallback}) {
      provider =
          shell::Interceptor::Current(kind, shell::Interceptor::Domain::kBTS);
      if (provider) break;
    }
  }
  if (!provider && observer.expired()) return nullptr;
  auto context = std::make_shared<NativeModuleInvocationContext>(
      std::move(observer), module, method, view);
  context->interception_ =
      NativeModuleInterception::Create(provider, context->info_);
  return context->interception_ || context->HasObserver() ? context : nullptr;
#else
  return nullptr;
#endif  // ENABLE_INSPECTOR
}

bool NativeModuleInvocationContext::HasObserver() const {
#if ENABLE_INSPECTOR
  return !observer_.expired();
#else
  return false;
#endif
}

shell::InterceptResult NativeModuleInvocationContext::Call(
    Runtime& rt, const Value* args, size_t count,
    std::vector<Value>& rewritten) {
  if (!interception_ || !interception_->HasHandlers(Kind::kCall)) return {};
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
  auto decision = interception_->Call(
      lepus::Value(values), lepus::Value(callbacks), lepus::Value(opaque));
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
  return decision;
}
std::optional<lepus::Value> NativeModuleInvocationContext::Result(
    Runtime& rt, Value& result) {
  if (!interception_ || !interception_->HasHandlers(Kind::kResult))
    return std::nullopt;
  std::vector<Object> parents;
  if (!Copyable(rt, result, parents)) return std::nullopt;
  result = Restore(rt, interception_->Result(Copy(rt, result)));
  return Copy(rt, result);
}
std::optional<lepus::Value> NativeModuleInvocationContext::PrepareCallback(
    int argument_index, std::unique_ptr<pub::Value>& args) {
  if (!args || !args->IsArray()) return std::nullopt;
  const bool intercept =
      interception_ && interception_->HasHandlers(Kind::kCallback);
  const bool observe = argument_index >= 0 && HasObserver();
  if (!intercept && !observe) return std::nullopt;
  auto snapshot = pub::ValueUtils::ConvertValueToLepusValue(*args);
  if (intercept) {
    snapshot = interception_->Callback(argument_index, snapshot);
    args = std::make_unique<pub::ValueImplLepus>(snapshot);
  }
  return observe ? std::make_optional(std::move(snapshot)) : std::nullopt;
}

void NativeModuleInvocationContext::RecordCallback(
    int argument_index, std::optional<lepus::Value> result) const {
#if ENABLE_INSPECTOR
  if (argument_index >= 0 && result && HasObserver()) {
    EmitRecord(BuildCallbackRecord(argument_index, std::move(*result)));
  }
#endif
}

#if ENABLE_INSPECTOR || defined(ENABLE_UNITTESTS)
lepus::Value NativeModuleInvocationContext::BuildInvokeRecord(
    lepus::Value arguments, const CallbackMap& callbacks, bool success,
    std::optional<lepus::Value> result, int32_t error_code,
    const std::string& error_message) const {
  if (arguments.IsArray() && !callbacks.empty()) {
    // The observer owns its presentation array; callback placeholders must not
    // overwrite a shared native snapshot or arguments used for dispatch.
    auto arguments_array = lepus::CArray::Create();
    for (size_t i = 0; i < arguments.Array()->size(); ++i) {
      arguments_array->push_back(arguments.Array()->get(i));
    }
    arguments = lepus::Value(arguments_array);
    for (const auto& [index, _] : callbacks) {
      if (index < 0 || static_cast<size_t>(index) >= arguments_array->size()) {
        continue;
      }
      arguments_array->set(
          static_cast<size_t>(index),
          BuildCallbackPlaceholder(static_cast<int32_t>(index)));
    }
  }
  return js::BuildInvokeRecord(info_->id, info_->module, info_->method,
                               std::move(arguments), success, std::move(result),
                               error_code, error_message);
}

lepus::Value NativeModuleInvocationContext::BuildCallbackRecord(
    int argument_index, lepus::Value result) const {
  return js::BuildCallbackRecord(info_->id, info_->module, info_->method,
                                 argument_index, std::move(result));
}

void NativeModuleInvocationContext::EmitRecord(
    const lepus::Value& record) const {
  if (auto observer = observer_.lock()) {
    observer->OnRecord(record);
  }
}

#endif
}  // namespace lynx::runtime::js
