// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_H_

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/shell/host_script/runtime/interceptor.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx::shell {

// The runtime owner installs this into an existing environment, on its owning
// thread. This component never creates a VM, thread or cross-thread JS handle.
// Each event kind accepts one active handler. Dispose it before registering
// another handler of the same kind. Handlers decide with `proceed`, optionally
// with a patch, or `mock` for jsb.call, and receive the event's `LynxView`
// proxy (or null) as their third argument. The view.created notification
// accepts handlers in every domain, passes only the proxy and ignores return
// values. A new view.created handler first receives the existing Views,
// asynchronously, and each handler is notified at most once per View.
class HostScriptLynxViews;

class HostScriptInterceptor final
    : public Interceptor,
      public std::enable_shared_from_this<HostScriptInterceptor> {
 public:
  using Guard = std::function<bool()>;
  using Reporter = std::function<void(const std::string&)>;
  // Whether the domain accepts any event kind. Install fails otherwise.
  static bool Supports(Domain domain);
  // Without `views`, handlers receive null proxies and view.created is never
  // delivered.
  static std::shared_ptr<HostScriptInterceptor> Install(
      Napi::Env env, Domain domain, Reporter reporter = nullptr,
      Guard guard = {}, std::shared_ptr<HostScriptLynxViews> views = nullptr);
  bool IsAttached() const override {
    return Interceptor::IsAttached() && (!guard_ || guard_());
  }
  void Uninstall() override;
  bool HasHandlers(InterceptKind kind) const override;
  InterceptResult Dispatch(InterceptKind kind,
                           const lepus::Value& event) override;
  void ReportError(const std::string& message) override;
  // Delivers one View to the view.created handler `entry`, or to the current
  // handler when `entry` is 0. Owner thread only.
  void NotifyCreated(base::LynxEntityId view, uint64_t entry = 0);

 private:
  struct CallbackData {
    std::weak_ptr<HostScriptInterceptor> provider;
    uint64_t id;
  };
  struct Entry {
    uint64_t id;
    InterceptKind kind;
    Napi::FunctionReference function;
    // Views already delivered to this view.created handler.
    std::unordered_set<base::LynxEntityId> created;
  };
  HostScriptInterceptor(Napi::Env env, Domain domain, Reporter reporter)
      : env_(env), domain_(domain), reporter_(std::move(reporter)) {}
  Napi::Function Bind(Napi::Env env, const char* name,
                      Napi::Value (*callback)(const Napi::CallbackInfo&),
                      uint64_t id = 0);
  static Napi::Value On(const Napi::CallbackInfo& info);
  static Napi::Value Dispose(const Napi::CallbackInfo& info);
  Napi::Value Add(const Napi::CallbackInfo& info);
  struct Pending {
    InterceptCompletion complete;
    lepus::Value event;
    uint64_t handler;
    bool mock;
  };
  std::function<void(InterceptCompletion)> WaitForCallbacks(
      Napi::Array promises, const lepus::Value& event, uint64_t handler,
      bool mock);
  void Complete(uint64_t id, Napi::Value value, bool failed,
                bool cancelled = false);
  void CancelPending(uint64_t handler = 0, const std::string& view = {});
  static Napi::Value Settled(const Napi::CallbackInfo& info);
  InterceptResult ReadReturned(Napi::Value returned, InterceptKind kind,
                               const lepus::Value& event, uint64_t handler);
  Napi::Value ViewOf(const lepus::Value& event);
  void Remove(uint64_t id);
  Napi::Env env_;
  Domain domain_;
  Reporter reporter_;
  Guard guard_;
  std::shared_ptr<HostScriptLynxViews> views_;
  uint64_t next_id_ = 1;
  bool active_ = true;
  bool dispatching_ = false;
  Napi::ObjectReference api_;
  Napi::FunctionReference then_;
  uint64_t next_pending_ = 1;
  std::unordered_map<uint64_t, Pending> pending_;
  std::array<std::shared_ptr<Entry>, static_cast<size_t>(InterceptKind::kCount)>
      entries_{};
};
}  // namespace lynx::shell
#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_H_
