// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_SESSION_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_SESSION_H_

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/shell/host_script/lynx_view/lynx_view_ref_proxy.h"
#include "core/shell/host_script/runtime/host_script_js_dispatcher.h"

namespace lynx {
namespace shell {

// Owns one runtime's protocol and JS state, never the platform LynxView.
class HostScriptSession final
    : public std::enable_shared_from_this<HostScriptSession> {
 public:
  using ResultReporter =
      std::function<void(const std::string&, const std::string&)>;
  static std::shared_ptr<HostScriptSession> Create(
      ResultReporter reporter = nullptr);
  ~HostScriptSession();

  // Attach/Detach and all N-API methods must run on the runtime's JS thread.
  bool Attach(napi_env env);
  void Detach();
  bool IsAttached() const;
  void ReportEntryResult(const std::string& status, const std::string& message);

  // Platform-thread entry points; binding is allowed before or after Attach.
  bool BindView(std::unique_ptr<LynxViewRefProxy> proxy);
  void InvalidateView();
  bool HasCurrentView() const;
  void Notify(const std::string& event, int32_t code = 0,
              std::string message = {});

  Napi::Promise WaitForCurrentView(Napi::Env env);
  template <typename Request>
  Napi::Promise Dispatch(
      Napi::Env env,
      void (LynxViewRefProxy::*method)(Request, LynxViewRefOperationCompletion),
      Request request) {
    return DispatchOperation(
        env, [method, request = std::move(request)](
                 LynxViewRefProxy& proxy,
                 LynxViewRefOperationCompletion completion) mutable {
          (proxy.*method)(std::move(request), std::move(completion));
        });
  }
  void AddListener(const std::string& event, const Napi::Function& listener);
  void RemoveListener(const std::string& event, const Napi::Function& listener);

 private:
  explicit HostScriptSession(ResultReporter reporter)
      : reporter_(std::move(reporter)) {}
  using Deferred = Napi::Promise::Deferred;
  using Task = std::function<void(HostScriptSession&, Napi::Env)>;
  void Post(Task task);
  void MaybePostReady();
  Napi::Promise DispatchOperation(
      Napi::Env env,
      std::function<void(LynxViewRefProxy&, LynxViewRefOperationCompletion)>
          invoke);
  void Complete(Napi::Env env, uint64_t id, LynxViewRefOperationResult result);
  void HandleDestroyed(Napi::Env env);
  void Emit(Napi::Env env, const std::string& event,
            const std::initializer_list<napi_value>& args = {});
  void SettleWaiters(const char* error = nullptr);
  void RejectPending(const char* message);

  mutable std::mutex mutex_;
  ResultReporter reporter_;
  std::shared_ptr<HostScriptJsDispatcher> dispatcher_;
  std::shared_ptr<LynxViewRefProxy> proxy_;
  bool attached_ = false;
  bool detached_ = false;
  bool invalidated_ = false;
  bool ready_posted_ = false;

  // Only touched on the JS thread, while the N-API environment is alive.
  bool destroyed_dispatched_ = false;
  uint64_t next_operation_id_ = 1;
  std::vector<Deferred> waiters_;
  std::unordered_map<uint64_t, Deferred> pending_;
  std::unordered_map<std::string, std::vector<Napi::FunctionReference>>
      listeners_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_SESSION_H_
