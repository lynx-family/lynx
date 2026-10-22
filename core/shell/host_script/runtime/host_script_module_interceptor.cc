// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_module_interceptor.h"

#include <atomic>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"
#include "core/runtime/js/bindings/modules/module_interceptor_registry.h"
#include "core/shell/host_script/runtime/host_script_value_snapshot.h"
#include "core/shell/host_script/runtime/interceptor.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx::shell {
namespace {
using runtime::CallbackMap;
using runtime::js::LynxModule;
using runtime::js::ModuleCallback;
using runtime::js::ModuleCallContext;
using runtime::js::ModuleDelegate;
using runtime::js::ModuleInterceptorResult;
using runtime::js::NativeModuleInfoCollectorPtr;
using runtime::js::Runtime;
using runtime::js::Value;
using Kind = InterceptKind;

BASE_STATIC_STRING_DECL(kModule, "module");
BASE_STATIC_STRING_DECL(kMethod, "method");
BASE_STATIC_STRING_DECL(kViewId, "viewId");
BASE_STATIC_STRING_DECL(kInvocationId, "invocationId");
BASE_STATIC_STRING_DECL(kArgs, "args");
BASE_STATIC_STRING_DECL(kCallback, "callback");
BASE_STATIC_STRING_DECL(kOpaque, "opaque");
BASE_STATIC_STRING_DECL(kCallbackIndices, "callbackIndices");
BASE_STATIC_STRING_DECL(kOpaqueIndices, "opaqueIndices");
BASE_STATIC_STRING_DECL(kValue, "value");
BASE_STATIC_STRING_DECL(kArgumentIndex, "argumentIndex");
BASE_STATIC_STRING_DECL(kReturnValue, "returnValue");
BASE_STATIC_STRING_DECL(kCallbacks, "callbacks");

constexpr std::initializer_list<Kind> kStages = {Kind::kCall, Kind::kResult,
                                                 Kind::kCallback};

std::unique_ptr<runtime::js::ModuleInterceptor> Create() {
  return std::make_unique<HostScriptModuleInterceptor>();
}

bool IsFunction(Runtime& rt, const Value& value) {
  return value.isObject() && value.getObject(rt).isFunction(rt);
}

// The patched field, or nil when the decision leaves it unchanged.
lepus::Value Patched(const InterceptResult& decision,
                     const base::String& field) {
  if (decision.failed || !decision.patch.IsTable() ||
      !decision.patch.Contains(field)) {
    return lepus::Value();
  }
  return decision.patch.GetProperty(field);
}
}  // namespace

// One BTS native module invocation, shared with the callbacks it registers.
class HostScriptModuleInterceptor::Call final : public ModuleCallContext {
 public:
  Call() = default;
  Call(std::weak_ptr<Interceptor> provider, base::LynxEntityId view,
       std::string module, std::string method)
      : provider_(std::move(provider)),
        id_(next_id_.fetch_add(1, std::memory_order_relaxed)),
        view_(view),
        module_(std::move(module)),
        method_(std::move(method)) {}

  // The provider selected at the start, while it still handles `kind`.
  std::shared_ptr<Interceptor> Provider(Kind kind) const {
    auto provider = provider_.lock();
    if (!provider || !provider->IsAttached() ||
        !Interceptor::IsViewAlive(view_) || !provider->HasHandlers(kind)) {
      return nullptr;
    }
    return provider;
  }

  lepus::Value Event() const {
    auto event = lepus::Value(lepus::Dictionary::Create());
    event.SetProperty(kModule, lepus::Value(module_));
    event.SetProperty(kMethod, lepus::Value(method_));
    event.SetProperty(kViewId, lepus::Value(std::to_string(view_)));
    event.SetProperty(kInvocationId, lepus::Value(std::to_string(id_)));
    return event;
  }

  // A mock is final for the whole invocation, including its callbacks.
  bool mocked() const { return mocked_; }
  void SetMock(lepus::Value mock) {
    mocked_ = mock.IsTable();
    mock_ = std::move(mock);
  }
  // Consumed once, so callbacks sharing this state never retain the payload.
  lepus::Value TakeMock() { return std::exchange(mock_, lepus::Value()); }

