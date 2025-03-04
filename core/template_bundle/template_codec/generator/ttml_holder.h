// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TTML_HOLDER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TTML_HOLDER_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/template_themed.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/template_bundle/template_codec/ttml_constant.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

enum class PackageInstanceType { CARD, DYNAMIC_COMPONENT };

class PackageInstance;
class App;
class Page;
class Component;
class Fragment;
class Template;
class DynamicComponent;

enum ListComponentType { Header = 0, Footer, Default };

extern const char *kTTMLResourceSuffix;
extern const char *kTTSSResourceSuffix;
extern const char *defaultSlotName;
extern const char *kFallbackName;
// TODO use a better way
extern int sPageIdGenerator;
extern int sComponentIdGenerator;
extern int sComponentInstanceIdGenerator;
extern int sFragmentIdGenerator;
extern int sTemplateIdGenerator;
extern int sElementIdGenerator;
extern int sDynamicIdGenerator;

typedef std::map<std::string, std::unique_ptr<Component>> ComponentMap;
typedef std::map<std::string, std::shared_ptr<Fragment>> FragmentMap;
typedef std::map<std::string, std::shared_ptr<Template>> TemplateMap;
typedef std::vector<std::pair<std::string, bool>> SlotConditionChainVec;

// Structure definition

// A class holds the ttml which is a group of instruction that use to
// generate view hierachy.
class TTMLHolder {
 public:
  TTMLHolder(rapidjson::Value *ttml) : ttml_(ttml) {}

  virtual ~TTMLHolder() {}

  inline const rapidjson::Value &ttml() const { return *ttml_; }

 private:
  rapidjson::Value *ttml_;
};

// A tool to record how many variable are used in a template. we
// can optimize source generating by decreasing useless variable
// definition and optimize binary size by remove useless data.
class VariableUsageRecorder {
 public:
  virtual ~VariableUsageRecorder() {}

  void MarkVariableInUse(const base::String &key) {
    if (key.str() == "$kTemplateAssembler" || key.str() == "$component" ||
        key.str() == "__globalProps" || key.str() == "SystemInfo") {
      return;
    }
    variables_in_use_.insert(key);
  }

  bool IsVariableInUse(const base::String &key) {
    return variables_in_use_.find(key) != variables_in_use_.end();
  }

  inline const std::set<base::String> &variables_in_use() {
    return variables_in_use_;
  }

 private:
  std::set<base::String> variables_in_use_;
};

// A minimum functional unit.
// The path is the name of template and path is repeatable.
class Template : public TTMLHolder, public VariableUsageRecorder {
 public:
  Template(const std::string &path, rapidjson::Value *ttml)
      : Template(path, ttml, sTemplateIdGenerator++) {}

  virtual ~Template() {}

  void AddTemplate(std::shared_ptr<Template> &tem) {
    templates_.insert({tem->path(), tem});
  }

  void AddLocalTemplate(std::shared_ptr<Template> &tem) {
    local_templates_.insert({tem->path(), tem});
  }

  void AddIncludeTemplate(std::shared_ptr<Template> &tem) {
    include_templates_.insert({tem->path(), tem});
  }

  bool HasTemplate(const std::string &name) {
    return templates_.find(name) != templates_.end();
  }

  Template *GetTemplate(const std::string &name) {
    return templates_[name].get();
  }

  bool HasSlotInHistory(const std::string &name,
                        const SlotConditionChainVec &slot_chain) {
    if (slot_history_.count(name) == 0) {
      return false;
    }
    if (slot_chain.empty()) {
      return true;
    }

    auto range = slot_history_.equal_range(name);
    for (auto it = range.first; it != range.second; ++it) {
      const SlotConditionChainVec &chain = it->second;
      if (chain.empty()) {
        return true;
      }

      for (auto i = chain.begin(), j = slot_chain.begin();
           i != chain.end() && j != slot_chain.end(); ++i, ++j) {
        if (i->first != j->first) {
          return true;
        } else if (i->second != j->second) {
          // see next SlotConditionChainVec in range
          break;
        } else if (i + 1 == chain.end() || j + 1 == slot_chain.end()) {
          return true;
        }
      }
    }
    return false;
  }

