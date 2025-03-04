// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_inspector_client_impl.h"

#include "devtool/fundamentals/js_inspect/inspector_client_delegate.h"
#include "devtool/js_inspect/inspector_const.h"

namespace lynx {
namespace devtool {

// LepusChannelImplNG begins.
LepusChannelImplNG::LepusChannelImplNG(
    const std::unique_ptr<lepus_inspector::LepusInspectorNG> &inspector,
    const std::shared_ptr<LepusInspectorClientImpl> &client)
    : client_wp_(client) {
  session_ = inspector->Connect(this);
}

void LepusChannelImplNG::SendResponse(int call_id, const std::string &message) {
  SendResponseToClient(message);
}

void LepusChannelImplNG::SendNotification(const std::string &message) {
  SendResponseToClient(message);
}

void LepusChannelImplNG::DispatchProtocolMessage(const std::string &message) {
  session_->DispatchProtocolMessage(message);
}

void LepusChannelImplNG::SchedulePauseOnNextStatement(
    const std::string &reason) {
  session_->SchedulePauseOnNextStatement(reason);
}

void LepusChannelImplNG::CancelPauseOnNextStatement() {
  session_->CancelPauseOnNextStatement();
}

void LepusChannelImplNG::SendResponseToClient(const std::string &message) {
  auto sp = client_wp_.lock();
  if (sp != nullptr) {
    sp->SendResponse(message, kDefaultViewID);
  }
}
// LepusChannelImplNG ends.

// LepusInspectorClientImpl begins.
void LepusInspectorClientImpl::RunMessageLoopOnPause() {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->RunMessageLoopOnPause(kSingleGroupStr);
  }
}

void LepusInspectorClientImpl::QuitMessageLoopOnPause() {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->QuitMessageLoopOnPause();
  }
}

void LepusInspectorClientImpl::SetStopAtEntry(bool stop_at_entry,
                                              int instance_id) {
  if (channel_ != nullptr) {
    if (stop_at_entry) {
      channel_->SchedulePauseOnNextStatement(kStopAtEntryReason);
    } else {
      channel_->CancelPauseOnNextStatement();
    }
  }
}

void LepusInspectorClientImpl::DispatchMessage(const std::string &message,
                                               int instance_id) {
  if (channel_ != nullptr) {
    channel_->DispatchProtocolMessage(message);
  }
}

void LepusInspectorClientImpl::InitInspector(lepus::Context *context,
                                             const std::string &name) {
  inspector_ = lepus_inspector::LepusInspectorNG::Create(context, this, name);
}

void LepusInspectorClientImpl::SetDebugInfo(const std::string &url,
                                            const std::string &debug_info) {
  inspector_->SetDebugInfo(url, debug_info);
}

void LepusInspectorClientImpl::ConnectSession() {
  channel_ = std::make_shared<LepusChannelImplNG>(
      inspector_,
      std::static_pointer_cast<LepusInspectorClientImpl>(shared_from_this()));
}

void LepusInspectorClientImpl::DisconnectSession() { channel_.reset(); }

void LepusInspectorClientImpl::DestroyInspector() { inspector_.reset(); }
// LepusInspectorClientImpl ends.

}  // namespace devtool
}  // namespace lynx
