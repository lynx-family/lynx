// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MOULDS_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MOULDS_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/vector.h"
#include "core/renderer/dom/component_config.h"
#include "core/renderer/template_themed.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {
class ComponentMould {
 public:
  ComponentMould() {}
  virtual ~ComponentMould() {}

  inline void set_id(int32_t id) { id_ = id; }
  inline void set_properties(const lepus::Value& properties) {
    properties_ = properties;
  }
  inline lepus::Value& get_properties() { return properties_; }
  inline void set_external_classes(const lepus::Value& external_classes) {
    external_classes_ = external_classes;
  }
  inline lepus::Value& get_external_classes() { return external_classes_; }
  inline void set_name(const std::string name) { name_ = name; }
  inline void set_path(const std::string path) { path_ = path; }
  inline void set_data(const lepus::Value& data) { data_ = data; }
  inline lepus::Value& get_data() { return data_; }
  inline void set_css_id(int32_t css_id) { css_id_ = css_id; }
  inline void AddDependentComponentId(int32_t id) {
    component_ids_.push_back(id);
  }

  inline int32_t id() const { return id_; }
  inline const std::string& name() const { return name_; }
  inline const std::string& path() const { return path_; }
  inline const lepus::Value& properties() const { return properties_; }
  inline const lepus::Value& external_classes() const {
    return external_classes_;
  }
  /**
   * The data is decoded from template.js, so it must be a LepusValue instead of
   * a JsValue. Deep copy to avoid the problem that the data is modified by the
   * user.
   */
  inline lepus::Value data() const { return lepus::Value::Clone(data_); }
  inline int32_t css_id() const { return css_id_; }
  inline const auto& component_ids() const { return component_ids_; }
  inline const std::unordered_map<std::string, int32_t>& name_id_map() const {
    return name_id_map_;
  }
  inline void SetComponentConfig(
      std::shared_ptr<ComponentConfig>& component_config) {
    component_config_ = component_config;
  }
  std::shared_ptr<ComponentConfig>& GetComponentConfig() {
    return component_config_;
  }

 protected:
  // For serialize
  friend class TemplateBinaryWriter;
  friend class TemplateBinaryReader;
  friend class TemplateBinaryReaderSSR;

  lepus::Value properties_;
  lepus::Value external_classes_;
  lepus::Value data_;

  int32_t id_;
  std::string name_;
  std::string path_;
  int32_t css_id_;
  base::InlineVector<int32_t, 16> component_ids_;
  std::unordered_map<std::string, int32_t> name_id_map_;
  std::shared_ptr<ComponentConfig> component_config_;
};

class PageMould : public ComponentMould {};

class DynamicComponentMould : public ComponentMould {
 public:
  DynamicComponentMould() : ComponentMould() {}
  virtual ~DynamicComponentMould() {}
};

struct AppMould {
  uint32_t main_page_id = 0;
  std::unordered_map<base::String, uint32_t> page_list;
  std::shared_ptr<ThemedTrans> themedTrans_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MOULDS_H_
