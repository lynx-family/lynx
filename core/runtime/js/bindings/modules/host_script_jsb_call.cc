// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/bindings/modules/host_script_jsb_call.h"

#include <atomic>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_impl_piper.h"

namespace lynx::runtime::js {
namespace {
using Kind = shell::InterceptKind;
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
  if (!value.isObject()) {
    if (value.isSymbol()) return false;
    copy = pub::ValueUtils::ConvertValueToLepusValue(
        pub::ValueImplPiper(rt, value));
    return true;
  }
  auto object = value.getObject(rt);
  if (object.isFunction(rt) || object.isHostObject(rt) ||
      object.hasProperty(rt, "then")) {
    return false;
  }
  if (!object.isArray(rt) && !object.isArrayBuffer(rt)) {
    auto constructor = rt.global().getPropertyAsObject(rt, "Object");
    if (!constructor) {
      return false;
    }
    auto prototype_of =
        constructor->getPropertyAsFunction(rt, "getPrototypeOf");
    if (!prototype_of) {
      return false;
    }
    auto prototype = prototype_of->call(rt, {Value(rt, value)});
    auto plain = constructor->getProperty(rt, "prototype");
    if (!prototype || !plain ||
        (!prototype->isNull() &&
         !Value::strictEquals(rt, *prototype, *plain))) {
      return false;
    }
  }
  if (object.isArrayBuffer(rt)) {
    copy = pub::ValueUtils::ConvertValueToLepusValue(
        pub::ValueImplPiper(rt, value));
    return true;
  }
  if (parents.size() >= 64) {
    return false;
  }
  for (const auto& parent : parents) {
    if (Object::strictEquals(rt, parent, object)) {
      return false;
    }
  }
  parents.push_back(value.getObject(rt));
  if (object.isArray(rt)) {
    auto array = object.getArray(rt);
    auto count = array.size(rt);
    if (!count) return false;
    auto values = lepus::CArray::Create();
    for (size_t i = 0; i < *count; ++i) {
      auto child = array.getValueAtIndex(rt, i);
      lepus::Value snapshot;
      if (!child || !TryCopy(rt, *child, snapshot, parents)) return false;
      values->push_back(std::move(snapshot));
    }
    parents.pop_back();
    copy = lepus::Value(std::move(values));
    return true;
  }
  auto keys = object.getPropertyNames(rt);
  if (!keys) {
    return false;
  }
  auto count = keys->size(rt);
  if (!count) {
    return false;
  }
  auto values = lepus::Dictionary::Create();
  // A plain object's "length" property is unrelated to its property count.
  values->reserve(*count);
  for (size_t i = 0; i < *count; ++i) {
    auto key = keys->getValueAtIndex(rt, i);
    if (!key || !key->isString()) {
      return false;
    }
    auto child = object.getProperty(rt, key->getString(rt));
    lepus::Value snapshot;
    if (!child || !TryCopy(rt, *child, snapshot, parents)) {
      return false;
    }
    values->SetValue(key->getString(rt).utf8(rt), std::move(snapshot));
  }
  parents.pop_back();
  copy = lepus::Value(std::move(values));
  return true;
}

bool CanCopyCallback(const pub::Value& value, size_t depth = 0) {
  // These values need their platform-specific conversion to reach JavaScript.
  // A Lepus snapshot cannot be used to replace them, even for a no-op patch.
  if (value.IsTransfer() || value.IsLynxObject() || value.IsTemplateData() ||
      value.IsFunction()) {
    return false;
  }
  if (value.IsArray() || value.IsMap()) {
    // Reject cyclic or excessively deep containers before snapshot conversion.
    if (depth >= 64) return false;
    bool copyable = true;
    if (value.IsArray()) {
      value.ForeachArray([&](int64_t, const pub::Value& child) {
        copyable = copyable && CanCopyCallback(child, depth + 1);
      });
    } else {
      value.ForeachMap([&](const pub::Value& key, const pub::Value& child) {
        copyable =
            copyable && key.IsString() && CanCopyCallback(child, depth + 1);
      });
    }
    return copyable;
  }
  return value.IsUndefined() || value.IsNil() || value.IsBool() ||
         value.IsString() || value.IsNumber() || value.IsArrayBuffer();
}

Value Restore(Runtime& rt, const lepus::Value& value) {
  return pub::ValueUtils::ConvertValueToPiperValue(rt,
                                                   pub::ValueImplLepus(value));
}
}  // namespace
std::shared_ptr<HostScriptJSBCall> HostScriptJSBCall::Create(
    const std::string& module, const std::string& method,
    base::LynxEntityId view) {
  if (!shell::Interceptor::IsEnabled()) {
    return nullptr;
  }
  auto provider =
      shell::Interceptor::Current({Kind::kCall, Kind::kResult, Kind::kCallback},
                                  shell::Interceptor::Domain::kBTS);
  if (!provider) {
    return nullptr;
  }
  if (!shell::Interceptor::IsViewAlive(view)) {
    for (auto kind : {Kind::kCall, Kind::kResult, Kind::kCallback}) {
      if (provider->HasHandlers(kind)) {
        shell::Interceptor::ReportCoverageGap(
            kind, "NativeModule has no registered view");
        break;
      }
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

std::shared_ptr<shell::Interceptor> HostScriptJSBCall::Provider(
    Kind kind) const {
  auto provider = owner_.lock();
  return provider && provider->IsAttached() &&
                 shell::Interceptor::IsViewAlive(view_) &&
                 provider->HasHandlers(kind)
             ? provider
             : nullptr;
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
  if (!provider) {
    return lepus::Value();
  }
  auto values = lepus::CArray::Create();
  auto callbacks = lepus::CArray::Create();
  auto opaque = lepus::CArray::Create();
  std::vector<bool> immutable(count, false);
  for (size_t i = 0; i < count; ++i) {
    bool callback = args[i].isObject() && args[i].getObject(rt).isFunction(rt);
    std::vector<Object> parents;
    lepus::Value snapshot;
    if (callback || !TryCopy(rt, args[i], snapshot, parents)) {
      auto marker = lepus::Value(lepus::Dictionary::Create());
      marker.SetProperty(callback ? kCallback : kOpaque,
                         lepus::Value(static_cast<double>(i)));
      values->push_back(marker);
      (callback ? callbacks : opaque)->emplace_back(static_cast<double>(i));
      immutable[i] = true;
    } else {
      values->push_back(std::move(snapshot));
    }
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
  if (!provider) {
    return false;
  }
  std::vector<Object> parents;
  lepus::Value snapshot;
  if (!TryCopy(rt, result, snapshot, parents)) {
    return false;
  }
  auto event = BuildEvent();
  event.SetProperty(kValue, std::move(snapshot));
  auto decision = provider->Dispatch(Kind::kResult, event);
  if (!decision.failed && decision.patch.IsTable() &&
      decision.patch.Contains(kValue)) {
    result = Restore(rt, decision.patch.GetProperty(kValue));
    return true;
  }
  return false;
}

void HostScriptJSBCall::OnCallback(int argument_index,
                                   std::unique_ptr<pub::Value>& args) {
  if (!args || !args->IsArray()) return;
  auto provider = Provider(Kind::kCallback);
  if (!provider) return;
  if (!CanCopyCallback(*args)) return;
  auto event = BuildEvent();
  event.SetProperty(kArgumentIndex, lepus::Value(argument_index));
  event.SetProperty(kArgs, pub::ValueUtils::ConvertValueToLepusValue(*args));
  auto decision = provider->Dispatch(Kind::kCallback, event);
  if (!decision.failed && decision.patch.IsTable() &&
      decision.patch.Contains(kArgs)) {
    args = std::make_unique<pub::ValueImplLepus>(
        decision.patch.GetProperty(kArgs));
  }
}

}  // namespace lynx::runtime::js
