// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_debugger.h"

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"

namespace lynx {
namespace debug {
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

void LepusNGDebugger::SetDebugInfo(const std::string& url,
                                   const std::string& debug_info) {
  const auto& top_level_function =
      context_->GetContext()->GetTopLevelFunction();
  if (LEPUS_IsUndefined(top_level_function)) {
    debug_info_[url] = {false, debug_info};
  } else {
    debug_info_[url] = {true, debug_info};
    PrepareDebugInfo(top_level_function, url, debug_info);
  }
}

static void FillFunctionBytecodeDebugInfo(
    LEPUSContext* ctx, LEPUSFunctionBytecode* b,
    rapidjson::Document::Object& debug_info) {
  uint32_t func_num = debug_info["function_number"].GetUint();
  uint32_t function_id = GetFunctionDebugId(b);
  uint32_t func_index = 0;
  for (; func_index < func_num; func_index++) {
    auto each_func = debug_info["function_info"][func_index].GetObject();
    auto each_func_id = each_func["function_id"].GetUint();
    // find the corresponding function domain for this function
    if (each_func_id == function_id) {
      break;
    }
  }
  // can not find the corresponding function domain, return
  if (func_index == func_num) {
    return;
  }

  auto func_info = debug_info["function_info"][func_index].GetObject();

  // filename
  if (func_info.HasMember("file_name")) {
    std::string function_file_name = func_info["file_name"].GetString();
    SetFunctionDebugFileName(ctx, b, function_file_name.c_str(),
                             static_cast<int>(function_file_name.length()));
  } else {
    SetFunctionDebugFileName(ctx, b, "", 0);
  }

  // line number
  int32_t debug_line_num = func_info["line_number"].GetInt();
  SetFunctionDebugLineNum(b, debug_line_num);

  // column number
  int64_t debug_column_num = func_info["column_number"].GetInt64();
  SetFunctionDebugColumnNum(b, debug_column_num);

  // pc2line_len
  int32_t pc2line_len = func_info["pc2line_len"].GetInt();

  // pc2line_buf
  if (func_info.HasMember("pc2line_buf")) {
    uint8_t* buf = static_cast<uint8_t*>(lepus_malloc(
        ctx, sizeof(uint8_t) * pc2line_len, ALLOC_TAG_WITHOUT_PTR));
    if (buf) {
      for (int32_t i = 0; i < pc2line_len; i++) {
        buf[i] = func_info["pc2line_buf"][i].GetUint();
      }
    }
    SetFunctionDebugPC2LineBufLen(ctx, b, buf, pc2line_len);
    if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, buf);
  } else {
    SetFunctionDebugPC2LineBufLen(ctx, b, nullptr, 0);
  }

  // child function source
  if (func_info.HasMember("function_source") &&
      func_info.HasMember("function_source_len")) {
    int32_t function_source_len = func_info["function_source_len"].GetInt();
    std::string function_source = func_info["function_source"].GetString();
    SetFunctionDebugSource(ctx, b, function_source.c_str(),
                           function_source_len);
  } else {
    SetFunctionDebugSource(ctx, b, nullptr, 0);
  }
}

static void SetTemplateDebugInfo(LEPUSContext* ctx, const std::string& url,
                                 const std::string& debug_info_json,
                                 LEPUSValue obj) {
  rapidjson::Document document;
  document.Parse(debug_info_json.c_str());
  if (document.HasMember("lepusNG_debug_info")) {
    auto debug_info = document["lepusNG_debug_info"].GetObject();
    if (!LEPUS_IsUndefined(obj) && debug_info.HasMember("function_number")) {
      uint32_t func_size = 0;
      auto* function_list = GetDebuggerAllFunction(ctx, obj, &func_size);
      uint32_t function_num = debug_info["function_number"].GetUint();
      if (function_num != func_size) {
        LOGE("error in set lepusNG debuginfo");
        if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, function_list);
        return;
      }
      if (function_list) {
        for (uint32_t i = 0; i < func_size; i++) {
          auto* b = function_list[i];
          if (b) {
            FillFunctionBytecodeDebugInfo(ctx, b, debug_info);
          }
        }
      } else {
        LOGE("lepusng debug: get all function fail");
      }
      if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, function_list);
    }

    if (debug_info.HasMember("function_source") &&
        debug_info.HasMember("end_line_num")) {
      std::string source = debug_info["function_source"].GetString();
      char* source_str = const_cast<char*>(source.c_str());
      SetDebuggerSourceCode(ctx, source_str);
      int32_t end_line_num = debug_info["end_line_num"].GetInt();
      SetDebuggerEndLineNum(ctx, end_line_num);
      AddDebuggerScript(ctx, source_str, static_cast<int32_t>(source.length()),
                        end_line_num);  // TODO(lqy): use param url as filename
    }
  }
}

void LepusNGDebugger::PrepareDebugInfo() {
  const auto& top_level_function =
      context_->GetContext()->GetTopLevelFunction();
  if (LEPUS_IsUndefined(top_level_function)) {
    return;
  }
  for (const auto& item : debug_info_) {
    if (!item.second.first) {
      PrepareDebugInfo(top_level_function, item.first, item.second.second);
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
                                       const std::string& url,
                                       const std::string& debug_info) {
  if (debug_info.empty()) {
    const std::string source = "debug-info.json download fail, please check!";
    AddDebuggerScript(context_->GetLepusContext(),
                      const_cast<char*>(source.c_str()),
                      static_cast<int32_t>(source.length()),
                      0);  // TODO(lqy): use param url as filename
    return;
  }

  SetTemplateDebugInfo(context_->GetLepusContext(), url, debug_info,
                       top_level_function);
}

}  // namespace debug
}  // namespace lynx
