// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepusng/quickjs_debug_info.h"

#include <algorithm>
#include <string>
#include <utility>

#include "core/renderer/tasm/config.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/template_bundle/template_codec/binary_encoder/encode_util.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace lepus {
namespace {
constexpr int kCompactDebugInfoVersion = 2;

constexpr char kKeyDebugInfoVersion[] = "v";
constexpr char kKeyFunctionSource[] = "function_source";
constexpr char kKeyFunctionSourceCompact[] = "fs";
constexpr char kKeyFunctionSourceOffset[] = "function_source_offset";
constexpr char kKeyFunctionSourceOffsetCompact[] = "fso";
constexpr char kKeyEndLineNumber[] = "end_line_num";
constexpr char kKeyEndLineNumberCompact[] = "el";
constexpr char kKeyFunctionNumber[] = "function_number";
constexpr char kKeyFunctionNumberCompact[] = "fn";
constexpr char kKeyFunctionInfo[] = "function_info";
constexpr char kKeyFunctionInfoCompact[] = "fi";
constexpr char kKeyFunctionId[] = "function_id";
constexpr char kKeyFunctionIdCompact[] = "id";
constexpr char kKeyFunctionName[] = "function_name";
constexpr char kKeyFunctionNameCompact[] = "n";
constexpr char kKeyFileName[] = "file_name";
constexpr char kKeyFileNameCompact[] = "f";
constexpr char kKeyLineNumber[] = "line_number";
constexpr char kKeyLineNumberCompact[] = "ln";
constexpr char kKeyColumnNumber[] = "column_number";
constexpr char kKeyColumnNumberCompact[] = "cn";
constexpr char kKeyLineCol[] = "line_col";
constexpr char kKeyLineColCompact[] = "lc";
constexpr char kKeyLine[] = "line";
constexpr char kKeyLineCompact[] = "l";
constexpr char kKeyColumn[] = "column";
constexpr char kKeyColumnCompact[] = "c";
constexpr char kKeyPc2LineLen[] = "pc2line_len";
constexpr char kKeyPc2LineLenCompact[] = "pll";
constexpr char kKeyPc2LineBuf[] = "pc2line_buf";
constexpr char kKeyPc2LineBufCompact[] = "plb";
constexpr char kKeyPc2CallerInfo[] = "pc2caller_info";
constexpr char kKeyPc2CallerInfoCompact[] = "pci";
constexpr char kKeyFunctionSourceLen[] = "function_source_len";
constexpr char kKeyFunctionSourceLenCompact[] = "fsl";

const char* DebugInfoKey(const char* key, const char* compact_key,
                         bool compact_debug_info) {
  return compact_debug_info ? compact_key : key;
}

size_t ComputeEndLineNum(const std::string& source) {
  size_t newline_character_num = std::count(source.begin(), source.end(), '\n');
  // line number start from 0
  return newline_character_num > 0 ? newline_character_num - 1
                                   : newline_character_num;
}
}  // namespace