  inline void AddSlotToHistory(const std::string &name,
                               const SlotConditionChainVec &slot_chain) {
    slot_history_.insert(std::make_pair(name, slot_chain));
  }

  // TODO(yxping): there will be duplicate id for component / template / page /
  // fragment
  inline int id() const { return id_; }

  // unique id
  // TODO(yxping): compose id and uid as id.
  inline std::string uid() {
    std::stringstream uid;
    if (IsPage()) {
      uid << "p";
    } else if (IsComponent()) {
      uid << "c";
    } else if (IsFragment()) {
      uid << "f";
    } else if (IsTemplate()) {
      uid << "t";
    }
    uid << id();
    return uid.str();
  }

  inline const std::string &path() const { return path_; }

  const TemplateMap &templates() { return templates_; }

  const TemplateMap &local_templates() { return local_templates_; }

  const TemplateMap &include_templates() { return include_templates_; }

  inline bool has_dynamic_template() { return has_dynamic_template_; }

  inline void set_has_dynamic_template(bool has) {
    has_dynamic_template_ = has;
  }

  virtual bool IsTemplate() { return true; }
  virtual bool IsFragment() { return false; }
  virtual bool IsComponent() { return false; }
  virtual bool IsDynamicComponent() { return false; }
  virtual bool IsPage() { return false; }

  inline const std::vector<std::string> &codes() { return codes_; }

  inline void set_codes(const std::vector<std::string> &codes) {
    codes_ = codes;
  }

 protected:
  Template(const std::string &path, rapidjson::Value *ttml, int id)
      : TTMLHolder(ttml), id_(id), path_(path), has_dynamic_template_(false) {}

 private:
  int id_;
  std::string path_;
  // Key: name / value: ttml
  TemplateMap templates_;
  // templates defined in current file, not imported
  TemplateMap local_templates_;
  // only used for included template_node, not registered into
  // dynamictemplaterender
  TemplateMap include_templates_;
  std::multimap<std::string, SlotConditionChainVec> slot_history_;
  bool has_dynamic_template_;
  std::vector<std::string> codes_;
};

// A fragment represet for a ttml file, can be included by other fragment
// or component. Fragment doesn't have an unique id as it doesn't need to
// generate render function for fragment.
class Fragment : public Template {
 public:
  Fragment(const std::string &path, rapidjson::Value *ttml)
      : Template(path, ttml, sFragmentIdGenerator++) {}

  inline void AddDependentFragment(std::shared_ptr<Fragment> &fragment) {
    dependent_fragments_[fragment->path()] = fragment;
  }

  inline const FragmentMap &dependent_fragments() {
    return dependent_fragments_;
  }

  bool IsFragment() override { return true; }

 protected:
  Fragment(const std::string &path, rapidjson::Value *ttml, int id)
      : Template(path, ttml, id) {}

 private:
  FragmentMap dependent_fragments_;
};

// A component is a fragment with data and props.
class Component : public Fragment {
 public:
  Component(const std::string &path, rapidjson::Value *component,
            rapidjson::Value *ttml, rapidjson::Value *templateApi,
            const std::string &config)
      : Component(path, component, ttml, templateApi, sComponentIdGenerator++,
                  config) {}

  virtual ~Component() {}

  bool IsDependentComponent(const std::string &name) const {
    return dependent_components_.find(name) != dependent_components_.end();
  }

  bool IsDependentDynamicComponent(const std::string &name) const {
    return dependent_dynamic_components_.find(name) !=
           dependent_dynamic_components_.end();
  }

  const std::string &GetDependentComponentPath(const std::string &name) const {
    return dependent_components_.find(name)->second;
  }

  inline const std::string &name() { return name_; }

  inline void set_name(std::string name) { name_ = name; }

  inline const std::string &config() { return config_; }

