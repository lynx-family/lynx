// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/encode_util.h"

#include <utility>

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider_src.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/quickjs_debug_info.h"
#include "core/template_bundle/template_codec/binary_encoder/template_binary_writer.h"
#include "core/template_bundle/template_codec/generator/source_generator.h"

namespace lynx {
namespace tasm {

void GetScopeInfo(lynx::lepus::Value& scopes, rapidjson::Value& function_scope,
                  rapidjson::Document::AllocatorType& allocator) {
  lynx::lepus::Value start = scopes.GetProperty(lepus::Function::kStartLine);
  lynx::lepus::Value end = scopes.GetProperty(lepus::Function::kEndLine);
  int64_t start_line_col = static_cast<uint64_t>(start.Int64());
  int64_t end_line_col = static_cast<uint64_t>(end.Int64());
  int32_t line, col = -1;
  lynx::lepus::Function::DecodeLineCol(start_line_col, line, col);
  // line start from 0
  line--;
  function_scope.AddMember("start_line", line, allocator);
  function_scope.AddMember("start_column", col, allocator);

  line = -1, col = -1;
  lynx::lepus::Function::DecodeLineCol(end_line_col, line, col);
  // line start from 0
  line--;

  function_scope.AddMember("end_line", line, allocator);
  function_scope.AddMember("end_column", col, allocator);
  rapidjson::Value variable_info(rapidjson::kArrayType);
  int variable_number = 0;
  for (auto& it : *scopes.Table()) {
    if (!it.second.IsUInt32()) continue;
    variable_number++;
    rapidjson::Value each_variable(rapidjson::kObjectType);
    each_variable.AddMember("variable_name",
                            rapidjson::StringRef(it.first.c_str()), allocator);
    each_variable.AddMember("variable_reg_info",
                            rapidjson::Value(it.second.UInt32()), allocator);
    variable_info.PushBack(each_variable, allocator);
  }
  function_scope.AddMember("variable_number", variable_number, allocator);
  function_scope.AddMember("variable_info", variable_info, allocator);

  // child_function_scope
  rapidjson::Value child_scopes(rapidjson::kObjectType);
  rapidjson::Value child_scope_array(rapidjson::kArrayType);
  lynx::lepus::Value childs = scopes.GetProperty(lepus::Function::kChilds);
  if (childs.Array()) {
    int32_t size = static_cast<int32_t>(childs.Array()->size());
    for (int32_t i = 0; i < size; i++) {
      rapidjson::Value child_scope(rapidjson::kObjectType);
      lynx::lepus::Value c = childs.Array()->get(i);
      GetScopeInfo(c, child_scope, allocator);
      child_scope_array.PushBack(child_scope, allocator);
    }
    child_scopes.AddMember("child_scope_number", rapidjson::Value(size),
                           allocator);
    child_scopes.AddMember("child_scope_info", child_scope_array, allocator);
    function_scope.AddMember("child_scope", child_scopes, allocator);
  }
}

void GetLineColInfo(const LepusFunction& current_func,
                    rapidjson::Value& function,
                    rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value line_col_info(rapidjson::kObjectType);
  lynx::lepus::Value line_col_value = current_func->GetLineInfo();
  int32_t pc_size = static_cast<int32_t>(line_col_value.Array()->size());
  line_col_info.AddMember("pc_size", pc_size, allocator);
  rapidjson::Value line_col(rapidjson::kArrayType);
  for (int32_t j = 0; j < pc_size; j++) {
    rapidjson::Value line_col_each(rapidjson::kObjectType);
    lynx::lepus::Value number = line_col_value.Array()->get(j);
    int64_t line_col_number = static_cast<int64_t>(number.Int64());
    int32_t line, col = -1;
    // line, col start from 1
    lynx::lepus::Function::DecodeLineCol(line_col_number, line, col);
    line_col_each.AddMember("line", line, allocator);
    line_col_each.AddMember("column", col, allocator);
    line_col.PushBack(line_col_each, allocator);
  }
  line_col_info.AddMember("line_col", line_col, allocator);
  function.AddMember("line_col_info", line_col_info, allocator);
}
void GetChildFunctionInfo(const LepusFunction& current_func,
                          rapidjson::Value& function,
                          rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value child_function(rapidjson::kObjectType);
  child_function.AddMember("child_function_number",
                           rapidjson::Value(static_cast<int32_t>(
                               current_func->GetChildFunction().size())),
                           allocator);
  rapidjson::Value child_function_info(rapidjson::kArrayType);
  int32_t child_function_size =
      static_cast<int32_t>(current_func->GetChildFunction().size());
  for (size_t x = 0; x < child_function_size; x++) {
    rapidjson::Value child_each(rapidjson::kObjectType);
    int64_t child_function_id =
        current_func->GetChildFunction(static_cast<long>(x))->GetFunctionId();
    // child id
    child_each.AddMember("child_id", child_function_id, allocator);
    child_function_info.PushBack(child_each, allocator);
  }
  child_function.AddMember("child_function_info", child_function_info,
                           allocator);
  function.AddMember("child_function", child_function, allocator);
}

void GetDebugInfo(const std::vector<LepusFunction>& funcs,
                  rapidjson::Value& template_debug_data,
                  rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value lepus_debug_info(rapidjson::kObjectType);
  int32_t function_number = static_cast<int32_t>(funcs.size());
  // function_number
  lepus_debug_info.AddMember("function_number", function_number, allocator);
  if (function_number > 0) {
    rapidjson::Value function_info(rapidjson::kArrayType);
    for (int32_t i = 0; i < function_number; i++) {
      auto current_func = funcs[i];
      rapidjson::Value function(rapidjson::kObjectType);
      // function id
      function.AddMember("function_id", current_func->GetFunctionId(),
                         allocator);
      // function_name
      rapidjson::Value func_name_json;
      std::string func_name = current_func->GetFunctionName();
      func_name_json.SetString(func_name.c_str(),
                               static_cast<unsigned int>(func_name.length()),
                               allocator);
      function.AddMember("function_name", func_name_json, allocator);
      // line_col_info
      GetLineColInfo(current_func, function, allocator);
      // function source
      function.AddMember("function_source", current_func->GetSource(),
                         allocator);
      // function scope
      rapidjson::Value function_scope(rapidjson::kObjectType);
      lynx::lepus::Value scopes = current_func->GetScope();
      GetScopeInfo(scopes, function_scope, allocator);
      function.AddMember("function_scope", function_scope, allocator);

      // child function
      GetChildFunctionInfo(current_func, function, allocator);
      function_info.PushBack(function, allocator);
    }
    lepus_debug_info.AddMember("function_info", function_info, allocator);
  }
  template_debug_data.AddMember("lepus_debug_info", lepus_debug_info,
                                allocator);
}

void CheckPackageInstance(const PackageInstanceType inst_type,
                          rapidjson::Document& document, std::string& error) {
  std::string instance_type = "";
  switch (inst_type) {
    case PackageInstanceType::CARD:
      instance_type = TEMPLATE_BUNDLE_PAGES;
      break;
    case PackageInstanceType::DYNAMIC_COMPONENT:
      instance_type = TEMPLATE_BUNDLE_DYNAMIC_COMPONENTS;
      break;
    default:
      error = "UNREACHABLE";
      return;
  }
  // package instance must be array
  rapidjson::Value::MemberIterator package_instance =
      document.FindMember(instance_type);
  if (package_instance == document.MemberEnd() ||
      !package_instance->value.IsArray()) {
    error = "error: don't find tag " + instance_type;
  }
}

std::string MakeSuccessResult() {
  rapidjson::Document document;

  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
  rapidjson::Value result(rapidjson::kObjectType);
  rapidjson::Value status(0);
  rapidjson::Value msg("");
  result.AddMember("status", status, allocator);
  result.AddMember("error_msg", msg, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  result.Accept(writer);
  std::string str = buffer.GetString();
  return str;
}

std::string MakeErrorResult(const char* errorMessage, const char* file,
                            const char* location) {
  rapidjson::Document document;

  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
  rapidjson::Value result(rapidjson::kObjectType);
  rapidjson::Value status(1);
  rapidjson::Value msg(errorMessage, allocator);
  result.AddMember(rapidjson::Value("status"), status, allocator);
  result.AddMember(rapidjson::Value("error_msg"), msg, allocator);

  rapidjson::Value data(rapidjson::kObjectType);
  rapidjson::Document location_doc;
  if (!location_doc.Parse(location).HasParseError()) {
    data.AddMember(rapidjson::Value("location"), location_doc, allocator);
  }
  data.AddMember(rapidjson::Value("filePath"),
                 rapidjson::Value(file, allocator), allocator);

  result.AddMember("data", data, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  result.Accept(writer);
  std::string str = buffer.GetString();
  return str;
}

std::string MakeRepackBufferResult(std::vector<uint8_t>&& data,
                                   BufferPool* pool) {
  rapidjson::Document document;

  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
  rapidjson::Value result(rapidjson::kObjectType);
  rapidjson::Value status(0);
  rapidjson::Value msg("");
  result.AddMember("status", status, allocator);
  result.AddMember("error_msg", msg, allocator);

  rapidjson::Value bufferIndex((unsigned int)pool->buffers.size());
  pool->buffers.push_back(data);
  result.AddMember("buffer", bufferIndex, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  result.Accept(writer);
  std::string str = buffer.GetString();
  return str;
}

lynx::tasm::EncodeResult CreateSuccessResult(const std::vector<uint8_t>& buffer,
                                             const std::string& lepus_code,
                                             const std::string& section_size,
                                             TemplateBinaryWriter* writer) {
  rapidjson::Document document;
  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
  rapidjson::Value template_debug_data(rapidjson::kObjectType);
  if (writer) {
    auto* context = writer->context();
    if (!context->IsLepusNGContext()) {
      auto info = writer->GetDebugInfo();
      GetDebugInfo(info.lepus_funcs_, template_debug_data, allocator);
    } else {
      template_debug_data = writer->TakeLepusNGDebugInfo();
    }

    for (const auto& [filename, js_debug_info] : writer->GetJsDebugInfo()) {
      template_debug_data.AddMember(
          rapidjson::Value{filename.c_str(), allocator},
          lepus::QuickjsDebugInfoBuilder::BuildJsDebugInfo(
              js_debug_info->context_, js_debug_info->top_level_func_,
              js_debug_info->source_, allocator, true),
          allocator);
    }
  }

  rapidjson::StringBuffer template_debug_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> template_debug_writer(
      template_debug_buffer);
  template_debug_data.Accept(template_debug_writer);
  std::string template_debug_str = template_debug_buffer.GetString();

  lynx::tasm::EncodeResult ret{.status = 0,
                               .buffer = std::move(buffer),
                               .lepus_code = std::move(lepus_code),
                               .lepus_debug = std::move(template_debug_str),
                               .section_size = std::move(section_size)};
  return ret;
};

lynx::tasm::EncodeResult CreateErrorResult(const std::string& error_msg) {
  lynx::tasm::EncodeResult ret{.status = 1, .error_msg = error_msg};
  return ret;
};

lynx::tasm::EncodeResult CreateSSRSuccessResult(
    const std::vector<uint8_t>& buffer) {
  lynx::tasm::EncodeResult ret{.status = 0, .buffer = std::move(buffer)};
  return ret;
}

lynx::tasm::EncodeResult CreateSSRErrorResult(int status,
                                              const std::string& error_msg) {
  lynx::tasm::EncodeResult ret{.status = status, .error_msg = error_msg};
  return ret;
}

std::string BinarySectionTypeToString(BinarySection section) {
  std::string res;
  switch (section) {
    case BinarySection::STRING:
      res = "string";
      break;
    case BinarySection::CSS:
      res = "css";
      break;
    case BinarySection::COMPONENT:
      res = "component";
      break;
    case BinarySection::PAGE:
      res = "page";
      break;
    case BinarySection::APP:
      res = "app";
      break;
    case BinarySection::JS:
      res = "js";
      break;
    case BinarySection::JS_BYTECODE:
      res = "js_bytecode";
      break;
    case BinarySection::CONFIG:
      res = "config";
      break;
    case BinarySection::DYNAMIC_COMPONENT:
      res = "dynamic_component";
      break;
    case BinarySection::THEMED:
      res = "themed";
      break;
    case BinarySection::USING_DYNAMIC_COMPONENT_INFO:
      res = "using_dynamic_component_info";
    case BinarySection::SECTION_ROUTE:
      res = "section_route";
      break;
    case BinarySection::ROOT_LEPUS:
      res = "root_lepus";
      break;
    case BinarySection::ELEMENT_TEMPLATE:
      res = "element_template";
      break;
    case BinarySection::PARSED_STYLES:
      res = "parsed_styles";
      break;
    case BinarySection::LEPUS_CHUNK:
      res = "lepus_chunk";
    case BinarySection::CUSTOM_SECTIONS:
      res = "custom_sections";
      break;
    case BinarySection::NEW_ELEMENT_TEMPLATE:
      res = "new_element_template";
      break;
  }
  return res;
}

// for debug
bool writefile(const std::string& filename, const std::string& src) {
  FILE* pf = fopen(filename.c_str(), "wb");
  if (pf == nullptr) {
    return false;
  }
  uint32_t size = src.length();
  fwrite((uint8_t*)&size, 1, 1, pf);
  fwrite(src.c_str(), size, 1, pf);
  fclose(pf);

  return true;
}

}  // namespace tasm
}  // namespace lynx
