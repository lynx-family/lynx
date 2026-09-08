// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_module.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "core/shell/host_script/runtime/host_script_session.h"

namespace lynx {
namespace shell {

using runtime::js::NapiEnvironment;

namespace {

constexpr char kModuleName[] = "host_script";
constexpr char kInvalidArgument[] = "INVALID_ARGUMENT";
constexpr char kInvalidState[] = "INVALID_STATE";

}  // namespace

HostScriptSession* HostScriptModule::SessionFromInfo(
    const Napi::CallbackInfo& info) {
  return static_cast<HostScriptSession*>(info.Data());
}

bool HostScriptModule::IsKnownEvent(const std::string& event) {
  return event == "ready" || event == "loadSuccess" || event == "firstScreen" ||
         event == "pageUpdate" || event == "dataUpdated" || event == "error" ||
         event == "destroyed";
}

Napi::Value HostScriptModule::ThrowApiError(Napi::Env env, const char* code,
                                            const std::string& message,
                                            bool type_error) {
  auto error = type_error ? Napi::TypeError::New(env, message.c_str())
                          : HostScriptError(env, code, message);
  error["code"] = Napi::String::New(env, code);
  error.ThrowAsJavaScriptException();
  return env.Undefined();
}

bool HostScriptModule::ReadOptionalString(const Napi::CallbackInfo& info,
                                          size_t index, std::string* result) {
  if (index >= info.Length() || info[index].IsUndefined()) {
    result->clear();
    return true;
  }
  if (!info[index].IsString()) {
    return false;
  }
  *result = info[index].As<Napi::String>().Utf8Value();
  return true;
}

bool HostScriptModule::ReadOptionalBoolean(const Napi::CallbackInfo& info,
                                           size_t index, bool* result) {
  if (index >= info.Length() || info[index].IsUndefined()) {
    *result = false;
    return true;
  }
  if (!info[index].IsBoolean()) {
    return false;
  }
  *result = info[index].As<Napi::Boolean>().Value();
  return true;
}

Napi::Value HostScriptModule::HasCurrent(const Napi::CallbackInfo& info) {
  auto* session = SessionFromInfo(info);
  return Napi::Boolean::New(info.Env(), session && session->HasCurrentView());
}

Napi::Value HostScriptModule::WaitForCurrent(const Napi::CallbackInfo& info) {
  auto* session = SessionFromInfo(info);
  if (!session || info.Length() != 0) {
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
    Napi::Promise promise = deferred.Promise();
    deferred.Reject(
        HostScriptError(info.Env(), session ? kInvalidArgument : kInvalidState,
                        session ? "waitForCurrent expects no arguments"
                                : "The current LynxView is not available"));
    return promise;
  }
  return session->WaitForCurrentView(info.Env());
}

Napi::Value HostScriptModule::LoadTemplate(const Napi::CallbackInfo& info) {
  auto* session = SessionFromInfo(info);
  if (!session || !session->HasCurrentView()) {
    return ThrowApiError(info.Env(), kInvalidState,
                         "The current LynxView is not available");
  }
  if (info.Length() < 2 || info.Length() > 6 ||
      (!info[0].IsUndefined() && !info[0].IsArrayBuffer()) ||
      !info[1].IsString()) {
    return ThrowApiError(info.Env(), kInvalidArgument,
                         "loadTemplate expects an optional ArrayBuffer, a "
                         "non-empty URL, and optional JSON",
                         true);
  }

  LynxViewRefLoadTemplateRequest request;
  request.has_template = info[0].IsArrayBuffer();
  if (request.has_template) {
    Napi::ArrayBuffer bytes = info[0].As<Napi::ArrayBuffer>();
    if (bytes.ByteLength() == 0 ||
        bytes.ByteLength() >
            static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
      return ThrowApiError(info.Env(), kInvalidArgument,
                           "loadTemplate expects an optional non-empty "
                           "ArrayBuffer, a non-empty URL, and optional JSON",
                           true);
    }
    auto* begin = static_cast<uint8_t*>(bytes.Data());
    request.template_data.assign(begin, begin + bytes.ByteLength());
  }
  request.url = info[1].As<Napi::String>().Utf8Value();
  if (request.url.empty() ||
      !ReadOptionalString(info, 2, &request.initial_data_json) ||
      !ReadOptionalString(info, 3, &request.global_props_json) ||
      !ReadOptionalString(info, 4, &request.processor_name) ||
      !ReadOptionalBoolean(info, 5, &request.read_only)) {
    return ThrowApiError(info.Env(), kInvalidArgument,
                         "loadTemplate expects an optional non-empty "
                         "ArrayBuffer, a non-empty URL, and optional JSON",
                         true);
  }
  return session->Dispatch(info.Env(), &LynxViewRefProxy::LoadTemplate,
                           std::move(request));
}

Napi::Value HostScriptModule::UpdateData(const Napi::CallbackInfo& info) {
  auto* session = SessionFromInfo(info);
  if (!session || !session->HasCurrentView()) {
    return ThrowApiError(info.Env(), kInvalidState,
                         "The current LynxView is not available");
  }
  if (info.Length() != 1 || !info[0].IsString()) {
    return ThrowApiError(info.Env(), kInvalidArgument,
                         "updateData expects one JSON string", true);
  }
  LynxViewRefUpdateDataRequest request;
  request.data_json = info[0].As<Napi::String>().Utf8Value();
  if (request.data_json.empty()) {
    return ThrowApiError(info.Env(), kInvalidArgument,
                         "updateData expects a non-empty JSON string", true);
  }
  return session->Dispatch(info.Env(), &LynxViewRefProxy::UpdateData,
                           std::move(request));
}

Napi::Value HostScriptModule::ReloadTemplate(const Napi::CallbackInfo& info) {
  auto* session = SessionFromInfo(info);
  if (!session || !session->HasCurrentView()) {
    return ThrowApiError(info.Env(), kInvalidState,
                         "The current LynxView is not available");
  }
  LynxViewRefReloadTemplateRequest request;
  if (info.Length() > 2 || !ReadOptionalString(info, 0, &request.data_json) ||
      !ReadOptionalString(info, 1, &request.global_props_json)) {
    return ThrowApiError(
        info.Env(), kInvalidArgument,
        "reloadTemplate expects optional data and global props JSON strings",
        true);
  }
  if (request.data_json.empty()) {
    request.data_json = "{}";
  }
  return session->Dispatch(info.Env(), &LynxViewRefProxy::ReloadTemplate,
                           std::move(request));
}

Napi::Value HostScriptModule::SendGlobalEvent(const Napi::CallbackInfo& info) {
  auto* session = SessionFromInfo(info);
  if (!session || !session->HasCurrentView()) {
    return ThrowApiError(info.Env(), kInvalidState,
                         "The current LynxView is not available");
  }
  if (info.Length() != 2 || !info[0].IsString() || !info[1].IsString()) {
    return ThrowApiError(
        info.Env(), kInvalidArgument,
        "sendGlobalEvent expects an event name and params JSON string", true);
  }
  LynxViewRefGlobalEventRequest request;
  request.name = info[0].As<Napi::String>().Utf8Value();
  request.params_json = info[1].As<Napi::String>().Utf8Value();
  if (request.name.empty() || request.params_json.empty()) {
    return ThrowApiError(
        info.Env(), kInvalidArgument,
        "sendGlobalEvent expects a non-empty event name and params JSON string",
        true);
  }
  return session->Dispatch(info.Env(), &LynxViewRefProxy::SendGlobalEvent,
                           std::move(request));
}

Napi::Value HostScriptModule::On(const Napi::CallbackInfo& info) {
  auto* state = SessionFromInfo(info);
  if (!state || !state->HasCurrentView()) {
    return ThrowApiError(info.Env(), kInvalidState,
                         "The current LynxView is not available");
  }
  if (info.Length() != 2 || !info[0].IsString() || !info[1].IsFunction()) {
    return ThrowApiError(info.Env(), kInvalidArgument,
                         "on expects an event name and listener", true);
  }
  std::string event = info[0].As<Napi::String>().Utf8Value();
  if (!IsKnownEvent(event)) {
    return ThrowApiError(info.Env(), kInvalidArgument,
                         "Unknown current LynxView event");
  }
  state->AddListener(event, info[1].As<Napi::Function>());
  return info.Env().Undefined();
}

Napi::Value HostScriptModule::Off(const Napi::CallbackInfo& info) {
  auto* session = SessionFromInfo(info);
  if (!session || !session->HasCurrentView()) {
    return ThrowApiError(info.Env(), kInvalidState,
                         "The current LynxView is not available");
  }
  if (info.Length() != 2 || !info[0].IsString() || !info[1].IsFunction()) {
    return ThrowApiError(info.Env(), kInvalidArgument,
                         "off expects an event name and listener", true);
  }
  std::string event = info[0].As<Napi::String>().Utf8Value();
  if (!IsKnownEvent(event)) {
    return ThrowApiError(info.Env(), kInvalidArgument,
                         "Unknown current LynxView event");
  }
  session->RemoveListener(event, info[1].As<Napi::Function>());
  return info.Env().Undefined();
}

Napi::Value HostScriptModule::ReportEntryResult(
    const Napi::CallbackInfo& info) {
  std::string message;
  if (info.Length() < 1 || info.Length() > 2 || !info[0].IsString() ||
      !ReadOptionalString(info, 1, &message)) {
    return ThrowApiError(
        info.Env(), kInvalidArgument,
        "reportEntryResult expects status and optional message", true);
  }
  SessionFromInfo(info)->ReportEntryResult(
      info[0].As<Napi::String>().Utf8Value(), message);
  return info.Env().Undefined();
}

// HostScriptModule
HostScriptModule::HostScriptModule(std::shared_ptr<HostScriptSession> session)
    : session_(std::move(session)) {}

bool HostScriptModule::IsLazy() { return true; }

void HostScriptModule::OnEnvDetach(Napi::Env) { session_->Detach(); }

void HostScriptModule::OnLoad(Napi::Object& target) {
  using Callback = Napi::Value (*)(const Napi::CallbackInfo&);
  const std::pair<const char*, Callback> methods[] = {
      {"reportEntryResult", ReportEntryResult},
      {"hasCurrent", HasCurrent},
      {"waitForCurrent", WaitForCurrent},
      {"loadTemplate", LoadTemplate},
      {"updateData", UpdateData},
      {"reloadTemplate", ReloadTemplate},
      {"sendGlobalEvent", SendGlobalEvent},
      {"on", On},
      {"off", Off}};
  for (const auto& method : methods) {
    target[method.first] = Napi::Function::New(target.Env(), method.second,
                                               method.first, session_.get());
  }
}

Napi::Error HostScriptModule::HostScriptError(Napi::Env env, const char* code,
                                              const std::string& message) {
  auto error = Napi::Error::New(env, message.c_str());
  error["code"] = Napi::String::New(env, code);
  return error;
}

void HostScriptModule::RegisterHostScriptModule(
    napi_env env, const std::shared_ptr<HostScriptSession>& session) {
  NapiEnvironment::From(Napi::Env(env))
      ->RegisterModule(kModuleName,
                       std::make_unique<HostScriptModule>(session));
}

}  // namespace shell
}  // namespace lynx