 private:
  static inline std::atomic<int64_t> next_id_{1};
  std::weak_ptr<Interceptor> provider_;
  int64_t id_ = 0;
  base::LynxEntityId view_ = -1;
  std::string module_;
  std::string method_;
  bool mocked_ = false;
  lepus::Value mock_;
};

void HostScriptModuleInterceptor::Register() {
  runtime::js::ModuleInterceptorRegistry::RegisterLeading(&Create);
}

HostScriptModuleInterceptor::Call* HostScriptModuleInterceptor::GetCall(
    ModuleCallContext* context) {
  // BeforeConvertArgs always supplies a Call, including when HSR is inactive.
  return static_cast<Call*>(context);
}

std::shared_ptr<ModuleCallContext>
HostScriptModuleInterceptor::BeforeConvertArgs(
    const std::shared_ptr<LynxModule>& module,
    const LynxModule::MethodMetadata& method, Runtime* runtime,
    const Value* args, size_t count, std::vector<Value>& rewritten) {
  if (!Interceptor::IsEnabled()) return std::make_shared<Call>();
  auto provider = Interceptor::Current(kStages, Interceptor::Domain::kBTS);
  if (!provider) return std::make_shared<Call>();
  auto view = module->GetLogContext().view_id;
  if (!Interceptor::IsViewAlive(view)) {
    for (auto kind : kStages) {
      if (provider->HasHandlers(kind)) {
        Interceptor::ReportCoverageGap(kind,
                                       "NativeModule has no registered view");
        break;
      }
    }
    return std::make_shared<Call>();
  }
  auto owned =
      std::make_shared<Call>(provider, view, module->name_, method.name);
  auto& call = *owned;

  auto handler = call.Provider(Kind::kCall);
  if (!handler) return owned;
  // Callbacks and values that cannot be copied are shown as placeholders and
  // always keep their original value.
  auto& rt = *runtime;
  auto values = lepus::CArray::Create();
  auto callbacks = lepus::CArray::Create();
  auto opaque = lepus::CArray::Create();
  std::vector<bool> kept(count, false);
  for (size_t i = 0; i < count; ++i) {
    const bool is_callback = IsFunction(rt, args[i]);
    auto copy = is_callback ? std::nullopt : SnapshotJSValue(rt, args[i]);
    if (copy) {
      values->push_back(std::move(*copy));
      continue;
    }
    kept[i] = true;
    auto index = lepus::Value(static_cast<double>(i));
    auto placeholder = lepus::Value(lepus::Dictionary::Create());
    placeholder.SetProperty(is_callback ? kCallback : kOpaque, index);
    (is_callback ? callbacks : opaque)->push_back(index);
    values->push_back(std::move(placeholder));
  }
  auto event = call.Event();
  event.SetProperty(kArgs, lepus::Value(std::move(values)));
  event.SetProperty(kCallbackIndices, lepus::Value(std::move(callbacks)));
  event.SetProperty(kOpaqueIndices, lepus::Value(std::move(opaque)));
  auto decision = handler->Dispatch(Kind::kCall, event);
  if (decision.failed) return owned;

  auto patched = Patched(decision, kArgs);
  if (patched.IsArray()) {
    std::vector<Value> replaced;
    replaced.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      replaced.push_back(
          kept[i] ? Value(rt, args[i])
                  : RestoreJSValue(
                        rt, patched.GetProperty(static_cast<uint32_t>(i))));
    }
    rewritten = std::move(replaced);
  }
  call.SetMock(std::move(decision.mock));
  return owned;
}

