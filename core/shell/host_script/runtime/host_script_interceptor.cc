// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_interceptor.h"

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

enum class Action { kInvalid, kProceed, kMock };

// Creation notifications are available in every domain. Decisions stay on
// the domain owning the intercepted operation.
bool Accepts(Interceptor::Domain domain, Kind kind) {
  if (kind == Kind::kCreated) return true;
  switch (domain) {
    case Interceptor::Domain::kUI:
      return kind == Kind::kLoadTemplate || kind == Kind::kUpdateMetaData;
    case Interceptor::Domain::kBTS:
      return kind == Kind::kCall || kind == Kind::kResult ||
             kind == Kind::kCallback;
    default:
      return false;
  }
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

Napi::Object CreateChain(Napi::Env env) {
  static const char* actions[] = {"proceed", "mock"};
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

Action ReadDecision(Napi::Value returned, Kind kind, const Value& original,
                    Value* payload) {
  std::vector<Napi::Value> parents;
  auto decision = FromJS(returned, parents);
  if (!decision || !decision->IsTable()) return Action::kInvalid;
  auto action = decision->GetProperty(kAction);
  if (!action.IsString()) return Action::kInvalid;
  *payload = decision->GetProperty(kPayload);
  auto name = action.StdString();
  if (name == "mock") {
    return kind == Kind::kCall && ValidMock(*payload, original)
               ? Action::kMock
               : Action::kInvalid;
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
                                        bool mock) {
  // Attach then only after the caller supplies its completion. No early-result
  // buffering or separate subscription lifetime is needed.
  auto refs = std::make_shared<Napi::Reference<Napi::Array>>(
      Napi::Persistent(promises));
  return [weak = weak_from_this(), refs, event, handler,
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
      self->pending_.emplace(id, Pending{complete, event, handler, mock});
      bool registered = false;
      for (const auto& entry : self->entries_)
        registered |= entry && entry->id == handler;
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
      result =
          ReadReturned(value, Kind::kCallback, pending.event, pending.handler);
    }
  }
  if (env_.IsExceptionPending()) {
    env_.GetAndClearPendingException();
    result.failed = true;
  }
  pending.complete(std::move(result));
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
  active_ = false;
  Interceptor::Detach(this);
  for (auto& entry : entries_) {
    if (entry) HandlerRemoved(entry->kind, domain_);
    entry.reset();
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
    if (name == Name(static_cast<Kind>(i))) kind = static_cast<Kind>(i);
  }
  if (kind == Kind::kCount || !Accepts(domain_, kind)) {
    Napi::TypeError::New(env,
                         "Unsupported interceptor kind or execution domain")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  auto& slot = entries_[static_cast<size_t>(kind)];
  if (slot) {
    Napi::Error::New(
        env, ("Interceptor handler already registered for " + name).c_str())
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  auto entry = std::make_shared<Entry>();
  entry->id = next_id_++;
  entry->kind = kind;
  entry->function = Napi::Persistent(info[1].As<Napi::Function>());
  slot = entry;
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
  for (auto& entry : entries_) {
    if (entry && entry->id == id) {
      HandlerRemoved(entry->kind, domain_);
      entry.reset();
      CancelPending(id);
      return;
    }
  }
}
bool HostScriptInterceptor::HasHandlers(Kind kind) const {
  return active_ && entries_[static_cast<size_t>(kind)] != nullptr;
}
void HostScriptInterceptor::ReportError(const std::string& message) {
  LOGE("Host Script interceptor: " << message);
  if (reporter_) reporter_(message);
}

Napi::Value HostScriptInterceptor::ViewOf(const Value& event) {
  Napi::Env env(env_);
  auto view = event.GetProperty(kViewId);
  if (!views_ || !view.IsString()) return env.Null();
  return views_->Get(std::strtoll(view.StdString().c_str(), nullptr, 10));
}

void HostScriptInterceptor::NotifyCreated(base::LynxEntityId view,
                                          uint64_t id) {
  if (!active_ || !IsAttached() || !views_) return;
  auto entry = entries_[static_cast<size_t>(Kind::kCreated)];
  if (!entry || (id && entry->id != id)) return;
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

InterceptResult HostScriptInterceptor::ReadReturned(Napi::Value returned,
                                                    Kind kind,
                                                    const Value& event,
                                                    uint64_t handler) {
  InterceptResult result;
  result.failed = true;
  if (env_.IsExceptionPending() || returned.IsEmpty()) return result;
  if (kind == Kind::kCall && returned.IsObject()) {
    auto decision = returned.As<Napi::Object>();
    auto action = decision.Get("action");
    if (env_.IsExceptionPending()) return result;
    auto payload = decision.Get("payload");
    if (env_.IsExceptionPending()) return result;
    if (action.IsString() && action.As<Napi::String>().Utf8Value() == "mock" &&
        payload.IsObject()) {
      auto mock = payload.As<Napi::Object>();
      auto callbacks = mock.Get("callbacks");
      if (env_.IsExceptionPending()) return result;
      if (callbacks.IsArray()) {
        auto array = callbacks.As<Napi::Array>();
        auto immediate = Napi::Array::New(env_);
        auto promises = Napi::Array::New(env_);
        uint32_t count = 0;
        for (uint32_t i = 0; i < array.Length(); ++i) {
          auto item = array.Get(i);
          if (env_.IsExceptionPending()) return result;
          if (!item.IsPromise()) {
            immediate.Set(count++, item);
            continue;
          }
          promises.Set(promises.Length(), item);
        }
        if (promises.Length())
          result.pending = WaitForCallbacks(promises, event, handler, true);
        auto copy = Napi::Object::New(env_);
        auto keys = mock.GetPropertyNames();
        if (env_.IsExceptionPending()) return result;
        for (uint32_t i = 0; i < keys.Length(); ++i) {
          auto key = keys.Get(i);
          auto field = mock.Get(key);
          if (env_.IsExceptionPending()) return result;
          copy.Set(key, field);
        }
        copy.Set("callbacks", immediate);
        auto replacement = Napi::Object::New(env_);
        replacement.Set("action", action);
        replacement.Set("payload", copy);
        returned = replacement;
      }
    }
  }
  Value patch;
  const auto& original =
      kind <= Kind::kUpdateMetaData ? event.GetProperty(kRequest) : event;
  auto action = ReadDecision(returned, kind, original, &patch);
  result.failed = action == Action::kInvalid || env_.IsExceptionPending();
  if (action == Action::kMock)
    result.mock = std::move(patch);
  else if (patch.GetLength())
    result.patch = std::move(patch);
  return result;
}

InterceptResult HostScriptInterceptor::Dispatch(Kind kind, const Value& event) {
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
  // Keep the current handler alive if it disposes itself or uninstalls HSR.
  auto entry = entries_[static_cast<size_t>(kind)];
  auto returned = entry->function.Value().Call(
      {ToJS(env, event), CreateChain(env), ViewOf(event)});
  if (!env.IsExceptionPending() && !returned.IsEmpty() &&
      returned.IsPromise() && kind == Kind::kCallback) {
    auto promises = Napi::Array::New(env_);
    promises.Set(uint32_t{0}, returned);
    result.pending = WaitForCallbacks(promises, event, entry->id, false);
  } else {
    result = ReadReturned(returned, kind, event, entry->id);
  }
  dispatching_ = false;
  if (result.failed || env.IsExceptionPending()) {
    std::string message = "Invalid interceptor decision";
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
