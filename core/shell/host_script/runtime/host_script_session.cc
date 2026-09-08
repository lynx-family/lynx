// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_session.h"

#include <algorithm>

#include "core/shell/host_script/runtime/host_script_module.h"
#include "third_party/binding/napi/callback_helper.h"

namespace lynx {
namespace shell {
namespace {

void Reject(Napi::Promise::Deferred& deferred, const char* message,
            const char* code = "INVALID_STATE") {
  deferred.Reject(
      HostScriptModule::HostScriptError(deferred.Env(), code, message));
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
  HostScriptModule::RegisterHostScriptModule(env, shared_from_this());
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
  RejectPending("The Host Script runtime was detached");
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

Napi::Promise HostScriptSession::DispatchOperation(
    Napi::Env env,
    std::function<void(LynxViewRefProxy&, LynxViewRefOperationCompletion)>
        invoke) {
  auto deferred = Deferred::New(env);
  auto promise = deferred.Promise();
  std::shared_ptr<LynxViewRefProxy> proxy;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (attached_) {
      proxy = proxy_;
    }
  }
  if (!proxy) {
    Reject(deferred, "The current LynxView is not available");
    return promise;
  }
  const uint64_t id = next_operation_id_++;
  pending_.emplace(id, std::move(deferred));
  invoke(*proxy,
         [weak = weak_from_this(), id](LynxViewRefOperationResult result) {
           if (auto session = weak.lock()) {
             session->Post([id, result = std::move(result)](
                               HostScriptSession& owner, Napi::Env env) {
               owner.Complete(env, id, result);
             });
           }
         });
  return promise;
}

void HostScriptSession::Complete(Napi::Env env, uint64_t id,
                                 LynxViewRefOperationResult result) {
  auto found = pending_.find(id);
  if (found == pending_.end()) {
    return;
  }
  auto deferred = std::move(found->second);
  pending_.erase(found);
  if (result.status == LynxViewRefOperationStatus::kTargetLost) {
    HandleDestroyed(env);
    Reject(deferred, "The current LynxView is no longer available");
  } else if (result.status == LynxViewRefOperationStatus::kPlatformError) {
    const auto message = result.message.empty()
                             ? "The LynxView operation failed"
                             : result.message;
    Emit(env, "error",
         {Napi::Number::New(env, -1), Napi::String::New(env, message)});
    Reject(deferred, message.c_str(), "PLATFORM_ERROR");
  } else {
    deferred.Resolve(env.Undefined());
  }
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
  RejectPending("The current LynxView is no longer available");
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
  listeners.erase(
      std::remove_if(listeners.begin(), listeners.end(),
                     [&](Napi::FunctionReference& candidate) {
                       return candidate.Value().StrictEquals(listener);
                     }),
      listeners.end());
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

void HostScriptSession::RejectPending(const char* message) {
  auto pending = std::move(pending_);
  pending_.clear();
  for (auto& entry : pending) {
    Reject(entry.second, message);
  }
}

}  // namespace shell
}  // namespace lynx