ModuleInterceptorResult HostScriptModuleInterceptor::InterceptModuleMethod(
    const std::shared_ptr<LynxModule>& module,
    const LynxModule::MethodMetadata& method, Runtime* rt,
    const std::shared_ptr<ModuleDelegate>& delegate, const Value* args,
    size_t count, const std::unique_ptr<pub::Value>& pub_args,
    const CallbackMap& callbacks, NativeModuleInfoCollectorPtr timing_collector,
    ModuleCallContext* context) const {
  auto* call = GetCall(context);
  if (!call || !call->mocked()) return {false, Value::null()};
  auto mock = call->TakeMock();

  // Each callback runs at most once, in a later JS task, so it never runs
  // before this invocation has returned and been recorded.
  std::unordered_set<int64_t> posted;
  auto deliveries = mock.GetProperty(kCallbacks);
  for (int i = 0; deliveries.IsArray() && i < deliveries.GetLength(); ++i) {
    auto delivery = deliveries.GetProperty(i);
    auto index =
        static_cast<int64_t>(delivery.GetProperty(kArgumentIndex).Number());
    auto entry = callbacks.find(index);
    if (entry == callbacks.end() || !posted.insert(index).second) continue;
    auto callback = std::static_pointer_cast<ModuleCallback>(entry->second);
    callback->SetArgs(
        std::make_unique<pub::ValueImplLepus>(delivery.GetProperty(kArgs)));
    if (callback->timing_collector_) {
      callback->timing_collector_->CallbackThreadSwitchStart();
    }
    delegate->CallJSCallback(callback, nullptr,
                             ModuleCallback::kInvalidCallbackId,
                             /*force_async=*/true);
  }
  for (const auto& [index, callback] : callbacks) {
    if (!posted.count(index)) {
      delegate->CallJSCallback(nullptr, nullptr, callback->CallbackId());
    }
  }
  return {true, mock.Contains(kReturnValue)
                    ? RestoreJSValue(*rt, mock.GetProperty(kReturnValue))
                    : Value::undefined()};
}

bool HostScriptModuleInterceptor::AfterInvokeMethod(Runtime* runtime,
                                                    ModuleCallContext* context,
                                                    Value& result) {
  auto* call = GetCall(context);
  if (!call || call->mocked()) return false;
  auto handler = call->Provider(Kind::kResult);
  if (!handler) return false;
  auto& rt = *runtime;
  auto copy = SnapshotJSValue(rt, result);
  if (!copy) return false;
  auto event = call->Event();
  event.SetProperty(kValue, std::move(*copy));
  auto decision = handler->Dispatch(Kind::kResult, event);
  if (!decision.failed && decision.patch.IsTable() &&
      decision.patch.Contains(kValue)) {
    result = RestoreJSValue(rt, decision.patch.GetProperty(kValue));
    return true;
  }
  return false;
}

void HostScriptModuleInterceptor::OnInvokeMethodEnd(
    ModuleCallContext* context, const std::shared_ptr<ModuleDelegate>& delegate,
    const CallbackMap& callbacks, bool args_converted) {
  // A mock whose arguments failed to convert never reaches interception.
  // Nothing will invoke the callbacks registered before the failure.
  auto* call = GetCall(context);
  if (!call || !call->mocked()) return;
  call->TakeMock();
  if (args_converted) return;
  for (const auto& [index, callback] : callbacks) {
    delegate->CallJSCallback(nullptr, nullptr, callback->CallbackId());
  }
}

void HostScriptModuleInterceptor::WillInvokeCallback(
    ModuleCallback& callback, std::unique_ptr<pub::Value>& args) {
  auto* call = GetCall(callback.call_context().get());
  if (!call || call->mocked()) return;
  auto handler = call->Provider(Kind::kCallback);
  if (!handler || !args->IsArray() || !CanSnapshot(*args)) return;
  auto event = call->Event();
  event.SetProperty(kArgumentIndex, lepus::Value(callback.argument_index()));
  event.SetProperty(kArgs, pub::ValueUtils::ConvertValueToLepusValue(*args));
  auto patched = Patched(handler->Dispatch(Kind::kCallback, event), kArgs);
  if (!patched.IsNil()) {
    args = std::make_unique<pub::ValueImplLepus>(std::move(patched));
  }
}

}  // namespace lynx::shell