  inline const rapidjson::Value &data() {
    return component_->GetObject()["data"];
  }
  inline const rapidjson::Value &props() {
    return component_->GetObject()["properties"];
  }
  inline const rapidjson::Value &external_classes() {
    return component_->GetObject()["externalClasses"];
  }
  inline std::string FullPath() const {
    std::string path = component_->GetObject()["path"].GetString();
    path += kTTMLResourceSuffix;
    return path;
  }
  inline const std::map<std::string, std::string> &dependent_components() {
    return dependent_components_;
  }
  inline const std::map<std::string, std::string> &
  dependent_dynamic_components() {
    return dependent_dynamic_components_;
  }

  bool IsComponent() override { return true; }

  rapidjson::Value *template_api() { return template_api_; }

 protected:
  Component(const std::string &path, rapidjson::Value *component,
            rapidjson::Value *ttml, rapidjson::Value *templateApi, int id,
            const std::string &config)
      : Fragment(path, ttml, id),
        component_(component),
        template_api_(templateApi),
        config_(config) {
    DCHECK(component->IsObject());

    // Check dependent components
    auto &dc = component->GetObject()["components"];
    if (dc.IsObject()) {
      for (auto it = dc.GetObject().begin(); it != dc.GetObject().end(); it++) {
        DCHECK(it->name.IsString() && it->value.IsString());
        dependent_components_[it->name.GetString()] = it->value.GetString();
      }
    }
    auto &ddc = component->GetObject()["dynamicComponents"];
    if (ddc.IsObject()) {
      for (auto it = ddc.GetObject().begin(); it != ddc.GetObject().end();
           it++) {
        DCHECK(it->name.IsString() && it->value.IsString());
        dependent_dynamic_components_[it->name.GetString()] =
            it->value.GetString();
      }
    }
  }

 private:
  // Component descriptor contains path & ttml & data & properties
  rapidjson::Value *component_;
  // Key: tag name / value: path
  std::map<std::string, std::string> dependent_components_;
  std::map<std::string, std::string> dependent_dynamic_components_;
  std::string name_;
  rapidjson::Value *template_api_;
  std::string config_;
};

//
class Page : public Component {
 public:
  Page(const std::string &path, rapidjson::Value *page,
       rapidjson::Value *templateApi, rapidjson::Value *ttml,
       bool default_entry, const std::string &config)
      : Component(path, page, ttml, templateApi, sPageIdGenerator++, config),
        page_(page),
        is_default_entry_(default_entry) {
    DCHECK(page_->IsObject());
  }

  ~Page() {}

  inline bool is_default_entry() { return is_default_entry_; }

  bool IsPage() override { return true; }

 private:
  rapidjson::Value *page_;
  bool is_default_entry_;
};

// TODO: templateApi?
class DynamicComponent : public Component {
 public:
  DynamicComponent(const std::string &path, rapidjson::Value *dynamic_component,
                   rapidjson::Value *templateApi, rapidjson::Value *ttml,
                   const std::string &config)
      : Component(path, dynamic_component, ttml, templateApi, config),
        dynamic_component_(dynamic_component) {
    DCHECK(dynamic_component_->IsObject());
  }

  ~DynamicComponent() {}

  virtual bool IsDynamicComponent() { return true; }

  rapidjson::Value &DynamicComponentJson() { return *dynamic_component_; }

 private:
  rapidjson::Value *dynamic_component_;
};

class PackageInstance {
 public:
  PackageInstance(const std::string &json, const lepus::Value &trial_options,
                  const rapidjson::Value &worklet,
                  const rapidjson::Value &script_map,
                  const rapidjson::Value &packed_script, PackageInstanceDSL dsl,
                  PackageInstanceBundleModuleMode bundle_module_mode)
      : dsl_(dsl),
        trial_options_(trial_options),
        bundle_module_mode_(bundle_module_mode) {
    // Reset id generator
    sPageIdGenerator = 0;
    sComponentIdGenerator = 0;
    sFragmentIdGenerator = 0;
    sTemplateIdGenerator = 0;
    sElementIdGenerator = 0;
    sDynamicIdGenerator = 0;

    json_.Parse(json.c_str());
    if (json_.HasParseError()) {
      LOGE("JSON parse Error");
    }
    DCHECK(json_.IsObject());

    if (worklet.IsObject()) {
      worklet_map_.CopyFrom(worklet, worklet_map_.GetAllocator());
    }
    if (script_map.IsObject()) {
      script_map_.CopyFrom(script_map, script_map_.GetAllocator());
    }

    if (packed_script.IsString()) {
      packed_script_ = packed_script.GetString();
    } else {
      packed_script_ = "";
    }
  }

