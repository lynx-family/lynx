// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynx_template_bundle_converter.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/utils/value_utils.h"
#include "core/runtime/lepusng/quick_context.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {

/// method to serialize compilerOptions in template_bundle into json string.
void SerializeCompilerOptions(const CompileOptions &compile_options,
                              rapidjson::Document &document) {
  auto &allocator = document.GetAllocator();
  rapidjson::Value compiler_options_json(rapidjson::kObjectType);

  compiler_options_json.AddMember("config_type", compile_options.config_type,
                                  allocator);

#define SERIALIZE_FIXED_LENGTH_FIELD(TYPE, NAME, ID)                         \
  if constexpr (std::is_same<decltype(compile_options.NAME), bool>::value) { \
    compiler_options_json.AddMember(                                         \
        #NAME, static_cast<bool>(compile_options.NAME), allocator);          \
  } else {                                                                   \
    compiler_options_json.AddMember(                                         \
        #NAME, static_cast<int64_t>(compile_options.NAME), allocator);       \
  }

  FOREACH_FIXED_LENGTH_FIELD(SERIALIZE_FIXED_LENGTH_FIELD);
#undef SERIALIZE_FIXED_LENGTH_FIELD

#define SERIALIZE_STRING_FIELD_IMPL(NAME, ID)                           \
  compiler_options_json.AddMember(                                      \
      #NAME, rapidjson::Value(compile_options.NAME.c_str(), allocator), \
      allocator);

  FOREACH_STRING_FIELD(SERIALIZE_STRING_FIELD_IMPL);
#undef SERIALIZE_STRING_FIELD_IMPL

  document.AddMember("compilerOptions", compiler_options_json, allocator);
}

/// method to serialize a template_bundle into json string,
/// be careful if you need to call this method.
void SerializeBTSBundle(const runtime::js::JsBundle &js_bundle,
                        rapidjson::Document &document) {
  auto js_files = js_bundle.GetAllJsFiles();
  auto &allocator = document.GetAllocator();
  std::for_each(
      js_files.begin(), js_files.end(),
      [&document,
       &allocator](const std::pair<std::string, runtime::js::JsContent> &pair) {
        auto js_content = pair.second;
        rapidjson::Document js_content_document(&allocator);
        js_content_document.SetObject();
        js_content_document.AddMember("path", pair.first, allocator);
        js_content_document.AddMember(
            "type",
            rapidjson::Value(js_content.IsByteCode() ? "bytecode" : "source",
                             allocator),
            allocator);
        std::string str(
            reinterpret_cast<const char *>(js_content.GetBuffer()->data()),
            js_content.GetBuffer()->size());
        js_content_document.AddMember(
            "content", rapidjson::Value(std::move(str), allocator), allocator);
        document.PushBack(rapidjson::Value(js_content_document, allocator),
                          allocator);
      });
}

void SerializeMTSBundle(
    const std::shared_ptr<lepus::ContextBundle> &context_bundle,
    rapidjson::Document &document) {
  // serialize just support lepusng
  if (context_bundle->IsLepusNG()) {
    auto &allocator = document.GetAllocator();
    lepus::QuickContextBundle *bundle =
        static_cast<lepus::QuickContextBundle *>(context_bundle.get());
    rapidjson::Value lepus_code(rapidjson::kArrayType);
    for (const auto &element : bundle->lepus_code()) {
      lepus_code.PushBack(element, allocator);
    }
    document.AddMember("lepus_code", lepus_code, allocator);
    document.AddMember("lepus_code_len",
                       rapidjson::Value(bundle->lepusng_code_len()), allocator);
  }
}

void SerializeCustomSections(const lepus::Value &custom_sections,
                             rapidjson::Document &document) {
  auto &allocator = document.GetAllocator();
  ForEachLepusValue(
      custom_sections, [&allocator, &document](const lepus::Value &key,
                                               const lepus::Value &value) {
        auto key_rapid = rapidjson::Value(key.String().c_str(), allocator);
        if (value.IsString()) {
          auto value_str = value.String().c_str();
          document.AddMember(key_rapid, rapidjson::Value(value_str, allocator),
                             allocator);
        } else if (value.IsByteArray()) {
          auto byte_array = value.ByteArray();
          auto *ptr = byte_array->GetPtr();
          rapidjson::Value lepus_code(rapidjson::kArrayType);
          std::vector<uint8_t> binary =
              std::vector<uint8_t>(ptr, ptr + byte_array->GetLength());
          for (const auto &element : binary) {
            lepus_code.PushBack(element, allocator);
          }
          document.AddMember(key_rapid, lepus_code, allocator);
        }
      });
}

std::string
LynxTemplateBundleConverter::ConvertTemplateBundleToSerializedString(
    LynxTemplateBundle &template_bundle) {
  rapidjson::Document main_document;
  main_document.SetObject();
  rapidjson::Document::AllocatorType &allocator = main_document.GetAllocator();

  // put header info;
  main_document.AddMember("total-size", template_bundle.total_size_, allocator);
  main_document.AddMember("is-lepusng-binary",
                          template_bundle.is_lepusng_binary_, allocator);
  main_document.AddMember("engine-version", template_bundle.target_sdk_version_,
                          allocator);
  main_document.AddMember("app-type", template_bundle.app_type_, allocator);
  main_document.AddMember("enable-css-variable",
                          template_bundle.enable_css_variable_, allocator);
  main_document.AddMember("enable-css-parser",
                          template_bundle.enable_css_parser_, allocator);

  // put compiler option;
  SerializeCompilerOptions(template_bundle.GetCompileOptions(), main_document);

  // put page config;
  auto page_config = template_bundle.GetPageConfig();
  main_document.AddMember("page-config", page_config->GetOriginalConfig(),
                          allocator);

  // css
  rapidjson::Document css_document(&allocator);
  css_document.SetObject();
  main_document.AddMember("css", rapidjson::Value(css_document, allocator),
                          allocator);

  // main-thread-script
  rapidjson::Document mts_document(&allocator);
  mts_document.SetObject();
  SerializeMTSBundle(template_bundle.context_bundle_, mts_document);
  main_document.AddMember("main-thread-script",
                          rapidjson::Value(mts_document, allocator), allocator);

  // background-thread-script
  rapidjson::Document bts_document(&allocator);
  bts_document.SetArray();
  SerializeBTSBundle(template_bundle.GetJsBundle(), bts_document);
  main_document.AddMember("background-thread-script",
                          rapidjson::Value(bts_document, allocator), allocator);

  // custom-sections
  rapidjson::Document custom_sections(&allocator);
  custom_sections.SetObject();
  SerializeCustomSections(template_bundle.custom_sections_, custom_sections);
  main_document.AddMember("custom-sections",
                          rapidjson::Value(custom_sections, allocator),
                          allocator);

  // cast to string;
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  main_document.Accept(writer);
  return buffer.GetString();
}
}  // namespace tasm
}  // namespace lynx