rapidjson::Value QuickjsDebugInfoBuilder::BuildJsDebugInfo(
    LEPUSContext* ctx, LEPUSValue top_level_function, const std::string& source,
    rapidjson::Document::AllocatorType& allocator, bool debuginfo_outside,
    bool var_defs_outside, bool compact_debug_info) {
  rapidjson::Value debug_info{rapidjson::kObjectType};
  if (compact_debug_info) {
    debug_info.AddMember(kKeyDebugInfoVersion, kCompactDebugInfoVersion,
                         allocator);
  }
  if (source.size()) {
    debug_info.AddMember(
        rapidjson::StringRef(DebugInfoKey(
            kKeyFunctionSource, kKeyFunctionSourceCompact, compact_debug_info)),
        rapidjson::Value(source.c_str(), source.size(), allocator), allocator);
    debug_info.AddMember(
        rapidjson::StringRef(DebugInfoKey(
            kKeyEndLineNumber, kKeyEndLineNumberCompact, compact_debug_info)),
        static_cast<int32_t>(ComputeEndLineNum(source)), allocator);
  }
  if (!debuginfo_outside) {
    return debug_info;
  }

  debug_info.AddMember(
      rapidjson::StringRef(DebugInfoKey(
          kKeyFunctionNumber, kKeyFunctionNumberCompact, compact_debug_info)),
      DebuggerGetFuncSize(ctx), allocator);
  rapidjson::Value function_info{rapidjson::kArrayType};
  uint32_t size = 0;
  auto* function_list = GetDebuggerAllFunction(ctx, top_level_function, &size);
  if (function_list) {
    for (uint32_t i = 0; i < size; ++i) {
      auto* bytecode = function_list[i];
      if (bytecode) {
        function_info.PushBack(
            BuildFunctionInfo(ctx, bytecode, i == 0, var_defs_outside,
                              compact_debug_info, allocator),
            allocator);
      }
    }
  }
  if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, function_list);
  debug_info.AddMember(
      rapidjson::StringRef(DebugInfoKey(
          kKeyFunctionInfo, kKeyFunctionInfoCompact, compact_debug_info)),
      std::move(function_info), allocator);
  return debug_info;
}

std::string QuickjsDebugInfoBuilder::BuildJsDebugInfo(
    LEPUSContext* ctx, LEPUSValue top_level_function, const std::string& source,
    bool debuginfo_outside, bool var_defs_outside, bool compact_debug_info) {
  rapidjson::Document document;
  auto& allocator = document.GetAllocator();
  auto debug_info =
      BuildJsDebugInfo(ctx, top_level_function, source, allocator,
                       debuginfo_outside, var_defs_outside, compact_debug_info);

  rapidjson::StringBuffer debug_info_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> debug_info_writer{
      debug_info_buffer};
  debug_info.Accept(debug_info_writer);
  return debug_info_buffer.GetString();
}

