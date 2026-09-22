// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/host_script/runtime/host_script_jsb_call.h"

#include <atomic>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_impl_piper.h"

namespace lynx::runtime::js {
namespace {
using Kind = shell::InterceptKind;
using pub::ValueUtils;
using shell::Interceptor;
BASE_STATIC_STRING_DECL(kModule, "module");
BASE_STATIC_STRING_DECL(kMethod, "method");
BASE_STATIC_STRING_DECL(kViewId, "viewId");
BASE_STATIC_STRING_DECL(kInvocationId, "invocationId");
BASE_STATIC_STRING_DECL(kCallback, "callback");
BASE_STATIC_STRING_DECL(kOpaque, "opaque");
BASE_STATIC_STRING_DECL(kArgs, "args");
BASE_STATIC_STRING_DECL(kCallbackIndices, "callbackIndices");
BASE_STATIC_STRING_DECL(kOpaqueIndices, "opaqueIndices");
BASE_STATIC_STRING_DECL(kValue, "value");
BASE_STATIC_STRING_DECL(kArgumentIndex, "argumentIndex");
// Validate and snapshot each value together so getters are read only once.
bool TryCopy(Runtime& rt, const Value& value, lepus::Value& copy,
             std::vector<Object>& parents) {
  auto object = value.asObject(rt);
  if (object && (object->isFunction(rt) || object->isHostObject(rt) ||
                 object->hasProperty(rt, "then")))
    return false;
  if (!object || object->isArrayBuffer(rt)) {
    if (value.isSymbol()) return false;
    copy = ValueUtils::ConvertValueToLepusValue(pub::ValueImplPiper(rt, value));
    return true;
  }
  if (!object->isArray(rt)) {
    auto constructor = rt.global().getPropertyAsObject(rt, "Object");
    if (!constructor) return false;
    auto get_proto = constructor->getPropertyAsFunction(rt, "getPrototypeOf");
    if (!get_proto) return false;
    auto prototype = get_proto->call(rt, {Value(rt, value)});
    auto plain = constructor->getProperty(rt, "prototype");
    if (!prototype || !plain ||
        (!prototype->isNull() && !Value::strictEquals(rt, *prototype, *plain)))
      return false;
  }
  if (parents.size() >= 64) return false;
  for (const auto& parent : parents)
    if (Object::strictEquals(rt, parent, *object)) return false;
  const bool is_array = object->isArray(rt);
  auto keys = is_array ? std::optional<Array>(object->getArray(rt))
                       : object->getPropertyNames(rt);
  auto count = keys ? keys->size(rt) : std::nullopt;
  if (!count) return false;
  auto values = is_array ? lepus::Value(lepus::CArray::Create())
                         : lepus::Value(lepus::Dictionary::Create());
  // Object storage follows property count; arrays visit every index.
  if (!is_array) values.Table()->reserve(*count);
  parents.push_back(value.getObject(rt));
  for (size_t i = 0; i < *count; ++i) {
    auto child = keys->getValueAtIndex(rt, i);
    if (!child || (!is_array && !child->isString())) return false;
    auto key = is_array ? std::string() : child->getString(rt).utf8(rt);
    if (!is_array) child = object->getProperty(rt, child->getString(rt));
    lepus::Value snapshot;
    if (!child || !TryCopy(rt, *child, snapshot, parents)) return false;
    if (is_array)
      values.Array()->push_back(std::move(snapshot));
    else
      values.Table()->SetValue(key, std::move(snapshot));
  }
  parents.pop_back();
  copy = std::move(values);
  return true;
}
bool CanCopyCallback(const pub::Value& value, size_t depth = 0) {
  // Reject platform-specific values and cyclic or excessively deep containers.
  if (value.IsTransfer() || value.IsLynxObject() || value.IsTemplateData() ||
      value.IsFunction())
    return false;
  if (!value.IsArray() && !value.IsMap())
    return value.IsUndefined() || value.IsNil() || value.IsBool() ||
           value.IsString() || value.IsNumber() || value.IsArrayBuffer();
  if (depth >= 64) return false;
  bool valid = true;
  if (value.IsArray()) {
    value.ForeachArray([&](int64_t, const pub::Value& child) {
      valid = valid && CanCopyCallback(child, depth + 1);
    });
  } else {
    value.ForeachMap([&](const pub::Value& key, const pub::Value& child) {
      valid = valid && key.IsString() && CanCopyCallback(child, depth + 1);
    });
  }
  return valid;
}
Value Restore(Runtime& rt, const lepus::Value& value) {
  return ValueUtils::ConvertValueToPiperValue(rt, pub::ValueImplLepus(value));
}
}  // namespace
std::shared_ptr<HostScriptJSBCall> HostScriptJSBCall::Create(
    const std::string& module, const std::string& method,
    base::LynxEntityId view) {
  if (!Interceptor::IsEnabled()) return nullptr;
  auto provider = Interceptor::Current(
      {Kind::kCall, Kind::kResult, Kind::kCallback}, Interceptor::Domain::kBTS);
  if (!provider) return nullptr;
  if (!Interceptor::IsViewAlive(view)) {
    for (auto kind : {Kind::kCall, Kind::kResult, Kind::kCallback})
      if (provider->HasHandlers(kind)) {
        Interceptor::ReportCoverageGap(kind,
                                       "NativeModule has no registered view");
        break;
      }
    return nullptr;
  }
  static std::atomic<int64_t> next{1};
  auto call = std::make_shared<HostScriptJSBCall>();
  call->owner_ = provider;
  call->id_ = next.fetch_add(1, std::memory_order_relaxed);
  call->module_ = module;
  call->method_ = method;
  call->view_ = view;
  return call;
}
std::shared_ptr<Interceptor> HostScriptJSBCall::Provider(Kind kind) const {
  auto provider = owner_.lock();
  if (!provider || !provider->IsAttached() ||
      !Interceptor::IsViewAlive(view_) || !provider->HasHandlers(kind))
    return nullptr;
  return provider;
}
lepus::Value HostScriptJSBCall::BuildEvent() const {
  auto event = lepus::Value(lepus::Dictionary::Create());
  event.SetProperty(kModule, lepus::Value(module_));
  event.SetProperty(kMethod, lepus::Value(method_));
  event.SetProperty(kViewId, lepus::Value(std::to_string(view_)));
  event.SetProperty(kInvocationId, lepus::Value(std::to_string(id_)));
  return event;
}
lepus::Value HostScriptJSBCall::OnCall(Runtime& rt, const Value* args,
                                       size_t count,
                                       std::vector<Value>& rewritten) {
  auto provider = Provider(Kind::kCall);
  if (!provider) return lepus::Value();
  auto values = lepus::CArray::Create();
  auto callbacks = lepus::CArray::Create();
  auto opaque = lepus::CArray::Create();
  std::vector<bool> immutable(count, false);
  for (size_t i = 0; i < count; ++i) {
    bool callback = args[i].isObject() && args[i].getObject(rt).isFunction(rt);
    std::vector<Object> parents;
    lepus::Value snapshot;
    if (callback || !TryCopy(rt, args[i], snapshot, parents)) {
      snapshot = lepus::Value(lepus::Dictionary::Create());
      snapshot.SetProperty(callback ? kCallback : kOpaque,
                           lepus::Value(static_cast<double>(i)));
      (callback ? callbacks : opaque)->emplace_back(static_cast<double>(i));
      immutable[i] = true;
    }
    values->push_back(std::move(snapshot));
  }
  auto event = BuildEvent();
  event.SetProperty(kArgs, lepus::Value(values));
  event.SetProperty(kCallbackIndices, lepus::Value(callbacks));
  event.SetProperty(kOpaqueIndices, lepus::Value(opaque));
  auto decision = provider->Dispatch(Kind::kCall, event);
  if (!decision.failed && decision.patch.IsTable() &&
      decision.patch.Contains(kArgs)) {
    auto replacement = decision.patch.GetProperty(kArgs);
    rewritten.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      rewritten.push_back(
          immutable[i]
              ? Value(rt, args[i])
              : Restore(rt, replacement.GetProperty(static_cast<uint32_t>(i))));
    }
  }
  return decision.failed ? lepus::Value() : std::move(decision.mock);
}
bool HostScriptJSBCall::OnResult(Runtime& rt, Value& result) {
  auto provider = Provider(Kind::kResult);
  if (!provider) return false;
  std::vector<Object> parents;
  lepus::Value snapshot;
  if (!TryCopy(rt, result, snapshot, parents)) return false;
  auto event = BuildEvent();
  event.SetProperty(kValue, std::move(snapshot));
  auto decision = provider->Dispatch(Kind::kResult, event);
  if (decision.failed || !decision.patch.IsTable() ||
      !decision.patch.Contains(kValue))
    return false;
  result = Restore(rt, decision.patch.GetProperty(kValue));
  return true;
}
void HostScriptJSBCall::OnCallback(int argument_index,
                                   std::unique_ptr<pub::Value>& args) {
  if (!args || !args->IsArray()) return;
  auto provider = Provider(Kind::kCallback);
  if (!provider || !CanCopyCallback(*args)) return;
  auto event = BuildEvent();
  event.SetProperty(kArgumentIndex, lepus::Value(argument_index));
  event.SetProperty(kArgs, ValueUtils::ConvertValueToLepusValue(*args));
  auto decision = provider->Dispatch(Kind::kCallback, event);
  if (!decision.failed && decision.patch.IsTable() &&
      decision.patch.Contains(kArgs)) {
    args = std::make_unique<pub::ValueImplLepus>(
        decision.patch.GetProperty(kArgs));
  }
}
}  // namespace lynx::runtime::js
