// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_debugger_impl.h"

namespace lynx {
namespace devtool {

InspectorLepusDebuggerImpl::InspectorLepusDebuggerImpl(
    const std::shared_ptr<LynxDevToolMediator> &devtool_mediator)
    : JavaScriptDebuggerNG(devtool_mediator) {}

const std::shared_ptr<InspectorLepusObserverImpl> &
InspectorLepusDebuggerImpl::GetInspectorLepusObserver() {
  if (observer_ == nullptr) {
    observer_ = std::make_shared<InspectorLepusObserverImpl>(
        std::static_pointer_cast<InspectorLepusDebuggerImpl>(
            shared_from_this()));
  }
  return observer_;
}

std::string InspectorLepusDebuggerImpl::GetDebugInfo(const std::string &url) {
  auto sp = devtool_platform_facade_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(
      sp, "lepus debug: devtool_platform_facade_ is null", "");
  return sp->GetLepusDebugInfo(url);
}

void InspectorLepusDebuggerImpl::SetDebugInfoUrl(const std::string &url) {
  auto sp = devtool_platform_facade_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(sp,
                            "lepus debug: devtool_platform_facade_ is null");
  sp->SetLepusDebugInfoUrl(url);
}

void InspectorLepusDebuggerImpl::OnInspectorInited(
    const std::string &vm_type, const std::string &name,
    const std::shared_ptr<devtool::InspectorClientNG> &client) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = delegates_.find(name);
  if (it == delegates_.end()) {
    auto delegate =
        InspectorClientDelegateProvider::GetInstance()->GetDelegate(vm_type);
    delegate->InsertDebugger(
        std::static_pointer_cast<InspectorLepusDebuggerImpl>(
            shared_from_this()),
        true);
    delegate->SetTargetId(name);
    it = (delegates_.emplace(name, delegate)).first;
  }
  // InspectorClientNG will be destroyed and recreated after reloading, so we
  // need to reset the pointer.
  auto delegate = it->second;
  delegate->SetInspectorClient(client);
  client->SetInspectorClientDelegate(delegate);

  delegate->OnTargetCreated();
  delegate->DispatchInitMessage(kDefaultViewID, false);
  delegate->SetStopAtEntry(true, kDefaultViewID);
}

void InspectorLepusDebuggerImpl::OnContextDestroyed(const std::string &name) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = delegates_.find(name);
  if (it != delegates_.end()) {
    it->second->OnTargetDestroyed();
  }
}

void InspectorLepusDebuggerImpl::DispatchMessage(
    const std::string &message, const std::string &session_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = delegates_.find(session_id);
  if (it != delegates_.end()) {
    it->second->DispatchMessageAsync(message, kDefaultViewID);
  }
}

void InspectorLepusDebuggerImpl::RunOnTargetThread(base::closure &&closure,
                                                   bool run_now) {
  auto sp = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(sp, "lepus debug: devtool_mediator_ is null");
  sp->RunOnTASMThread(std::move(closure), run_now);
}

}  // namespace devtool
}  // namespace lynx
