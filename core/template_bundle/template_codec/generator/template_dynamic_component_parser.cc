// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/generator/template_dynamic_component_parser.h"

namespace lynx {
namespace tasm {

TemplateDynamicComponentParser::TemplateDynamicComponentParser(
    const EncoderOptions &encoder_options)
    : TemplateParser(encoder_options) {}

TemplateDynamicComponentParser::~TemplateDynamicComponentParser() = default;

void TemplateDynamicComponentParser::Parse() { ParseDynamicComponent(); }

std::string TemplateDynamicComponentParser::GenDynamicComponentSource(
    DynamicComponent *dynamic_component) {
  if (!Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                               FEATURE_DYNAMIC_COMPONENT_VERSION)) {
    LOGW(
        "[Warning]: use dynamic component when engine version "
        "< "
        << FEATURE_DYNAMIC_COMPONENT_VERSION);
    return "";
  }
  if (lepus_js_code_.empty()) {
    return GenDynamicComponentSourceForTT(dynamic_component);
  } else {
    return GenDynamicComponentSourceForReactCompilerNG(dynamic_component);
  }
}

std::string TemplateDynamicComponentParser::GenDynamicComponentSourceForTT(
    DynamicComponent *dynamic_component) {
  auto &json_component = dynamic_component->DynamicComponentJson();

  DCHECK(json_component.IsObject());
  DCHECK(json_component["path"].IsString());

  std::stringstream id;
  id << dynamic_component->id();

  // Template API
  std::string preprocessFunction = GenComponentPreprocessFunction(
      dynamic_component->path(), dynamic_component);

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

  auto necessary_renders = GenNecessaryRenders(dynamic_component);
  auto &template_render = necessary_renders["template"];
  auto &component_render = necessary_renders["component"];

  LepusGenRuleMap rule = {
      {"topLevelDeclaration",
       "let __globalProps = null; let SystemInfo = null;"},
      {"lepusRuntime", this->lepusRuntime()},
      {"ID", id.str()},
      {"declareTemplates", template_render.first},
      {"declareComponents", component_render.first},
      {"defineTemplateFunction", template_render.second},
      {"defineComponentFunction", component_render.second},
      {"preprocessFunction", preprocessFunction},
      {"packedScript", packed_script},
  };

  // clang-format off
  static std::string source =
      "{topLevelDeclaration}"
      "{lepusRuntime}"
      "{preprocessFunction}"
      "{packedScript}"
      "{declareTemplates}"
      "{declareComponents}"
      "{defineTemplateFunction}"
      "{defineComponentFunction}"
      "let $renderEntranceDynamicComponent = null;"
      "$renderEntranceDynamicComponent = function ($component, $data, $props, $recursive) {"
      "$renderComponent{ID}($component, $data, $props, $recursive);"
      "}";
  // clang-format on

  auto dynamic_component_source = FormatStringWithRule(source, rule);

  result_[0] = dynamic_component_source;
  return dynamic_component_source;
}

std::string
TemplateDynamicComponentParser::GenDynamicComponentSourceForReactCompilerNG(
    DynamicComponent *dynamic_component) {
  GenComponentMouldForCompilerNG(dynamic_component);
  result_[0] = lepus_js_code_;
  return lepus_js_code_;
}

}  // namespace tasm
}  // namespace lynx
