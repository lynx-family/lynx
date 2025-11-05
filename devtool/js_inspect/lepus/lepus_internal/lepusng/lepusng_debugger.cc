// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_debugger.h"

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"

namespace lynx {
namespace debug {

namespace {
constexpr char kKeyFunctionNumber[] = "function_number";
constexpr char kKeyFunctionInfo[] = "function_info";
constexpr char kKeyFunctionId[] = "function_id";
constexpr char kKeyFileName[] = "file_name";
constexpr char kKeyLineNumber[] = "line_number";
constexpr char kKeyColumnNumber[] = "column_number";
constexpr char kKeyPc2LineLen[] = "pc2line_len";
constexpr char kKeyPc2LineBuf[] = "pc2line_buf";
constexpr char kKeyFunctionSource[] = "function_source";
constexpr char kKeyFunctionSourceLen[] = "function_source_len";
constexpr char kKeyEndLineNumber[] = "end_line_num";
constexpr char kKeyLepusNGDebugInfo[] = "lepusNG_debug_info";
}  // namespace

LepusNGDebugger::LepusNGDebugger(
    lepus_inspector::LepusNGInspectedContextImpl* context,
    lepus_inspector::LepusInspectorNGImpl* inspector, const std::string& name)
    : context_(context), inspector_(inspector) {
  QJSDebuggerInitialize(context_->GetLepusContext());
  SetJSDebuggerName(context->GetLepusContext(), name.c_str());
}

LepusNGDebugger::~LepusNGDebugger() {
  QJSDebuggerFree(context_->GetLepusContext());
}

void LepusNGDebugger::DebuggerSendNotification(const char* message) {
  inspector_->GetSession()->SendProtocolNotification(message);
}

void LepusNGDebugger::DebuggerSendResponse(int32_t message_id,
                                           const char* message) {
  inspector_->GetSession()->SendProtocolResponse(message_id, message);
}

void LepusNGDebugger::SetDebugInfo(const std::string& filename,
                                   const std::string& debug_info_str,
                                   int debug_info_id,
                                   const std::string& debug_info_url) {
  auto it = debug_info_details_map_.find(debug_info_id);
  if (it == debug_info_details_map_.end()) {
    DebugInfoDetail item = {debug_info_id, debug_info_url, debug_info_str};
    it = debug_info_details_map_.emplace(debug_info_id, item).first;
  }
  it->second.filename_parsed_pairs_.emplace_back(filename, false);
}

static void FillFunctionBytecodeDebugInfo(LEPUSContext* ctx,
                                          LEPUSFunctionBytecode* b,
                                          rapidjson::Value& debug_info) {
  uint32_t func_num = debug_info[kKeyFunctionNumber].GetUint();
  uint32_t function_id = GetFunctionDebugId(b);
  uint32_t func_index = 0;
  for (; func_index < func_num; func_index++) {
    auto each_func = debug_info[kKeyFunctionInfo][func_index].GetObject();
    auto each_func_id = each_func[kKeyFunctionId].GetUint();
    // find the corresponding function domain for this function
    if (each_func_id == function_id) {
      break;
    }
  }
  // can not find the corresponding function domain, return
  if (func_index == func_num) {
    return;
  }

  auto func_info = debug_info[kKeyFunctionInfo][func_index].GetObject();

  // filename
  if (func_info.HasMember(kKeyFileName)) {
    std::string function_file_name = func_info[kKeyFileName].GetString();
    SetFunctionDebugFileName(ctx, b, function_file_name.c_str(),
                             static_cast<int>(function_file_name.length()));
  } else {
    SetFunctionDebugFileName(ctx, b, "", 0);
  }

  // line number
  int32_t debug_line_num = func_info[kKeyLineNumber].GetInt();
  SetFunctionDebugLineNum(b, debug_line_num);

  // column number
  int64_t debug_column_num = func_info[kKeyColumnNumber].GetInt64();
  SetFunctionDebugColumnNum(b, debug_column_num);

  // pc2line_len
  int32_t pc2line_len = func_info[kKeyPc2LineLen].GetInt();

  // pc2line_buf
  if (func_info.HasMember(kKeyPc2LineBuf)) {
    uint8_t* buf = static_cast<uint8_t*>(lepus_malloc(
        ctx, sizeof(uint8_t) * pc2line_len, ALLOC_TAG_WITHOUT_PTR));
    LepusNGDebugger::Scope scope(ctx, buf);
    if (buf) {
      for (int32_t i = 0; i < pc2line_len; i++) {
        buf[i] = func_info[kKeyPc2LineBuf][i].GetUint();
      }
    }
    SetFunctionDebugPC2LineBufLen(ctx, b, buf, pc2line_len);
  } else {
    SetFunctionDebugPC2LineBufLen(ctx, b, nullptr, 0);
  }

  // child function source
  if (func_info.HasMember(kKeyFunctionSource) &&
      func_info.HasMember(kKeyFunctionSourceLen)) {
    int32_t function_source_len = func_info[kKeyFunctionSourceLen].GetInt();
    std::string function_source = func_info[kKeyFunctionSource].GetString();
    SetFunctionDebugSource(ctx, b, function_source.c_str(),
                           function_source_len);
  } else {
    SetFunctionDebugSource(ctx, b, nullptr, 0);
  }
}

void LepusNGDebugger::ParseDebugInfo(const LEPUSValue& top_level_function,
                                     const std::string& filename,
                                     const std::string& debug_info,
                                     const std::string& debug_info_url,
                                     bool is_default) {
  if (LEPUS_IsUndefined(top_level_function)) {
    HandleInvalidDebugInfo(MTSDebugInfoError{
        "Failed to get top-level function!",
        "The top-level function is undefined.", filename, debug_info_url});
    return;
  }

  rapidjson::Document document;
  document.Parse(debug_info.c_str());
  if (document.HasParseError()) {
    HandleInvalidDebugInfo(MTSDebugInfoError{"Failed to parse debug-info!",
                                             document.GetParseErrorMsg(),
                                             filename, debug_info_url});
    return;
  }

  uint32_t func_size = 0;
  LEPUSContext* ctx = context_->GetLepusContext();
  LEPUSFunctionBytecode** function_list =
      GetDebuggerAllFunction(ctx, top_level_function, &func_size);
  if (function_list == nullptr) {
    HandleInvalidDebugInfo(MTSDebugInfoError{"Failed to get all functions!",
                                             "The function list is empty.",
                                             filename, debug_info_url});
    return;
  }

  Scope scope(ctx, function_list);
  rapidjson::Value debug_info_entry;
  bool has_function_info = false;
  std::string error_message;
  bool res =
      GetDebugInfoEntry(document, filename, func_size, is_default,
                        debug_info_entry, has_function_info, error_message);
  if (!res) {
    HandleInvalidDebugInfo(MTSDebugInfoError{"Failed to parse debug-info!",
                                             error_message, filename,
                                             debug_info_url});
    return;
  }

  LEPUSScriptSource* script = nullptr;
  if (debug_info_entry.HasMember(kKeyFunctionSource) &&
      debug_info_entry.HasMember(kKeyEndLineNumber)) {
    std::string source = debug_info_entry[kKeyFunctionSource].GetString();
    char* source_str = const_cast<char*>(source.c_str());
    SetDebuggerSourceCode(ctx, source_str);
    int32_t end_line_num = debug_info_entry[kKeyEndLineNumber].GetInt();
    SetDebuggerEndLineNum(ctx, end_line_num);
    script =
        AddDebuggerScript(ctx, source_str, const_cast<char*>(filename.c_str()),
                          static_cast<int32_t>(source.length()), end_line_num);
  }

  if (script == nullptr) {
    HandleInvalidDebugInfo(MTSDebugInfoError{
        "Failed to get `function_source`!",
        "The debug-info does not contain `function_source` or "
        "`end_line_number`.",
        filename, debug_info_url});
    return;
  }

  for (uint32_t i = 0; i < func_size; i++) {
    auto* b = function_list[i];
    if (b) {
      if (has_function_info) {
        FillFunctionBytecodeDebugInfo(ctx, b, debug_info_entry);
      }
      SetFunctionScript(b, script);
    }
  }
  InitDebuggerScript(ctx, script);
}

bool LepusNGDebugger::GetDebugInfoEntry(rapidjson::Document& document,
                                        const std::string& url,
                                        uint32_t func_size, bool is_default,
                                        rapidjson::Value& entry,
                                        bool& has_function_info,
                                        std::string& error_message) {
  uint32_t function_num = 0;
  if (is_default) {
    if (document.HasMember(kKeyLepusNGDebugInfo)) {
      entry = document[kKeyLepusNGDebugInfo].GetObject();
      has_function_info = entry.HasMember(kKeyFunctionNumber);
      if (has_function_info) {
        function_num = entry[kKeyFunctionNumber].GetUint();
      }
    }
  } else {
    for (auto it = document.MemberBegin(); it != document.MemberEnd(); it++) {
      std::string name = it->name.GetString();
      auto value = it->value.GetObject();
      bool item_has_function_info = value.HasMember(kKeyFunctionNumber);
      if (item_has_function_info) {
        function_num = value[kKeyFunctionNumber].GetUint() - function_num;
      }
      if (url.find(name) != std::string::npos) {
        entry = value;
        has_function_info = item_has_function_info;
        break;
      }
    }
  }
  if (!entry.IsObject() || entry.MemberCount() == 0) {
    error_message = "Cannot find the target entry in debug-info.";
    return false;
  }
  if (has_function_info && function_num != func_size) {
    std::stringstream error_message_stream;
    error_message_stream << "The `function_number` in debug-info does not "
                            "match the actual function number: expected "
                         << func_size << ", but got " << function_num;
    error_message = error_message_stream.str();
    return false;
  }
  return true;
}

void LepusNGDebugger::PrepareDebugInfo() {
  const auto& top_level_function =
      context_->GetContext()->GetTopLevelFunction();
  if (LEPUS_IsUndefined(top_level_function)) {
    return;
  }
  for (auto& debug_info : debug_info_details_map_) {
    auto& vec = debug_info.second.filename_parsed_pairs_;
    auto it = std::find_if(vec.begin(), vec.end(),
                           [](const auto& pair) { return !pair.second; });
    if (it != vec.end()) {
      it->second = true;
      PrepareDebugInfo(top_level_function, it->first,
                       debug_info.second.debug_info_str_,
                       debug_info.second.debug_info_url_, it == vec.begin());
      break;
    }
  }
}

void LepusNGDebugger::DebuggerRunMessageLoopOnPause() {
  inspector_->GetClient()->RunMessageLoopOnPause();
}

void LepusNGDebugger::DebuggerQuitMessageLoopOnPause() {
  inspector_->GetClient()->QuitMessageLoopOnPause();
}

// for each pc, first call this function for debugging
void LepusNGDebugger::InspectorCheck() {
  DoInspectorCheck(context_->GetLepusContext());
}

void LepusNGDebugger::DebuggerException() {
  HandleDebuggerException(context_->GetLepusContext());
}

void LepusNGDebugger::ProcessPausedMessages(const std::string& message) {
  LEPUSDebuggerInfo* info = GetDebuggerInfo(context_->GetLepusContext());
  if (!info) return;
  if (message != "") {
    PushBackQueue(GetDebuggerMessageQueue(info), message.c_str());
  }
  ProcessProtocolMessages(info);
}

void LepusNGDebugger::DebuggerSendConsoleMessage(LEPUSValue* message) {
  SendConsoleAPICalledNotification(context_->GetLepusContext(), message);
}

void LepusNGDebugger::DebuggerSendScriptParsedMessage(
    LEPUSScriptSource* script) {
  SendScriptParsedNotification(context_->GetLepusContext(), script);
}

void LepusNGDebugger::DebuggerSendScriptFailToParseMessage(
    LEPUSScriptSource* script) {
  SendScriptFailToParseNotification(context_->GetLepusContext(), script);
}

void LepusNGDebugger::PrepareDebugInfo(const LEPUSValue& top_level_function,
                                       const std::string& filename,
                                       const std::string& debug_info,
                                       const std::string& debug_info_url,
                                       bool is_default) {
  if (debug_info.empty()) {
    HandleInvalidDebugInfo(MTSDebugInfoError{
        "Failed to download debug-info.json!",
        "The content of debug-info.json is empty, or the MTS Debug switch is "
        "not enabled.",
        filename, debug_info_url});
    return;
  }

  ParseDebugInfo(top_level_function, filename, debug_info, debug_info_url,
                 is_default);
}

void LepusNGDebugger::HandleInvalidDebugInfo(const MTSDebugInfoError& error) {
  const std::string source = error.GetFormattedErrorMessage();
  LOGE("lepusng debug: " << source);
  AddDebuggerScript(context_->GetLepusContext(),
                    const_cast<char*>(source.c_str()),
                    const_cast<char*>(error.file_name_.c_str()),
                    static_cast<int32_t>(source.length()), 0);
}

}  // namespace debug
}  // namespace lynx
