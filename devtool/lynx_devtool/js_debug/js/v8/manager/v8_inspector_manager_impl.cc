// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/v8/manager/v8_inspector_manager_impl.h"

#include "core/runtime/jsi/v8/v8_runtime.h"
#include "core/runtime/piper/js/js_executor.h"
#include "core/runtime/piper/js/runtime_manager.h"
#include "devtool/js_inspect/v8/v8_inspector_client_provider.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"

namespace lynx {
namespace piper {

void V8InspectorManagerImpl::InitInspector(
    Runtime *runtime,
    const std::shared_ptr<InspectorRuntimeObserverNG> &observer) {
  observer_wp_ = observer;
  inspector_client_ =
      devtool::V8InspectorClientProvider::GetInstance()->GetInspectorClient();
  auto v8_runtime = static_cast<V8Runtime *>(runtime);
  group_id_ = v8_runtime->getGroupId();
  instance_id_ = observer->GetViewId();
  runtime_id_ = v8_runtime->getRuntimeId();

  Scope scope(*v8_runtime);
  inspector_group_id_ = inspector_client_->InitInspector(
      v8_runtime->getIsolate(), v8_runtime->getContext(), group_id_,
      devtool::kTargetJSPrefix + group_id_);
  inspector_client_->ConnectSession(instance_id_, inspector_group_id_);

  static thread_local std::once_flag set_release_vm_callback;
  static thread_local std::once_flag set_release_ctx_callback;
  std::call_once(
      set_release_vm_callback, [inspector_client = inspector_client_] {
        auto runtime_manager_delegate =
            JSExecutor::GetCurrentRuntimeManagerInstance()
                ->GetRuntimeManagerDelegate();
        runtime_manager_delegate->SetReleaseVMCallback(
            piper::JSRuntimeType::v8,
            [inspector_client]() { inspector_client->DestroyInspector(); });
      });
  if (group_id_ != devtool::kSingleGroupStr) {
    std::call_once(set_release_ctx_callback,
                   [inspector_client = inspector_client_] {
                     auto runtime_manager_delegate =
                         JSExecutor::GetCurrentRuntimeManagerInstance()
                             ->GetRuntimeManagerDelegate();
                     runtime_manager_delegate->SetReleaseContextCallback(
                         piper::JSRuntimeType::v8,
                         [inspector_client](const std::string &group_id) {
                           inspector_client->DestroyContext(group_id);
                         });
                   });
  }

  observer->OnInspectorInited(
      devtool::kKeyEngineV8, runtime_id_, std::to_string(inspector_group_id_),
      group_id_ == devtool::kSingleGroupStr, inspector_client_);
}

void V8InspectorManagerImpl::DestroyInspector() {
  auto sp = observer_wp_.lock();
  if (sp != nullptr) {
    sp->OnRuntimeDestroyed(runtime_id_);
  }
  if (inspector_client_ != nullptr) {
    inspector_client_->DisconnectSession(instance_id_);
    // Only call DestroyContext() when using single group, because the
    // v8::Context will be destroyed.
    if (group_id_ == devtool::kSingleGroupStr) {
      inspector_client_->DestroyContext(inspector_group_id_);
    }
  }
}

void V8InspectorManagerImpl::PrepareForScriptEval() {
  auto sp = observer_wp_.lock();
  if (sp != nullptr) {
    sp->PrepareForScriptEval();
  }
}

}  // namespace piper
}  // namespace lynx
