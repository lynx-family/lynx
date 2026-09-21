// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspector_impl.h"

#include <utility>

#include "third_party/rapidjson/document.h"

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
  for (const auto& context : inspector_->GetContexts()) {
    context->GetDebugger()->InitEnableState(session_id);
  }
}

QJSInspectorSessionImpl::~QJSInspectorSessionImpl() {
  inspector_->RemoveSession(session_id_);
  for (const auto& context : inspector_->GetContexts()) {
    context->GetDebugger()->RemoveEnableState(session_id_);
  }
}

void QJSInspectorSessionImpl::DispatchProtocolMessage(
    const std::string& message) {
  auto* context = inspector_->GetContext(context_);
  rapidjson::Document command;
  command.Parse(message.c_str());
  if (!context || command.HasParseError() || !command.IsObject() ||
      !command.HasMember("method") || !command["method"].IsString()) {
    return;
  }
  const std::string method = command["method"].GetString();
  if (method == "Debugger.setBreakpointByUrl" && command.HasMember("params") &&
      command["params"].IsObject() && command["params"].HasMember("url") &&
      command["params"]["url"].IsString()) {
    if (auto* owner = inspector_->FindScriptURLContext(
            command["params"]["url"].GetString())) {
      context = inspector_->GetContext(owner);
    }
  }
  // Domain state and URL breakpoints apply to every realm in this VM group.
  const bool broadcast =
      method == "Debugger.enable" || method == "Debugger.disable" ||
      method == "Runtime.enable" || method == "Runtime.disable" ||
      method == "Profiler.enable" || method == "Profiler.disable" ||
      method == "Debugger.setBreakpointByUrl" ||
      method == "Debugger.removeBreakpoint" ||
      method == "Debugger.setPauseOnExceptions" ||
      method == "Debugger.setBreakpointsActive";
  if (broadcast) {
    for (const auto& item : inspector_->GetContexts()) {
      if (item.get() != context) {
        suppress_response_ = true;
        item->GetDebugger()->ProcessPausedMessages(message, session_id_);
      }
    }
    suppress_response_ = false;
  } else if (command.HasMember("params") && command["params"].IsObject()) {
    const auto& params = command["params"];
    const rapidjson::Value* script_id = nullptr;
    if (params.HasMember("scriptId")) {
      script_id = &params["scriptId"];
    } else if (params.HasMember("location") && params["location"].IsObject() &&
               params["location"].HasMember("scriptId")) {
      script_id = &params["location"]["scriptId"];
    }
    if (script_id && script_id->IsString()) {
      if (auto* owner = inspector_->FindScriptContext(script_id->GetString())) {
        context = inspector_->GetContext(owner);
      }
    } else if (auto* paused = inspector_->GetPausedContext()) {
      context = paused;
    }
  } else if (auto* paused = inspector_->GetPausedContext()) {
    context = paused;
  }
  context->GetDebugger()->ProcessPausedMessages(message, session_id_);
}

void QJSInspectorSessionImpl::SchedulePauseOnNextStatement(
    const std::string& reason) {
  inspector_->GetContext(context_)->GetDebugger()->ProcessPausedMessages(
      kMesDebuggerPauseOnNextStatementPrefix + reason +
          kMesDebuggerPauseOnNextStatementSuffix,
      session_id_);
}

void QJSInspectorSessionImpl::SetEnableConsoleInspect(bool enable) {
  for (const auto& context : inspector_->GetContexts()) {
    context->GetDebugger()->SetContextConsoleInspect(enable, session_id_);
  }
}

void QJSInspectorSessionImpl::SendProtocolResponse(int callId,
                                                   const std::string& message) {
  if (!suppress_response_) {
    channel_->SendResponse(callId, message);
  }
}

void QJSInspectorSessionImpl::SendProtocolNotification(
    const std::string& message) {
  channel_->SendNotification(message);
}

void QJSInspectorSessionImpl::OnConsoleMessage(const std::string& message,
                                               const std::string& url) {
  channel_->OnConsoleMessage(message, url);
}
// QJSInspectorSessionImpl ends.

