// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_DEBUGGER_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_DEBUGGER_H_

#include <string>
#include <unordered_map>
#include <utility>

#include "core/runtime/vm/lepus/context.h"
#include "third_party/rapidjson/document.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

namespace lepus_inspector {
class LepusNGInspectedContextImpl;
class LepusInspectorNGImpl;
}  // namespace lepus_inspector

namespace lynx {
namespace lepus {
class QuickContext;
class Context;
};  // namespace lepus

namespace debug {  // TODO(lqy): change namespace to lepus_inspector
// for lepusNG debugger
class LepusNGDebugger {
 public:
  LepusNGDebugger(lepus_inspector::LepusNGInspectedContextImpl* context,
                  lepus_inspector::LepusInspectorNGImpl* inspector,
                  const std::string& name);
  ~LepusNGDebugger();

  // get debugger info for lepusNG
  void SetDebugInfo(const std::string& url, const std::string& debug_info);

  void PrepareDebugInfo();

  // send protocol notification
  void DebuggerSendNotification(const char* message);
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

  // process protocol message sent here when then paused
  void ProcessPausedMessages(const std::string& message);

  void DebuggerSendConsoleMessage(LEPUSValue* message);

  void DebuggerSendScriptParsedMessage(LEPUSScriptSource* script);

  void DebuggerSendScriptFailToParseMessage(LEPUSScriptSource* script);

 private:
  void PrepareDebugInfo(const LEPUSValue& top_level_function,
                        const std::string& url, const std::string& debug_info);

  lepus_inspector::LepusNGInspectedContextImpl* context_;
  lepus_inspector::LepusInspectorNGImpl* inspector_;
  std::unordered_map<std::string, std::pair<bool, std::string>>
      debug_info_;  // url -> (is_prepared, debug info)
};
}  // namespace debug
}  // namespace lynx
#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_DEBUGGER_H_
