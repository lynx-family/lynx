// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_DEBUGGER_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_DEBUGGER_H_

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

struct DebugInfoDetail {
  int id_{-1};
  std::string debug_info_url_;
  std::string debug_info_str_;
  // file name -> has been parsed
  std::vector<std::pair<std::string, bool>> filename_parsed_pairs_;
};

struct MTSDebugInfoError {
  std::string GetFormattedErrorMessage() const {
    std::stringstream error_message;
    error_message << "MTS debug-info error!\nReason: " << reason_ << " "
                  << "\nDetail: " << detail_ << " "
                  << "\nFile name: " << file_name_ << " "
                  << "\nDebug-info URL: " << debug_info_url_;
    return error_message.str();
  }

  std::string reason_;
  std::string detail_;
  std::string file_name_;
  std::string debug_info_url_;
};

// for lepusNG debugger
class LepusNGDebugger {
 public:
  class Scope {
   public:
    Scope(LEPUSContext* context, void* ptr) : context_(context), ptr_(ptr) {}
    ~Scope() {
      if (ptr_ && !LEPUS_IsGCMode(context_)) {
        lepus_free(context_, ptr_);
      }
    }

   private:
    LEPUSContext* context_;
    void* ptr_;
  };

  LepusNGDebugger(lepus_inspector::LepusNGInspectedContextImpl* context,
                  lepus_inspector::LepusInspectorNGImpl* inspector,
                  const std::string& name);
  ~LepusNGDebugger();

  // set debugger info for lepusNG
  void SetDebugInfo(const std::string& filename,
                    const std::string& debug_info_str, int debug_info_id,
                    const std::string& debug_info_url);

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
                        const std::string& filename,
                        const std::string& debug_info,
                        const std::string& debug_info_url, bool is_default);
  void ParseDebugInfo(const LEPUSValue& top_level_function,
                      const std::string& filename,
                      const std::string& debug_info,
                      const std::string& debug_info_url, bool is_default);
  bool GetDebugInfoEntry(rapidjson::Document& document,
                         const std::string& filename, uint32_t func_size,
                         bool is_default, rapidjson::Value& entry,
                         bool& has_function_info, std::string& error_message);

  void HandleInvalidDebugInfo(const MTSDebugInfoError& error);

  lepus_inspector::LepusNGInspectedContextImpl* context_;
  lepus_inspector::LepusInspectorNGImpl* inspector_;
  std::unordered_map<int, DebugInfoDetail> debug_info_details_map_;
};
}  // namespace debug
}  // namespace lynx
#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_DEBUGGER_H_