rapidjson::Value QuickjsDebugInfoBuilder::BuildFunctionInfo(
    LEPUSContext* ctx, LEPUSFunctionBytecode* bytecode, bool is_top_level,
    bool var_defs_outside, bool compact_debug_info,
    rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value function_info{rapidjson::kObjectType};
  // TODO: @zhangyuping
  // function_id should add 1 with primjs version 2.6
  uint32_t function_id = GetFunctionDebugId(bytecode) + 1;
  function_info.AddMember(
      rapidjson::StringRef(DebugInfoKey(kKeyFunctionId, kKeyFunctionIdCompact,
                                        compact_debug_info)),
      function_id, allocator);

  // function name
  auto* name = GetFunctionName(ctx, bytecode);
  std::string name_str(name ? name : "");
  if (name && !LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, name);
  if (name_str.size()) {
    function_info.AddMember(
        rapidjson::StringRef(DebugInfoKey(
            kKeyFunctionName, kKeyFunctionNameCompact, compact_debug_info)),
        rapidjson::Value(name_str.c_str(), name_str.size(), allocator),
        allocator);
  } else {
    static constexpr const char anonymous[] = "<anonymous>";
    function_info.AddMember(
        rapidjson::StringRef(DebugInfoKey(
            kKeyFunctionName, kKeyFunctionNameCompact, compact_debug_info)),
        anonymous, allocator);
  }

  // filename
  const char* debug_filename = GetFunctionDebugFileName(ctx, bytecode);
  if (debug_filename) {
    function_info.AddMember(
        rapidjson::StringRef(DebugInfoKey(kKeyFileName, kKeyFileNameCompact,
                                          compact_debug_info)),
        rapidjson::Value(debug_filename, strlen(debug_filename), allocator),
        allocator);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, debug_filename);
  }

  // line number
  function_info.AddMember(
      rapidjson::StringRef(DebugInfoKey(kKeyLineNumber, kKeyLineNumberCompact,
                                        compact_debug_info)),
      GetFunctionDebugLineNum(ctx, bytecode) + 1, allocator);

  // column number
  function_info.AddMember(
      rapidjson::StringRef(DebugInfoKey(
          kKeyColumnNumber, kKeyColumnNumberCompact, compact_debug_info)),
      GetFunctionDebugColumnNum(ctx, bytecode), allocator);

  // line col for logbox
  function_info.AddMember(
      rapidjson::StringRef(
          DebugInfoKey(kKeyLineCol, kKeyLineColCompact, compact_debug_info)),
      GetFunctionLineAndColInfo(ctx, bytecode, allocator, compact_debug_info),
      allocator);
  // pc2line_len
  int32_t pc2line_len = GetFunctionDebugPC2LineLen(ctx, bytecode);
  function_info.AddMember(
      rapidjson::StringRef(DebugInfoKey(kKeyPc2LineLen, kKeyPc2LineLenCompact,
                                        compact_debug_info)),
      pc2line_len, allocator);

  // pc2line_buf, maybe unused.
  rapidjson::Value pc2line_buf{rapidjson::kArrayType};
  const auto* pc2line_buffer = GetFunctionDebugPC2LineBuf(ctx, bytecode);
  if (pc2line_buffer) {
    for (int32_t i = 0; i < pc2line_len; ++i) {
      pc2line_buf.PushBack(pc2line_buffer[i], allocator);
    }
  }

  function_info.AddMember(
      rapidjson::StringRef(DebugInfoKey(kKeyPc2LineBuf, kKeyPc2LineBufCompact,
                                        compact_debug_info)),
      std::move(pc2line_buf), allocator);

  rapidjson::Value pc2caller_obj{rapidjson::kObjectType};
  auto pc2caller_ret = GetFunctionCallerString(ctx, bytecode);

  using process_jsobject =
      base::MoveOnlyClosure<void, LEPUSContext*, LEPUSValue, LEPUSValue>;

  process_jsobject inner_processor = [&pc2caller_obj, &allocator](
                                         LEPUSContext* ctx, LEPUSValue pc,
                                         LEPUSValue string) {
    size_t len = 0;
    const char* pc_idx = LEPUS_ToCStringLen(ctx, &len, pc);
    rapidjson::Value key{pc_idx, static_cast<uint32_t>(len), allocator};
    const char* caller_str = LEPUS_ToCStringLen(ctx, &len, string);
    pc2caller_obj.AddMember(key, rapidjson::Value(caller_str, len, allocator),
                            allocator);
    if (!LEPUS_IsGCMode(ctx)) {
      LEPUS_FreeCString(ctx, pc_idx);
      LEPUS_FreeCString(ctx, caller_str);
    }
    return;
  };

  LEPUS_IterateObject(
      ctx, pc2caller_ret,
      [](LEPUSContext* ctx, LEPUSValue pc, LEPUSValue caller_str, void* p_func,
         void*) {
        // p_func == &pc2caller_obj is true.
        reinterpret_cast<process_jsobject*>(p_func)->operator()(ctx, pc,
                                                                caller_str);
      },
      &inner_processor, nullptr);

  function_info.AddMember(
      rapidjson::StringRef(DebugInfoKey(
          kKeyPc2CallerInfo, kKeyPc2CallerInfoCompact, compact_debug_info)),
      std::move(pc2caller_obj), allocator);

  LEPUS_FreeValue(ctx, pc2caller_ret);

  // source code for child function
  if (!is_top_level && bytecode) {
    int32_t source_offset = GetFunctionDebugSourceOffset(ctx, bytecode);
    int32_t source_len = GetFunctionDebugSourceLen(ctx, bytecode);
    function_info.AddMember(
        rapidjson::StringRef(DebugInfoKey(kKeyFunctionSourceLen,
                                          kKeyFunctionSourceLenCompact,
                                          compact_debug_info)),
        source_len, allocator);
    if (compact_debug_info && source_offset >= 0) {
      function_info.AddMember(
          rapidjson::StringRef(DebugInfoKey(kKeyFunctionSourceOffset,
                                            kKeyFunctionSourceOffsetCompact,
                                            compact_debug_info)),
          source_offset, allocator);
    } else {
      const char* func_source = GetFunctionDebugSource(ctx, bytecode);
      std::string source(func_source ? func_source : "");
      function_info.AddMember(
          rapidjson::StringRef(DebugInfoKey(kKeyFunctionSource,
                                            kKeyFunctionSourceCompact,
                                            compact_debug_info)),
          rapidjson::Value(source.c_str(), source.size(), allocator),
          allocator);
    }
  }

  // vardefs (for varinfo outside)
  if (var_defs_outside) {
    uint32_t var_defs_count = GetFunctionVarDefCount(bytecode);
    if (var_defs_count > 0) {
      rapidjson::Value var_defs_arr{rapidjson::kArrayType};
      for (uint32_t i = 0; i < var_defs_count; ++i) {
        rapidjson::Value vd{rapidjson::kObjectType};
        const char* name = GetFunctionVarDefName(ctx, bytecode, i);
        std::string name_str(name ? name : "");
        if (name && !LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, name);
        vd.AddMember(
            kKeyVarDefName,
            rapidjson::Value(name_str.c_str(), name_str.size(), allocator),
            allocator);
        vd.AddMember(kKeyVarDefScopeLevel,
                     GetFunctionVarDefScopeLevel(bytecode, i), allocator);
        vd.AddMember(kKeyVarDefScopeNext,
                     GetFunctionVarDefScopeNext(bytecode, i), allocator);
        vd.AddMember(kKeyVarDefFlags,
                     static_cast<int>(GetFunctionVarDefFlags(bytecode, i)),
                     allocator);
        var_defs_arr.PushBack(std::move(vd), allocator);
      }
      function_info.AddMember(kKeyVarDefs, std::move(var_defs_arr), allocator);
    }
  }

  return function_info;
}

