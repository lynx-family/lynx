// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/component_element.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

ComponentElement::ComponentElement(ElementManager* manager,
                                   const base::String& component_id,
                                   int32_t component_css_id,
                                   const base::String& entry_name,
                                   const base::String& name,
                                   const base::String& path)
    : ComponentElement(manager, component_id, component_css_id, entry_name,
                       name, path, BASE_STATIC_STRING(kElementComponentTag)) {}

ComponentElement::ComponentElement(ElementManager* manager,
                                   const base::String& component_id,
                                   int32_t component_css_id,
                                   const base::String& entry_name,
                                   const base::String& name,
                                   const base::String& path,
                                   const base::String& tag_name)
    : WrapperElement(manager, tag_name),
      component_css_id_(component_css_id),
      entry_name_(entry_name),
      name_(name),
      path_(path) {
  is_layout_only_ = false;
  MarkCanBeLayoutOnly(true);
  set_component_id(component_id);

  if (element_manager_ == nullptr) {
    return;
  }
  SetDefaultOverflow(element_manager_->GetDefaultOverflowVisible());
}

ComponentElement::ComponentElement(const ComponentElement& element,
                                   bool clone_resolved_props)
    : WrapperElement(element, clone_resolved_props),
      component_id_(element.component_id_),
      component_css_id_(element.component_css_id_),
      entry_name_(element.entry_name_),
      name_(element.name_),
      path_(element.path_),
      is_wrapper_component_(element.is_wrapper_component_) {
  MarkCanBeLayoutOnly(true);
}

void ComponentElement::SetBuiltinAttribute(ElementBuiltInAttributeEnum key,
                                           const lepus::Value& value) {
  FiberElement::SetBuiltinAttribute(key, value);
  switch (key) {
    case ElementBuiltInAttributeEnum::CSS_ID:
      component_css_id_ = static_cast<int32_t>(value.Number());
      break;
    case ElementBuiltInAttributeEnum::COMPONENT_ID:
      set_component_id(value.String());
      break;
    case ElementBuiltInAttributeEnum::COMPONENT_NAME:
      name_ = value.String();
      break;
    case ElementBuiltInAttributeEnum::COMPONENT_PATH:
      path_ = value.String();
      break;
    default:
      break;
  }
}

void ComponentElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  SetDefaultOverflow(manager->GetDefaultOverflowVisible());
  manager->RecordComponent(component_id_.str(), this);
  set_style_sheet_manager(style_manager);
}

ComponentElement::~ComponentElement() {
  if (!will_destroy_ && element_manager()) {
    element_manager()->EraseComponentRecord(component_id().str(), this);
  }
}

CSSFragment* ComponentElement::GetCSSFragment() {
  if (!style_sheet_) {
    if (css_style_sheet_manager_) {
      style_sheet_ = std::make_shared<CSSFragmentDecorator>(
          css_style_sheet_manager_->GetCSSStyleSheetForComponent(
              component_css_id_));
    }
    if (style_sheet_) {
      // for css variable in `:root` css
      PrepareForRootCSSVariables();
      PrepareForFontFaceIfNeeded();
    }
  }

  return style_sheet_.get();
}

void ComponentElement::PrepareForRootCSSVariables() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ComponentElement::PrepareForRootCSSVariables");
  auto* rule_set = style_sheet_->rule_set();
  if (rule_set) {
    const auto& root_css_token = rule_set->GetRootToken();
    if (root_css_token) {
      UpdateRootCSSVariables(data_model(), root_css_token);
    }
    return;
  }
  auto root_css = style_sheet_->GetSharedCSSStyle(kRootCSSId);
  if (root_css != nullptr) {
    UpdateRootCSSVariables(data_model(), root_css);
  }
}

void ComponentElement::PrepareForFontFaceIfNeeded() {
  if (IsFiberArch()) {
    return;
  }
  // In the Radon architecture, the FontFace is flushed during the
  // parsing of each node, making FontFace global to the page and
  // usable across components.
  //
  // However, in Fiber, FontFace is only flushed when a FontFamily
  // style is specified on a Text node (see
  // TextElement::ResolveAndFlushFontFaces), which means FontFace might not be
  // usable across components.
  //
  // To prevent business logic breaks and maintain alignment with
  // the behavior of the Radon architecture.
  // in the Radon-Fiber architecture, FontFace is flushed for each style_sheet.
  if (style_sheet_ && !style_sheet_->GetFontFaceRuleMap().empty() &&
      !style_sheet_->HasFontFacesResolved()) {
    SetFontFaces(style_sheet_->GetFontFaceRuleMap());
    style_sheet_->MarkFontFacesResolved(true);
  }
}

