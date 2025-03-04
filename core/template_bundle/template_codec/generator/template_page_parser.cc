// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/generator/template_page_parser.h"

#include <set>
#include <unordered_map>

#include "core/runtime/vm/lepus/exception.h"
#include "core/template_bundle/template_codec/generator/template_scope.h"

namespace lynx {
namespace tasm {

TemplatePageParser::TemplatePageParser(const EncoderOptions &encoder_options)
    : TemplateParser(encoder_options) {}

TemplatePageParser::~TemplatePageParser() = default;

void TemplatePageParser::Parse() { ParseCard(); }

std::string TemplatePageParser::GenPageSource(Page *page) {
  if (lepus_js_code_.empty()) {
    return GenPageSourceForTT(page);
  } else {
    return GenPageSourceForReactCompilerNG(page);
  }
}

std::string TemplatePageParser::GenPageSourceForTT(Page *page) {
  std::stringstream page_renders;
  page_renders << GenPageRenderer(page);

  auto necessary_renders = GenNecessaryRenders(page);

  // Declare page data
  std::stringstream declare_page_data;
  // Put "SystemInfo" in variables
  declare_page_data << "let __globalProps = null;";
  declare_page_data << "let SystemInfo = null;";
  for (auto var : page->variables_in_use()) {
    LepusGenRuleMap rule = {{"let", var.str()}};
    // clang-format off
    static std::string source = "let {let} = null;";
    // clang-format on
    declare_page_data << FormatStringWithRule(source, rule);
  }

  // Main page id
  std::stringstream page_id;
  page_id << page->id();

  // CSS id
  std::stringstream css_id;
  css_id << GetCSSForComponent(page);

  // Template API
  std::string preprocessFunction = GenPagePreprocessFunction(page);

  // Template Script
  std::string packed_script =
      compile_options_.enable_lepus_ng_ &&
              Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                      FEATURE_TEMPLATE_SCRIPT)
          ? package_instance_->packed_script()
          : "";

  // Template API of components
  const ComponentMap &components = package_instance_->components();
  ComponentMap::const_iterator itr = components.begin();
  for (; itr != components.end(); itr++) {
    Component *component = itr->second.get();
    preprocessFunction += GenComponentPreprocessFunction(itr->first, component);
  }

  auto &template_render = necessary_renders["template"];
  auto &component_render = necessary_renders["component"];
  LepusGenRuleMap rule = {
      {"lepusRuntime", this->lepusRuntime()},
      {"declareTemplates", template_render.first},
      {"declareComponents", component_render.first},
      {"defineTemplateFunction", template_render.second},
      {"defineComponentFunction", component_render.second},
      {"definePageFunction", page_renders.str()},
      {"declarePageData", declare_page_data.str()},
      {"pageId", page_id.str()},
      {"cssId", css_id.str()},
      {"preBuildInFunctions", GenPreBuildInFunctions()},
      {"preprocessFunction", preprocessFunction},
      {"packedScript", packed_script},
  };

  // clang-format off
  static std::string source =
      "{declarePageData}"
      "{lepusRuntime}"
      "{preprocessFunction}"
      "{packedScript}"
      "{preBuildInFunctions}"
      "{declareTemplates}"
      "{declareComponents}"
      "{defineTemplateFunction}"
      "{defineComponentFunction}"
      "{definePageFunction}"
      "let $page = _CreatePage({pageId}, $kTemplateAssembler);"
      "_AttachPage($kTemplateAssembler, $page);";
  // clang-format on

  auto page_source = FormatStringWithRule(source, rule);

