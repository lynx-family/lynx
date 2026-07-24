// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_debugger.h"

#include <string>
#include <vector>

#include "core/runtime/lepusng/quickjs_debug_info.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"

namespace lynx {
namespace debug {

namespace {
constexpr int kCompactDebugInfoVersion = 2;

constexpr char kKeyDebugInfoVersion[] = "v";
constexpr char kKeyFunctionNumber[] = "function_number";
constexpr char kKeyFunctionNumberCompact[] = "fn";
constexpr char kKeyFunctionInfo[] = "function_info";
constexpr char kKeyFunctionInfoCompact[] = "fi";
constexpr char kKeyFunctionId[] = "function_id";
constexpr char kKeyFunctionIdCompact[] = "id";
constexpr char kKeyFileName[] = "file_name";
constexpr char kKeyFileNameCompact[] = "f";
constexpr char kKeyLineNumber[] = "line_number";
constexpr char kKeyLineNumberCompact[] = "ln";
constexpr char kKeyColumnNumber[] = "column_number";
constexpr char kKeyColumnNumberCompact[] = "cn";
constexpr char kKeyPc2LineLen[] = "pc2line_len";
constexpr char kKeyPc2LineLenCompact[] = "pll";
constexpr char kKeyPc2LineBuf[] = "pc2line_buf";
constexpr char kKeyPc2LineBufCompact[] = "plb";
constexpr char kKeyFunctionSource[] = "function_source";
constexpr char kKeyFunctionSourceCompact[] = "fs";
constexpr char kKeyFunctionSourceOffset[] = "function_source_offset";
constexpr char kKeyFunctionSourceOffsetCompact[] = "fso";
constexpr char kKeyFunctionSourceLen[] = "function_source_len";
constexpr char kKeyFunctionSourceLenCompact[] = "fsl";
constexpr char kKeyEndLineNumber[] = "end_line_num";
constexpr char kKeyEndLineNumberCompact[] = "el";
constexpr char kKeyLepusNGDebugInfo[] = "lepusNG_debug_info";

const rapidjson::Value* FindMember(const rapidjson::Value& value,
                                   const char* key, const char* compact_key) {
  if (value.HasMember(compact_key)) {
    return &value[compact_key];
  }
  if (value.HasMember(key)) {
    return &value[key];
  }
  return nullptr;
}

bool HasMember(const rapidjson::Value& value, const char* key,
               const char* compact_key) {
  return FindMember(value, key, compact_key) != nullptr;
}

bool IsCompactDebugInfo(const rapidjson::Value& debug_info) {
  return debug_info.HasMember(kKeyDebugInfoVersion) &&
         debug_info[kKeyDebugInfoVersion].IsInt() &&
         debug_info[kKeyDebugInfoVersion].GetInt() >= kCompactDebugInfoVersion;
}

std::string GetFunctionSourceFromRoot(const std::string& root_source,
                                      int32_t source_offset,
                                      int32_t source_len) {
  if (root_source.empty() || source_offset < 0 || source_len < 0) {
    return "";
  }

  size_t start = static_cast<size_t>(source_offset);
  size_t len = static_cast<size_t>(source_len);
  if (start > root_source.size() || start + len > root_source.size()) {
    return "";
  }
  return root_source.substr(start, len);
}
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

void FillFunctionBytecodeDebugInfo(LEPUSContext* ctx, LEPUSFunctionBytecode* b,
                                   rapidjson::Value& debug_info,
                                   const std::string& root_source) {
  const auto* function_number =
      FindMember(debug_info, kKeyFunctionNumber, kKeyFunctionNumberCompact);
  const auto* function_info =
      FindMember(debug_info, kKeyFunctionInfo, kKeyFunctionInfoCompact);
  if (!function_number || !function_info || !function_info->IsArray()) {
    return;
  }
  uint32_t func_num = function_number->GetUint();
  uint32_t function_id = GetFunctionDebugId(b);
  uint32_t func_index = 0;
  for (; func_index < func_num; func_index++) {
    const auto& each_func = (*function_info)[func_index];
    if (!each_func.IsObject()) {
      continue;
    }
    const auto* each_func_id_value =
        FindMember(each_func, kKeyFunctionId, kKeyFunctionIdCompact);
    if (!each_func_id_value) {
      continue;
    }
    auto each_func_id = each_func_id_value->GetUint();
    // find the corresponding function domain for this function
    if (each_func_id == function_id) {
      break;
    }
  }
  // can not find the corresponding function domain, return
  if (func_index == func_num) {
    return;
  }

  const auto& func_info = (*function_info)[func_index];

  // filename
  if (const auto* file_name =
          FindMember(func_info, kKeyFileName, kKeyFileNameCompact)) {
    std::string function_file_name = file_name->GetString();
    SetFunctionDebugFileName(ctx, b, function_file_name.c_str(),
                             static_cast<int>(function_file_name.length()));
  } else {
    SetFunctionDebugFileName(ctx, b, "", 0);
  }

  // line number
  const auto* line_number =
      FindMember(func_info, kKeyLineNumber, kKeyLineNumberCompact);
  if (!line_number) {
    return;
  }
  int32_t debug_line_num = line_number->GetInt();
  SetFunctionDebugLineNum(b, debug_line_num);

  // column number
  const auto* column_number =
      FindMember(func_info, kKeyColumnNumber, kKeyColumnNumberCompact);
  if (!column_number) {
    return;
  }
  int64_t debug_column_num = column_number->GetInt64();
  SetFunctionDebugColumnNum(b, debug_column_num);

  // pc2line_len
  const auto* pc2line_len_value =
      FindMember(func_info, kKeyPc2LineLen, kKeyPc2LineLenCompact);
  if (!pc2line_len_value) {
    return;
  }
  int32_t pc2line_len = pc2line_len_value->GetInt();

  // pc2line_buf
  if (pc2line_len <= 0) {
    SetFunctionDebugPC2LineBufLen(ctx, b, nullptr, 0);
  } else if (const auto* pc2line_buf_value =
                 FindMember(func_info, kKeyPc2LineBuf, kKeyPc2LineBufCompact)) {
    uint8_t* buf = static_cast<uint8_t*>(lepus_malloc(
        ctx, sizeof(uint8_t) * pc2line_len, ALLOC_TAG_WITHOUT_PTR));
    LepusNGDebugger::Scope scope(ctx, buf);
    HandleScope handle_scope(ctx, buf, HANDLE_TYPE_DIR_HEAP_OBJ);
    if (buf) {
      for (int32_t i = 0; i < pc2line_len; i++) {
        buf[i] = (*pc2line_buf_value)[i].GetUint();
      }
    }
    SetFunctionDebugPC2LineBufLen(ctx, b, buf, pc2line_len);
  } else {
    SetFunctionDebugPC2LineBufLen(ctx, b, nullptr, 0);
  }

  // child function source
  const auto* function_source_len_value = FindMember(
      func_info, kKeyFunctionSourceLen, kKeyFunctionSourceLenCompact);
  const auto* function_source_offset_value = FindMember(
      func_info, kKeyFunctionSourceOffset, kKeyFunctionSourceOffsetCompact);
  const auto* function_source_value =
      FindMember(func_info, kKeyFunctionSource, kKeyFunctionSourceCompact);
  if (function_source_len_value && function_source_value) {
    int32_t function_source_len = function_source_len_value->GetInt();
    std::string function_source = function_source_value->GetString();
    SetFunctionDebugSource(ctx, b, function_source.c_str(),
                           function_source_len);
    if (function_source_offset_value) {
      SetFunctionDebugSourceOffset(b, function_source_offset_value->GetInt());
    }
  } else if (function_source_len_value && IsCompactDebugInfo(debug_info)) {
    int32_t function_source_len = function_source_len_value->GetInt();
    int32_t function_source_offset =
        function_source_offset_value ? function_source_offset_value->GetInt()
                                     : -1;
    std::string function_source = GetFunctionSourceFromRoot(
        root_source, function_source_offset, function_source_len);
    if (!function_source.empty() ||
        (function_source_len == 0 && function_source_offset >= 0)) {
      SetFunctionDebugSource(ctx, b, function_source.c_str(),
                             static_cast<int32_t>(function_source.length()));
      SetFunctionDebugSourceOffset(b, function_source_offset);
    } else {
      SetFunctionDebugSource(ctx, b, nullptr, 0);
      SetFunctionDebugSourceOffset(b, -1);
    }
  } else {
    SetFunctionDebugSource(ctx, b, nullptr, 0);
    SetFunctionDebugSourceOffset(b, -1);
  }

  // restore vardefs from debug-info
  if (func_info.HasMember(lepus::kKeyVarDefs) &&
      func_info[lepus::kKeyVarDefs].IsArray()) {
    const auto& var_defs_arr = func_info[lepus::kKeyVarDefs];
    uint32_t count = var_defs_arr.Size();
    if (count > 0) {
      std::vector<const char*> var_names(count);
      std::vector<std::string> name_storage(count);
      std::vector<int32_t> scope_levels(count);
      std::vector<int32_t> scope_next_info(count);
      std::vector<uint8_t> flags(count);
      for (uint32_t i = 0; i < count; ++i) {
        const auto& vd = var_defs_arr[i];
        if (!vd.IsObject() || !vd.HasMember(lepus::kKeyVarDefName) ||
            !vd.HasMember(lepus::kKeyVarDefScopeLevel) ||
            !vd.HasMember(lepus::kKeyVarDefScopeNext) ||
            !vd.HasMember(lepus::kKeyVarDefFlags)) {
          return;
        }
        name_storage[i] = vd[lepus::kKeyVarDefName].GetString();
        var_names[i] = name_storage[i].c_str();
        scope_levels[i] = vd[lepus::kKeyVarDefScopeLevel].GetInt();
        scope_next_info[i] = vd[lepus::kKeyVarDefScopeNext].GetInt();
        int flags_val = vd[lepus::kKeyVarDefFlags].GetInt();
        flags[i] = static_cast<uint8_t>(flags_val & 0xFF);
      }
      SetFunctionVarDefs(ctx, b, var_names.data(), scope_levels.data(),
                         scope_next_info.data(), flags.data(), count);
    }
  }
}

void FillFunctionBytecodeDebugInfo(LEPUSContext* ctx, LEPUSFunctionBytecode* b,
                                   rapidjson::Value& debug_info) {
  FillFunctionBytecodeDebugInfo(ctx, b, debug_info, "");
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
  HandleScope handle_scope(ctx, function_list, HANDLE_TYPE_DIR_HEAP_OBJ);
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
  std::string source;
  const auto* function_source = FindMember(debug_info_entry, kKeyFunctionSource,
                                           kKeyFunctionSourceCompact);
  const auto* end_line_number =
      FindMember(debug_info_entry, kKeyEndLineNumber, kKeyEndLineNumberCompact);
  if (function_source && end_line_number) {
    source = function_source->GetString();
    char* source_str = const_cast<char*>(source.c_str());
    SetDebuggerSourceCode(ctx, source_str);
    int32_t end_line_num = end_line_number->GetInt();
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
        FillFunctionBytecodeDebugInfo(ctx, b, debug_info_entry, source);
      }
      LEPUS_WriteBarrierNoStore(ctx, script);
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
      entry.CopyFrom(document[kKeyLepusNGDebugInfo], document.GetAllocator());
      has_function_info =
          HasMember(entry, kKeyFunctionNumber, kKeyFunctionNumberCompact);
      if (has_function_info) {
        function_num =
            FindMember(entry, kKeyFunctionNumber, kKeyFunctionNumberCompact)
                ->GetUint();
      }
    }
  } else {
    for (auto it = document.MemberBegin(); it != document.MemberEnd(); it++) {
      std::string name = it->name.GetString();
      const auto& value = it->value;
      if (!value.IsObject()) {
        continue;
      }
      bool item_has_function_info =
          HasMember(value, kKeyFunctionNumber, kKeyFunctionNumberCompact);
      if (item_has_function_info) {
        function_num =
            FindMember(value, kKeyFunctionNumber, kKeyFunctionNumberCompact)
                ->GetUint() -
            function_num;
      }
      if (url.find(name) != std::string::npos) {
        entry.CopyFrom(value, document.GetAllocator());
        has_function_info = item_has_function_info;
        break;
      }
    }
  }
  if (!entry.IsObject() || entry.MemberCount() == 0) {
    error_message = "Cannot find the target entry in debug-info.";
    return false;
  }
  if (entry.HasMember(kKeyDebugInfoVersion) &&
      entry[kKeyDebugInfoVersion].IsInt() &&
      entry[kKeyDebugInfoVersion].GetInt() > kCompactDebugInfoVersion) {
    std::stringstream error_message_stream;
    error_message_stream << "Unsupported debug-info schema version: "
                         << entry[kKeyDebugInfoVersion].GetInt();
    error_message = error_message_stream.str();
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
