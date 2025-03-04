// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_PARSER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/value/base_string.h"
#include "core/template_bundle/template_codec/generator/source_generator.h"
#include "core/template_bundle/template_codec/generator/ttml_holder.h"
#include "core/template_bundle/template_codec/moulds.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

typedef std::unordered_map<std::string, std::pair<std::string, std::string>>
    TemplateRenderMap;

class TemplateParser : public SourceGenerator {
  friend class ListParser;

 public:
  TemplateParser(const EncoderOptions& encoder_options);
  virtual ~TemplateParser() override;

  virtual void Parse() override{};

 protected:
  std::string AddAttributes(std::string& source, std::string key,
                            std::string value);

  // Renderer function generator
  TemplateRenderMap GenNecessaryRenders(Component* component);
  std::string GenTemplateDynamicRendererInFragment(Fragment* fragment);
  std::string GenTemplateRenderer(Template* tem);
  std::string GenComponentRenderer(Component* component);
  std::string GenDependentComponentInfoMapDefinition(Component* component);

  // Instruction generator
  std::string GenInstruction(const rapidjson::Value& instruction,
                             const TemplateMap* const templates = nullptr);
  std::string GenIf(const rapidjson::Value& content);
  std::string GenRepeat(const rapidjson::Value& repeat);
  std::string GenTemplate(const rapidjson::Value& tem, bool is_include = false);
  std::string GenTemplateNode(const rapidjson::Value& template_node,
                              const TemplateMap* const templates);
  std::string GenImport(const rapidjson::Value& import,
                        bool is_include = false);
  void GenFragment(const rapidjson::Value& import);
  std::string GenInclude(const rapidjson::Value& include);
  std::string GenComponentPlug(const rapidjson::Value& component);
  std::string GenComponentNode(const rapidjson::Value& component);
  std::string GenChildrenInComponentElement(const rapidjson::Value& children,
                                            bool in_dynamic_component);
  std::string GenDynamicComponentPlug(const rapidjson::Value& component);
  std::string GenDynamicComponentNode(
      const rapidjson::Value& component,
      const rapidjson::Value& slot_content = rapidjson::Value());

  std::string GenComponentPlugInTemplate(const rapidjson::Value& component);
  std::string GenComponentNodeInTemplate(const rapidjson::Value& component);

  std::string GenList(const rapidjson::Value& element);
  std::string GenComponentProps(const rapidjson::Value& element);
  std::string GenComponentEvent(const rapidjson::Value& element);

  // Element generator
  std::string GenElement(const rapidjson::Value& element);
  std::string GenRawElement(const rapidjson::Value& element);
  std::string GenElementSlot(const rapidjson::Value& slot);
  std::string GenElementPlug(const rapidjson::Value& element);
  std::string GenElementNode(const rapidjson::Value& element,
                             bool should_gen_children = true);
  std::string GenClasses(const rapidjson::Value& classes);
  std::string GenStyles(const rapidjson::Value& styles);
  std::string GenId(const rapidjson::Value& attrs);
  std::string GenAttributes(const rapidjson::Value& attrs);
  std::string GenDataSet(const rapidjson::Value& attrs);
  std::string GenEvents(const rapidjson::Value& element);
  std::string GenGestures(const rapidjson::Value& gestures);
  std::string GenChildrenInElement(const rapidjson::Value& children);
  std::string GenRawText(const rapidjson::Value& element);

  // plug generator
  std::string GenPlugNode(const rapidjson::Value& node,
                          const std::string& content,
                          const bool is_component_in_template);
  std::string GenPlugNode(const std::string& plug_name,
                          const std::string& content,
                          const bool is_component_in_template);
  std::string GetPlugName(const rapidjson::Value& node);

  void GenComponentMouldForCompilerNG(Component* component);

  rapidjson::Value SegregateAttrsFromPropsForComponent(
      const rapidjson::Value& props, std::stringstream& set_props_content,
      bool component_is = true, Component* component = nullptr);

 private:
  friend TemplateScope;
  friend FragmentScope;
  friend ComponentScope;
  friend PageScope;

  std::string GetCurrentPath();

  int text_count_{0};
  std::unordered_set<std::string> opening_files_;
  std::unordered_set<std::string> including_chain_;
  bool need_handle_fallback_{false};

  // `document` needs to have the same lifetime as the `TemplateParser` itself
  // because JSON values allocated during parsing have their allocation roots
  // here.
  rapidjson::Document document;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_PARSER_H_
