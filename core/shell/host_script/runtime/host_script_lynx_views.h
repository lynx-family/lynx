// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_LYNX_VIEWS_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_LYNX_VIEWS_H_

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "core/shell/host_script/lynx_view/lynx_view_registry.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace shell {

// The `LynxView` proxies of one Host Script context. All N-API work happens on
// the owner runner. Each live View has one cached proxy per context; proxies
// never keep a platform View alive and never cross VMs. Commands are posted to
// the platform UI runner and dropped once this context's generation closes.
class HostScriptLynxViews final
    : public LynxViewRegistry::Observer,
      public std::enable_shared_from_this<HostScriptLynxViews> {
 public:
  using CreatedHandler = std::function<void(base::LynxEntityId)>;
  using Guard = std::function<bool()>;

  static std::shared_ptr<HostScriptLynxViews> Create(
      Napi::Env env, fml::RefPtr<fml::TaskRunner> owner, Guard guard);
  ~HostScriptLynxViews() override;

  // Owner thread, while the environment is alive. Releases every JS reference
  // and cancels commands that have not started yet.
  void Detach();
  // The cached proxy, created on first use, or null for a View that is not
  // operable. Requires an entered handle scope.
  Napi::Value Get(base::LynxEntityId view);
  // Runs `task` later on the owner, inside the context, while attached.
  void Post(std::function<void()> task);
  // Receives live View creations on the owner.
  void SetCreatedHandler(CreatedHandler handler);

  void OnViewEvent(const LynxViewEvent& event) override;

 private:
  struct CallbackData;
  struct View {
    Napi::ObjectReference object;
    std::unordered_map<std::string, std::vector<Napi::FunctionReference>>
        listeners;
  };
  HostScriptLynxViews(Napi::Env env, fml::RefPtr<fml::TaskRunner> owner,
                      Guard guard);
  bool IsActive() const;
  Napi::Object CreateObject(base::LynxEntityId view);
  Napi::Function Bind(const char* name,
                      Napi::Value (*callback)(const Napi::CallbackInfo&),
                      base::LynxEntityId view);
  bool Dispatch(base::LynxEntityId view, LynxViewRegistry::Command command);
  void HandleEvent(const LynxViewEvent& event);
  void Emit(base::LynxEntityId view, const std::string& name, int32_t code,
            const std::string& message);

  static HostScriptLynxViews* From(const Napi::CallbackInfo& info,
                                   base::LynxEntityId* view);
  static Napi::Value GetUrl(const Napi::CallbackInfo& info);
  static Napi::Value GetAvailable(const Napi::CallbackInfo& info);
  static Napi::Value LoadURL(const Napi::CallbackInfo& info);
  static Napi::Value LoadTemplate(const Napi::CallbackInfo& info);
  static Napi::Value LoadSSR(const Napi::CallbackInfo& info);
  static Napi::Value HydrateSSR(const Napi::CallbackInfo& info);
  static Napi::Value LoadSsr(const Napi::CallbackInfo& info, bool hydrate);
  static Napi::Value UpdateMetaData(const Napi::CallbackInfo& info);
  static Napi::Value ReloadTemplate(const Napi::CallbackInfo& info);
  static Napi::Value SendGlobalEvent(const Napi::CallbackInfo& info);
  static Napi::Value On(const Napi::CallbackInfo& info);
  static Napi::Value Off(const Napi::CallbackInfo& info);

  Napi::Env env_;
  const fml::RefPtr<fml::TaskRunner> owner_;
  // Shared with queued commands; cleared on Detach from the owner thread.
  const std::shared_ptr<std::atomic<bool>> alive_;
  const Guard guard_;
  CreatedHandler created_handler_;
  std::unordered_map<base::LynxEntityId, View> views_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_LYNX_VIEWS_H_
