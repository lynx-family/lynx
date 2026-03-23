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

  void SetPreExecute(bool pre_execute);
  void TakeOver(const std::shared_ptr<InspectorLepusDebuggerImpl>& other);

  const std::shared_ptr<InspectorLepusObserverImpl>&
  GetInspectorLepusObserver();
  void DecodeDebugInfo(const std::string& debug_info, std::string& result);
  std::string GetDebugInfo(const std::string& url);
  void SetDebugInfoUrl(const std::string& url, const std::string& file_name);
  std::string GetDebugInfoUrl(const std::string& file_name);

  void OnInspectorInited(
      const std::string& vm_type, const std::string& name,
      const std::shared_ptr<devtool::InspectorClientNG>& client);
  void OnContextDestroyed(const std::string& name);

  void PrepareForScriptEval(const std::string& name);

  void DispatchMessage(const std::string& message,
                       const std::string& session_id) override;

  void RunOnTargetThread(base::closure&& closure, bool run_now = true) override;

  void UpdateTarget();

  void SetRecordID(int64_t record_id);

 private:
  std::shared_ptr<InspectorLepusObserverImpl> observer_;
  // There may be multiple lepus contexts if the LynxView contains lazy
  // components, and each context needs a delegate. So we use a map to manage
  // them, with the context name as the key.
  std::unordered_map<std::string, std::shared_ptr<InspectorClientDelegateImpl>>
      delegates_;
  std::unordered_map<std::string, std::string> file_name_to_debug_info_url_;

  std::mutex mutex_;

  ALLOW_UNUSED_TYPE int64_t record_id_ = 0;

  bool pre_execute_{false};
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_INSPECTOR_LEPUS_DEBUGGER_IMPL_H_
