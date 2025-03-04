// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_INSPECTOR_RUNTIME_OBSERVER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_INSPECTOR_RUNTIME_OBSERVER_IMPL_H_

#include "core/inspector/observer/inspector_runtime_observer_ng.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"

namespace lynx {
namespace devtool {
class InspectorJavaScriptDebuggerImpl;
class LynxDevToolNG;

class InspectorRuntimeObserverImpl : public piper::InspectorRuntimeObserverNG {
 public:
  explicit InspectorRuntimeObserverImpl(
      const std::shared_ptr<InspectorJavaScriptDebuggerImpl>& debugger);
  ~InspectorRuntimeObserverImpl() override = default;

  void SetDevToolMediator(
      const std::shared_ptr<LynxDevToolMediator>& mediator_ptr) {
    mediator_ptr_ = mediator_ptr;
  }

  int GetViewId() override { return view_id_; }
  std::unique_ptr<runtime::RuntimeManagerDelegate>
  CreateRuntimeManagerDelegate() override;
  std::unique_ptr<piper::RuntimeInspectorManager> CreateRuntimeInspectorManager(
      const std::string& vm_type) override;
  std::shared_ptr<piper::ConsoleMessagePostMan> CreateConsoleMessagePostMan()
      override;

  // Runtime may be destroyed after reloading but InspectorRuntimeObserverImpl
  // only be destroyed when destroying the LynxView, so that the runtime_id may
  // be changed and we cannot save it.
  void OnInspectorInited(
      const std::string& vm_type, int64_t runtime_id,
      const std::string& group_id, bool single_group,
      const std::shared_ptr<devtool::InspectorClientNG>& client) override;
  void OnRuntimeDestroyed(int64_t runtime_id) override;

  void PrepareForScriptEval() override;

  void OnConsoleMessagePosted(const piper::ConsoleMessage& message);

 private:
  std::weak_ptr<InspectorJavaScriptDebuggerImpl> debugger_wp_;
  std::weak_ptr<LynxDevToolMediator> mediator_ptr_;
  int view_id_{-1};
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_INSPECTOR_RUNTIME_OBSERVER_IMPL_H_
