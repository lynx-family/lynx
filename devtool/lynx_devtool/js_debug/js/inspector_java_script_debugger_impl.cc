// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"

#include "base/include/no_destructor.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/lynx_env.h"
#include "devtool/lynx_devtool/config/devtool_config.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"

namespace lynx {
namespace devtool {

namespace {

int GenerateViewId() {
  static base::NoDestructor<std::atomic<int>> id{1};
  return (*id)++;
}

}  // namespace

InspectorJavaScriptDebuggerImpl::InspectorJavaScriptDebuggerImpl(
    const std::shared_ptr<lynx::devtool::LynxDevToolMediator>& devtool_mediator)
    : JavaScriptDebuggerNG(devtool_mediator) {
  view_id_ = GenerateViewId();
}

InspectorJavaScriptDebuggerImpl::~InspectorJavaScriptDebuggerImpl() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (delegate_ != nullptr) {
    RunOnTargetThread([delegate = delegate_, view_id = view_id_]() {
      delegate->RemoveDebugger(view_id);
    });
  }
}

const std::shared_ptr<InspectorRuntimeObserverImpl>&
InspectorJavaScriptDebuggerImpl::GetInspectorRuntimeObserver() {
  if (observer_ == nullptr) {
    observer_ = std::make_shared<InspectorRuntimeObserverImpl>(
        std::static_pointer_cast<InspectorJavaScriptDebuggerImpl>(
            shared_from_this()));
  }
  return observer_;
}

void InspectorJavaScriptDebuggerImpl::OnInspectorInited(
    const std::string& vm_type, int64_t runtime_id, const std::string& group_id,
    bool single_group, const std::shared_ptr<InspectorClientNG>& client) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (delegate_ == nullptr) {
    delegate_ =
        InspectorClientDelegateProvider::GetInstance()->GetDelegate(vm_type);
    delegate_->InsertDebugger(
        std::static_pointer_cast<InspectorJavaScriptDebuggerImpl>(
            shared_from_this()),
        single_group);
    // There are two kinds of delegate instances (V8/Quickjs), so we cannot call
    // SetInspectorClient/SetInspectorClientDelegate once.
    delegate_->SetInspectorClient(client);
    client->SetInspectorClientDelegate(delegate_);
  }

  delegate_->OnInspectorInited(view_id_, runtime_id, group_id);

  // TODO(lqy): These functions(SetBreakpointWhenReload() and SetStopAtEntry())
  // may not be called after reloading when using reloadTemplate, because
  // Runtime won't be destroyed and reconstruct, so we need another interface to
  // call them.
  if (tasm::LynxEnv::GetInstance().IsDevToolConnected()) {
    // When first time the LynxView is created:
    // If OnInspectorInited is called earlier than enable messages received from
    // the frontend, we need to send Debugger.enable to the JS engine if we
    // need to stop executing of the js at entry, but we cannot send
    // Runtime.enable then (too early), since the frontend can process
    // Runtime.consoleAPICalled messages only after receiving the response of
    // Page.getResourceTree.
    // If the JS thread is busy and the enable messages received from the
    // frontend is earlier than OnInspectorInited is called, these messages
    // cannot be processed, so we need to send
    // Debugger.enable/Runtime.enable/Profiler.enable to the JS engine actively.
    //
    // After the LynxView is reloading:
    // We need to send Debugger.enable/Runtime.enable/Profiler.enable to the JS
    // engine, since the frontend won't send, and some breakpoints may be
    // triggered very early, we must send these messages fisrt to avoid missing
    // the triggering time.
    delegate_->DispatchInitMessage(view_id_, runtime_enable_needed_);
    delegate_->SetStopAtEntry(DevToolConfig::ShouldStopAtEntry(), view_id_);
  }
}

void InspectorJavaScriptDebuggerImpl::OnRuntimeDestroyed(int64_t runtime_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (delegate_ != nullptr) {
    delegate_->OnRuntimeDestroyed(view_id_, runtime_id);
  }
}

void InspectorJavaScriptDebuggerImpl::StopDebug() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (delegate_ != nullptr) {
    delegate_->StopDebug(view_id_);
  }
}

void InspectorJavaScriptDebuggerImpl::PrepareForScriptEval() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (delegate_ != nullptr) {
    delegate_->SetStopAtEntry(DevToolConfig::ShouldStopAtEntry(), view_id_);
  }
}

void InspectorJavaScriptDebuggerImpl::DispatchMessage(
    const std::string& message, const std::string& session_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (delegate_ != nullptr) {
    delegate_->DispatchMessageAsync(message, view_id_);
  } else {
    // TODO(lqy): Delete after e2e can send Page.getResourceTree message.
    if (message.find(kMethodRuntimeEnable) != std::string::npos) {
      SetRuntimeEnableNeeded(true);
    }
  }
}

void InspectorJavaScriptDebuggerImpl::FlushConsoleMessages() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (delegate_ != nullptr) {
    RunOnTargetThread([delegate = delegate_, view_id = view_id_]() {
      delegate->FlushConsoleMessages(view_id);
    });
  } else {
    auto ref = std::static_pointer_cast<InspectorJavaScriptDebuggerImpl>(
        shared_from_this());
    RunOnTargetThread([ref]() { ref->FlushConsoleMessages(); }, false);
  }
}

void InspectorJavaScriptDebuggerImpl::GetConsoleObject(
    const std::string& object_id, bool need_stringify, int callback_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (delegate_ != nullptr) {
    RunOnTargetThread([delegate = delegate_, object_id, need_stringify,
                       callback_id, view_id = view_id_]() {
      delegate->GetConsoleObject(object_id, view_id, need_stringify,
                                 callback_id);
    });
  }
}

void InspectorJavaScriptDebuggerImpl::OnConsoleMessage(
    const std::string& message) {
  auto sp = devtool_platform_facade_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(sp, "js debug: devtool_platform_facade_ is null");
  sp->OnConsoleMessage(message);
}

void InspectorJavaScriptDebuggerImpl::OnConsoleObject(const std::string& detail,
                                                      int callback_id) {
  auto sp = devtool_platform_facade_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(sp, "js debug: devtool_platform_facade_ is null");
  sp->OnConsoleObject(detail, callback_id);
}

void InspectorJavaScriptDebuggerImpl::RunOnTargetThread(base::closure&& closure,
                                                        bool run_now) {
  auto sp = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(sp, "js debug: devtool_mediator_ is null");
  sp->RunOnJSThread(std::move(closure), run_now);
}

}  // namespace devtool
}  // namespace lynx