  virtual ~PackageInstance() {}

  virtual std::string EntryName() { return ""; }

  const rapidjson::Value &GetTTMLHolder(const std::string &path) const {
    return json_[path];
  }

  const rapidjson::Value &worklet_map() const { return worklet_map_; }

  const rapidjson::Value &script_map() const { return script_map_; }

  const std::string &packed_script() const { return packed_script_; }

  const rapidjson::Value &GetWorklet(const std::string &path) const {
    return worklet_map_[path];
  }

  Component *GetComponent(const std::string &path) {
    return components_[path].get();
  }

  bool HasFragment(const std::string &path) {
    return fragments_.find(path) != fragments_.end();
  }

  std::shared_ptr<Fragment> GetFragment(const std::string &path) {
    if (HasFragment(path)) {
      return fragments_[path];
    }
    return nullptr;
  }

  void RegisterFragment(std::shared_ptr<Fragment> fragment) {
    fragments_[fragment->path()] = std::move(fragment);
  }

  const ComponentMap &components() { return components_; }
  const FragmentMap &fragments() { return fragments_; }

  virtual PackageInstanceType InstanceType() = 0;

  PackageInstanceDSL dsl() { return dsl_; }

 protected:
  PackageInstanceDSL dsl_;
  void PrepareComponents() {
    auto &components = json_["components"];
    if (!components.IsObject()) {
      return;
    }

    DCHECK(components.IsObject());
    std::set<std::string> keys = std::set<std::string>();
    for (auto it = components.GetObject().begin();
         it != components.GetObject().end(); ++it) {
      std::string component_path = it->name.GetString();
      keys.insert(component_path);
    }
    for (auto it = keys.begin(); it != keys.end(); ++it) {
      std::string component_path = *it;
      auto &component =
          components.GetObject().FindMember(component_path)->value;
      DCHECK(component.IsObject());
      std::stringstream ttml_path;
      ttml_path << component_path << kTTMLResourceSuffix;
      auto &ttml = json_[ttml_path.str()];
      DCHECK(ttml.IsArray());

      const std::string kTemplateApiStr = "templateApi";
      auto &templateApi = component[kTemplateApiStr];
      constexpr const char *const kConfig = "config";
      std::string config_str = "{}";
      if (component.HasMember(kConfig)) {
        auto &component_config = component[kConfig];
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        component_config.Accept(writer);
        config_str = buffer.GetString();
      }
      components_[component_path] = std::make_unique<Component>(
          component_path, &component, &ttml, &templateApi, config_str);
    }
  }

  // key: fragment path / value: fragment descriptor
  FragmentMap fragments_;
  // key: component path / value: component descriptor
  ComponentMap components_;

  // hold trial_options_ here, and encode this into the config section.
  lepus::Value trial_options_;
  rapidjson::Document json_;
  rapidjson::Document worklet_map_;
  rapidjson::Document script_map_;
  std::string packed_script_;
  PackageInstanceBundleModuleMode bundle_module_mode_;
};

class App : public PackageInstance {
 public:
  App(const std::string &json, const lepus::Value &trial_options,
      const rapidjson::Value &worklet, const rapidjson::Value &script_map,
      const rapidjson::Value &packed_script, PackageInstanceDSL dsl,
      PackageInstanceBundleModuleMode bundle_module_mode)
      : PackageInstance(json, trial_options, worklet, script_map, packed_script,
                        dsl, bundle_module_mode) {
    PreparePages();
    PrepareComponents();
  }
  virtual ~App() {}

  const std::vector<std::unique_ptr<Page>> &pages() { return pages_; }

  virtual PackageInstanceType InstanceType() {
    return PackageInstanceType::CARD;
  }

  std::shared_ptr<ThemedTrans> getThemedTrans() { return trans_; }