rapidjson::Value QuickjsDebugInfoBuilder::GetFunctionLineAndColInfo(
    LEPUSContext* ctx, const LEPUSFunctionBytecode* bytecode,
    rapidjson::Document::AllocatorType& allocator, bool compact_debug_info) {
  rapidjson::Value line_col{rapidjson::kArrayType};
  size_t size = 0;
  int64_t* line_col_info = GetFunctionLineNums(ctx, bytecode, &size);
  for (size_t i = 0; i < size; ++i) {
    int32_t line = -1;
    int64_t column = -1;
    ComputeLineCol(line_col_info[i], &line, &column);
    rapidjson::Value line_col_each(rapidjson::kObjectType);
    line_col_each.AddMember(rapidjson::StringRef(DebugInfoKey(
                                kKeyLine, kKeyLineCompact, compact_debug_info)),
                            line + 1, allocator);
    line_col_each.AddMember(
        rapidjson::StringRef(
            DebugInfoKey(kKeyColumn, kKeyColumnCompact, compact_debug_info)),
        column + 1, allocator);
    line_col.PushBack(std::move(line_col_each), allocator);
  }
  if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, line_col_info);
  return line_col;
}

void QuickjsDebugInfoBuilder::AddDebugInfo(
    const std::string& filename, const tasm::LepusDebugInfo& debug_info,
    QuickContext* ctx) {
  auto& allocator = document_.GetAllocator();
  bool debuginfo_outside =
      tasm::Config::IsHigherOrEqual(ctx->GetSdkVersion(), LYNX_VERSION_2_5) &&
      ctx->debuginfo_outside();
  bool var_defs_outside =
      tasm::Config::IsHigherOrEqual(ctx->GetSdkVersion(), LYNX_VERSION_4_1) &&
      ctx->debuginfo_outside();
  bool compact_debug_info =
      tasm::Config::IsHigherOrEqual(ctx->GetSdkVersion(), LYNX_VERSION_4_2) &&
      ctx->debuginfo_outside();
  template_debug_data_.AddMember(
      rapidjson::Value{filename.c_str(), allocator},
      lepus::QuickjsDebugInfoBuilder::BuildJsDebugInfo(
          ctx->context(), debug_info.debug_info_.top_level_function,
          debug_info.debug_info_.source_code, allocator, debuginfo_outside,
          var_defs_outside, compact_debug_info),
      allocator);
}

}  // namespace lepus
}  // namespace lynx
