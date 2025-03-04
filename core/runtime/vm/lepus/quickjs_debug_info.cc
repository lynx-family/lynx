// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/quickjs_debug_info.h"

#include <algorithm>
#include <string>
#include <utility>

#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/template_bundle/template_codec/binary_encoder/encode_util.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace lepus {
namespace {
size_t ComputeEndLineNum(const std::string& source) {
  size_t newline_character_num = std::count(source.begin(), source.end(), '\n');
  // line number start from 0
  return newline_character_num > 0 ? newline_character_num - 1
                                   : newline_character_num;
}
}  // namespace

rapidjson::Value QuickjsDebugInfoBuilder::BuildJsDebugInfo(
    LEPUSContext* ctx, LEPUSValue top_level_function, const std::string& source,
    rapidjson::Document::AllocatorType& allocator, bool debuginfo_outside) {
  rapidjson::Value debug_info{rapidjson::kObjectType};
  if (source.size()) {
    debug_info.AddMember(
        "function_source",
        rapidjson::Value(source.c_str(), source.size(), allocator), allocator);
    debug_info.AddMember("end_line_num",
                         static_cast<int32_t>(ComputeEndLineNum(source)),
                         allocator);
  }
  if (!debuginfo_outside) {
    return debug_info;
  }

  debug_info.AddMember("function_number", DebuggerGetFuncSize(ctx), allocator);
  rapidjson::Value function_info{rapidjson::kArrayType};
  uint32_t size = 0;
  auto* function_list = GetDebuggerAllFunction(ctx, top_level_function, &size);
  if (function_list) {
    for (uint32_t i = 0; i < size; ++i) {
      auto* bytecode = function_list[i];
      if (bytecode) {
        function_info.PushBack(
            BuildFunctionInfo(ctx, bytecode, i == 0, allocator), allocator);
      }
    }
  }
  if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, function_list);
  debug_info.AddMember("function_info", std::move(function_info), allocator);
  return debug_info;
}

std::string QuickjsDebugInfoBuilder::BuildJsDebugInfo(
    LEPUSContext* ctx, LEPUSValue top_level_function, const std::string& source,
    bool debuginfo_outside) {
  rapidjson::Document document;
  auto& allocator = document.GetAllocator();
  auto debug_info = BuildJsDebugInfo(ctx, top_level_function, source, allocator,
                                     debuginfo_outside);

  rapidjson::StringBuffer debug_info_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> debug_info_writer{
      debug_info_buffer};
  debug_info.Accept(debug_info_writer);
  return debug_info_buffer.GetString();
}

rapidjson::Value QuickjsDebugInfoBuilder::BuildFunctionInfo(
    LEPUSContext* ctx, LEPUSFunctionBytecode* bytecode, bool is_top_level,
    rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value function_info{rapidjson::kObjectType};
  // TODO: @zhangyuping
  // function_id should add 1 with primjs version 2.6
  uint32_t function_id = GetFunctionDebugId(bytecode) + 1;
  function_info.AddMember("function_id", function_id, allocator);

  // function name
  auto* name = GetFunctionName(ctx, bytecode);
  std::string name_str(name ? name : "");
  LEPUS_FreeCString(ctx, name);
  if (name_str.size()) {
    function_info.AddMember(
        "function_name",
        rapidjson::Value(name_str.c_str(), name_str.size(), allocator),
        allocator);
  } else {
    static constexpr const char anonymous[] = "<anonymous>";
    function_info.AddMember("function_name", anonymous, allocator);
  }

  // filename
  const char* debug_filename = GetFunctionDebugFileName(ctx, bytecode);
  if (debug_filename) {
    function_info.AddMember(
        "file_name", rapidjson::Value(debug_filename, strlen(debug_filename)),
        allocator);
    LEPUS_FreeCString(ctx, debug_filename);
  }

  // line number
  function_info.AddMember(
      "line_number", GetFunctionDebugLineNum(ctx, bytecode) + 1, allocator);

  // column number
  function_info.AddMember("column_number",
                          GetFunctionDebugColumnNum(ctx, bytecode), allocator);

  // line col for logbox
  function_info.AddMember("line_col",
                          GetFunctionLineAndColInfo(ctx, bytecode, allocator),
                          allocator);
  // pc2line_len
  int32_t pc2line_len = GetFunctionDebugPC2LineLen(ctx, bytecode);
  function_info.AddMember("pc2line_len", pc2line_len, allocator);

  // pc2line_buf, maybe unused.
  rapidjson::Value pc2line_buf{rapidjson::kArrayType};
  const auto* pc2line_buffer = GetFunctionDebugPC2LineBuf(ctx, bytecode);
  if (pc2line_buffer) {
    for (int32_t i = 0; i < pc2line_len; ++i) {
      pc2line_buf.PushBack(pc2line_buffer[i], allocator);
    }
  }

  function_info.AddMember("pc2line_buf", std::move(pc2line_buf), allocator);

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

  function_info.AddMember("pc2caller_info", std::move(pc2caller_obj),
                          allocator);

  LEPUS_FreeValue(ctx, pc2caller_ret);

  // source code for child function
  if (!is_top_level && bytecode) {
    function_info.AddMember("function_source_len",
                            GetFunctionDebugSourceLen(ctx, bytecode),
                            allocator);
    const char* func_source = GetFunctionDebugSource(ctx, bytecode);
    std::string source(func_source ? func_source : "");
    function_info.AddMember(
        "function_source",
        rapidjson::Value(source.c_str(), source.size(), allocator), allocator);
  }

  return function_info;
}

rapidjson::Value QuickjsDebugInfoBuilder::GetFunctionLineAndColInfo(
    LEPUSContext* ctx, const LEPUSFunctionBytecode* bytecode,
    rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value line_col{rapidjson::kArrayType};
  size_t size = 0;
  int64_t* line_col_info = GetFunctionLineNums(ctx, bytecode, &size);
  for (size_t i = 0; i < size; ++i) {
    int32_t line = -1;
    int64_t column = -1;
    ComputeLineCol(line_col_info[i], &line, &column);
    rapidjson::Value line_col_each(rapidjson::kObjectType);
    line_col_each.AddMember("line", line + 1, allocator);
    line_col_each.AddMember("column", column + 1, allocator);
    line_col.PushBack(std::move(line_col_each), allocator);
  }
  if (!LEPUS_IsGCMode(ctx)) lepus_free(ctx, line_col_info);
  return line_col;
}

void QuickjsDebugInfoBuilder::AddDebugInfo(
    const std::string& filename, const tasm::LepusDebugInfo& debug_info,
    QuickContext* ctx) {
  auto& allocator = document_.GetAllocator();
  template_debug_data_.AddMember(
      rapidjson::Value{filename.c_str(), allocator},
      lepus::QuickjsDebugInfoBuilder::BuildJsDebugInfo(
          ctx->context(), debug_info.debug_info_.top_level_function,
          debug_info.debug_info_.source_code, allocator,
          tasm::Config::IsHigherOrEqual(ctx->GetSdkVersion(),
                                        LYNX_VERSION_2_5) &&
              ctx->debuginfo_outside()),
      allocator);
}

}  // namespace lepus
}  // namespace lynx