 private:
  void PreparePages() {
    rapidjson::Document document;
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

    auto &pages = json_["pages"];
    if (!pages.IsArray()) {
      return;
    }

    DCHECK(pages.IsArray());
    for (rapidjson::SizeType i = 0; i < pages.GetArray().Size(); ++i) {
      auto &page = pages[i];
      DCHECK(page.IsObject());
      auto &path = page["path"];
      DCHECK(path.IsString());

      // get the page config (json string)
      const char *config = "config";
      std::string config_str = "{}";
      if (page.HasMember(config)) {
        rapid_value &card_config = page[config];

        // add dsl
        rapid_value config_temp(card_config, allocator);
        rapidjson::Value dsl((int)dsl_);
        config_temp.AddMember(TEMPLATE_BUNDLE_APP_DSL, dsl, allocator);

        rapidjson::Value bundle_module_mode((int)bundle_module_mode_);
        config_temp.AddMember(TEMPLATE_BUNDLE_MODULE_MODE, bundle_module_mode,
                              allocator);

        std::string cli_version = "unknown";
        if (json_.HasMember(TEMPLATE_SUPPORTED_VERSIONS) &&
            json_[TEMPLATE_SUPPORTED_VERSIONS].HasMember(
                TEMPLATE_CLI_VERSION) &&
            json_[TEMPLATE_SUPPORTED_VERSIONS][TEMPLATE_CLI_VERSION]
                .IsString()) {
          cli_version = json_[TEMPLATE_SUPPORTED_VERSIONS][TEMPLATE_CLI_VERSION]
                            .GetString();
        }

        config_temp.AddMember(TEMPLATE_CLI_VERSION, cli_version, allocator);

        rapidjson::Document trial;
        if (trial_options_.IsTable()) {
          if (!trial.Parse(lepusValueToJSONString(trial_options_, true))
                   .HasParseError()) {
            constexpr const static char *kTrialOptions = "trialOptions";
            config_temp.AddMember(rapidjson::StringRef(kTrialOptions), trial,
                                  allocator);
          }
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        config_temp.Accept(writer);
        config_str = buffer.GetString();
      }
      const std::string kTemplateApiStr = "templateApi";

      auto &templateApi = page[kTemplateApiStr];

      std::stringstream ttml_path;
      ttml_path << path.GetString() << kTTMLResourceSuffix;
      auto &ttml = json_[ttml_path.str()];
      DCHECK(ttml.IsArray());

      auto newPage = std::make_unique<Page>(
          path.GetString(), &page, &templateApi, &ttml, i == 0, config_str);

      const int pageId = newPage->id();
      pages_.push_back(std::move(newPage));

      if (page.HasMember(config)) {
        rapid_value &card_config = page[config];
        const char *trans = "usingTranslations";
        // config for themed translations
        if (card_config.IsObject() && card_config.HasMember(trans)) {
          InternalPrepareThemedTranslations(card_config[trans], pageId);
        }
      }
    }
  }

  void InternalPrepareThemedTranslations(rapid_value &trans, int pageIndex) {
    if (!trans.IsObject()) {
      return;
    }
    if (trans_ == nullptr) {
      trans_ = std::make_shared<ThemedTrans>();
    }
    std::shared_ptr<ThemedTrans::TransMap> pageTrans =
        std::make_shared<ThemedTrans::TransMap>();

    trans_->pageTransMap_.insert({pageIndex, pageTrans});
    std::set<std::string> nameSet;
    auto &priority_ = pageTrans->priority_;

    auto transObj = trans.GetObject();
    for (auto it = transObj.begin(); it != transObj.end(); ++it) {
      if (!it->name.IsString()) continue;
      const std::string name = it->name.GetString();
      auto &value = it->value;

      if (name.empty()) continue;
      if (strncmp(name.c_str(), "__", 2) == 0) {
        if (name == "__default") {
          InternalParseTransRes(pageTrans->default_, value);
        } else if (name == "__finalFallback") {
          InternalParseTransRes(pageTrans->fallback_, value);
        } else if (name == "__priority" && value.IsArray()) {
          auto priorityArr = value.GetArray();
          for (rapidjson::SizeType i = 0; i < priorityArr.Size(); ++i) {
            if (!priorityArr[i].IsString()) continue;
            const std::string key = priorityArr[i].GetString();
            if (!key.empty() && std::find(priority_.begin(), priority_.end(),
                                          key) == priority_.end()) {
              priority_.push_back(key);
            }
          }
        }
        continue;
      }

      nameSet.insert(name);
      std::shared_ptr<ThemedRes> pathItemMap = std::make_shared<ThemedRes>();
      if (!InternalParseTransRes(*pathItemMap, value)) {
        continue;
      }

      // parse related config files
      for (auto &item : *pathItemMap) {
        DCHECK(trans_ != nullptr);
        if (trans_->fileMap_.find(item.second) != trans_->fileMap_.end()) {
          continue;
        }
        rapid_value &trans = json_[item.second];
        if (!trans.IsObject()) {
          continue;
        }
        std::shared_ptr<ThemedRes> resMap = std::make_shared<ThemedRes>();
        if (!InternalParseTransRes(*resMap, trans)) {
          continue;
        }
        trans_->fileMap_.insert({item.second, std::move(resMap)});
      }

      pageTrans->pathMap_.insert({std::move(name), std::move(pathItemMap)});
    }

    for (auto &name : nameSet) {
      if (std::find(priority_.begin(), priority_.end(), name) ==
          priority_.end()) {
        priority_.push_back(name);
      }
    }
  }

  inline bool InternalParseTransRes(ThemedRes &dst, rapid_value &src) {
    dst.clear();
    if (!src.IsObject()) {
      return false;
    }
    auto srcObj = src.GetObject();
    for (auto it = srcObj.begin(); it != srcObj.end(); ++it) {
      if (it->name.IsString() && it->value.IsString()) {
        dst.insert({it->name.GetString(), it->value.GetString()});
      }
    }
    return !dst.empty();
  }

  std::vector<std::unique_ptr<Page>> pages_;
  std::shared_ptr<ThemedTrans> trans_;
};

class HotSwapApp : public PackageInstance {
 public:
  HotSwapApp(const std::string &json, const lepus::Value &trial_options,
             const rapidjson::Value &worklet,
             const rapidjson::Value &script_map,
             const rapidjson::Value &packed_script, PackageInstanceDSL dsl,
             PackageInstanceBundleModuleMode bundle_module_mode)
      : PackageInstance(json, trial_options, worklet, script_map, packed_script,
                        dsl, bundle_module_mode) {
    PrepareDynamicComponents();
    PrepareComponents();
  }
  virtual ~HotSwapApp() {}
  virtual PackageInstanceType InstanceType() {
    return PackageInstanceType::DYNAMIC_COMPONENT;
  }
  const std::vector<std::unique_ptr<DynamicComponent>> &DynamicComponents() {
    return dynamic_components_;
  }

