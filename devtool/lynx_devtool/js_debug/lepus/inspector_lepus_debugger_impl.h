// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_INSPECTOR_LEPUS_DEBUGGER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_INSPECTOR_LEPUS_DEBUGGER_IMPL_H_

#include <memory>
#include <unordered_map>

#include "devtool/lynx_devtool/js_debug/inspector_client_delegate_impl.h"
#include "devtool/lynx_devtool/js_debug/java_script_debugger_ng.h"
#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_observer_impl.h"

namespace lynx {
namespace devtool {

class InspectorLepusDebuggerImpl : public JavaScriptDebuggerNG {
 public:
  explicit InspectorLepusDebuggerImpl(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorLepusDebuggerImpl() override = default;

  const std::shared_ptr<InspectorLepusObserverImpl>&
  GetInspectorLepusObserver();
  std::string GetDebugInfo(const std::string& url);
  void SetDebugInfoUrl(const std::string& url);

  void OnInspectorInited(
      const std::string& vm_type, const std::string& name,
      const std::shared_ptr<devtool::InspectorClientNG>& client);
  void OnContextDestroyed(const std::string& name);

  void DispatchMessage(const std::string& message,
                       const std::string& session_id) override;

  void RunOnTargetThread(base::closure&& closure, bool run_now = true) override;

 private:
  std::shared_ptr<InspectorLepusObserverImpl> observer_;
  // There may be multiple lepus contexts if the LynxView contains lazy
  // components, and each context needs a delegate. So we use a map to manage
  // them, with the context name as the key.
  std::unordered_map<std::string, std::shared_ptr<InspectorClientDelegateImpl>>
      delegates_;

  std::mutex mutex_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_INSPECTOR_LEPUS_DEBUGGER_IMPL_H_
