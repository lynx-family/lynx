// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspected_context_callbacks.h"

#include <string>

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspected_context.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspector_impl.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace quickjs_inspector {

#define QJSDebuggerCallBack(V)         \
  V(0, RunMessageLoopOnPauseCB)        \
  V(1, QuitMessageLoopOnPauseCB)       \
  V(2, NULL)                           \
  V(3, SendResponseCB)                 \
  V(4, SendNotificationCB)             \
  V(5, NULL)                           \
  V(6, DebuggerExceptionCB)            \
  V(7, InspectorCheckCB)               \
  V(8, ConsoleMessageCB)               \
  V(9, ScriptParsedCB)                 \
  V(10, SendConsoleAPICalledCB)        \
  V(11, ScriptFailToParsedCB)          \
  V(12, DebuggerPauseCB)               \
  V(13, NULL)                          \
  V(14, SendResponseWithViewIDCB)      \
  V(15, SendNotificationWithViewIDCB)  \
  V(16, ScriptParsedWithViewIDCB)      \
  V(17, ScriptFailToParseWithViewIDCB) \
  V(18, SetSessionEnableStateCB)       \
  V(19, GetSessionStateCB)             \
  V(20, SendConsoleAPICalledWithRIDCB) \
  V(21, GetSessionEnableStateCB)       \
  V(22, NULL)                          \
  V(23, OnConsoleMessageCB)

typedef enum ProtocolType {
  DEBUGGER_ENABLE,
  DEBUGGER_DISABLE,
  RUNTIME_ENABLE,
  RUNTIME_DISABLE,
  PROFILER_ENABLE,
  PROFILER_DISABLE
} ProtocolType;

// debugger related callbacks
// pause the vm
static void RunMessageLoopOnPauseCB(LEPUSContext *ctx) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerRunMessageLoopOnPause();
  }
}

// quit pause
static void QuitMessageLoopOnPauseCB(LEPUSContext *ctx) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerQuitMessageLoopOnPause();
  }
}

// send response to front end
static void SendResponseCB(LEPUSContext *ctx, int32_t message_id,
                           const char *message) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendResponse(message_id, message);
  }
}

// send response to front end
static void SendResponseWithViewIDCB(LEPUSContext *ctx, int32_t message_id,
                                     const char *message, int32_t session_id) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendResponseWithViewID(
        message_id, message, session_id);
  }
}

// send notification to front end
static void SendNotificationWithViewIDCB(LEPUSContext *ctx, const char *message,
                                         int32_t session_id) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendNotification(message,
                                                               session_id);
  }
}

// send notification to front end
static void SendNotificationCB(LEPUSContext *ctx, const char *message) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerSendNotification(message, -1);
  }
}

// for each opcode, do debugger related check
static void InspectorCheckCB(LEPUSContext *ctx) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->InspectorCheck();
  }
}

// call this function to handle exception
static void DebuggerExceptionCB(LEPUSContext *ctx) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerException();
  }
}

// print console.xxx messages
static void ConsoleMessageCB(LEPUSContext *ctx, int tag, LEPUSValueConst *argv,
                             int argc) {
  int i;
  const char *str;

  for (i = 0; i < argc; i++) {
    if (i != 0) putchar(' ');
    str = LEPUS_ToCString(ctx, argv[i]);
    if (!str) return;
    fputs(str, stdout);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, str);
  }
  putchar('\n');
}

// send Runtime.consoleAPICalled event for console.xxx
static void SendConsoleAPICalledCB(LEPUSContext *ctx, LEPUSValue *console_msg) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->ConsoleAPICalled(console_msg);
  }
}

// send Runtime.consoleAPICalled event for console.xxx, with runtime id for
// shared context debugger
static void SendConsoleAPICalledWithRIDCB(LEPUSContext *ctx,
                                          LEPUSValue *console_msg) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->ConsoleAPICalledMessageWithRID(
        console_msg);
  }
}

// send Debugger.scriptFailedToParse when vm fail to parse the script
static void ScriptFailToParseWithViewIDCB(LEPUSContext *ctx,
                                          LEPUSScriptSource *script,
                                          int32_t session_id) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->ScriptFailToParseWithViewID(script,
                                                                  session_id);
  }
}

// send Debugger.scriptParsed event when vm parses script
static void ScriptParsedWithViewIDCB(LEPUSContext *ctx,
                                     LEPUSScriptSource *script,
                                     int32_t session_id) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->ScriptParsedWithViewID(script,
                                                             session_id);
  }
}

// send Debugger.scriptFailedToParse when vm fail to parse the script
static void ScriptFailToParsedCB(LEPUSContext *ctx, LEPUSScriptSource *script) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->ScriptFailToParse(script);
  }
}

// pause on debugger keyword
static void DebuggerPauseCB(LEPUSContext *ctx, const uint8_t *pc) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->DebuggerPauseOnDebuggerKeyword(pc);
  }
}