 private:
  void PrepareDynamicComponents() {
    auto &dynamic_components = json_["dynamic_components"];
    DCHECK(dynamic_components.IsArray());
    for (rapidjson::SizeType i = 0; i < dynamic_components.GetArray().Size();
         ++i) {
      auto &dynamic_component = dynamic_components[i];
      DCHECK(dynamic_component.IsObject());
      auto &path = dynamic_component["path"];
      DCHECK(path.IsString());

      // get the page config (json string)
      const char *config = "config";
      std::string config_str = "{}";
      if (dynamic_component.HasMember(config)) {
        rapid_value &card_config = dynamic_component[config];
        rapidjson::StringBuffer bufferTTss;
        rapidjson::Writer<rapidjson::StringBuffer> writer(bufferTTss);
        card_config.Accept(writer);
        config_str = bufferTTss.GetString();
      }

      std::stringstream ttml_path;
      ttml_path << path.GetString() << kTTMLResourceSuffix;
      auto &ttml = json_[ttml_path.str()];
      DCHECK(ttml.IsArray());

      auto &templateApi = dynamic_component["templateApi"];

      dynamic_components_.push_back(std::make_unique<DynamicComponent>(
          path.GetString(), &dynamic_component, &templateApi, &ttml,
          config_str));
    }
  }

  std::vector<std::unique_ptr<DynamicComponent>> dynamic_components_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TTML_HOLDER_H_
