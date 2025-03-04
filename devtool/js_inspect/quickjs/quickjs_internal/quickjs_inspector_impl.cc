// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspector_impl.h"

#include <utility>

namespace quickjs_inspector {

namespace {

constexpr char kMesDebuggerPauseOnNextStatementPrefix[] =
    "{\"id\":0,\"method\":\"Debugger.pauseOnNextStatement\",\"params\":{"
    "\"reason\":\"";
constexpr char kMesDebuggerPauseOnNextStatementSuffix[] = "\"}}";

}  // namespace

// QJSInspectorSessionImpl begins.
std::unique_ptr<QJSInspectorSessionImpl> QJSInspectorSessionImpl::Create(
    QJSInspectorImpl* inspector, int32_t session_id,
    QJSInspector::QJSChannel* channel) {
  return std::unique_ptr<QJSInspectorSessionImpl>(
      new QJSInspectorSessionImpl(inspector, session_id, channel));
}

QJSInspectorSessionImpl::QJSInspectorSessionImpl(
    QJSInspectorImpl* inspector, int32_t session_id,
    QJSInspector::QJSChannel* channel)
    : channel_(channel), inspector_(inspector), session_id_(session_id) {
  inspector_->GetContext()->GetDebugger()->InitEnableState(session_id);
}

QJSInspectorSessionImpl::~QJSInspectorSessionImpl() {
  inspector_->RemoveSession(session_id_);
  inspector_->GetContext()->GetDebugger()->RemoveEnableState(session_id_);
}

void QJSInspectorSessionImpl::DispatchProtocolMessage(
    const std::string& message) {
  inspector_->GetContext()->GetDebugger()->ProcessPausedMessages(message,
                                                                 session_id_);
}

void QJSInspectorSessionImpl::SchedulePauseOnNextStatement(
    const std::string& reason) {
  inspector_->GetContext()->GetDebugger()->ProcessPausedMessages(
      kMesDebuggerPauseOnNextStatementPrefix + reason +
          kMesDebuggerPauseOnNextStatementSuffix,
      session_id_);
}

void QJSInspectorSessionImpl::SetEnableConsoleInspect(bool enable) {
  inspector_->GetContext()->GetDebugger()->SetContextConsoleInspect(
      enable, session_id_);
}

void QJSInspectorSessionImpl::SendProtocolResponse(int callId,
                                                   const std::string& message) {
  channel_->SendResponse(callId, message);
}

void QJSInspectorSessionImpl::SendProtocolNotification(
    const std::string& message) {
  channel_->SendNotification(message);
}

void QJSInspectorSessionImpl::OnConsoleMessage(const std::string& message,
                                               int32_t runtime_id) {
  channel_->OnConsoleMessage(message, runtime_id);
}
// QJSInspectorSessionImpl ends.

// QJSInspectorImpl begins.
QJSInspectorImpl::QJSInspectorImpl(LEPUSContext* ctx,
                                   QJSInspectorClient* client,
                                   const std::string& group_id,
                                   const std::string& name)
    : client_(client), group_id_(group_id) {
  context_ = std::make_unique<QJSInspectedContext>(this, ctx, name);
}

std::unique_ptr<QJSInspector> QJSInspector::Create(LEPUSContext* ctx,
                                                   QJSInspectorClient* client,
                                                   const std::string& group_id,
                                                   const std::string& name) {
  std::unique_ptr<QJSInspector> inspector = std::unique_ptr<QJSInspector>(
      new QJSInspectorImpl(ctx, client, group_id, name));
  return inspector;
}

std::unique_ptr<QJSInspectorSession> QJSInspectorImpl::Connect(
    QJSChannel* channel, const std::string& group_id, int32_t session_id) {
  std::unique_ptr<QJSInspectorSessionImpl> session =
      QJSInspectorSessionImpl::Create(this, session_id, channel);
  sessions_[session_id] = session.get();
  return std::move(session);
}

QJSInspectorSessionImpl* QJSInspectorImpl::GetSession(int32_t session_id) {
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    return it->second;
  }
  return nullptr;
}

void QJSInspectorImpl::RemoveSession(int32_t session_id) {
  sessions_.erase(session_id);
}

bool QJSInspectorImpl::IsFullFuncEnabled() {
  return client_->IsFullFuncEnabled();
}
// QJSInspectorImpl ends.

}  // namespace quickjs_inspector