// send Debugger.scriptParsed event when vm parses script
static void ScriptParsedCB(LEPUSContext *ctx, LEPUSScriptSource *script) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    inspected_context->GetDebugger()->ScriptParsed(script);
  }
}

// set session enable state given session_id
static void SetSessionEnableStateCB(LEPUSContext *ctx, int32_t session_id,
                                    int32_t method_type) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    switch (method_type) {
      case DEBUGGER_ENABLE: {
        // Debugger.enable
        inspected_context->GetDebugger()->SetDebuggerEnableState(session_id,
                                                                 true);
        break;
      }
      case DEBUGGER_DISABLE: {
        // Debugger.disable
        inspected_context->GetDebugger()->SetDebuggerEnableState(session_id,
                                                                 false);
        auto session_enable_map =
            inspected_context->GetDebugger()->GetEnableMap();
        bool all_session_disable = true;
        for (const auto &session : session_enable_map) {
          if (session.second[0]) {
            all_session_disable = false;
            break;
          }
        }
        // if current context is paused and all the sessions are in disable
        // mode, quit message loop on pause
        bool is_paused = inspected_context->GetDebugger()->IsPaused();
        if (all_session_disable && is_paused) {
          const char *resume_msg = "{'method':'Debugger.resumed', 'params':{}}";
          SendNotificationWithViewIDCB(ctx, resume_msg, session_id);
          QuitMessageLoopOnPauseCB(ctx);
        }
        break;
      }
      case RUNTIME_ENABLE: {
        // Runtime.enable
        inspected_context->GetDebugger()->SetRuntimeEnableState(session_id,
                                                                true);
        break;
      }
      case RUNTIME_DISABLE: {
        // Runtime.disable
        inspected_context->GetDebugger()->SetRuntimeEnableState(session_id,
                                                                false);
        break;
      }
      case PROFILER_ENABLE: {
        // Profiler.enable
        inspected_context->GetDebugger()->SetProfilerEnableState(session_id,
                                                                 true);
        break;
      }
      case PROFILER_DISABLE: {
        // Profiler.disable
        inspected_context->GetDebugger()->SetProfilerEnableState(session_id,
                                                                 false);
        break;
      }
      default:
        break;
    }
  }
}

// get session debugger enable state and pause state given session_id
static void GetSessionStateCB(LEPUSContext *ctx, int32_t session_id,
                              bool *already_enabled, bool *is_paused) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    *already_enabled =
        inspected_context->GetDebugger()->GetDebuggerEnableState(session_id);
    *is_paused = inspected_context->GetDebugger()->IsPaused();
  }
}

// get Debugger/Runtime/ enable state
static void GetSessionEnableStateCB(LEPUSContext *ctx, int32_t session_id,
                                    int32_t type, bool *ret) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);
  if (inspected_context != nullptr) {
    switch (type) {
      case DEBUGGER_ENABLE:
      case DEBUGGER_DISABLE: {
        // Debugger
        *ret = inspected_context->GetDebugger()->GetDebuggerEnableState(
            session_id);
        break;
      }
      case RUNTIME_ENABLE:
      case RUNTIME_DISABLE: {
        // Runtime
        *ret =
            inspected_context->GetDebugger()->GetRuntimeEnableState(session_id);
        break;
      }
      case PROFILER_ENABLE:
      case PROFILER_DISABLE: {
        // Profiler
        *ret = inspected_context->GetDebugger()->GetProfilerEnableState(
            session_id);
        break;
      }
      default:
        *ret = true;
        break;
    }
  }
}

static void OnConsoleMessageCB(LEPUSContext *ctx, LEPUSValue console_message,
                               int32_t runtime_id) {
  auto *inspected_context = QJSInspectedContext::GetFromJsContext(ctx);

  if (inspected_context != nullptr) {
    auto json = LEPUS_ToJSON(ctx, console_message, 0);
    HandleScope block_scope(ctx, &json, HANDLE_TYPE_LEPUS_VALUE);
    auto *str = LEPUS_ToCString(ctx, json);
    if (!LEPUS_IsGCMode(ctx)) {
      LEPUS_FreeValue(ctx, json);
    }
    inspected_context->GetDebugger()->OnConsoleMessage(
        std::string{str ? str : ""}, runtime_id);
    if (!LEPUS_IsGCMode(ctx)) {
      LEPUS_FreeCString(ctx, str);
    }
  }
  return;
}

// get qjs debugger related callbacks
std::vector<void *> &GetQJSCallbackFuncs() {
#define Name(idx, name) reinterpret_cast<void *>(name),
  static std::vector<void *> qjs_callbacks = {QJSDebuggerCallBack(Name)};
#undef Name
  return qjs_callbacks;
}
}  // namespace quickjs_inspector
