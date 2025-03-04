// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/quickjs/quickjs_inspector_client_impl.h"

#include <atomic>

#include "base/include/no_destructor.h"
#include "devtool/fundamentals/js_inspect/inspector_client_delegate.h"
#include "devtool/js_inspect/inspector_const.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#ifdef __cplusplus
}
#endif

namespace lynx {
namespace devtool {

namespace {

int GenerateGroupId() {
  static base::NoDestructor<std::atomic<int>> id(1);
  return (*id)++;
}

}  // namespace

// QJSChannelImplNG begins.
QJSChannelImplNG::QJSChannelImplNG(
    const std::unique_ptr<quickjs_inspector::QJSInspector> &inspector,
    const std::shared_ptr<QJSInspectorClientImpl> &client,
    const std::string &group_id, int instance_id)
    : client_wp_(client), instance_id_(instance_id), group_id_(group_id) {
  session_ = inspector->Connect(this, group_id_, instance_id_);
}

void QJSChannelImplNG::SendResponse(int call_id, const std::string &message) {
  SendResponseToClient(message);
}

void QJSChannelImplNG::SendNotification(const std::string &message) {
  SendResponseToClient(message);
}

void QJSChannelImplNG::OnConsoleMessage(const std::string &message,
                                        int32_t runtime_id) {
  auto sp = client_wp_.lock();
  if (sp != nullptr) {
    sp->OnConsoleMessage(message, instance_id_, runtime_id);
  }
}

void QJSChannelImplNG::DispatchProtocolMessage(const std::string &message) {
  session_->DispatchProtocolMessage(message);
}

void QJSChannelImplNG::SchedulePauseOnNextStatement(const std::string &reason) {
  session_->SchedulePauseOnNextStatement(reason);
}

void QJSChannelImplNG::CancelPauseOnNextStatement() {
  session_->CancelPauseOnNextStatement();
}

void QJSChannelImplNG::SetEnableConsoleInspect(bool enable) {
  session_->SetEnableConsoleInspect(enable);
}

void QJSChannelImplNG::SendResponseToClient(const std::string &message) {
  auto sp = client_wp_.lock();
  if (sp != nullptr) {
    sp->SendResponse(message, instance_id_);
  }
}
// QJSChannelImplNG ends.

// QJSInspectorClientImpl begins.
void QJSInspectorClientImpl::RunMessageLoopOnPause(
    const std::string &group_id) {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->RunMessageLoopOnPause(group_id);
  }
}

void QJSInspectorClientImpl::QuitMessageLoopOnPause() {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->QuitMessageLoopOnPause();
  }
}

bool QJSInspectorClientImpl::IsFullFuncEnabled() {
  if (full_func_enable_callback_ != nullptr) {
    return full_func_enable_callback_();
  }
  return true;
}

void QJSInspectorClientImpl::SetStopAtEntry(bool stop_at_entry,
                                            int instance_id) {
  auto it = channels_.find(instance_id);
  if (it != channels_.end()) {
    if (stop_at_entry) {
      it->second->SchedulePauseOnNextStatement(kStopAtEntryReason);
    } else {
      it->second->CancelPauseOnNextStatement();
    }
  }
}

void QJSInspectorClientImpl::DispatchMessage(const std::string &message,
                                             int instance_id) {
  auto it = channels_.find(instance_id);
  if (it != channels_.end()) {
    it->second->DispatchProtocolMessage(message);
  }
}

void QJSInspectorClientImpl::SetEnableConsoleInspect(bool enable,
                                                     int instance_id) {
  auto it = channels_.find(instance_id);
  if (it != channels_.end()) {
    it->second->SetEnableConsoleInspect(enable);
  }
}

void QJSInspectorClientImpl::GetConsoleObject(
    const std::string &object_id, const std::string &group_id,
    std::function<void(const std::string &)> callback) {
  auto it = contexts_.find(group_id);
  if (it != contexts_.end()) {
    auto *context = it->second;
    auto *res = ::GetConsoleObject(context, object_id.c_str());
    if (res != nullptr) {
      callback(res);
    }
    if (!LEPUS_IsGCMode(context)) LEPUS_FreeCString(context, res);
  }
}

void QJSInspectorClientImpl::OnConsoleMessage(const std::string &message,
                                              int instance_id, int runtime_id) {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->OnConsoleMessage(message, instance_id, runtime_id);
  }
}

std::string QJSInspectorClientImpl::InitInspector(LEPUSContext *context,
                                                  const std::string &group_id,
                                                  const std::string &name) {
  std::string group = MapGroupId(group_id);
  CreateQJSInspector(context, group, name);
  SetContext(context, group);
  return group;
}

void QJSInspectorClientImpl::ConnectSession(int instance_id,
                                            const std::string &group_id) {
  if (channels_.find(instance_id) == channels_.end()) {
    auto it = inspectors_.find(group_id);
    if (it != inspectors_.end()) {
      channels_.emplace(instance_id,
                        std::make_shared<QJSChannelImplNG>(
                            it->second,
                            std::static_pointer_cast<QJSInspectorClientImpl>(
                                shared_from_this()),
                            group_id, instance_id));
    }
  }
}

void QJSInspectorClientImpl::DisconnectSession(int instance_id) {
  auto it = channels_.find(instance_id);
  if (it != channels_.end()) {
    auto &group_id = it->second->GroupId();
    channels_.erase(it);
    auto sp = delegate_wp_.lock();
    if (sp != nullptr) {
      sp->OnSessionDestroyed(instance_id, group_id);
    }
  }
}

void QJSInspectorClientImpl::DestroyInspector(const std::string &group_id) {
  inspectors_.erase(group_id);
  auto it = contexts_.find(group_id);
  if (it != contexts_.end()) {
    int context_id = GetExecutionContextId(it->second);
    contexts_.erase(it);
    auto sp = delegate_wp_.lock();
    if (sp != nullptr) {
      sp->OnContextDestroyed(group_id, context_id);
    }
  }
}

void QJSInspectorClientImpl::SetFullFuncEnableCallback(
    std::function<bool()> callback) {
  full_func_enable_callback_ = callback;
}

void QJSInspectorClientImpl::RemoveScript(const std::string &group_id,
                                          const std::string &url) {
  auto it = contexts_.find(group_id);
  if (it != contexts_.end()) {
    DeleteScriptByURL(it->second, url.c_str());
  }
}

void QJSInspectorClientImpl::RemoveConsole(const std::string &group_id,
                                           int runtime_id) {
  auto it = contexts_.find(group_id);
  if (it != contexts_.end()) {
    DeleteConsoleMessageWithRID(it->second, runtime_id);
  }
}

void QJSInspectorClientImpl::SetContext(LEPUSContext *context,
                                        const std::string &group_id) {
  auto it = contexts_.find(group_id);
  if (it == contexts_.end()) {
    contexts_.emplace(group_id, context);
  }
}

void QJSInspectorClientImpl::CreateQJSInspector(LEPUSContext *context,
                                                const std::string &group_id,
                                                const std::string &name) {
  auto it = inspectors_.find(group_id);
  if (it == inspectors_.end()) {
    inspectors_.emplace(group_id, quickjs_inspector::QJSInspector::Create(
                                      context, this, group_id, name));
  }
}

std::string QJSInspectorClientImpl::MapGroupId(const std::string &group_id) {
  if (group_id == kSingleGroupStr) {
    return kSingleGroupPrefix + std::to_string(GenerateGroupId());
  } else {
    return group_id;
  }
}
// QJSInspectorClientImpl ends.

}  // namespace devtool
}  // namespace lynx
