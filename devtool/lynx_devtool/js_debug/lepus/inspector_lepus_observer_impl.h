// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_INSPECTOR_LEPUS_OBSERVER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_INSPECTOR_LEPUS_OBSERVER_IMPL_H_

#include "core/inspector/observer/inspector_lepus_observer.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "devtool/lynx_devtool/config/devtool_config.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"

namespace lynx {
namespace devtool {
class InspectorLepusDebuggerImpl;
class LynxDevToolNG;

class InspectorLepusObserverImpl : public lepus::InspectorLepusObserver {
 public:
  explicit InspectorLepusObserverImpl(
      const std::shared_ptr<InspectorLepusDebuggerImpl>& debugger);
  ~InspectorLepusObserverImpl() override = default;

  std::unique_ptr<lepus::LepusInspectorManager> CreateLepusInspectorManager(
      runtime::ContextType context_type) override;
  bool ShouldFetchDebugInfo() override {
    return DevToolConfig::ShouldFetchDebugInfo(true);
  }
  std::string GetDebugInfo(const std::string& url) override;
  void SetDebugInfoUrl(const std::string& url,
                       const std::string& file_name) override;

  void SetConsolePostNeeded(bool need) { need_post_console_ = need; }
  void SetDevToolMediator(
      const std::shared_ptr<LynxDevToolMediator>& mediator_ptr) {
    mediator_ptr_ = mediator_ptr;
  }

  void OnInspectorInited(
      const std::string& vm_type, const std::string& name,
      const std::shared_ptr<devtool::InspectorClientNG>& client) override;
  void OnContextDestroyed(const std::string& name) override;

  void TakeOver(
      const std::shared_ptr<lepus::InspectorLepusObserver>& other) override;

  void OnConsoleEvent(const std::string& func_name,
                      const std::string& args) override;

  void PrepareForScriptEval(const std::string& name) override;

 private:
  void CopyMembersFrom(
      const std::shared_ptr<InspectorLepusObserverImpl>& other);

  std::weak_ptr<InspectorLepusDebuggerImpl> debugger_wp_;
  std::weak_ptr<LynxDevToolMediator> mediator_ptr_;
  bool need_post_console_{false};
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_INSPECTOR_LEPUS_OBSERVER_IMPL_H_
