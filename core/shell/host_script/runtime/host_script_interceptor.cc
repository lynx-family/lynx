// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_interceptor.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"

namespace lynx::shell {
namespace {
using Kind = InterceptKind;
using Value = lepus::Value;

enum class Action { kInvalid, kNext, kProceed, kMock };

Napi::Value MakeDecision(const Napi::CallbackInfo& info) {
  auto name = *static_cast<const char**>(info.Data());
  auto decision = Napi::Object::New(info.Env());
  decision.Set("action", Napi::String::New(info.Env(), name));
  decision.Set("payload",
               info.Length() ? info[0] : Napi::Object::New(info.Env()));
  return decision;
}

Napi::Object CreateChain(Napi::Env env) {
  static const char* actions[] = {"next", "proceed", "mock"};
  auto chain = Napi::Object::New(env);
  for (auto& action : actions) {
    chain.Set(action, Napi::Function::New(env, MakeDecision, action, &action));
  }
  return chain;
}

Napi::Value ToJS(Napi::Env env, const Value& value) {
  if (value.IsUndefined()) return env.Undefined();
  if (value.IsNil()) return env.Null();
  if (value.IsBool()) return Napi::Boolean::New(env, value.Bool());
  if (value.IsNumber()) return Napi::Number::New(env, value.Number());
  if (value.IsString()) return Napi::String::New(env, value.StdString());
  if (value.IsByteArray()) {
    auto bytes = value.ByteArray();
    auto result = Napi::ArrayBuffer::New(env, bytes->GetLength());
    memcpy(result.Data(), bytes->GetPtr(), bytes->GetLength());
    return result;
  }
  if (value.IsArray()) {
    auto result = Napi::Array::New(env, value.GetLength());
    for (int i = 0; i < value.GetLength(); ++i)
      result.Set(i, ToJS(env, value.GetProperty(static_cast<uint32_t>(i))));
    return result;
  }
  auto result = Napi::Object::New(env);
  if (value.IsTable()) {
    for (const auto& item : *value.Table())
      result.Set(item.first.c_str(), ToJS(env, item.second));
  }
  return result;
}

// Copy data between VMs without sharing JS references. Reject executable,
// cyclic and exotic objects instead of silently serializing them to {}.
std::optional<Value> FromJS(Napi::Value input,
                            std::vector<napi_value>& parents) {
  if (input.IsEmpty()) return std::nullopt;
  auto env = input.Env();
  if (env.IsExceptionPending() || parents.size() >= 64) return std::nullopt;
  if (input.IsUndefined()) {
    Value value;
    value.SetUndefined();
    return value;
  }
  if (input.IsNull()) return Value();
  if (input.IsBoolean()) return Value(input.As<Napi::Boolean>().Value());
  if (input.IsNumber()) {
    double number = input.As<Napi::Number>().DoubleValue();
    return std::isfinite(number) ? std::optional<Value>(Value(number))
                                 : std::nullopt;
  }
  if (input.IsString()) return Value(input.As<Napi::String>().Utf8Value());
  if (input.IsArrayBuffer()) {
    auto buffer = input.As<Napi::ArrayBuffer>();
    auto bytes = std::make_unique<uint8_t[]>(buffer.ByteLength());
    memcpy(bytes.get(), buffer.Data(), buffer.ByteLength());
    return Value(
        lepus::ByteArray::Create(std::move(bytes), buffer.ByteLength()));
  }
  if (!input.IsObject() || input.IsFunction() || input.IsPromise())
    return std::nullopt;
  for (auto parent : parents)
    if (input.StrictEquals(Napi::Value(env, parent))) return std::nullopt;

  auto object = input.As<Napi::Object>();
  const bool is_array = input.IsArray();
  if (!is_array) {
    // Only plain records (including null-prototype records) are supported.
    auto constructor = env.Global().Get("Object").As<Napi::Object>();
    auto prototype =
        constructor.Get("getPrototypeOf").As<Napi::Function>().Call({object});
    if (env.IsExceptionPending()) return std::nullopt;
    if (!prototype.IsNull() &&
        !prototype.StrictEquals(constructor.Get("prototype")))
      return std::nullopt;
  }
  auto keys = is_array ? Napi::Array(env, nullptr) : object.GetPropertyNames();
  if (env.IsExceptionPending()) return std::nullopt;
  uint32_t length = is_array ? input.As<Napi::Array>().Length() : keys.Length();
  Value result = is_array ? Value(lepus::CArray::Create())
                          : Value(lepus::Dictionary::Create());
  parents.push_back(input);
  for (uint32_t i = 0; i < length; ++i) {
    auto key = is_array ? std::string() : keys.Get(i).ToString().Utf8Value();
    auto child =
        FromJS(is_array ? object.Get(i) : object.Get(key.c_str()), parents);
    if (!child) return std::nullopt;
    if (is_array)
      result.Array()->push_back(std::move(*child));
    else
      result.SetProperty(std::move(key), std::move(*child));
  }
  parents.pop_back();
  return env.IsExceptionPending() ? std::nullopt : std::optional<Value>(result);
}

bool IsData(const Value& value) { return value.IsNil() || value.IsTable(); }

bool IsInteger(const Value& value, int32_t min, int32_t max) {
  return value.IsNumber() && value.Number() >= min && value.Number() <= max &&
         std::floor(value.Number()) == value.Number();
}

bool ValidPatchField(Kind kind, const std::string& key, const Value& value,
                     const Value& original) {
  switch (kind) {
    case Kind::kCreate:
      if (key == "fontScale") return value.IsNumber() && value.Number() > 0;
      if (key == "threadStrategy") return IsInteger(value, 0, 3);
      if (key == "screenSize" || key == "presetMeasuredSpec") {
        int32_t min = key == "presetMeasuredSpec" ? INT32_MIN : 0;
        return value.IsTable() && value.GetLength() == 2 &&
               IsInteger(value.GetProperty("width"), min, INT32_MAX) &&
               IsInteger(value.GetProperty("height"), min, INT32_MAX);
      }
      return key == "lynxViewConfig" && value.IsTable() &&
             std::all_of(
                 value.Table()->begin(), value.Table()->end(),
                 [](const auto& field) { return field.second.IsString(); });
    case Kind::kLoadTemplate:
      if (key == "url") return value.IsString() && !value.StdString().empty();
      if (key == "initialData" || key == "globalProps") return IsData(value);
      if (key == "source" && value.IsTable()) {
        auto tag = value.GetProperty("kind");
        if (!tag.IsString()) return false;
        auto type = tag.StdString();
        return (type == "url" && value.GetLength() == 1) ||
               (type == "bytes" && value.GetLength() == 2 &&
                value.GetProperty("bytes").IsByteArray() &&
                value.GetProperty("bytes").ByteArray()->GetLength() > 0) ||
               (value == original.GetProperty("source"));
      }
      return false;
    case Kind::kUpdateMetaData:
      return (key == "updateData" || key == "globalProps") && IsData(value);
    case Kind::kCall:
      if (key != "args" || !value.IsArray() ||
          value.GetLength() != original.GetProperty("args").GetLength())
        return false;
      for (const char* field : {"callbackIndices", "opaqueIndices"}) {
        const auto& indices = original.GetProperty(field);
        for (int i = 0; indices.IsArray() && i < indices.GetLength(); ++i) {
          auto index = static_cast<uint32_t>(indices.GetProperty(i).Number());
          if (value.GetProperty(index) !=
              original.GetProperty("args").GetProperty(index))
            return false;
        }
      }
      return true;
    case Kind::kResult:
      return key == "value";
    case Kind::kCallback:
      return key == "args" && value.IsArray();
    default:
      return false;
  }
}

bool ValidPatch(Kind kind, const Value& patch, const Value& original) {
  if (!patch.IsTable()) return false;
  for (const auto& item : *patch.Table())
    if (!ValidPatchField(kind, item.first.str(), item.second, original))
      return false;
  return true;
}

bool ValidMock(const Value& mock, const Value& event) {
  if (!mock.IsTable()) return false;
  for (const auto& field : *mock.Table()) {
    if (field.first.str() != "returnValue" && field.first.str() != "callbacks")
      return false;
  }
  if (!mock.Contains("callbacks")) return true;
  auto callbacks = mock.GetProperty("callbacks");
  if (!callbacks.IsArray()) return false;
  auto indices = event.GetProperty("callbackIndices");
  for (int i = 0; i < callbacks.GetLength(); ++i) {
    auto callback = callbacks.GetProperty(i);
    if (!callback.IsTable() || callback.GetLength() != 2 ||
        !callback.GetProperty("args").IsArray())
      return false;
    bool found = false;
    for (int j = 0; j < indices.GetLength(); ++j)
      found |= callback.GetProperty("argumentIndex") == indices.GetProperty(j);
    if (!found) return false;
  }
  return true;
}

Action ReadDecision(Napi::Value returned, Kind kind, const Value& original,
                    Value* payload) {
  std::vector<napi_value> parents;
  auto decision = FromJS(returned, parents);
  if (!decision || !decision->IsTable()) return Action::kInvalid;
  auto action = decision->GetProperty("action");
  if (!action.IsString()) return Action::kInvalid;
  *payload = decision->GetProperty("payload");
  auto name = action.StdString();
  if (name == "mock") {
    return kind == Kind::kCall && ValidMock(*payload, original)
               ? Action::kMock
               : Action::kInvalid;
  }
  if ((name != "next" && name != "proceed") ||
      !ValidPatch(kind, *payload, original))
    return Action::kInvalid;
  return name == "next" ? Action::kNext : Action::kProceed;
}
}  // namespace

std::shared_ptr<HostScriptInterceptor> HostScriptInterceptor::Install(
    napi_env env, Domain domain, Reporter reporter, Guard guard) {
  Napi::ContextScope context_scope(env);
  Napi::Env napi(env);
  if (napi.Global().Has("interceptor")) return nullptr;
  auto provider = std::shared_ptr<HostScriptInterceptor>(
      new HostScriptInterceptor(env, domain, std::move(reporter)));
  provider->guard_ = std::move(guard);
  if (!Interceptor::Attach(provider, domain)) return nullptr;
  auto api = Napi::Object::New(env);
  api.Set("on", provider->Bind(env, "on", On));
  provider->api_ = Napi::Persistent(api);
  napi.Global().Set("interceptor", api);
  return provider;
}

void HostScriptInterceptor::Uninstall() {
  if (!active_) return;
  Napi::ContextScope context_scope(env_);
  active_ = false;
  Interceptor::Detach(this);
  for (const auto& entry : entries_) HandlerRemoved(entry->kind, domain_);
  entries_.clear();
  auto global = Napi::Env(env_).Global();
  if (global.Get("interceptor").StrictEquals(api_.Value()))
    global.Delete("interceptor");
  api_.Reset();
}

Napi::Function HostScriptInterceptor::Bind(
    Napi::Env env, const char* name,
    Napi::Value (*callback)(const Napi::CallbackInfo&), uint64_t id) {
  // JS may retain a callback after Uninstall and provider destruction.
  auto* data = new CallbackData{weak_from_this(), id};
  auto function = Napi::Function::New(env, callback, name, data);
  function.AddFinalizer(data, [](napi_env, void* data, void*) {
    delete static_cast<CallbackData*>(data);
  });
  return function;
}

Napi::Value HostScriptInterceptor::On(const Napi::CallbackInfo& info) {
  auto* data = static_cast<CallbackData*>(info.Data());
  if (auto self = data->provider.lock(); self && self->active_)
    return self->Add(info);
  Napi::Error::New(info.Env(), "Interceptor environment is detached")
      .ThrowAsJavaScriptException();
  return info.Env().Undefined();
}

Napi::Value HostScriptInterceptor::Dispose(const Napi::CallbackInfo& info) {
  auto* data = static_cast<CallbackData*>(info.Data());
  if (auto self = data->provider.lock(); self && self->active_)
    self->Remove(data->id);
  return info.Env().Undefined();
}

Napi::Value HostScriptInterceptor::Add(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  if (info.Length() != 2 || !info[0].IsString() || !info[1].IsFunction()) {
    Napi::TypeError::New(env, "Expected an event name and function")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  auto name = info[0].As<Napi::String>().Utf8Value();
  Kind kind = Kind::kCount;
  for (int i = 0; i < static_cast<int>(Kind::kCount); ++i) {
    if (name == Name(static_cast<Kind>(i))) kind = static_cast<Kind>(i);
  }
  const bool allowed =
      (domain_ == Domain::kUI &&
       (kind == Kind::kCreate || kind == Kind::kCreated ||
        kind == Kind::kLoadTemplate)) ||
      (domain_ == Domain::kMTS && kind == Kind::kUpdateMetaData) ||
      (domain_ != Domain::kUI &&
       (kind == Kind::kCall || kind == Kind::kResult ||
        kind == Kind::kCallback));
  if (!allowed) {
    Napi::TypeError::New(env,
                         "Unsupported interceptor kind or execution domain")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  auto entry = std::make_shared<Entry>();
  entry->id = next_id_++;
  entry->kind = kind;
  entry->function = Napi::Persistent(info[1].As<Napi::Function>());
  entries_.push_back(entry);
  HandlerAdded(kind, domain_);
  auto result = Napi::Object::New(env);
  result.Set("dispose", Bind(env, "dispose", Dispose, entry->id));
  return result;
}

void HostScriptInterceptor::Remove(uint64_t id) {
  auto entry = std::find_if(entries_.begin(), entries_.end(),
                            [id](const auto& item) { return item->id == id; });
  if (entry != entries_.end()) {
    HandlerRemoved((*entry)->kind, domain_);
    entries_.erase(entry);
  }
}
bool HostScriptInterceptor::HasHandlers(Kind kind) const {
  return active_ &&
         std::any_of(entries_.begin(), entries_.end(),
                     [kind](const auto& entry) { return entry->kind == kind; });
}
void HostScriptInterceptor::ReportError(const std::string& message) {
  LOGE("Host Script interceptor: " << message);
  if (reporter_) reporter_(message);
}

InterceptResult HostScriptInterceptor::Dispatch(Kind kind, const Value& event) {
  InterceptResult result;
  if (!active_ || !IsAttached()) return result;
  if (dispatching_) {
    ReportCoverageGap(kind, "Reentrant interceptor dispatch is not supported");
    result.failed = true;
    return result;
  }
  dispatching_ = true;
  Napi::Env env(env_);
  Napi::ContextScope context_scope(env);
  Napi::HandleScope scope(env);
  auto snapshot = entries_;
  const bool view_event = kind <= Kind::kUpdateMetaData;
  const Value& original = view_event ? event.GetProperty("request") : event;
  // Only native copies are reused. Every handler receives a fresh JS snapshot.
  Value request = Value::ShallowCopy(original);
  auto current = view_event ? Value::ShallowCopy(event) : request;
  if (view_event) current.SetProperty("request", request);
  Value merged(lepus::Dictionary::Create());
  bool failed = false;
  for (const auto& entry : snapshot) {
    if (entry->kind != kind) continue;
    auto returned =
        entry->function.Value().Call({ToJS(env, current), CreateChain(env)});
    if (env.IsExceptionPending() || returned.IsPromise()) {
      failed = true;
      break;
    }
    if (kind == Kind::kCreated) continue;
    Value patch;
    auto action = ReadDecision(returned, kind, original, &patch);
    if (action == Action::kInvalid) {
      failed = true;
      break;
    }
    if (action == Action::kMock) {
      result.mock = std::move(patch);
      break;
    }
    for (const auto& item : *patch.Table()) {
      request.SetProperty(item.first, item.second);
      merged.SetProperty(item.first, item.second);
    }
    if (action == Action::kProceed) break;
  }
  dispatching_ = false;
  if (failed) {
    std::string message = "Invalid synchronous interceptor decision";
    if (env.IsExceptionPending()) {
      auto exception = env.GetAndClearPendingException();
      auto text = exception.ToString();
      if (env.IsExceptionPending())
        env.GetAndClearPendingException();
      else
        message = text.Utf8Value();
    }
    ReportCoverageGap(kind, message.c_str());
    result = {};
    result.failed = true;
  } else if (merged.GetLength())
    result.patch = std::move(merged);
  return result;
}
}  // namespace lynx::shell
