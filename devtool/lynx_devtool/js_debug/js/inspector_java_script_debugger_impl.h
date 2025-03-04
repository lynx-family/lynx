// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_INSPECTOR_JAVA_SCRIPT_DEBUGGER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_INSPECTOR_JAVA_SCRIPT_DEBUGGER_IMPL_H_

#include "devtool/lynx_devtool/js_debug/inspector_client_delegate_impl.h"
#include "devtool/lynx_devtool/js_debug/java_script_debugger_ng.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_runtime_observer_impl.h"

namespace lynx {
namespace devtool {

class InspectorJavaScriptDebuggerImpl : public JavaScriptDebuggerNG {
 public:
  explicit InspectorJavaScriptDebuggerImpl(
      const std::shared_ptr<lynx::devtool::LynxDevToolMediator>&
          devtool_mediator);
  ~InspectorJavaScriptDebuggerImpl() override;

  int GetViewId() override { return view_id_; }
  const std::shared_ptr<InspectorRuntimeObserverImpl>&
  GetInspectorRuntimeObserver();

  void OnInspectorInited(const std::string& vm_type, int64_t runtime_id,
                         const std::string& group_id, bool single_group,
                         const std::shared_ptr<InspectorClientNG>& client);
  void OnRuntimeDestroyed(int64_t runtime_id);
  void StopDebug();  // Only be called when destroying LynxView.

  void PrepareForScriptEval();

  void SetRuntimeEnableNeeded(bool enable) { runtime_enable_needed_ = enable; }

  void DispatchMessage(const std::string& message,
                       const std::string& session_id = "") override;

  void FlushConsoleMessages();
  void GetConsoleObject(const std::string& object_id, bool need_stringify,
                        int callback_id);
  void OnConsoleMessage(const std::string& message);
  void OnConsoleObject(const std::string& detail, int callback_id);

  void RunOnTargetThread(base::closure&& closure, bool run_now = true) override;

 private:
  std::shared_ptr<InspectorRuntimeObserverImpl> observer_;
  std::shared_ptr<InspectorClientDelegateImpl> delegate_;

  int view_id_;
  std::atomic_bool runtime_enable_needed_{false};
  std::mutex mutex_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_INSPECTOR_JAVA_SCRIPT_DEBUGGER_IMPL_H_
