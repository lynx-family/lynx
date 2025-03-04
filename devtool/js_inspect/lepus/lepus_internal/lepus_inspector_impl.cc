// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspector_impl.h"

#include "core/runtime/vm/lepus/context.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspected_context_provider.h"

namespace lepus_inspector {

namespace {

constexpr char kMesDebuggerPauseOnNextStatementPrefix[] =
    "{\"id\":0,\"method\":\"Debugger.pauseOnNextStatement\",\"params\":{"
    "\"reason\":\"";
constexpr char kMesDebuggerPauseOnNextStatementSuffix[] = "\"}}";

}  // namespace

// LepusInspectorSessionNGImpl begins.
std::unique_ptr<LepusInspectorSessionNGImpl>
LepusInspectorSessionNGImpl::Create(LepusInspectorNGImpl* inspector,
                                    LepusInspectorNG::LepusChannel* channel) {
  return std::unique_ptr<LepusInspectorSessionNGImpl>(
      new LepusInspectorSessionNGImpl(inspector, channel));
}

LepusInspectorSessionNGImpl::LepusInspectorSessionNGImpl(
    LepusInspectorNGImpl* inspector, LepusInspectorNG::LepusChannel* channel)
    : inspector_(inspector), channel_(channel) {}

LepusInspectorSessionNGImpl::~LepusInspectorSessionNGImpl() {
  inspector_->RemoveSession();
}

void LepusInspectorSessionNGImpl::DispatchProtocolMessage(
    const std::string& message) {
  inspector_->GetContext()->ProcessMessage(message);
}

void LepusInspectorSessionNGImpl::SchedulePauseOnNextStatement(
    const std::string& reason) {
  inspector_->GetContext()->ProcessMessage(
      kMesDebuggerPauseOnNextStatementPrefix + reason +
      kMesDebuggerPauseOnNextStatementSuffix);
}

void LepusInspectorSessionNGImpl::SendProtocolResponse(
    int call_id, const std::string& message) {
  channel_->SendResponse(call_id, message);
}

void LepusInspectorSessionNGImpl::SendProtocolNotification(
    const std::string& message) {
  channel_->SendNotification(message);
}
// LepusInspectorSessionNGImpl ends.

// LepusInspectorNGImpl begins.
std::unique_ptr<LepusInspectorNG> LepusInspectorNG::Create(
    lynx::lepus::Context* context, LepusInspectorClientNG* client,
    const std::string& name) {
  return std::unique_ptr<LepusInspectorNG>(std::unique_ptr<LepusInspectorNG>(
      new LepusInspectorNGImpl(context, client, name)));
}

LepusInspectorNGImpl::LepusInspectorNGImpl(lynx::lepus::Context* context,
                                           LepusInspectorClientNG* client,
                                           const std::string& name)
    : client_(client) {
  context_ =
      LepusInspectedContextProvider::GetInspectedContext(context, this, name);
}

std::unique_ptr<LepusInspectorSessionNG> LepusInspectorNGImpl::Connect(
    LepusChannel* channel) {
  std::unique_ptr<LepusInspectorSessionNGImpl> session =
      LepusInspectorSessionNGImpl::Create(this, channel);
  session_ = session.get();
  return session;
}

void LepusInspectorNGImpl::SetDebugInfo(const std::string& url,
                                        const std::string& debug_info) {
  context_->SetDebugInfo(url, debug_info);
}

// LepusInspectorNGImpl ends.

}  // namespace lepus_inspector