// QJSInspectorImpl begins.
QJSInspectorImpl::QJSInspectorImpl(LEPUSContext* ctx,
                                   QJSInspectorClient* client,
                                   const std::string& group_id,
                                   const std::string& name)
    : client_(client), group_id_(group_id) {
  AddContext(ctx, name);
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
    QJSChannel* channel, const std::string& group_id, int32_t session_id,
    LEPUSContext* context) {
  std::unique_ptr<QJSInspectorSessionImpl> session =
      QJSInspectorSessionImpl::Create(this, session_id, channel);
  session->SetContext(context);
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

QJSInspectedContext* QJSInspectorImpl::GetContext(LEPUSContext* context) {
  for (const auto& item : contexts_) {
    if (item->GetContext() == context) {
      return item.get();
    }
  }
  return contexts_.empty() ? nullptr : contexts_.back().get();
}

void QJSInspectorImpl::AddContext(LEPUSContext* ctx, const std::string& name) {
  for (const auto& item : contexts_) {
    if (item->GetContext() == ctx) {
      return;
    }
  }
  auto context = std::make_unique<QJSInspectedContext>(this, ctx, name);
  for (const auto& session : sessions_) {
    auto& debugger = context->GetDebugger();
    debugger->InitEnableState(session.first);
    auto& previous = contexts_.back()->GetDebugger();
    if (previous->GetDebuggerEnableState(session.first)) {
      debugger->ProcessPausedMessages(R"({"id":0,"method":"Debugger.enable"})",
                                      session.first);
    }
    if (previous->GetRuntimeEnableState(session.first)) {
      debugger->ProcessPausedMessages(R"({"id":0,"method":"Runtime.enable"})",
                                      session.first);
    }
    if (previous->GetProfilerEnableState(session.first)) {
      debugger->ProcessPausedMessages(R"({"id":0,"method":"Profiler.enable"})",
                                      session.first);
    }
    debugger->SetContextConsoleInspect(
        previous->GetConsoleInspectEnableState(session.first), session.first);
  }
  contexts_.push_back(std::move(context));
}

void QJSInspectorImpl::RemoveContext(LEPUSContext* ctx) {
  for (auto it = script_contexts_.begin(); it != script_contexts_.end();) {
    if (it->second == ctx) {
      it = script_contexts_.erase(it);
    } else {
      ++it;
    }
  }
  for (auto it = script_url_contexts_.begin();
       it != script_url_contexts_.end();) {
    if (it->second == ctx) {
      it = script_url_contexts_.erase(it);
    } else {
      ++it;
    }
  }
  for (auto it = contexts_.begin(); it != contexts_.end(); ++it) {
    if ((*it)->GetContext() == ctx) {
      contexts_.erase(it);
      return;
    }
  }
}

LEPUSContext* QJSInspectorImpl::FindScriptContext(
    const std::string& script_id) {
  auto it = script_contexts_.find(script_id);
  return it == script_contexts_.end() ? nullptr : it->second;
}

LEPUSContext* QJSInspectorImpl::FindScriptURLContext(const std::string& url) {
  auto it = script_url_contexts_.find(url);
  return it == script_url_contexts_.end() ? nullptr : it->second;
}

void QJSInspectorImpl::RecordScript(const std::string& message,
                                    LEPUSContext* context) {
  rapidjson::Document event;
  event.Parse(message.c_str());
  if (!event.HasParseError() && event.IsObject() && event.HasMember("method") &&
      event["method"].IsString() &&
      std::string(event["method"].GetString()) == "Debugger.scriptParsed" &&
      event.HasMember("params") && event["params"].IsObject() &&
      event["params"].HasMember("scriptId") &&
      event["params"]["scriptId"].IsString()) {
    script_contexts_[event["params"]["scriptId"].GetString()] = context;
    if (event["params"].HasMember("url") && event["params"]["url"].IsString()) {
      script_url_contexts_[event["params"]["url"].GetString()] = context;
    }
  }
}

QJSInspectedContext* QJSInspectorImpl::GetPausedContext() {
  for (const auto& context : contexts_) {
    if (context->GetDebugger()->IsPaused()) {
      return context.get();
    }
  }
  return nullptr;
}

bool QJSInspectorImpl::IsFullFuncEnabled() {
  return client_->IsFullFuncEnabled();
}
// QJSInspectorImpl ends.

}  // namespace quickjs_inspector
