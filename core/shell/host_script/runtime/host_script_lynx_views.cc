// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_lynx_views.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <utility>

#include "third_party/binding/napi/callback_helper.h"

namespace lynx {
namespace shell {
namespace {
constexpr char kInvalidArgument[] = "INVALID_ARGUMENT";
constexpr char kInvalidState[] = "INVALID_STATE";

bool IsKnownEvent(const std::string& event) {
  return event == "loadSuccess" || event == "firstScreen" ||
         event == "pageUpdate" || event == "dataUpdated" || event == "error" ||
         event == "destroyed";
}

// Argument errors are TypeErrors; both carry the documented `code`.
Napi::Value Throw(Napi::Env env, const char* code, const char* message) {
  auto error = std::string(code) == kInvalidArgument
                   ? Napi::TypeError::New(env, message)
                   : Napi::Error::New(env, message);
  error["code"] = Napi::String::New(env, code);
  error.ThrowAsJavaScriptException();
  return env.Undefined();
}

bool IsRecord(const Napi::Value& value) {
  return value.IsObject() && !value.IsArray() && !value.IsFunction();
}

// Copies page data (or event params) as JSON. Undefined means absent.
bool Json(const Napi::Value& value, bool array, std::string* result) {
  result->clear();
  if (value.IsUndefined() && !array) return true;
  if (array ? !value.IsArray() : !IsRecord(value)) return false;
  auto env = value.Env();
  auto stringify = env.Global()
                       .Get("JSON")
                       .As<Napi::Object>()
                       .Get("stringify")
                       .As<Napi::Function>();
  auto encoded = stringify.Call({value});
  if (env.IsExceptionPending()) {
    env.GetAndClearPendingException();
    return false;
  }
  if (!encoded.IsString()) return false;
  *result = encoded.As<Napi::String>().Utf8Value();
  return true;
}

Napi::Value DataError(Napi::Env env) {
  return Throw(env, kInvalidArgument,
               "Page data must be a JSON-serializable object");
}

// Reads a string argument; an empty result means invalid.
std::string ReadString(const Napi::Value& value) {
  return value.IsString() ? value.As<Napi::String>().Utf8Value()
                          : std::string();
}

bool ReadBytes(const Napi::Value& value, std::vector<uint8_t>* bytes) {
  if (!value.IsArrayBuffer()) return false;
  auto buffer = value.As<Napi::ArrayBuffer>();
  if (buffer.ByteLength() == 0 ||
      buffer.ByteLength() >
          static_cast<size_t>(std::numeric_limits<int32_t>::max()))
    return false;
  auto* begin = static_cast<uint8_t*>(buffer.Data());
  bytes->assign(begin, begin + buffer.ByteLength());
  return true;
}

// Options default to an empty record; anything else must be a plain object.
bool ReadOptions(const Napi::CallbackInfo& info, size_t index,
                 Napi::Object* options) {
  if (index >= info.Length() || info[index].IsUndefined()) {
    *options = Napi::Object::New(info.Env());
    return true;
  }
  if (!IsRecord(info[index])) return false;
  *options = info[index].As<Napi::Object>();
  return true;
}
}  // namespace

struct HostScriptLynxViews::CallbackData {
  std::weak_ptr<HostScriptLynxViews> views;
  base::LynxEntityId view;
};

std::shared_ptr<HostScriptLynxViews> HostScriptLynxViews::Create(
    Napi::Env env, fml::RefPtr<fml::TaskRunner> owner, Guard guard) {
  auto views = std::shared_ptr<HostScriptLynxViews>(
      new HostScriptLynxViews(env, std::move(owner), std::move(guard)));
  LynxViewRegistry::GetInstance().AddObserver(views);
  return views;
}

HostScriptLynxViews::HostScriptLynxViews(Napi::Env env,
                                         fml::RefPtr<fml::TaskRunner> owner,
                                         Guard guard)
    : env_(env),
      owner_(std::move(owner)),
      alive_(std::make_shared<std::atomic<bool>>(true)),
      guard_(std::move(guard)) {}

bool HostScriptLynxViews::IsActive() const {
  return alive_->load() && (!guard_ || guard_());
}

HostScriptLynxViews::~HostScriptLynxViews() {
  alive_->store(false);
  LynxViewRegistry::GetInstance().RemoveObserver(this);
}

void HostScriptLynxViews::Detach() {
  if (!alive_->exchange(false)) return;
  LynxViewRegistry::GetInstance().RemoveObserver(this);
  created_handler_ = nullptr;
  Napi::ContextScope context_scope(env_);
  views_.clear();
}

void HostScriptLynxViews::SetCreatedHandler(CreatedHandler handler) {
  created_handler_ = std::move(handler);
}

void HostScriptLynxViews::Post(std::function<void()> task) {
  owner_->PostTask([weak = weak_from_this(), task = std::move(task)] {
    auto self = weak.lock();
    if (!self || !self->IsActive()) return;
    Napi::ContextScope context_scope(self->env_);
    Napi::HandleScope scope(self->env_);
    task();
  });
}

void HostScriptLynxViews::OnViewEvent(const LynxViewEvent& event) {
  // Always posted, including on the owner thread, so script never runs inside
  // platform View construction or destruction.
  Post([weak = weak_from_this(), event] {
    if (auto self = weak.lock()) self->HandleEvent(event);
  });
}

void HostScriptLynxViews::HandleEvent(const LynxViewEvent& event) {
  switch (event.type) {
    case LynxViewEvent::Type::kCreated:
      if (created_handler_) created_handler_(event.view);
      break;
    case LynxViewEvent::Type::kLifecycle:
      Emit(event.view, event.name, event.code, event.message);
      break;
    case LynxViewEvent::Type::kDestroyed: {
      if (!views_.count(event.view)) break;
      Emit(event.view, "destroyed", 0, {});
      // A listener may have detached this context.
      if (alive_->load()) views_.erase(event.view);
      break;
    }
  }
}

void HostScriptLynxViews::Emit(base::LynxEntityId view, const std::string& name,
                               int32_t code, const std::string& message) {
  auto found = views_.find(view);
  if (found == views_.end()) return;
  auto listeners = found->second.listeners.find(name);
  if (listeners == found->second.listeners.end()) return;
  Napi::Env env(env_);
  // Listeners may register or remove listeners while being called.
  std::vector<Napi::FunctionReference> snapshot;
  for (auto& listener : listeners->second)
    snapshot.push_back(Napi::Persistent(listener.Value()));
  auto self = found->second.object.Value();
  for (auto& listener : snapshot) {
    if (!IsActive()) return;
    if (name == "error") {
      listener.Value().Call(self, {Napi::Number::New(env, code),
                                   Napi::String::New(env, message)});
    } else {
      listener.Value().Call(self, {});
    }
    if (env.IsExceptionPending()) {
      binding::CallbackHelper::ReportException(
          env.GetAndClearPendingException().As<Napi::Object>());
    }
  }
}

Napi::Value HostScriptLynxViews::Get(base::LynxEntityId view) {
  Napi::Env env(env_);
  if (!IsActive() || !LynxViewRegistry::GetInstance().IsRegistered(view))
    return env.Null();
  auto found = views_.find(view);
  if (found != views_.end()) return found->second.object.Value();
  auto object = CreateObject(view);
  views_[view].object = Napi::Persistent(object);
  return object;
}

Napi::Function HostScriptLynxViews::Bind(
    const char* name, Napi::Value (*callback)(const Napi::CallbackInfo&),
    base::LynxEntityId view) {
  // JS may retain a method after its View or context is gone.
  auto* data = new CallbackData{weak_from_this(), view};
  auto function = Napi::Function::New(env_, callback, name, data);
  function.AddFinalizer(data, [](auto, void* data, void*) {
    delete static_cast<CallbackData*>(data);
  });
  return function;
}

Napi::Object HostScriptLynxViews::CreateObject(base::LynxEntityId view) {
  Napi::Env env(env_);
  auto object = Napi::Object::New(env);
  object.Set("viewId", Napi::String::New(env, std::to_string(view)));
  const std::pair<const char*, Napi::Value (*)(const Napi::CallbackInfo&)>
      methods[] = {{"loadURL", LoadURL},
                   {"loadTemplate", LoadTemplate},
                   {"loadSSR", LoadSSR},
                   {"hydrateSSR", HydrateSSR},
                   {"updateMetaData", UpdateMetaData},
                   {"reloadTemplate", ReloadTemplate},
                   {"sendGlobalEvent", SendGlobalEvent},
                   {"on", On},
                   {"off", Off}};
  for (const auto& [name, callback] : methods)
    object.Set(name, Bind(name, callback, view));
  auto constructor = env.Global().Get("Object").As<Napi::Object>();
  auto define = constructor.Get("defineProperty").As<Napi::Function>();
  for (const auto& [name, getter] :
       {std::make_pair("url", GetUrl),
        std::make_pair("available", GetAvailable)}) {
    auto descriptor = Napi::Object::New(env);
    descriptor.Set("get", Bind(name, getter, view));
    descriptor.Set("enumerable", Napi::Boolean::New(env, true));
    define.Call({object, Napi::String::New(env, name), descriptor});
  }
  constructor.Get("freeze").As<Napi::Function>().Call({object});
  return object;
}

bool HostScriptLynxViews::Dispatch(base::LynxEntityId view,
                                   LynxViewRegistry::Command command) {
  if (!IsActive()) return false;
  return LynxViewRegistry::GetInstance().Post(
      view, std::move(command), [alive = alive_, guard = guard_] {
        return alive->load() && (!guard || guard());
      });
}

HostScriptLynxViews* HostScriptLynxViews::From(const Napi::CallbackInfo& info,
                                               base::LynxEntityId* view) {
  auto* data = static_cast<CallbackData*>(info.Data());
  *view = data->view;
  auto views = data->views.lock();
  // The owning bindings keep the instance alive while its context runs.
  return views && views->IsActive() ? views.get() : nullptr;
}

Napi::Value HostScriptLynxViews::GetUrl(const Napi::CallbackInfo& info) {
  base::LynxEntityId view;
  From(info, &view);
  return Napi::String::New(info.Env(),
                           LynxViewRegistry::GetInstance().Url(view));
}

Napi::Value HostScriptLynxViews::GetAvailable(const Napi::CallbackInfo& info) {
  base::LynxEntityId view;
  auto* views = From(info, &view);
  return Napi::Boolean::New(
      info.Env(), views && LynxViewRegistry::GetInstance().IsRegistered(view));
}

Napi::Value HostScriptLynxViews::LoadURL(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  base::LynxEntityId view;
  auto* views = From(info, &view);
  LynxViewRefLoadTemplateRequest request;
  Napi::Object options;
  if (info.Length() < 1 || info.Length() > 2 ||
      (request.url = ReadString(info[0])).empty() ||
      !ReadOptions(info, 1, &options)) {
    return Throw(env, kInvalidArgument,
                 "loadURL expects a non-empty URL and optional options");
  }
  if (!Json(options.Get("data"), false, &request.initial_data_json) ||
      !Json(options.Get("globalProps"), false, &request.global_props_json))
    return DataError(env);
  return Napi::Boolean::New(
      env, views && views->Dispatch(view, [request](LynxViewRefProxy& proxy) {
        proxy.LoadTemplate(request);
      }));
}

Napi::Value HostScriptLynxViews::LoadTemplate(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  base::LynxEntityId view;
  auto* views = From(info, &view);
  LynxViewRefLoadTemplateRequest request;
  Napi::Object options;
  if (info.Length() != 1 || !IsRecord(info[0]) ||
      !ReadBytes((options = info[0].As<Napi::Object>()).Get("template"),
                 &request.template_data) ||
      (request.url = ReadString(options.Get("url"))).empty()) {
    return Throw(env, kInvalidArgument,
                 "loadTemplate expects a non-empty template ArrayBuffer and "
                 "URL");
  }
  request.has_template = true;
  auto processor = options.Get("processor");
  auto read_only = options.Get("readOnly");
  if ((!processor.IsUndefined() && !processor.IsString()) ||
      (!read_only.IsUndefined() && !read_only.IsBoolean())) {
    return Throw(env, kInvalidArgument,
                 "processor must be a string and readOnly a boolean");
  }
  if (processor.IsString())
    request.processor_name = processor.As<Napi::String>().Utf8Value();
  request.read_only =
      read_only.IsBoolean() && read_only.As<Napi::Boolean>().Value();
  if (!Json(options.Get("initialData"), false, &request.initial_data_json) ||
      !Json(options.Get("globalProps"), false, &request.global_props_json))
    return DataError(env);
  return Napi::Boolean::New(
      env, views && views->Dispatch(view, [request](LynxViewRefProxy& proxy) {
        proxy.LoadTemplate(request);
      }));
}

Napi::Value HostScriptLynxViews::LoadSsr(const Napi::CallbackInfo& info,
                                         bool hydrate) {
  auto env = info.Env();
  base::LynxEntityId view;
  auto* views = From(info, &view);
  LynxViewRefSsrRequest request;
  Napi::Object options;
  if (info.Length() != 1 || !IsRecord(info[0]) ||
      !ReadBytes((options = info[0].As<Napi::Object>()).Get("data"),
                 &request.data) ||
      (request.url = ReadString(options.Get("url"))).empty()) {
    return Throw(env, kInvalidArgument,
                 hydrate ? "hydrateSSR expects non-empty data and URL"
                         : "loadSSR expects non-empty data and URL");
  }
  if (!Json(options.Get("initialData"), false, &request.initial_data_json))
    return DataError(env);
  return Napi::Boolean::New(
      env, views && views->Dispatch(view, [request,
                                           hydrate](LynxViewRefProxy& proxy) {
        hydrate ? proxy.HydrateSSR(request) : proxy.LoadSSR(request);
      }));
}

Napi::Value HostScriptLynxViews::LoadSSR(const Napi::CallbackInfo& info) {
  return LoadSsr(info, false);
}

Napi::Value HostScriptLynxViews::HydrateSSR(const Napi::CallbackInfo& info) {
  return LoadSsr(info, true);
}

Napi::Value HostScriptLynxViews::UpdateMetaData(
    const Napi::CallbackInfo& info) {
  auto env = info.Env();
  base::LynxEntityId view;
  auto* views = From(info, &view);
  LynxViewRefUpdateMetaDataRequest request;
  if (info.Length() != 1 || !info[0].IsObject()) {
    return Throw(env, kInvalidArgument, "Provide updateData or globalProps");
  }
  auto meta = info[0].As<Napi::Object>();
  auto data = meta.Get("updateData");
  auto props = meta.Get("globalProps");
  request.has_data = !data.IsUndefined();
  request.has_global_props = !props.IsUndefined();
  if (!request.has_data && !request.has_global_props) {
    return Throw(env, kInvalidArgument, "Provide updateData or globalProps");
  }
  if (!Json(data, false, &request.data_json) ||
      !Json(props, false, &request.global_props_json))
    return DataError(env);
  return Napi::Boolean::New(
      env, views && views->Dispatch(view, [request](LynxViewRefProxy& proxy) {
        proxy.UpdateMetaData(request);
      }));
}

Napi::Value HostScriptLynxViews::ReloadTemplate(
    const Napi::CallbackInfo& info) {
  auto env = info.Env();
  base::LynxEntityId view;
  auto* views = From(info, &view);
  LynxViewRefReloadTemplateRequest request;
  Napi::Object options;
  if (info.Length() > 1 || !ReadOptions(info, 0, &options)) {
    return Throw(env, kInvalidArgument,
                 "reloadTemplate expects optional options");
  }
  if (!Json(options.Get("data"), false, &request.data_json) ||
      !Json(options.Get("globalProps"), false, &request.global_props_json))
    return DataError(env);
  if (request.data_json.empty()) request.data_json = "{}";
  return Napi::Boolean::New(
      env, views && views->Dispatch(view, [request](LynxViewRefProxy& proxy) {
        proxy.ReloadTemplate(request);
      }));
}

Napi::Value HostScriptLynxViews::SendGlobalEvent(
    const Napi::CallbackInfo& info) {
  auto env = info.Env();
  base::LynxEntityId view;
  auto* views = From(info, &view);
  LynxViewRefGlobalEventRequest request;
  if (info.Length() < 1 || (request.name = ReadString(info[0])).empty()) {
    return Throw(env, kInvalidArgument,
                 "sendGlobalEvent expects a non-empty event name");
  }
  auto params = Napi::Array::New(env, info.Length() - 1);
  for (size_t i = 1; i < info.Length(); ++i)
    params.Set(static_cast<uint32_t>(i - 1), info[i]);
  if (!Json(params, true, &request.params_json)) {
    return Throw(env, kInvalidArgument,
                 "Event params must be a JSON-serializable array");
  }
  return Napi::Boolean::New(
      env, views && views->Dispatch(view, [request](LynxViewRefProxy& proxy) {
        proxy.SendGlobalEvent(request);
      }));
}

Napi::Value HostScriptLynxViews::On(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  base::LynxEntityId view;
  auto* views = From(info, &view);
  if (info.Length() != 2 || !info[0].IsString() || !info[1].IsFunction()) {
    return Throw(env, kInvalidArgument,
                 "on expects an event name and listener");
  }
  auto event = info[0].As<Napi::String>().Utf8Value();
  if (!IsKnownEvent(event)) {
    return Throw(env, kInvalidArgument, "Unknown LynxView event");
  }
  if (!views || !LynxViewRegistry::GetInstance().IsRegistered(view) ||
      !views->views_.count(view)) {
    return Throw(env, kInvalidState, "The LynxView is not available");
  }
  views->views_[view].listeners[event].push_back(
      Napi::Persistent(info[1].As<Napi::Function>()));
  return info.This();
}

Napi::Value HostScriptLynxViews::Off(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  base::LynxEntityId view;
  auto* views = From(info, &view);
  if (info.Length() != 2 || !info[0].IsString() || !info[1].IsFunction()) {
    return Throw(env, kInvalidArgument,
                 "off expects an event name and listener");
  }
  if (!IsKnownEvent(info[0].As<Napi::String>().Utf8Value())) {
    return Throw(env, kInvalidArgument, "Unknown LynxView event");
  }
  if (!views) return info.This();
  auto found = views->views_.find(view);
  if (found == views->views_.end()) return info.This();
  auto listeners =
      found->second.listeners.find(info[0].As<Napi::String>().Utf8Value());
  if (listeners == found->second.listeners.end()) return info.This();
  auto& list = listeners->second;
  auto match = std::find_if(list.rbegin(), list.rend(),
                            [&](Napi::FunctionReference& candidate) {
                              return candidate.Value().StrictEquals(info[1]);
                            });
  if (match != list.rend()) list.erase(std::next(match).base());
  return info.This();
}

}  // namespace shell
}  // namespace lynx
