// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_interceptor.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/shell/host_script/lynx_view/lynx_view_registry.h"
#include "core/shell/host_script/runtime/host_script_lynx_views.h"

namespace lynx::shell {
namespace {
using Kind = InterceptKind;
using Value = lepus::Value;
BASE_STATIC_STRING_DECL(kKind, "kind");
BASE_STATIC_STRING_DECL(kBytes, "bytes");
BASE_STATIC_STRING_DECL(kSource, "source");
BASE_STATIC_STRING_DECL(kArgs, "args");
BASE_STATIC_STRING_DECL(kCallbacks, "callbacks");
BASE_STATIC_STRING_DECL(kCallbackIndices, "callbackIndices");
BASE_STATIC_STRING_DECL(kArgumentIndex, "argumentIndex");
BASE_STATIC_STRING_DECL(kAction, "action");
BASE_STATIC_STRING_DECL(kPayload, "payload");
BASE_STATIC_STRING_DECL(kRequest, "request");
BASE_STATIC_STRING_DECL(kViewId, "viewId");
BASE_STATIC_STRING_DECL(kMethods, "methods");
BASE_STATIC_STRING_DECL(kUrl, "url");
BASE_STATIC_STRING_DECL(kWidth, "width");
BASE_STATIC_STRING_DECL(kHeight, "height");

enum class Action { kInvalid, kProceed, kMock, kNext };

bool SupportsNext(Kind kind) {
  return kind == Kind::kResolveModule || kind == Kind::kCall ||
         kind == Kind::kResult || kind == Kind::kCallback;
}

// Creation notifications are available in every domain. Decisions stay on
// the domain owning the intercepted operation.
bool Accepts(Interceptor::Domain domain, Kind kind) {
  if (kind == Kind::kCreated) return true;
  switch (domain) {
    case Interceptor::Domain::kUI:
      return kind == Kind::kCreate || kind == Kind::kLoadTemplate ||
             kind == Kind::kUpdateMetaData || kind == Kind::kUpdateViewport ||
             kind == Kind::kResourceFetch;
    case Interceptor::Domain::kBTS:
      return SupportsNext(kind);
    default:
      return false;
  }
}

// Operation events carry the operation in `request`; JSB events are flat.
bool HasRequest(Kind kind) {
  return kind == Kind::kCreate || kind == Kind::kLoadTemplate ||
         kind == Kind::kUpdateMetaData || kind == Kind::kUpdateViewport ||
         kind == Kind::kResourceFetch;
}

Napi::Value MakeDecision(const Napi::CallbackInfo& info) {
  auto name = *static_cast<const char**>(info.Data());
  auto decision = Napi::Object::New(info.Env());
  decision.Set("action", Napi::String::New(info.Env(), name));
  // An explicit undefined argument is the same decision as no argument.
  decision.Set("payload", info.Length() && !info[0].IsUndefined()
                              ? info[0]
                              : Napi::Object::New(info.Env()));
  return decision;
}

Napi::Object CreateChain(Napi::Env env, Kind kind) {
  static const char* actions[] = {"proceed", "mock"};
  static const char* next = "next";
  auto chain = Napi::Object::New(env);
  for (auto& action : actions) {
    chain.Set(action, Napi::Function::New(env, MakeDecision, action, &action));
  }
  if (SupportsNext(kind))
    chain.Set(next, Napi::Function::New(env, MakeDecision, next, &next));
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
                            std::vector<Napi::Value>& parents) {
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
    return Value(input.As<Napi::Number>().DoubleValue());
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
    if (input.StrictEquals(parent)) return std::nullopt;

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

bool IsInteger(const Value& value, double min, double max) {
  if (!value.IsNumber()) return false;
  double number = value.Number();
  return number >= min && number <= max && std::floor(number) == number;
}

bool IsPositive(const Value& value) {
  return value.IsNumber() && std::isfinite(value.Number()) &&
         value.Number() > 0;
}

// Android MeasureSpec encodes the mode in the top two bits of an int32.
bool IsMeasureSpec(const Value& value) {
  return IsInteger(value, -2147483648.0, 2147483647.0) &&
         (static_cast<uint32_t>(static_cast<int32_t>(value.Number())) >> 30) !=
             3;
}

bool ValidSize(const Value& value, bool measure_spec) {
  return value.IsTable() && value.GetLength() == 2 &&
         (measure_spec
              ? IsMeasureSpec(value.GetProperty(kWidth)) &&
                    IsMeasureSpec(value.GetProperty(kHeight))
              : IsInteger(value.GetProperty(kWidth), 1, 2147483647.0) &&
                    IsInteger(value.GetProperty(kHeight), 1, 2147483647.0));
}

bool ValidStringRecord(const Value& value) {
  if (!value.IsTable()) return false;
  for (const auto& item : *value.Table())
    if (!item.second.IsString()) return false;
  return true;
}

bool ValidCreateField(const std::string& key, const Value& value,
                      const Value& original) {
  if (key == "threadStrategy") return IsInteger(value, 0, 3);
  if (key == "enableJSRuntime" || key == "enableAirStrictMode" ||
      key == "enableMTSModule" || key == "forceDarkAllowed")
    return value.IsBool();
  if (key == "presetMeasuredSpec") return ValidSize(value, true);
  if (key == "screenSize") return ValidSize(value, false);
  if (key == "density" || key == "fontScale") return IsPositive(value);
  if (key == "lynxViewConfig") return ValidStringRecord(value);
  // Read-only; tolerated unchanged so a spread request remains a valid patch.
  if (key == "deviceScreenSize") return value == original.GetProperty(key);
  return false;
}

bool ValidPatchField(Kind kind, const std::string& key, const Value& value,
                     const Value& original) {
  switch (kind) {
    case Kind::kLoadTemplate:
      if (key == "url") return value.IsString() && !value.StdString().empty();
      if (key == "initialData" || key == "globalProps") return IsData(value);
      if (key == "source" && value.IsTable()) {
        auto tag = value.GetProperty(kKind);
        if (!tag.IsString()) return false;
        auto type = tag.StdString();
        return (type == "url" && value.GetLength() == 1) ||
               (type == "bytes" && value.GetLength() == 2 &&
                value.GetProperty(kBytes).IsByteArray() &&
                value.GetProperty(kBytes).ByteArray()->GetLength() > 0) ||
               (value == original.GetProperty(kSource));
      }
      return false;
    case Kind::kUpdateMetaData:
      return (key == "updateData" || key == "globalProps") && IsData(value);
    case Kind::kCall:
      if (key == "context") return true;  // Already validated/copied by FromJS.
      if (key == "method")
        return value.IsString() && !value.StdString().empty();
      if (key != "args" || !value.IsArray() ||
          value.GetLength() != original.GetProperty(kArgs).GetLength())
        return false;
      for (const char* field : {"callbackIndices", "opaqueIndices"}) {
        const auto& indices = original.GetProperty(field);
        for (int i = 0; indices.IsArray() && i < indices.GetLength(); ++i) {
          auto index = static_cast<uint32_t>(indices.GetProperty(i).Number());
          if (value.GetProperty(index) !=
              original.GetProperty(kArgs).GetProperty(index))
            return false;
        }
      }
      return true;
    case Kind::kResult:
      return key == "value";
    case Kind::kCallback:
      return key == "args" && value.IsArray();
    case Kind::kCreate:
      return ValidCreateField(key, value, original);
    case Kind::kUpdateViewport:
      return (key == "width" || key == "height") && IsMeasureSpec(value);
    case Kind::kResourceFetch:
      return key == "url" && value.IsString() && !value.StdString().empty();
    default:
      return false;
  }
}

// A fetch answered by the script: exactly the nonempty resource bytes.
bool ValidResourceMock(const Value& mock) {
  return mock.IsTable() && mock.GetLength() == 1 &&
         mock.GetProperty(kBytes).IsByteArray() &&
         mock.GetProperty(kBytes).ByteArray()->GetLength() > 0;
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
  if (!mock.Contains(kCallbacks)) return true;
  auto callbacks = mock.GetProperty(kCallbacks);
  if (!callbacks.IsArray()) return false;
  auto indices = event.GetProperty(kCallbackIndices);
  std::unordered_set<int> delivered_indices;
  for (int i = 0; i < callbacks.GetLength(); ++i) {
    auto callback = callbacks.GetProperty(i);
    if (!callback.IsTable() || callback.GetLength() != 2 ||
        !callback.GetProperty(kArgs).IsArray())
      return false;
    bool found = false;
    for (int j = 0; j < indices.GetLength(); ++j)
      found |= callback.GetProperty(kArgumentIndex) == indices.GetProperty(j);
    if (!found) return false;
    // A callback registration is consumed by one delivery.
    auto index =
        static_cast<int>(callback.GetProperty(kArgumentIndex).Number());
    if (!delivered_indices.insert(index).second) return false;
  }
  return true;
}

bool ValidModuleMock(const Value& mock) {
  if (!mock.IsTable() || mock.GetLength() != 1) return false;
  const auto& methods = mock.GetProperty(kMethods);
  if (!methods.IsArray() || methods.GetLength() == 0) return false;
  for (int i = 0; i < methods.GetLength(); ++i) {
    const auto& name = methods.GetProperty(i);
    if (!name.IsString() || name.StdString().empty()) return false;
  }
  return true;
}

Action ReadDecision(Napi::Value returned, Kind kind, const Value& original,
                    Value* payload) {
  std::vector<Napi::Value> parents;
  auto decision = FromJS(returned, parents);
  if (!decision || !decision->IsTable()) return Action::kInvalid;
  auto action = decision->GetProperty(kAction);
  if (!action.IsString()) return Action::kInvalid;
  *payload = decision->GetProperty(kPayload);
  auto name = action.StdString();
  if (name == "next" && SupportsNext(kind) && payload->IsTable() &&
      payload->GetLength() == 0)
    return Action::kNext;
  if (name == "mock") {
    const bool valid =
        (kind == Kind::kCall && ValidMock(*payload, original)) ||
        (kind == Kind::kResourceFetch && ValidResourceMock(*payload)) ||
        (kind == Kind::kResolveModule && ValidModuleMock(*payload));
    return valid ? Action::kMock : Action::kInvalid;
  }
  if (name != "proceed") return Action::kInvalid;
  // Explicit undefined fields mean omission, except that a result value may
  // be replaced with undefined.
  if (kind != Kind::kResult && payload->IsTable()) {
    std::vector<base::String> omitted;
    for (const auto& item : *payload->Table())
      if (item.second.IsUndefined()) omitted.push_back(item.first);
    for (const auto& key : omitted) payload->Table()->Erase(key);
  }
  if (!ValidPatch(kind, *payload, original)) return Action::kInvalid;
  return Action::kProceed;
}
}  // namespace

std::function<void(InterceptCompletion)>
HostScriptInterceptor::WaitForCallbacks(Napi::Array promises,
                                        const Value& event, uint64_t handler,
                                        bool mock, uint64_t last_handler,
                                        const std::string& view_url) {
  // Attach then only after the caller supplies its completion. No early-result
  // buffering or separate subscription lifetime is needed.
  auto refs = std::make_shared<Napi::Reference<Napi::Array>>(
      Napi::Persistent(promises));
  return [weak = weak_from_this(), refs, event, handler, last_handler, view_url,
          mock](InterceptCompletion complete) mutable {
    if (!refs) return;
    auto self = weak.lock();
    if (!self || !self->IsAttached()) {
      InterceptResult result;
      result.failed = result.cancelled = true;
      complete(std::move(result));
      refs.reset();
      return;
    }
    Napi::ContextScope context(self->env_);
    Napi::HandleScope scope(self->env_);
    auto promises = refs->Value().As<Napi::Array>();
    for (uint32_t i = 0; i < promises.Length(); ++i) {
      uint64_t id = self->next_pending_++;
      self->pending_.emplace(
          id, Pending{complete, event, handler, last_handler, view_url, mock});
      bool registered = false;
      for (const auto& entries : self->entries_)
        for (const auto& entry : entries) registered |= entry->id == handler;
      if (!registered) {
        self->Complete(id, self->env_.Undefined(), true, true);
        continue;
      }
      self->then_.Value().Call(
          promises.Get(i),
          {self->Bind(self->env_, "fulfilled", Settled, id * 2),
           self->Bind(self->env_, "rejected", Settled, id * 2 + 1)});
      if (self->env_.IsExceptionPending()) {
        self->env_.GetAndClearPendingException();
        self->Complete(id, self->env_.Undefined(), true);
      }
    }
    refs.reset();
  };
}

void HostScriptInterceptor::Complete(uint64_t id, Napi::Value value,
                                     bool failed, bool cancelled) {
  auto found = pending_.find(id);
  if (found == pending_.end()) return;
  auto pending = std::move(found->second);
  pending_.erase(found);
  InterceptResult result;
  result.failed = failed || !IsAttached();
  result.cancelled = cancelled;
  if (!result.failed) {
    if (pending.mock) {
      std::vector<Napi::Value> parents;
      auto data = FromJS(value, parents);
      auto callbacks = lepus::CArray::Create();
      if (data) callbacks->push_back(*data);
      result.mock = Value(lepus::Dictionary::Create());
      result.mock.SetProperty(kCallbacks, Value(callbacks));
      result.failed = !data || !ValidMock(result.mock, pending.event);
    } else {
      const auto action =
          ReadDecision(value, Kind::kCallback, pending.event, &result.patch);
      if (action == Action::kNext) {
        result =
            DispatchFrom(Kind::kCallback, pending.event, pending.handler,
                         pending.last_handler, std::move(pending.view_url));
      } else {
        result.failed = action != Action::kProceed;
      }
    }
  }
  if (env_.IsExceptionPending()) {
    env_.GetAndClearPendingException();
    result.failed = true;
  }
  if (result.pending && !result.failed) {
    result.pending(std::move(pending.complete));
  } else {
    pending.complete(std::move(result));
  }
}

Napi::Value HostScriptInterceptor::Settled(const Napi::CallbackInfo& info) {
  auto* data = static_cast<CallbackData*>(info.Data());
  if (auto self = data->provider.lock())
    self->Complete(data->id / 2,
                   info.Length() ? info[0] : info.Env().Undefined(),
                   data->id % 2);
  return info.Env().Undefined();
}

void HostScriptInterceptor::CancelPending(uint64_t handler,
                                          const std::string& view) {
  std::vector<uint64_t> ids;
  for (const auto& [id, pending] : pending_)
    if ((!handler || pending.handler == handler) &&
        (view.empty() || pending.event.GetProperty(kViewId) == Value(view)))
      ids.push_back(id);
  for (auto id : ids) Complete(id, env_.Undefined(), true, true);
}

bool HostScriptInterceptor::Supports(Domain domain) {
  for (size_t i = 0; i < static_cast<size_t>(Kind::kCount); ++i)
    if (Accepts(domain, static_cast<Kind>(i))) return true;
  return false;
}

std::shared_ptr<HostScriptInterceptor> HostScriptInterceptor::Install(
    Napi::Env env, Domain domain, Reporter reporter, Guard guard,
    std::shared_ptr<HostScriptLynxViews> views) {
  Napi::ContextScope context_scope(env);
  Napi::HandleScope scope(env);
  Napi::Env napi(env);
  if (!Supports(domain) || napi.Global().Has("interceptor")) return nullptr;
  auto provider = std::shared_ptr<HostScriptInterceptor>(
      new HostScriptInterceptor(env, domain, std::move(reporter)));
  auto promise = napi.Global().Get("Promise").As<Napi::Object>();
  provider->then_ = Napi::Persistent(promise.Get("prototype")
                                         .As<Napi::Object>()
                                         .Get("then")
                                         .As<Napi::Function>());
  provider->guard_ = std::move(guard);
  provider->views_ = std::move(views);
  if (!Interceptor::Attach(provider, domain)) return nullptr;
  if (provider->views_) {
    provider->views_->SetDestroyedHandler(
        [weak = std::weak_ptr<HostScriptInterceptor>(provider)](
            base::LynxEntityId view) {
          if (auto self = weak.lock())
            self->CancelPending(0, std::to_string(view));
        });
    provider->views_->SetCreatedHandler(
        [weak = std::weak_ptr<HostScriptInterceptor>(provider)](
            base::LynxEntityId view) {
          if (auto self = weak.lock()) self->NotifyCreated(view);
        });
  }
  auto api = Napi::Object::New(env);
  api.Set("on", provider->Bind(env, "on", On));
  provider->api_ = Napi::Persistent(api);
  napi.Global().Set("interceptor", api);
  return provider;
}

void HostScriptInterceptor::Uninstall() {
  if (!active_) return;
  Napi::ContextScope context_scope(env_);
  Napi::HandleScope scope(env_);
  active_ = false;
  Interceptor::Detach(this);
  for (auto& entries : entries_) {
    for (const auto& entry : entries) HandlerRemoved(entry->kind, domain_);
    entries.clear();
  }
  auto global = Napi::Env(env_).Global();
  if (global.Get("interceptor").StrictEquals(api_.Value()))
    global.Delete("interceptor");
  api_.Reset();
  CancelPending();
  then_.Reset();
}

Napi::Function HostScriptInterceptor::Bind(
    Napi::Env env, const char* name,
    Napi::Value (*callback)(const Napi::CallbackInfo&), uint64_t id) {
  // JS may retain a callback after Uninstall and provider destruction.
  auto* data = new CallbackData{weak_from_this(), id};
  auto function = Napi::Function::New(env, callback, name, data);
  function.AddFinalizer(data, [](auto, void* data, void*) {
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
    auto candidate = static_cast<Kind>(i);
    if (name == Name(candidate)) kind = candidate;
  }
  if (kind == Kind::kCount || !Accepts(domain_, kind)) {
    Napi::TypeError::New(env,
                         "Unsupported interceptor kind or execution domain")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  auto& entries = entries_[static_cast<size_t>(kind)];
  if (!SupportsNext(kind) && !entries.empty()) {
    Napi::Error::New(
        env, ("Interceptor handler already registered for " + name).c_str())
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  auto entry = std::make_shared<Entry>();
  entry->id = next_id_++;
  entry->kind = kind;
  entry->function = Napi::Persistent(info[1].As<Napi::Function>());
  entries.push_back(entry);
  HandlerAdded(kind, domain_);
  if (kind == Kind::kCreated && views_) {
    // Replay later, so the handler never runs inside its own registration.
    views_->Post([weak = weak_from_this(), id = entry->id] {
      auto self = weak.lock();
      if (!self) return;
      for (const auto& view : LynxViewRegistry::GetInstance().List())
        self->NotifyCreated(view.view, id);
    });
  }
  auto result = Napi::Object::New(env);
  result.Set("dispose", Bind(env, "dispose", Dispose, entry->id));
  return result;
}

void HostScriptInterceptor::Remove(uint64_t id) {
  for (auto& entries : entries_) {
    auto found =
        std::find_if(entries.begin(), entries.end(),
                     [id](const auto& entry) { return entry->id == id; });
    if (found == entries.end()) continue;
    HandlerRemoved((*found)->kind, domain_);
    (*found)->active = false;
    entries.erase(found);
    CancelPending(id);
    return;
  }
}

bool HostScriptInterceptor::HasHandlers(Kind kind) const {
  return active_ && !entries_[static_cast<size_t>(kind)].empty();
}

void HostScriptInterceptor::ReportError(const std::string& message) {
  LOGE("Host Script interceptor: " << message);
  if (reporter_) reporter_(message);
}

void HostScriptInterceptor::NotifyCreated(base::LynxEntityId view,
                                          uint64_t id) {
  if (!active_ || !IsAttached() || !views_) return;
  const auto& entries = entries_[static_cast<size_t>(Kind::kCreated)];
  if (entries.empty()) return;
  auto entry = entries.front();
  if (id && entry->id != id) return;
  Napi::Env env(env_);
  Napi::ContextScope context_scope(env);
  Napi::HandleScope scope(env);
  // A View destroyed before delivery is skipped rather than handed out stale.
  auto proxy = views_->Get(view);
  if (proxy.IsNull() || !entry->created.insert(view).second) return;
  entry->function.Value().Call({proxy});
  if (env.IsExceptionPending()) {
    auto exception = env.GetAndClearPendingException();
    auto text = exception.ToString();
    ReportCoverageGap(Kind::kCreated, env.IsExceptionPending()
                                          ? "view.created handler failed"
                                          : text.Utf8Value().c_str());
    if (env.IsExceptionPending()) env.GetAndClearPendingException();
  }
}

std::function<void(InterceptCompletion)> HostScriptInterceptor::ReadCallbacks(
    Napi::Value& returned, Kind kind, const Value& event, uint64_t handler,
    uint64_t last_handler, const std::string& view_url) {
  if (env_.IsExceptionPending() || returned.IsEmpty()) return {};
  auto promises = Napi::Array::New(env_);
  if (kind == Kind::kCallback && returned.IsPromise()) {
    promises.Set(uint32_t{0}, returned);
    return WaitForCallbacks(promises, event, handler, false, last_handler,
                            view_url);
  }
  if (kind != Kind::kCall || !returned.IsObject()) return {};
  auto decision = returned.As<Napi::Object>();
  auto action = decision.Get("action");
  if (env_.IsExceptionPending()) return {};
  auto payload = decision.Get("payload");
  if (env_.IsExceptionPending() || !action.IsString() ||
      action.As<Napi::String>().Utf8Value() != "mock" || !payload.IsObject())
    return {};
  auto mock = payload.As<Napi::Object>();
  auto callbacks = mock.Get("callbacks");
  if (env_.IsExceptionPending() || !callbacks.IsArray()) return {};
  auto array = callbacks.As<Napi::Array>();
  auto immediate = Napi::Array::New(env_);
  for (uint32_t i = 0; i < array.Length(); ++i) {
    auto item = array.Get(i);
    if (env_.IsExceptionPending()) return {};
    auto target = item.IsPromise() ? promises : immediate;
    target.Set(target.Length(), item);
  }
  auto copy = Napi::Object::New(env_);
  auto keys = mock.GetPropertyNames();
  if (env_.IsExceptionPending()) return {};
  for (uint32_t i = 0; i < keys.Length(); ++i) {
    auto key = keys.Get(i);
    auto field = mock.Get(key);
    if (env_.IsExceptionPending()) return {};
    copy.Set(key, field);
  }
  copy.Set("callbacks", immediate);
  auto replacement = Napi::Object::New(env_);
  replacement.Set("action", action);
  replacement.Set("payload", copy);
  returned = replacement;
  return promises.Length() ? WaitForCallbacks(promises, event, handler, true)
                           : std::function<void(InterceptCompletion)>();
}

InterceptResult HostScriptInterceptor::Dispatch(Kind kind, const Value& event) {
  return DispatchFrom(kind, event, 0, 0, {});
}

InterceptResult HostScriptInterceptor::DispatchFrom(Kind kind,
                                                    const Value& event,
                                                    uint64_t after_handler,
                                                    uint64_t last_handler,
                                                    std::string view_url) {
  InterceptResult result;
  if (!active_ || !IsAttached()) return result;
  // Creation is delivered by NotifyCreated with a View proxy.
  if (kind == Kind::kCreated || !HasHandlers(kind)) return result;
  if (dispatching_) {
    ReportCoverageGap(kind, "Reentrant interceptor dispatch is not supported");
    result.failed = true;
    return result;
  }
  dispatching_ = true;
  Napi::Env env(env_);
  Napi::ContextScope context_scope(env);
  Napi::HandleScope scope(env);
  // Registration can change from a handler. Hold the original order and
  // release its references while the owning NAPI context is entered.
  const auto entries = entries_[static_cast<size_t>(kind)];
  if (last_handler == 0) last_handler = entries.back()->id;
  const Value& original =
      HasRequest(kind) ? event.GetProperty(kRequest) : event;
  const auto view_id = event.GetProperty(kViewId);
  const auto view = view_id.IsString()
                        ? static_cast<base::LynxEntityId>(std::strtoll(
                              view_id.StdString().c_str(), nullptr, 10))
                        : base::kUnavailableLynxEntityId;
  if (kind == Kind::kLoadTemplate) {
    const auto url = original.GetProperty(kUrl);
    if (url.IsString()) view_url = url.StdString();
  } else if (after_handler == 0) {
    view_url = ViewUrl(view);
  }
  auto proxy = views_ ? views_->Get(view) : env.Null();
  bool failed = false;
  for (const auto& entry : entries) {
    if (!active_ || !IsAttached()) break;
    if (entry->id > last_handler) break;
    if (!entry->active || entry->id <= after_handler) continue;
    Napi::HandleScope handler_scope(env);
    auto snapshot = ToJS(env, event).As<Napi::Object>();
    snapshot.DefineProperty(Napi::PropertyDescriptor::Value(
        "viewURL", Napi::String::New(env, view_url), napi_enumerable));
    auto returned =
        entry->function.Value().Call({snapshot, CreateChain(env, kind), proxy});
    result.pending =
        ReadCallbacks(returned, kind, event, entry->id, last_handler, view_url);
    failed = returned.IsEmpty() || env.IsExceptionPending() ||
             (returned.IsPromise() && !result.pending);
    if (failed || returned.IsPromise()) break;

    Value patch;
    auto action = ReadDecision(returned, kind, original, &patch);
    if (action == Action::kNext) continue;
    if (action == Action::kInvalid) {
      failed = true;
    } else if (action == Action::kMock) {
      if (kind != Kind::kResolveModule || (active_ && entry->active)) {
        result.mock = std::move(patch);
        if (kind == Kind::kResolveModule) result.subscription = entry;
      }
    } else if (patch.GetLength()) {
      result.patch = std::move(patch);
    }
    break;
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
  }
  return result;
}
}  // namespace lynx::shell
