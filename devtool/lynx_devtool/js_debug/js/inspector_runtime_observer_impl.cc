// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/inspector_runtime_observer_impl.h"

#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"
#include "devtool/lynx_devtool/js_debug/js/runtime_manager_delegate_impl.h"
#if !ENABLE_UNITTESTS
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
#include "devtool/lynx_devtool/js_debug/js/v8/manager/v8_inspector_manager_impl.h"
#endif
#include "devtool/lynx_devtool/js_debug/js/quickjs/manager/quickjs_inspector_manager_impl.h"
#endif
#include "devtool/lynx_devtool/js_debug/js/console_message_postman_impl.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"

namespace lynx {
namespace devtool {

InspectorRuntimeObserverImpl::InspectorRuntimeObserverImpl(
    const std::shared_ptr<InspectorJavaScriptDebuggerImpl>& debugger)
    : debugger_wp_(debugger) {
  view_id_ = debugger->GetViewId();
}

std::unique_ptr<runtime::RuntimeManagerDelegate>
InspectorRuntimeObserverImpl::CreateRuntimeManagerDelegate() {
  return std::make_unique<RuntimeManagerDelegateImpl>();
}

std::unique_ptr<piper::RuntimeInspectorManager>
InspectorRuntimeObserverImpl::CreateRuntimeInspectorManager(
    const std::string& vm_type) {
#if !ENABLE_UNITTESTS
  if (vm_type == kKeyEngineV8) {
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
    return std::make_unique<piper::V8InspectorManagerImpl>();
#endif
  } else if (vm_type == kKeyEngineQuickjs) {
    return std::make_unique<piper::QuickjsInspectorManagerImpl>();
  }
#endif
  return nullptr;
}

std::shared_ptr<piper::ConsoleMessagePostMan>
InspectorRuntimeObserverImpl::CreateConsoleMessagePostMan() {
  return std::make_shared<ConsoleMessagePostManImpl>();
}

void InspectorRuntimeObserverImpl::OnInspectorInited(
    const std::string& vm_type, int64_t runtime_id, const std::string& group_id,
    bool single_group,
    const std::shared_ptr<devtool::InspectorClientNG>& client) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->OnInspectorInited(vm_type, runtime_id, group_id, single_group, client);
  }
}

void InspectorRuntimeObserverImpl::OnRuntimeDestroyed(int64_t runtime_id) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->OnRuntimeDestroyed(runtime_id);
  }
}

void InspectorRuntimeObserverImpl::PrepareForScriptEval() {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->PrepareForScriptEval();
  }
}

void InspectorRuntimeObserverImpl::OnConsoleMessagePosted(
    const piper::ConsoleMessage& message) {
  auto sp = mediator_ptr_.lock();
  if (sp != nullptr) {
    sp->SendLogEntryAddedEvent(message);
  }
}

}  // namespace devtool
}  // namespace lynx