void ComponentElement::UpdateRootCSSVariables(
    AttributeHolder* holder, const std::shared_ptr<CSSParseToken>& root_token) {
  auto style_variables = root_token->GetStyleVariables();
  if (style_variables.empty()) {
    return;
  }

  for (const auto& pair : style_variables) {
    data_model()->UpdateCSSVariable(pair.first, pair.second);
  }
}

void ComponentElement::MarkHasLayoutOnlyPropsIfNecessary(
    const base::String& attribute_key) {
  // ComponentID attribute should not stop this component being layout only
  if (!attribute_key.IsEqual(kComponentID)) {
    has_layout_only_props_ = false;
  }
}

bool ComponentElement::CanBeLayoutOnly() const {
  return enable_component_layout_only_ && FiberElement::CanBeLayoutOnly();
}

void ComponentElement::set_component_id(const base::String& id) {
  if (component_id_ == id) {
    return;
  }

  if (element_manager() == nullptr) {
    return;
  }

  // In fiber mode, the component id of component element may be updated by
  // lepus runtime. If component_element_1 is updated from id1 to id2,
  // component_element_2 is updated from id2 to id1 in one data process. Then
  // according to the previous logic, delete id1 <-> component_element_1, insert
  // id2 <-> component_element_1, delete id2 <-> component_element_1, insert id1
  // <-> component_element_2. Eventually component_element_1 could not be
  // recorded. In order to solve this problem, a verification is performed
  // during the deletion operation. If the component element corresponding to
  // the deleted id is inconsistent with the current element, the deletion
  // operation will not be performed.

  element_manager()->EraseComponentRecord(component_id().str(), this);
  component_id_ = id;
  element_manager()->RecordComponent(component_id().str(), this);

  // Set ComponentID attribute for component element in fiber mode to trigger
  // markDirty and update.
  // The page element does not need to set the ComponentID attribute
  if (tag_ != kElementPageTag) {
    FiberElement::SetAttribute(BASE_STATIC_STRING(kComponentID),
                               lepus::Value(component_id()));
  }
}

const lepus::Value& ComponentElement::GetData() { return data_; }

const lepus::Value& ComponentElement::GetProperties() { return prop_; }

const std::string& ComponentElement::GetEntryName() const {
  return entry_name_.str();
}

std::string ComponentElement::ComponentStrId() { return component_id_.str(); }

void ComponentElement::SetComponentCSSID(int32_t id) {
  if (component_css_id_ != id) {
    style_sheet_ = nullptr;
    component_css_id_ = id;
  }
}

double ComponentElement::GetFontSize() {
  if (is_wrapper()) {
    return WrapperElement::GetFontSize();
  } else {
    return FiberElement::GetFontSize();
  }
}

const FiberElement::InheritedProperty&
ComponentElement::GetInheritedProperty() {
  if (is_wrapper()) {
    return WrapperElement::GetInheritedProperty();
  } else {
    return FiberElement::GetInheritedProperty();
  }
}

ParallelFlushReturn ComponentElement::PrepareForCreateOrUpdate() {
  if (is_wrapper()) {
    return WrapperElement::PrepareForCreateOrUpdate();
  } else {
    return FiberElement::PrepareForCreateOrUpdate();
  }
}

void ComponentElement::UpdateInheritedProperty() {
  if (is_wrapper()) {
    return WrapperElement::UpdateInheritedProperty();
  } else {
    return FiberElement::UpdateInheritedProperty();
  }
}

void ComponentElement::MarkAsListItem() {
  if (is_wrapper()) {
    return WrapperElement::MarkAsListItem();
  } else {
    return Element::MarkAsListItem();
  }
}

void ComponentElement::SetAttribute(const base::String& key,
                                    const lepus::Value& value,
                                    bool need_update_data_model) {
  if (is_wrapper()) {
    return WrapperElement::SetAttribute(key, value, need_update_data_model);
  } else {
    return FiberElement::SetAttribute(key, value, need_update_data_model);
  }
}

void ComponentElement::OnNodeAdded(FiberElement* child) {
  if (is_wrapper()) {
    return WrapperElement::OnNodeAdded(child);
  } else {
    return FiberElement::OnNodeAdded(child);
  }
}

void ComponentElement::OnNodeRemoved(FiberElement* child) {
  if (is_wrapper()) {
    return WrapperElement::OnNodeRemoved(child);
  } else {
    return FiberElement::OnNodeRemoved(child);
  }
}

}  // namespace tasm
}  // namespace lynx
