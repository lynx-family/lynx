// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPONENT_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_COMPONENT_ELEMENT_H_

#include <memory>
#include <string>

#include "base/include/base_export.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/vdom/radon/base_component.h"

namespace lynx {
namespace tasm {

class ComponentElement : public WrapperElement, public BaseComponent {
 public:
  ComponentElement(ElementManager* manager, const base::String& component_id,
                   int32_t css_id, const base::String& entry_name,
                   const base::String& name, const base::String& path);
  ComponentElement(ElementManager* manager, const base::String& component_id,
                   int32_t css_id, const base::String& entry_name,
                   const base::String& name, const base::String& path,
                   const base::String& tag_name);

  virtual ~ComponentElement();

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new ComponentElement(*this, clone_resolved_props));
  }

  bool is_component() const override { return true; }

  bool is_wrapper() const override { return is_wrapper_component_; }

  void set_component_id(const base::String& component_id);

  virtual void MarkHasLayoutOnlyPropsIfNecessary(
      const base::String& attribute_key) override;

  const base::String& component_id() const { return component_id_; }

  const base::String& component_name() const { return name_; }

  void set_component_name(const base::String& name) { name_ = name; }

  const base::String& component_path() const { return path_; }

  void set_component_path(const base::String& path) { path_ = path; }

  const base::String& component_entry() const { return entry_name_; }

  void set_component_entry(const base::String& entry) { entry_name_ = entry; }

  void MarkAsWrapperComponent() {
    is_layout_only_ = true;
    is_wrapper_component_ = true;
  }

  virtual bool CanBeLayoutOnly() const override;

  BASE_EXPORT_FOR_DEVTOOL CSSFragment* GetCSSFragment();

  const lepus::Value& GetData() override;

  const lepus::Value& GetProperties() override;

  const std::string& GetEntryName() const override;

  std::string ComponentStrId() override;

  int32_t GetComponentCSSID() const { return component_css_id_; }

  void SetBuiltinAttribute(ElementBuiltInAttributeEnum key,
                           const lepus::Value& value) override;

  virtual void SetComponentCSSID(int32_t id);

  double GetFontSize() override;

  const InheritedProperty& GetInheritedProperty() override;

  ParallelFlushReturn PrepareForCreateOrUpdate() override;

  void UpdateInheritedProperty() override;

  void MarkAsListItem() override;

  void SetAttribute(const base::String& key, const lepus::Value& value,
                    bool need_update_data_model = true) override;

  // The element object created using the clone interface of ComponentElement is
  // not attached to the element manager. Use this function to attach it to the
  // element manager.
  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

 protected:
  ComponentElement(const ComponentElement& element, bool clone_resolved_props);

  void OnNodeAdded(FiberElement* child) override;
  void OnNodeRemoved(FiberElement* child) override;

  void UpdateRootCSSVariables(AttributeHolder* holder,
                              const std::shared_ptr<CSSParseToken>& root_token);
  void PrepareForRootCSSVariables();
  void PrepareForFontFaceIfNeeded();

  // In Nodiff mode, the data and prop variables are initialized and not
  // modified afterwards. The LepusRuntime intercepts the functions setStore,
  // getStore, getData, and getProperties.
  lepus::Value data_{};
  lepus::Value prop_{};

  base::String component_id_{};
  int32_t component_css_id_{-1};
  base::String entry_name_{};
  base::String name_{};
  base::String path_{};

  bool is_wrapper_component_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPONENT_ELEMENT_H_
