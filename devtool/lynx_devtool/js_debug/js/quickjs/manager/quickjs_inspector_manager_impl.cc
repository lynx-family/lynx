// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/quickjs/manager/quickjs_inspector_manager_impl.h"

#include "core/runtime/jsi/quickjs/quickjs_runtime.h"
#include "core/runtime/piper/js/js_executor.h"
#include "core/runtime/piper/js/runtime_manager.h"
#include "devtool/js_inspect/quickjs/quickjs_inspector_client_provider.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"

namespace lynx {
namespace piper {

void QuickjsInspectorManagerImpl::InitInspector(
    Runtime *runtime,
    const std::shared_ptr<InspectorRuntimeObserverNG> &observer) {
  observer_wp_ = observer;
  inspector_client_ = devtool::QuickjsInspectorClientProvider::GetInstance()
                          ->GetInspectorClient();
  auto quickjs_runtime = static_cast<QuickjsRuntime *>(runtime);
  runtime_id_ = quickjs_runtime->getRuntimeId();
  instance_id_ = observer->GetViewId();
  group_id_ = quickjs_runtime->getGroupId();

  static thread_local std::once_flag set_full_func_callback;
  std::call_once(
      set_full_func_callback, [inspector_client = inspector_client_] {
        inspector_client->SetFullFuncEnableCallback(
            []() { return tasm::LynxEnv::GetInstance().IsDevToolConnected(); });
      });

  inspector_group_id_ = inspector_client_->InitInspector(
      quickjs_runtime->getJSContext(), group_id_,
      devtool::kTargetJSPrefix + group_id_);
  inspector_client_->ConnectSession(instance_id_, inspector_group_id_);

  static thread_local std::once_flag set_release_ctx_callback;
  if (group_id_ != devtool::kSingleGroupStr) {
    std::call_once(set_release_ctx_callback,
                   [inspector_client = inspector_client_] {
                     auto runtime_manager_delegate =
                         JSExecutor::GetCurrentRuntimeManagerInstance()
                             ->GetRuntimeManagerDelegate();
                     runtime_manager_delegate->SetReleaseContextCallback(
                         piper::JSRuntimeType::quickjs,
                         [inspector_client](const std::string &group_id) {
                           inspector_client->DestroyInspector(group_id);
                         });
                   });
  }

  observer->OnInspectorInited(
      devtool::kKeyEngineQuickjs, runtime_id_, inspector_group_id_,
      group_id_ == devtool::kSingleGroupStr, inspector_client_);
}

void QuickjsInspectorManagerImpl::DestroyInspector() {
  auto sp = observer_wp_.lock();
  if (sp != nullptr) {
    sp->OnRuntimeDestroyed(runtime_id_);
  }
  if (inspector_client_ != nullptr) {
    inspector_client_->DisconnectSession(instance_id_);
    // Only call DestroyInspector() when using single group, because the
    // LEPUSContext will be destroyed.
    if (group_id_ == devtool::kSingleGroupStr) {
      inspector_client_->DestroyInspector(inspector_group_id_);
    } else {
      // Remove scripts and console messages saved in inspector when using
      // shared-context.
      // TODO(lqy): If using reloadTemplate, we also need to call these
      // functions when reloading.
      for (const auto &url : scripts_) {
        inspector_client_->RemoveScript(inspector_group_id_, url);
      }
      inspector_client_->RemoveConsole(inspector_group_id_, runtime_id_);
    }
  }
}

void QuickjsInspectorManagerImpl::InsertScript(const std::string &url) {
  scripts_.emplace(url);
}

void QuickjsInspectorManagerImpl::PrepareForScriptEval() {
  auto sp = observer_wp_.lock();
  if (sp != nullptr) {
    sp->PrepareForScriptEval();
  }
}

}  // namespace piper
}  // namespace lynx
