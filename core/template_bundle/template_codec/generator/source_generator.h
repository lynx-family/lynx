// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_SOURCE_GENERATOR_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_SOURCE_GENERATOR_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "core/template_bundle/template_codec/compile_options.h"
#include "core/template_bundle/template_codec/generator/base_struct.h"
#include "core/template_bundle/template_codec/generator/ttml_holder.h"
#include "core/template_bundle/template_codec/moulds.h"
namespace lynx {
namespace tasm {

class TemplateScope;
class FragmentScope;
class ComponentScope;
class PageScope;
class DynamicComponentScope;

class TemplateHelper;

using ComponentMouldMap =
    std::unordered_map<std::string, std::unique_ptr<ComponentMould>>;
using DynamicComponentMouldMap =
    std::unordered_map<std::string, std::unique_ptr<DynamicComponentMould>>;
using PageMouldMap =
    std::unordered_map<std::string, std::unique_ptr<PageMould>>;
using LepusGenRuleMap = std::unordered_map<std::string, std::string>;

// Page id: source
using TemplateParseResult = std::unordered_map<int32_t, std::string>;
using TemplateConfig = std::unordered_map<int32_t, std::string>;
using ComponentTemplateConfig = std::unordered_map<int32_t, std::string>;

#define THROW_ERROR_MSG(msg)                                               \
  std::string path =                                                       \
      current_component_ != nullptr ? current_component_->FullPath() : ""; \
  throw lepus::ParseException(msg, path.c_str());

#define THROW_ERROR_MSG_WITH_LOC(msg, loc)                                 \
  std::string path =                                                       \
      current_component_ != nullptr ? current_component_->FullPath() : ""; \
  throw lepus::ParseException(msg, path.c_str(), loc);

#define VALUE(obj) (obj).GetObject()["value"]
#define LOC(obj) (obj).GetObject()["loc"]

class SourceGenerator {
 public:
  SourceGenerator(const std::string &json, const rapidjson::Value &worklet,
                  const rapidjson::Value &script_map,
                  const rapidjson::Value &packed_script,
                  PackageInstanceType type, PackageInstanceDSL dsl,
                  const CompileOptions &compile_options,
                  const lepus::Value &trial_options,
                  const SourceGeneratorOptions &generator_options,
                  bool single_page,
                  PackageInstanceBundleModuleMode bundle_module_mode,
                  const std::string &lepus_js_code);
  // We can find id of ttss so that we can apply ttss to component
  void set_ttss_ids(const std::unordered_map<std::string, uint32_t> &ttss_ids) {
    ttss_ids_ = ttss_ids;
  }

  virtual ~SourceGenerator() = default;

  static std::unique_ptr<SourceGenerator> GenerateParser(
      const EncoderOptions &encoder_options);

  virtual void Parse() = 0;

  void ParseCard();
  void ParseDynamicComponent();
  std::string lepusRuntime();

  std::string DumpJSON(const rapidjson::Value &element);

  std::string GetLepusCode() const;

  inline const TemplateParseResult &result() const { return result_; }
  std::string DynamicComponentEntryName();

  inline const TemplateConfig &page_config() { return page_config_; }

  inline const ComponentTemplateConfig &component_config() {
    return component_config_;
  }

  inline const ComponentMouldMap &component_moulds() {
    return component_moulds_;
  }

  inline const PageMouldMap &page_moulds() { return page_moulds_; }

  inline const DynamicComponentMouldMap &dynamic_component_moulds() {
    return dynamic_component_moulds_;
  }

  inline const std::unordered_map<std::string, std::string> &
  dynamic_component_declarations() {
    return dynamic_component_declarations_;
  }

  inline const AppMould &app_mould() { return app_mould_; }

  void SetClosureFix(bool v) { closure_fix_ = v; }

  // TemplateAPI
  std::string GenTemplateFunctions(Component *component);
  std::string GenScriptMapDefines(Component *component,
                                  const std::string &component_str);
  std::string GenPagePreprocessFunction(Page *page);
  std::string GenComponentPreprocessFunction(std::string path,
                                             Component *component);
  virtual std::string GenPreBuildInFunctions();

  // ElementWorklet
  std::string GenElementWorkletRequire(Component *component);

 protected:
  // Format utils
  std::string FormatValue(std::string value);
  std::string FormatCSSPropertyID(std::string name);
  std::string FormatStringWithRule(
      const std::string &format,
      const std::unordered_map<std::string, std::string> &rule);

  int32_t GetCSSForComponent(Component *component);

  bool IsVariableString(const std::string &str);
  bool IsNonVariableString(const std::string &str);
  bool IsSlot(const rapidjson::Value &element);
  bool IsComponentAttrs(const std::string &str);
  static bool JsonArrayContains(const rapidjson::Value &haystack,
                                const std::string &needle);

  // Generator helper
  std::string GenStatement(const std::string &statement);
  std::string GenStatement(const rapidjson::Value &statement);
  void GenPageMould(Page *page);
  void GenComponentMould(Component *component);

  bool IsAvailableString(const rapidjson::Value &str);

  std::string GetComponentPath(const std::string &tag);
  bool IsComponent(const std::string &tag);
  bool IsComponentIs(const rapid_value &element);
  bool IsDynamicComponent(const std::string &tag);

  std::string FormatThemedStatement(const std::string &str,
                                    std::string::size_type pos);

  std::string FormatContextDataStatement(const std::string &str,
                                         std::string::size_type pos);

  void AddDynamicComponentDeclaration(const std::string &key,
                                      const std::string &value);

  friend class TemplateScope;
  friend class FragmentScope;
  friend class ComponentScope;
  friend class PageScope;
  friend class DynamicComponentScope;

 protected:
  virtual std::string GenPageSource(Page *page) { return ""; };
  virtual std::string GenDynamicComponentSource(
      DynamicComponent *dynamic_component) {
    return "";
  };

  const std::string json_;
  const std::string lepus_js_code_;
  Page *current_page_;
  DynamicComponent *current_dynamic_component_;
  Component *current_component_;
  Fragment *current_fragment_;
  Template *current_template_;
  TemplateParseResult result_;
  AppMould app_mould_;
  std::unique_ptr<PackageInstance> package_instance_;
  std::unique_ptr<TemplateHelper> template_helper_;
  PackageInstanceDSL dsl_;
  const CompileOptions compile_options_;
  SourceGeneratorOptions generator_options_;
  int main_page_id_{0};
  int main_component_id_{0};
  bool is_in_template_render_{false};
  bool is_air_strict_{false};

 private:
  bool is_single_page_{false};
  bool enable_css_property_id_optimization_{true};
  bool closure_fix_{false};
  TemplateConfig page_config_;
  ComponentTemplateConfig component_config_;
  PageMouldMap page_moulds_;
  ComponentMouldMap component_moulds_;
  DynamicComponentMouldMap dynamic_component_moulds_;
  std::unordered_map<std::string, std::string> dynamic_component_declarations_;
  // Key: path & value:id
  std::unordered_map<std::string, uint32_t> ttss_ids_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_SOURCE_GENERATOR_H_
