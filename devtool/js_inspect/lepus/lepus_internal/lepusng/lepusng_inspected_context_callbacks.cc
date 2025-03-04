// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_callbacks.h"

#include <memory>

#include "core/runtime/vm/lepus/quick_context.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"

namespace lepus_inspector {

#define QJSDebuggerCallBack(V)   \
  V(0, RunMessageLoopOnPauseCB)  \
  V(1, QuitMessageLoopOnPauseCB) \
  V(2, NULL)                     \
  V(3, SendResponseCB)           \
  V(4, SendNotificationCB)       \
  V(5, NULL)                     \
  V(6, DebuggerExceptionCB)      \
  V(7, InspectorCheckCB)         \
  V(8, ConsoleMessageCB)         \
  V(9, ScriptParsedCB)           \
  V(10, ConsoleAPICalledCB)      \
  V(11, ScriptFailToParsedCB)

static std::shared_ptr<LepusNGInspectedContextImpl> GetLepusNGInspectedContext(
    LEPUSContext* lepus_context) {
  lynx::lepus::Context* context =
      lynx::lepus::QuickContext::GetFromJsContext(lepus_context);
  if (context == nullptr) {
    return nullptr;
  }
  auto debug_delegate = static_cast<lynx::lepus::QuickContext*>(context)
                            ->GetDebugDelegate()
                            .lock();
  return std::static_pointer_cast<LepusNGInspectedContextImpl>(debug_delegate);
}

static void RunMessageLoopOnPauseCB(LEPUSContext* ctx) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerRunMessageLoopOnPause();
  }
}

static void QuitMessageLoopOnPauseCB(LEPUSContext* ctx) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerQuitMessageLoopOnPause();
  }
}

static void SendResponseCB(LEPUSContext* ctx, int32_t message_id,
                           const char* message) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendResponse(message_id, message);
  }
}

static void SendNotificationCB(LEPUSContext* ctx, const char* message) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendNotification(message);
  }
}

static void DebuggerExceptionCB(LEPUSContext* ctx) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerException();
  }
}

static void InspectorCheckCB(LEPUSContext* ctx) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->InspectorCheck();
  }
}

static void ConsoleMessageCB(LEPUSContext* ctx, int tag, LEPUSValueConst* argv,
                             int argc) {
  int i;
  const char* str;
  for (i = 0; i < argc; i++) {
    if (i != 0) putchar(' ');
    str = LEPUS_ToCString(ctx, argv[i]);
    if (!str) return;
    fputs(str, stdout);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, str);
  }
  putchar('\n');
}

static void ConsoleAPICalledCB(LEPUSContext* ctx, LEPUSValue* console_msg) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendConsoleMessage(console_msg);
  }
}

static void ScriptFailToParsedCB(LEPUSContext* ctx, LEPUSScriptSource* script) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendScriptFailToParseMessage(
        script);
  }
}

static void ScriptParsedCB(LEPUSContext* ctx, LEPUSScriptSource* script) {
  auto inspected_context = GetLepusNGInspectedContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendScriptParsedMessage(script);
  }
}

std::vector<void*>& GetDebuggerCallbackFuncs() {
#define Name(idx, name) reinterpret_cast<void*>(name),
  static std::vector<void*> callbacks = {QJSDebuggerCallBack(Name)};
#undef Name
  return callbacks;
}

}  // namespace lepus_inspector
