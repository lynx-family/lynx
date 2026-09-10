// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_session.h"

#include <algorithm>
#include <iterator>

#include "core/shell/host_script/runtime/host_script_module.h"
#include "third_party/binding/napi/callback_helper.h"

namespace lynx {
namespace shell {
namespace {

void Reject(Napi::Promise::Deferred& deferred, const char* message,
            const char* code = "INVALID_STATE") {
  deferred.Reject(HostScriptModule::CreateError(deferred.Env(), code, message));
}

}  // namespace

std::shared_ptr<HostScriptSession> HostScriptSession::Create(
    ResultReporter reporter) {
  return std::shared_ptr<HostScriptSession>(
      new HostScriptSession(std::move(reporter)));
}

HostScriptSession::~HostScriptSession() { Detach(); }

bool HostScriptSession::Attach(napi_env env) {
  auto dispatcher = HostScriptJsDispatcher::Create(env);
  if (!dispatcher) {
    return false;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (attached_ || detached_) {
      return false;
    }
    dispatcher_ = std::move(dispatcher);
    attached_ = true;
  }
  HostScriptModule::Register(env, shared_from_this());
  MaybePostReady();
  return true;
}

void HostScriptSession::Detach() {
  std::shared_ptr<HostScriptJsDispatcher> dispatcher;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (detached_) {
      return;
    }
    detached_ = true;
    attached_ = false;
    dispatcher = std::move(dispatcher_);
  }
  InvalidateView();
  SettleWaiters("The Host Script runtime was detached");
  listeners_.clear();
  if (dispatcher) {
    dispatcher->Detach();
  }
}

bool HostScriptSession::IsAttached() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return attached_;
}

void HostScriptSession::ReportEntryResult(const std::string& status,
                                          const std::string& message) {
  if (IsAttached() && reporter_) {
    reporter_(status, message);
  }
}

bool HostScriptSession::BindView(std::unique_ptr<LynxViewRefProxy> proxy) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!proxy || proxy_ || invalidated_ || detached_) {
      return false;
    }
    proxy_ = std::move(proxy);
  }
  MaybePostReady();
  return true;
}

void HostScriptSession::InvalidateView() {
  std::shared_ptr<LynxViewRefProxy> proxy;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    invalidated_ = true;
    proxy = std::move(proxy_);
  }
  if (proxy) {
    proxy->Invalidate();
  }
}

bool HostScriptSession::HasCurrentView() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return attached_ && proxy_;
}

void HostScriptSession::Post(Task task) {
  std::shared_ptr<HostScriptJsDispatcher> dispatcher;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    dispatcher = dispatcher_;
  }
  if (dispatcher) {
    dispatcher->Post(
        [weak = weak_from_this(), task = std::move(task)](Napi::Env env) {
          if (auto session = weak.lock(); session && session->IsAttached()) {
            Napi::HandleScope scope(env);
            task(*session, env);
          }
        });
  }
}

void HostScriptSession::MaybePostReady() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!attached_ || !proxy_ || ready_posted_) {
      return;
    }
    ready_posted_ = true;
  }
  Post([](HostScriptSession& session, Napi::Env env) {
    if (session.HasCurrentView()) {
      session.SettleWaiters();
      session.Emit(env, "ready");
    }
  });
}

Napi::Promise HostScriptSession::WaitForCurrentView(Napi::Env env) {
  auto deferred = Deferred::New(env);
  auto promise = deferred.Promise();
  bool can_wait;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    can_wait = attached_ && !invalidated_;
  }
  if (!can_wait) {
    Reject(deferred, "The current LynxView is not available");
  } else if (HasCurrentView()) {
    deferred.Resolve(env.Undefined());
  } else {
    waiters_.push_back(std::move(deferred));
  }
  return promise;
}

void HostScriptSession::Notify(const std::string& event, int32_t code,
                               std::string message) {
  if (event == "destroyed") {
    InvalidateView();
  }
  Post([event, code, message = std::move(message)](HostScriptSession& session,
                                                   Napi::Env env) {
    if (event == "destroyed") {
      session.HandleDestroyed(env);
    } else if (event == "error") {
      session.Emit(
          env, event,
          {Napi::Number::New(env, code), Napi::String::New(env, message)});
    } else {
      session.Emit(env, event);
    }
  });
}

void HostScriptSession::HandleDestroyed(Napi::Env env) {
  InvalidateView();
  if (destroyed_dispatched_) {
    return;
  }
  destroyed_dispatched_ = true;
  SettleWaiters("The current LynxView is no longer available");
  Emit(env, "destroyed");
  listeners_.clear();
}

void HostScriptSession::AddListener(const std::string& event,
                                    const Napi::Function& listener) {
  listeners_[event].push_back(Napi::Persistent(listener));
}

void HostScriptSession::RemoveListener(const std::string& event,
                                       const Napi::Function& listener) {
  auto found = listeners_.find(event);
  if (found == listeners_.end()) {
    return;
  }
  auto& listeners = found->second;
  auto found_listener =
      std::find_if(listeners.rbegin(), listeners.rend(),
                   [&](Napi::FunctionReference& candidate) {
                     return candidate.Value().StrictEquals(listener);
                   });
  if (found_listener != listeners.rend()) {
    listeners.erase(std::next(found_listener).base());
  }
}

void HostScriptSession::Emit(Napi::Env env, const std::string& event,
                             const std::initializer_list<napi_value>& args) {
  if (destroyed_dispatched_ && event != "destroyed") {
    return;
  }
  auto found = listeners_.find(event);
  if (found == listeners_.end()) {
    return;
  }
  Napi::HandleScope scope(env);
  std::vector<Napi::FunctionReference> snapshot;
  for (auto& listener : found->second) {
    snapshot.push_back(Napi::Persistent(listener.Value()));
  }
  for (auto& listener : snapshot) {
    listener.Value().Call(env.Global(), args);
    if (env.IsExceptionPending()) {
      binding::CallbackHelper::ReportException(
          env.GetAndClearPendingException().As<Napi::Object>());
    }
  }
}

void HostScriptSession::SettleWaiters(const char* error) {
  auto waiters = std::move(waiters_);
  waiters_.clear();
  for (auto& deferred : waiters) {
    if (error) {
      Reject(deferred, error);
    } else {
      deferred.Resolve(deferred.Env().Undefined());
    }
  }
}

}  // namespace shell
}  // namespace lynx
