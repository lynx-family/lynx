// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_debugger_ng.h"

#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspected_context.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspector_impl.h"

namespace quickjs_inspector {

namespace {

enum EnableType {
  kKeyDebuggerEnable = 0,
  kKeyRuntimeEnable,
  kKeyProfilerEnable,
  kKeyConsoleInspectEnable,
};

constexpr char kMesDebuggerDisable[] =
    "{\"id\": 0, \"method\": \"Debugger.disable\"}";
constexpr char kMesRuntimeDisable[] =
    "{\"id\": 0, \"method\": \"Runtime.disable\"}";
constexpr char kMesProfilerDisable[] =
    "{\"id\": 0, \"method\": \"Profiler.disable\"}";

}  // namespace

QuickjsDebugger::QuickjsDebugger(QJSInspectedContext *context,
                                 const std::string &name)
    : context_(context), inspector_(context->GetInspector()) {
  // Initialize related debugger info
  QJSDebuggerInitialize(context_->GetContext());
  SetJSDebuggerName(context->GetContext(), name.c_str());
}

QuickjsDebugger::~QuickjsDebugger() { QJSDebuggerFree(context_->GetContext()); }

void QuickjsDebugger::SetDebuggerEnableState(int32_t session_id, bool value) {
  session_enable_map_[session_id][kKeyDebuggerEnable] = value;
}

void QuickjsDebugger::SetRuntimeEnableState(int32_t session_id, bool value) {
  session_enable_map_[session_id][kKeyRuntimeEnable] = value;
}

void QuickjsDebugger::SetProfilerEnableState(int32_t session_id, bool value) {
  session_enable_map_[session_id][kKeyProfilerEnable] = value;
}

void QuickjsDebugger::SetConsoleInspectEnableState(int32_t session_id,
                                                   bool value) {
  session_enable_map_[session_id][kKeyConsoleInspectEnable] = value;
}

bool QuickjsDebugger::GetDebuggerEnableState(int32_t session_id) const {
  auto it = session_enable_map_.find(session_id);
  if (it == session_enable_map_.end()) {
    return false;
  }
  return it->second[kKeyDebuggerEnable];
}

bool QuickjsDebugger::GetRuntimeEnableState(int32_t session_id) const {
  auto it = session_enable_map_.find(session_id);
  if (it == session_enable_map_.end()) {
    return false;
  }
  return it->second[kKeyRuntimeEnable];
}

bool QuickjsDebugger::GetProfilerEnableState(int32_t session_id) const {
  auto it = session_enable_map_.find(session_id);
  if (it == session_enable_map_.end()) {
    return false;
  }
  return it->second[kKeyProfilerEnable];
}

bool QuickjsDebugger::GetConsoleInspectEnableState(int32_t session_id) const {
  auto it = session_enable_map_.find(session_id);
  if (it == session_enable_map_.end()) {
    return false;
  }
  return it->second[kKeyConsoleInspectEnable];
}

void QuickjsDebugger::InitEnableState(int32_t session_id) {
  for (size_t i = 0; i < 4; i++) {
    session_enable_map_[session_id][i] = false;
  }
}

void QuickjsDebugger::RemoveEnableState(int32_t session_id) {
  ProcessPausedMessages(kMesDebuggerDisable, session_id);
  ProcessPausedMessages(kMesRuntimeDisable, session_id);
  ProcessPausedMessages(kMesProfilerDisable, session_id);
  session_enable_map_.erase(session_id);
}

void QuickjsDebugger::DebuggerSendNotification(const char *message,
                                               int32_t session_id) {
  if (session_id == -1) {
    // only send to session which is not null and enabled
    for (const auto &session : inspector_->GetSessions()) {
      if (session.second && GetDebuggerEnableState(session.first)) {
        session.second->SendProtocolNotification(message);
      }
    }
  } else {
    auto session = inspector_->GetSession(session_id);
    // if session is not null and enabled, send this protocol message
    if (session && GetDebuggerEnableState(session_id)) {
      session->SendProtocolNotification(message);
    }
  }
}

void QuickjsDebugger::DebuggerSendResponse(int32_t message_id,
                                           const char *message) {
  for (const auto &session : inspector_->GetSessions()) {
    if (session.second && GetDebuggerEnableState(session.first)) {
      session.second->SendProtocolResponse(message_id, message);
    }
  }
}

void QuickjsDebugger::DebuggerRunMessageLoopOnPause() {
  paused_ = true;
  inspector_->GetClient()->RunMessageLoopOnPause(inspector_->GetGroupID());
  paused_ = false;
}

void QuickjsDebugger::DebuggerQuitMessageLoopOnPause() {
  paused_ = false;
  inspector_->GetClient()->QuitMessageLoopOnPause();
}

void QuickjsDebugger::InspectorCheck() {
  DoInspectorCheck(context_->GetContext());
}

void QuickjsDebugger::DebuggerException() {
  HandleDebuggerException(context_->GetContext());
}

void QuickjsDebugger::ConsoleAPICalled(LEPUSValue *message) {
  SendConsoleAPICalledNotification(context_->GetContext(), message);
}

void QuickjsDebugger::ScriptParsed(LEPUSScriptSource *script) {
  SendScriptParsedNotification(context_->GetContext(), script);
}

void QuickjsDebugger::ScriptFailToParse(LEPUSScriptSource *script) {
  SendScriptFailToParseNotification(context_->GetContext(), script);
}

void QuickjsDebugger::ProcessPausedMessages(const std::string &message,
                                            int32_t session_id) {
  LEPUSDebuggerInfo *info = GetDebuggerInfo(context_->GetContext());
  if (!info) return;
  if (message != "") {
    PushBackQueue(GetDebuggerMessageQueue(info), message.c_str());
  }
  ProcessProtocolMessagesWithViewID(info, session_id);
}

void QuickjsDebugger::DebuggerSendResponseWithViewID(int32_t message_id,
                                                     const char *message,
                                                     int32_t session_id) {
  auto *session = inspector_->GetSession(session_id);
  if (session) {
    session->SendProtocolResponse(message_id, message);
  }
}

void QuickjsDebugger::ConsoleAPICalledMessageWithRID(LEPUSValue *message) {
  SendConsoleAPICalledNotificationWithRID(context_->GetContext(), message);
}

void QuickjsDebugger::ScriptParsedWithViewID(LEPUSScriptSource *script,
                                             int32_t session_id) {
  SendScriptParsedNotificationWithViewID(context_->GetContext(), script,
                                         session_id);
}

void QuickjsDebugger::ScriptFailToParseWithViewID(LEPUSScriptSource *script,
                                                  int32_t session_id) {
  SendScriptFailToParseNotificationWithViewID(context_->GetContext(), script,
                                              session_id);
}

void QuickjsDebugger::DebuggerPauseOnDebuggerKeyword(const uint8_t *pc) {
  PauseOnDebuggerKeyword(GetDebuggerInfo(context_->GetContext()), pc);
}

void QuickjsDebugger::OnConsoleMessage(const std::string &message,
                                       int32_t runtime_id) {
  for (const auto &session : inspector_->GetSessions()) {
    if (session.second && GetConsoleInspectEnableState(session.first)) {
      session.second->OnConsoleMessage(message, runtime_id);
    }
  }
}

void QuickjsDebugger::SetContextConsoleInspect(bool enable,
                                               int32_t session_id) {
  SetConsoleInspectEnableState(session_id, enable);
  ::SetContextConsoleInspect(context_->GetContext(), enable);
}

}  // namespace quickjs_inspector