  result_[main_page_id_] = page_source;
  return page_source;
}

std::string TemplatePageParser::GenPageSourceForReactCompilerNG(Page *page) {
  app_mould_.page_list[page->path()] = page->id();
  GenPageMould(page);
  GenComponentMouldForCompilerNG(page);
  result_[main_page_id_] = lepus_js_code_;
  return lepus_js_code_;
}

std::string TemplatePageParser::GenPageRenderer(Page *page) {
  PageScope page_scope(this, page);
  app_mould_.page_list[page->path()] = page->id();
  std::stringstream id;
  std::stringstream content;
  auto &ttml = page->ttml();
  CheckPageElementValid(ttml);
  if (new_page_element_enabled_) {
    content << "_MarkPageElement();";
  }
  size_t size = ttml.Size();
  for (int i = 0; i < size; ++i) {
    content << GenInstruction(ttml[i]);
  }

  for (auto &iter : current_fragment_->templates()) {
    auto tem = iter.second;
    // Register and Trigger
    // TODO(zhoupeng): seem to be no need to add template to current_component_
    current_component_->AddTemplate(tem);
    template_helper_->RecordAvailableInfo(tem.get(), current_component_);
    if (tem->codes().empty()) {
      tem->set_codes({GenTemplateRenderer(tem.get())});
    }
  }

  for (auto &iter : current_fragment_->include_templates()) {
    auto tem = iter.second;
    // Register and Trigger
    template_helper_->RecordAvailableInfo(tem.get(), current_component_);
    if (tem->codes().empty()) {
      tem->set_codes({GenTemplateRenderer(tem.get())});
    }
  }

  // Generate page mould
  GenPageMould(page);

  // Template Script
  std::string script_defines = GenScriptMapDefines(page, "$page");

  // Template API
  std::string templateFunctions = GenTemplateFunctions(page);
  bool enableKeepPageData =
      Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_NEW_RENDER_PAGE) &&
      compile_options_.enable_keep_page_data;

  std::string data_defines = "";
  if (enableKeepPageData) {
    // Defines data
    std::stringstream data_defines_ss;
    for (auto var_name : page->variables_in_use()) {
      // clang-format off
          LepusGenRuleMap dataRule = {
                  {"varName", var_name.str()},
          };
          static std::string source =
                  "let {varName} = $data.{varName};";
      // clang-format on
      data_defines_ss << FormatStringWithRule(source, dataRule);
    }
    data_defines = data_defines_ss.str();
  }

  id << page->id();
  std::string render_page_header_raw =
      enableKeepPageData
          ? "function $renderPage{ID} ($page, $recursive, $data) {"
          : "function $renderPage{ID} ($page, $recursive) {";
  LepusGenRuleMap header_rule = {{"ID", id.str()}};
  std::string render_page_header =
      FormatStringWithRule(render_page_header_raw, header_rule);

  LepusGenRuleMap rule = {
      {"renderPageHeader", render_page_header},
      {"ID", id.str()},
      {"content", content.str()},
      {"componentInfoMapDefinition",
       GenDependentComponentInfoMapDefinition(page)},
      {"templateFunctions", templateFunctions},
      {"dataDefine", data_defines},
      {"scriptDefine", script_defines},
  };

  // clang-format off
  static std::string source =
          "{renderPageHeader}"
          "{templateFunctions}"
          "{dataDefine}"
          "{scriptDefine}"
          "let $parent = $page;"
          "{componentInfoMapDefinition}"
          "let $child = null;"
          "{content}"
      "}";
  // clang-format on
  return FormatStringWithRule(source, rule);
}

void TemplatePageParser::CheckPageElementValid(const rapidjson::Value &ttml) {
  if (!ttml.IsArray()) {
    THROW_ERROR_MSG("ttml is not array");
  }

  int size = ttml.Size();
  // ttml is empty.
  if (size == 0) {
    return;
  }

  // the first root element is page tag, but ttml size larger than 1
  if (ttml[0].IsObject() && ttml[0]["type"] == "node" &&
      ttml[0]["tagName"]["value"] == "page") {
    if (size > 1) {
      THROW_ERROR_MSG("page tag can only be the only child of ttml");
    }
    new_page_element_enabled_ = true;
  }
}

}  // namespace tasm
}  // namespace lynx
