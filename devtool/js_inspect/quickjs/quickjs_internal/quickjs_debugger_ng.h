// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_DEBUGGER_NG_H_
#define DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_DEBUGGER_NG_H_

#include <string>
#include <unordered_map>

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

namespace quickjs_inspector {

class QJSInspectedContext;
class QJSInspectorImpl;

using EnableMap = std::unordered_map<int32_t, bool[4]>;

// This class is used to connect the Quickjs and the inspector instances
// directly:
// Manage all the inspector state.
// Process tasks called by the DevTool via QJSInspectorSessionImpl.
// Process tasks called by the Quickjs via callbacks.
class QuickjsDebugger {
 public:
  QuickjsDebugger(QJSInspectedContext* context, const std::string& name);
  virtual ~QuickjsDebugger();

  // set/get session enable state
  const EnableMap& GetEnableMap() { return session_enable_map_; }
  void SetDebuggerEnableState(int32_t session_id, bool value);
  void SetRuntimeEnableState(int32_t session_id, bool value);
  void SetProfilerEnableState(int32_t session_id, bool value);
  void SetConsoleInspectEnableState(int32_t session_id, bool value);
  bool GetDebuggerEnableState(int32_t session_id) const;
  bool GetRuntimeEnableState(int32_t session_id) const;
  bool GetProfilerEnableState(int32_t session_id) const;
  bool GetConsoleInspectEnableState(int32_t session_id) const;
  void InitEnableState(int32_t session_id);
  void RemoveEnableState(int32_t session_id);

  // send protocol notification
  void DebuggerSendNotification(const char* message, int32_t session_id);
  // send protocol response
  void DebuggerSendResponse(int32_t message_id, const char* message);
  // pause the vm
  void DebuggerRunMessageLoopOnPause();

  // quit pause and run the vm
  void DebuggerQuitMessageLoopOnPause();

  // for each pc, first call this function for debugging
  void InspectorCheck();

  // when there is an exception, call this function for debugger
  void DebuggerException();

  // send console messages
  void ConsoleAPICalled(LEPUSValue* message);

  // send script parsed notification
  void ScriptParsed(LEPUSScriptSource* script);

  // send script fail to parse notification
  void ScriptFailToParse(LEPUSScriptSource* script);

  // process protocol message sent here when paused
  void ProcessPausedMessages(const std::string& message, int32_t session_id);

  // send response message with view id
  void DebuggerSendResponseWithViewID(int32_t message_id, const char* message,
                                      int32_t session_id);

  // send console messages with runtime id
  void ConsoleAPICalledMessageWithRID(LEPUSValue* message);

  // send script parsed event with view id
  void ScriptParsedWithViewID(LEPUSScriptSource* script, int32_t session_id);

  // send script fail to parse event with view id
  void ScriptFailToParseWithViewID(LEPUSScriptSource* script,
                                   int32_t session_id);

  // get session paused state
  bool IsPaused() { return paused_; }

  // pause on debugger keyword
  void DebuggerPauseOnDebuggerKeyword(const uint8_t* pc);

  void OnConsoleMessage(const std::string& message, int32_t runtime_id);

  void SetContextConsoleInspect(bool enable, int32_t session_id);

 private:
  QJSInspectedContext* context_;
  QJSInspectorImpl* inspector_;

  // session enable state map
  // key: session_id,
  // value: is enabled: Debugger/Runtime/Profiler/ConsoleInspect
  EnableMap session_enable_map_;
  bool paused_{false};
};
}  // namespace quickjs_inspector

#endif  // DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_DEBUGGER_NG_H_
