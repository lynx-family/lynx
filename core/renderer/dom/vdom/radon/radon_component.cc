// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_component.h"

#include <set>
#include <utility>

#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/component_config.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/services/feature_count/global_feature_counter.h"
#include "core/services/timing_handler/timing_constants.h"

namespace lynx {
namespace tasm {

RadonComponent::RadonComponent(
    PageProxy* page_proxy, int tid, CSSFragment* style_sheet,
    std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
    ComponentMould* mould, lepus::Context* context, uint32_t node_index,
    const base::String& tag_name)
    : RadonNode(page_proxy, tag_name, node_index),
      tid_(tid),
      mould_(mould),
      context_(context),
      dsl_(tasm::PackageInstanceDSL::TT),
      intrinsic_style_sheet_(style_sheet),
      style_sheet_manager_(std::move(style_sheet_manager)) {
  if (mould_) {
    DeriveFromMould(mould_);
  }
  node_type_ = kRadonComponent;
  UpdateLepusTopLevelVariableToData();
  SetRenderType(RenderType::FirstRender);
  SetComponent(this);
  if (page_proxy) {
    radon_slots_helper_ = std::make_unique<RadonSlotsHelper>(this);
    compile_render_ = page_proxy_->element_manager()->GetCompileRender();
  }
}

RadonComponent::RadonComponent(const RadonComponent& node, PtrLookupMap& map)
    : RadonNode(node, map),
      tid_(node.tid_),
      mould_(node.mould_),
      context_(node.context_),
      dsl_(node.dsl_),
      intrinsic_style_sheet_(node.intrinsic_style_sheet_),
      style_sheet_manager_(node.style_sheet_manager_) {
  if (mould_) {
    DeriveFromMould(mould_);
  }
  entry_name_ = node.entry_name_;
  name_ = node.name_;
  path_ = node.path_;
  style_sheet_ = node.style_sheet_;  // TODO: Move to Base.
  UpdateSystemInfo(GenerateSystemInfo(nullptr));
  SetRenderType(node.render_type_);
  SetComponent(this);
  dsl_ = node.dsl_;
  if (!page_proxy_->GetEnableGlobalComponentMap()) {
    for (const auto& iter : *node.component_info_map_.Table()) {
      component_info_map_.Table()->SetValue(iter.first, iter.second);
    }
    for (const auto& iter : *node.component_path_map_.Table()) {
      component_path_map_.Table()->SetValue(iter.first, iter.second);
    }
  }
  get_derived_state_from_props_function_ =
      node.get_derived_state_from_props_function_;

  get_derived_state_from_error_function_ =
      node.get_derived_state_from_error_function_;

  ForEachLepusValue(node.properties_,
                    [this](const lepus::Value& key, const lepus::Value& value) {
                      this->SetProperties(key.String(), value, false);
                    });

  ForEachLepusValue(node.data_,
                    [this](const lepus::Value& key, const lepus::Value& value) {
                      this->SetData(key.String(), value);
                    });
  SetGlobalPropsFromTasm();
  radon_slots_helper_ = std::make_unique<RadonSlotsHelper>(this);
  for (auto& plug : node.plugs_) {
    auto plug_name = plug.first;
    auto* plug_ptr = plug.second.get();
    auto* copied_plug_ptr =
        radon_factory::CopyRadonDiffSubTree(*static_cast<RadonBase*>(plug_ptr));
    AddRadonPlug(plug_name, std::unique_ptr<RadonBase>(copied_plug_ptr));
  }
}

RadonComponent::~RadonComponent() { OnElementRemoved(0); }

fml::RefPtr<Element> RadonComponent::CreateFiberElement() {
  auto fiber_element = page_proxy_->element_manager()->CreateFiberComponent(
      ComponentStrId(), GetCSSId(), GetEntryName(), name(), path());
  fiber_element->SetAttributeHolder(attribute_holder_);
  fiber_element->SetParentComponentUniqueIdForFiber(ParentComponentElementId());
  fiber_element->set_style_sheet_manager(style_sheet_manager());
  if (ShouldRemoveComponentElement()) {
    fiber_element->MarkAsWrapperComponent();
  }
  return fiber_element;
}

void RadonComponent::UpdateTable(lepus::Value& target,
                                 const lepus::Value& update, bool reset) {
  if (update.IsEmpty()) return;
  if (reset) {
    target.SetTable(lepus::Dictionary::Create());
  }
  lepus::Value::MergeValue(target, update);
}

void RadonComponent::SetData(const base::String& key,
                             const lepus::Value& value) {
  data_.SetProperty(key, value);
}

bool RadonComponent::UpdateGlobalProps(const lepus::Value& table) {
  if (!NeedsExtraData()) {
    // If there is no need for extra data, do not set GlobalProps to data to
    // avoid extra copy.
    return false;
  }

  if (!data_.IsEqual(table)) {
    if (!table.IsNil()) {
      DCHECK(table.IsObject());
      data_.SetProperty(BASE_STATIC_STRING(kGlobalPropsKey), table);
      return true;
    }
  }
  return false;
}

void RadonComponent::SetProperties(const base::String& key,
                                   const lepus::Value& value,
                                   bool strict_prop_type) {
  if (IsPropertiesUndefined(value) && ShouldBlockEmptyProperty()) {
    return;
  }
  if (!properties_.IsObject()) {
    properties_ = lepus::Value(lepus::Dictionary::Create());
  }
  // TODO(kechenglong): should use component_attributes to check.
  static base::NoDestructor<std::set<std::string>> kAttributeNames({
      "flatten",
      // The focusable and focus-index props are not valid for platform
      // rendering.
      "focusable",
      "focus-index",
      "__lynx_timing_flag",
  });
  if (kAttributeNames->find(key.str()) != kAttributeNames->end()) {
    if (!attributes()[key].IsEqual(value)) {
      SetDynamicAttribute(key, value);
      properties_dirty_ = true;
    }
  } else {
    const lepus::Value& v = properties_.GetProperty(key);
    // if value type mismatch, set value to default.
    // default_value.IsNil() means any type is permitted.
    bool same_type =
        v.Type() == value.Type() || (v.IsNumber() && value.IsNumber());
    bool use_default_value = strict_prop_type && !v.IsNil() && !same_type;
    lepus::Value new_value = use_default_value ? GetDefaultValue(v) : value;
    if (!(v == new_value)) {
      properties_.SetProperty(key, new_value);
      properties_dirty_ = true;
    }
  }

  // Each property may also be an external class mapping.
  // This is done at run time since there's no way to tell if a prop is also
  // declared as an external class for <component is="{{}}"/>.
  if (value.IsString()) {
    auto value_str = value.String();
    if (!value_str.empty()) {
      SetExternalClass(key, value_str);
    }
  }
}

CSSFragment* RadonComponent::GetStyleSheetBase(AttributeHolder* holder) {
  if (!style_sheet_) {
    if (!intrinsic_style_sheet_ && style_sheet_manager_ != nullptr) {
      intrinsic_style_sheet_ =
          style_sheet_manager_->GetCSSStyleSheetForComponent(mould_->css_id());
    }
    style_sheet_ =
        std::make_shared<CSSFragmentDecorator>(intrinsic_style_sheet_);
    if (intrinsic_style_sheet_ && style_sheet_ &&
        intrinsic_style_sheet_->HasTouchPseudoToken()) {
      style_sheet_->MarkHasTouchPseudoToken();
    }
    PrepareComponentExternalStyles(holder);
    PrepareRootCSSVariables(holder);
  }
  return style_sheet_.get();
}

void RadonComponent::ExtractExternalClass(ComponentMould* data) {
  if (data->external_classes().IsArrayOrJSArray()) {
    for (int i = 0; i < data->external_classes().GetLength(); ++i) {
      const auto& item = data->external_classes().GetProperty(i);
      if (item.IsString()) {
        external_classes_[item.String()] = ClassList();
      }
    }
  }
}

void RadonComponent::PrepareComponentExternalStyles(AttributeHolder* holder) {
  // Make sure we look for external. Return when this is top level component.
  if (IsPageForBaseComponent()) {
    return;
  }

  CSSFragmentDecorator* style_sheet =
      static_cast<CSSFragmentDecorator*>(holder->ParentStyleSheet());
  if (!style_sheet) {
    return;
  }
  for (const auto& pair : external_classes_) {
    for (const auto& clazz : pair.second) {
      const std::string rule = std::string(".") + clazz.str();
      auto token = style_sheet->GetSharedCSSStyle(rule);

      if (token) {
        report::GlobalFeatureCounter::Count(
            report::LynxFeature::CPP_ENABLE_EXTERNAL_CLASS_CSS,
            page_proxy_->element_manager()->GetInstanceId());
        // Translate into component class names and store.
        const std::string new_rule = std::string(".") + pair.first.str();
        style_sheet_->AddExternalStyle(new_rule, std::move(token));
      }
    }
  }
}

static void update_root_css_variable(
    AttributeHolder* holder, const std::shared_ptr<CSSParseToken>& root) {
  auto& variables = root->GetStyleVariables();
  if (variables.empty()) {
    return;
  }

  for (const auto& it : variables) {
    const auto& map = holder->css_variables_map();
    if (map.find(it.first) == map.end()) {
      holder->UpdateCSSVariable(it.first, it.second);
    }
  }
}

void RadonComponent::PrepareRootCSSVariables(AttributeHolder* holder) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonComponent::PrepareRootCSSVariables");
  // component may be empty
  if (!intrinsic_style_sheet_) {
    return;
  }

  auto* rule_set = intrinsic_style_sheet_->rule_set();
  if (rule_set) {
    const auto& root_css_token = rule_set->GetRootToken();
    if (root_css_token) {
      update_root_css_variable(holder, root_css_token);
    }
    return;
  }
  auto root_css = intrinsic_style_sheet_->GetSharedCSSStyle(kRootCSSId);
  if (root_css != nullptr) {
    update_root_css_variable(holder, root_css);
  }
}

void RadonComponent::UpdateSystemInfo(const lynx::lepus::Value& info) {
  if (!NeedsExtraData()) {
    // If there is no need for extra data, do not set SystemInfo to data
    // to avoid extra copy.
    return;
  }

  data_.SetProperty(BASE_STATIC_STRING(kSystemInfo), info);
  data_dirty_ = true;
}

void RadonComponent::DeriveFromMould(ComponentMould* data) {
  if (data != nullptr) {
    init_properties_ = data->properties();
    if (init_properties_.IsNil()) {
      init_properties_ = lepus::Value(lepus::Dictionary::Create());
    }
    init_data_ = data->data();
    if (init_data_.IsNil()) {
      init_data_ = lepus::Value(lepus::Dictionary::Create());
    }
    properties_ = lepus::Value::Clone(init_properties_, IsInLepusNGContext());
    data_ = lepus::Value::Clone(init_data_, IsInLepusNGContext());

    ExtractExternalClass(data);

    const auto& component_config = data->GetComponentConfig();
    if (component_config != nullptr) {
      remove_extra_data_ = component_config->GetEnableRemoveExtraData();
      remove_component_element_ = component_config->GetRemoveComponentElement();
    }
  }

  // make sure the data is table
  if (!data_.IsObject()) {
    data_ = lepus::Value(lepus::Dictionary::Create());
  }

  if (!properties_.IsObject()) {
    properties_ = lepus::Value(lepus::Dictionary::Create());
  }
}

lepus_value RadonComponent::PreprocessData() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "PreprocessData");
  if (get_derived_state_from_props_function_.IsCallable() && context_) {
    lepus_value data = context_->CallClosure(
        get_derived_state_from_props_function_, properties_, data_);
    return data;
  }
  return lepus::Value();
}

lepus_value RadonComponent::PreprocessErrorData() {
  if (dsl_ == PackageInstanceDSL::REACT &&
      get_derived_state_from_error_function_.IsCallable() && context_) {
    lepus_value data = context_->CallClosure(
        get_derived_state_from_error_function_, render_error_);
    return data;
  }
  return lepus::Value();
}

bool RadonComponent::PreRender(const RenderType& render_type) {
  if (dsl_ == PackageInstanceDSL::REACT) {
    return PreRenderReact(render_type);
  }
  return PreRenderTT(render_type);
}

bool RadonComponent::PreRenderReact(const RenderType& render_type) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "PreRenderReact");
  switch (render_type) {
    case RenderType::UpdateFromJSBySelf:
      return true;
    case RenderType::FirstRender:
    case RenderType::UpdateByParentComponent:
    case RenderType::UpdateByNativeList:
    case RenderType::UpdateByNative: {
      lepus_value new_data;
      if (get_derived_state_from_props_function_.IsCallable()) {
        new_data = PreprocessData();
        if (new_data.IsObject()) {
          UpdateTable(data_, new_data);
          LOGI("getDerivedStateFromProps component " << this->path().str());
        }

        // Add extra version fields when there could be conflicts for native
        // and JS to update data simultaneously. For child components this
        // could happen with getDerivedStateFromProps() generating states from
        // props set by its parent.
        AttachDataVersions(new_data);
      }
      //
      // case 'RenderType::FirstRender' don't execute 'shouldComponentUpdate'
      //
      if (render_type == RenderType::FirstRender) {
        return true;
      }

      //
      // check shouldComponentUpdate
      // case 'RenderType::UpdateByParentComponent' and
      // 'RenderType::UpdateByNative'
      //
      bool should_component_update = ShouldComponentUpdate();
      OnReactComponentRenderBase(new_data, should_component_update);
      return should_component_update;
    }
    case RenderType::UpdateByRenderError: {
      lepus_value new_data{};
      if (get_derived_state_from_error_function_.IsCallable()) {
        new_data = PreprocessErrorData();
        if (new_data.IsObject()) {
          new_data.SetProperty(
              BASE_STATIC_STRING(REACT_RENDER_ERROR_KEY),
              lepus::Value(BASE_STATIC_STRING(LEPUS_RENDER_ERROR)));
          UpdateTable(data_, new_data);
          LOGI("UpdateByRenderError" << this->path().str()
                                     << ", new_data: " << new_data);
        }
        AttachDataVersions(new_data);
      }
      // clear render error info, then call js render
      SetRenderError(lepus::Value());
      OnReactComponentRenderBase(new_data, true);
      return true;
    }
    default:
      break;
  }
  return true;
}

RadonComponent* RadonComponent::GetErrorBoundary() {
  RadonComponent* parent_node = GetParentComponent();
  while (parent_node != nullptr) {
    if (parent_node->get_derived_state_from_error_function_.IsCallable()) {
      return parent_node;
    }
    parent_node = parent_node->GetParentComponent();
  }
  return nullptr;
}

void RadonComponent::AttachDataVersions(lepus::Value& update_data) {
  // List descendants don't support states currently, but unfortunately they are
  // used anyway (e.g. issue #4249). Don't try to mess with those.
  if (IsInList()) {
    return;
  }

  if (update_data.IsNil()) {
    update_data.SetTable(lepus::Dictionary::Create());
  }

  // Version starts from 0, 0 means JS side has not sent any update yet.
  int64_t ui_data_version = 0;
  auto REACT_NATIVE_STATE_VERSION_KEY_str =
      BASE_STATIC_STRING(REACT_NATIVE_STATE_VERSION_KEY);
  auto REACT_JS_STATE_VERSION_KEY_str =
      BASE_STATIC_STRING(REACT_JS_STATE_VERSION_KEY);
  if (data_.Contains(REACT_NATIVE_STATE_VERSION_KEY_str)) {
    ui_data_version =
        data_.GetProperty(REACT_NATIVE_STATE_VERSION_KEY_str).Number();
  }
  ++ui_data_version;
  lepus::Value ui_version_value(ui_data_version);
  data_.SetProperty(REACT_NATIVE_STATE_VERSION_KEY_str, ui_version_value);
  update_data.SetProperty(REACT_NATIVE_STATE_VERSION_KEY_str, ui_version_value);
  update_data.SetProperty(REACT_JS_STATE_VERSION_KEY_str,
                          data_.GetProperty(REACT_JS_STATE_VERSION_KEY_str));

  LOGI("AttachDataVersions native: "
       << ui_data_version
       << ", js: " << data_.GetProperty(REACT_JS_STATE_VERSION_KEY_str).Number()
       << ", path: " << path().str());
}

void RadonComponent::ResetDataVersions() {
  // List descendants don't support states currently, but unfortunately they are
  // used anyway (e.g. issue #4249). Don't try to mess with those.
  if (IsInList()) {
    return;
  }

  // Reset both ui and js versions to 0 (which is the default value)
  // ui version will be bumped up to 1 by AttachDataVersions later
  lepus::Value ui_version_value(0);
  lepus::Value js_version_value(0);
  data_.SetProperty(BASE_STATIC_STRING(REACT_NATIVE_STATE_VERSION_KEY),
                    ui_version_value);
  data_.SetProperty(BASE_STATIC_STRING(REACT_JS_STATE_VERSION_KEY),
                    js_version_value);

  LOGI("ResetDataVersions native: " << 0 << ", js: " << 0
                                    << ", path: " << path().str());
}

bool RadonComponent::PreRenderTT(const RenderType& render_type) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "PreRenderTT");
  if (render_type == RenderType::UpdateFromJSBySelf) {
    // update from js, no need to call `getDerivedStateFromProps`
    return ShouldComponentUpdate();
  }
  lepus_value new_data;
  if (get_derived_state_from_props_function_.IsCallable()) {
    new_data = PreprocessData();
    if (new_data.IsObject()) {
      UpdateTable(data_, new_data);
      LOGI("getDerivedStateFromProps for TTML component "
           << this->path().str());
    }
  }

  // check shouldComponentUpdate
  return render_type == RenderType::FirstRender || ShouldComponentUpdate();
}

bool RadonComponent::ShouldComponentUpdate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ShouldComponentUpdate");
  if (should_component_update_function_.IsCallable() && context_) {
    lepus::Value result =
        context_->CallClosure(should_component_update_function_, properties_,
                              data_, pre_properties_, pre_data_);
    if (result.IsBool()) {
      return result.Bool();
    }
    LOGE("ShouldComponentUpdate should return bool value!");
  }
  return true;
}

bool RadonComponent::CheckReactShouldAbortUpdating(const lepus::Value& table) {
  auto REACT_NATIVE_STATE_VERSION_KEY_str =
      BASE_STATIC_STRING(REACT_NATIVE_STATE_VERSION_KEY);
  if (table.Contains(REACT_NATIVE_STATE_VERSION_KEY_str)) {
    int64_t expected_native_state_version =
        table.GetProperty(REACT_NATIVE_STATE_VERSION_KEY_str).Number();
    int64_t ui_data_version =
        data_.GetProperty(REACT_NATIVE_STATE_VERSION_KEY_str).Number();
    // List descendants don't support states currently, but unfortunately they
    // are used anyway (e.g. issue #4249). Don't try to mess with those.
    if (!IsInList() && expected_native_state_version < ui_data_version) {
      LOGI("CheckReactShouldAbortUpdating conflicts detected, "
           << "expecting native version: " << expected_native_state_version
           << ", actual version: " << ui_data_version << ", aborting");
      return true;
    }
    // Update versions upfront for later correct determination of "data changed"
    data_.SetProperty(REACT_NATIVE_STATE_VERSION_KEY_str,
                      table.GetProperty(REACT_NATIVE_STATE_VERSION_KEY_str));
    auto REACT_JS_STATE_VERSION_KEY_str =
        BASE_STATIC_STRING(REACT_JS_STATE_VERSION_KEY);
    data_.SetProperty(REACT_JS_STATE_VERSION_KEY_str,
                      table.GetProperty(REACT_JS_STATE_VERSION_KEY_str));
  }
  return false;
}

bool RadonComponent::CheckReactShouldComponentUpdateKey(
    const lepus::Value& table) {
  auto REACT_SHOULD_COMPONENT_UPDATE_KEY_str =
      BASE_STATIC_STRING(REACT_SHOULD_COMPONENT_UPDATE_KEY);
  if (table.IsObject() &&
      table.Contains(REACT_SHOULD_COMPONENT_UPDATE_KEY_str)) {
    bool should_component_render =
        table.GetProperty(REACT_SHOULD_COMPONENT_UPDATE_KEY_str).Bool();
    if (!should_component_render) {
      ForEachLepusValue(
          table, [this, &REACT_SHOULD_COMPONENT_UPDATE_KEY_str](
                     const lepus::Value& key, const lepus::Value& value) {
            auto key_str = key.String();
            if (key_str.str() != REACT_SHOULD_COMPONENT_UPDATE_KEY_str.str()) {
              this->data_.SetProperty(key_str, value);
            }
          });
      return true;
    }
  }
  return false;
}

bool RadonComponent::CheckReactShouldAbortRenderError(
    const lepus::Value& table) {
  auto REACT_RENDER_ERROR_KEY_str = BASE_STATIC_STRING(REACT_RENDER_ERROR_KEY);
  if (table.Contains(REACT_RENDER_ERROR_KEY_str)) {
    if (table.GetProperty(REACT_RENDER_ERROR_KEY_str).StdString() ==
            JS_RENDER_ERROR ||
        table.GetProperty(REACT_RENDER_ERROR_KEY_str).StdString() ==
            LEPUS_RENDER_ERROR) {
      LOGI("CheckReactShouldAbortRenderError");
      SetRenderError(lepus::Value());
      return true;
    }
  }
  return false;
}

lepus::Value RadonComponent::GetDefaultValue(
    const lepus::Value& template_value) {
  lepus::Value default_value;
  switch (template_value.Type()) {
    case lepus::Value_Double:
    case lepus::Value_NaN:
      default_value.SetNumber(static_cast<double>(0.0));
      break;
    case lepus::Value_Bool:
      default_value.SetBool(false);
      break;
    case lepus::Value_String:
      default_value.SetString(base::String());
      break;
    case lepus::Value_Int32:
      default_value.SetNumber(static_cast<int32_t>(0));
      break;
    case lepus::Value_Int64:
      default_value.SetNumber(static_cast<int64_t>(0));
      break;
    case lepus::Value_UInt32:
      default_value.SetNumber(static_cast<uint32_t>(0));
      break;
    case lepus::Value_UInt64:
      default_value.SetNumber(static_cast<uint64_t>(0));
      break;
    case lepus::Value_Table:
      default_value.SetTable(lepus::Dictionary::Create());
      break;
    case lepus::Value_Array:
      default_value.SetArray(lepus::CArray::Create());
      break;
    case lepus::Value_Nil:
      default_value.SetNil();
      break;
    case lepus::Value_Undefined:
      default_value.SetUndefined();
      break;
    default:
      default_value = template_value;
  }
  return default_value;
}

void RadonComponent::UpdateLepusTopLevelVariableToData() {
  UpdateSystemInfo(GenerateSystemInfo(nullptr));
  SetGlobalPropsFromTasm();
}

// for remove element
int RadonComponent::ImplId() const {
  Element* element = TopLevelViewElement();
  if (!element) {
    return kInvalidImplId;
  }
  return element->impl_id();
}

void RadonComponent::OnComponentRemovedInPostOrder() {
  RadonBase::OnComponentRemovedInPostOrder();
  OnElementRemoved(0);
}

void RadonComponent::SetComponent(RadonComponent* component) {
  radon_component_ = component;
  for (auto& child : radon_children_) {
    child->SetComponent(this);
  }
}

bool RadonComponent::SetRemoveComponentElement(const base::String& key,
                                               const lepus::Value& value) {
  if (key.IsEqual(kRemoveComponentElement) && value.IsBool()) {
    if (value.Bool()) {
      remove_component_element_ = BooleanProp::TrueValue;
    } else {
      remove_component_element_ = BooleanProp::FalseValue;
    }
    return true;
  }
  return false;
}

void RadonComponent::InitStyleSheetBySSR(
    std::shared_ptr<tasm::CSSFragmentDecorator> style_sheet) {
  style_sheet_ = std::move(style_sheet);
}

bool RadonComponent::SetSpecialComponentAttribute(const base::String& key,
                                                  const lepus::Value& value) {
  if (SetRemoveComponentElement(key, value)) {
    return true;
  } else if (SetLynxKey(key, value)) {
    // SetLynxKey function only store value in radon_base
    // set lynx-key attribute then component is consistent with other nodes
    SetDynamicAttribute(key, value);
    return true;
  } else {
    return false;
  }
}

void RadonComponent::AddChild(std::unique_ptr<RadonBase> child) {
  AddChildWithoutSetComponent(std::move(child));
  // need to set component to this after child is added
  radon_children_.back()->SetComponent(this);
}

void RadonComponent::AddSubTree(std::unique_ptr<RadonBase> child) {
  AddChild(std::move(child));
  for (auto& plug : plugs_) {
    AddRadonPlug(plug.first, std::move(plug.second));
  }
  radon_children_.back()->NeedModifySubTreeComponent(this);
}

int32_t RadonComponent::ComponentId() { return component_id_; }

bool RadonComponent::IsPropertiesUndefined(const lepus::Value& value) const {
  // methods to check properties undefined.
  // it's result will differ according to pageConfig `enableComponentNullProps`
  // if enableComponentNullProps on, it depends on whether value isEmpty, else
  // it depends on whether value inUndefined
  if (page_proxy_->GetEnableComponentNullProp()) {
    return value.IsUndefined();
  } else {
    // compatible for sdk 2.8 and before versions.
    // in before versions, we only block Undefined type and Value_Nil
    return value.IsUndefined() || value.Type() == lepus::Value_Nil;
  }
}

void RadonComponent::SetGlobalPropsFromTasm() {
  if (page_proxy_) {
    auto global_props = page_proxy_->GetGlobalPropsFromTasm();
    UpdateGlobalProps(global_props);
  }
}

bool RadonComponent::ShouldBlockEmptyProperty() {
  if (IsInList()) {
    // This is a bit tricky.
    // For history reason: Block empty props in list only when engineVersion
    // higher than 2.1
    if (page_proxy_->element_manager()->GetIsTargetSdkVerionHigherThan21()) {
      return true;
    }
    return false;
  }
  // normal component that not in list, should block empty props
  // unconditionally.
  return true;
}

bool RadonComponent::UpdateRadonComponentWithoutDispatch(
    RenderType render_type, const lepus::Value& incoming_property,
    const lepus::Value& incoming_data) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "RadonComponent::UpdateRadonComponentWithoutDispatch",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  RenderType ori_render_type = render_type;
  if (!dispatched_) {
    render_type = RenderType::FirstRender;
  }
  SetRenderType(render_type);
  if (NeedSavePreState(render_type)) {
    if (incoming_property.IsObject()) {
      set_pre_properties(lepus::Value::ShallowCopy(properties_));
    } else {
      set_pre_properties(properties_);
    }
    if (incoming_data.IsObject()) {
      set_pre_data(lepus::Value::ShallowCopy(data_));
    } else {
      set_pre_data(data_);
    }
  }

  if (IsReact() && render_type == RenderType::UpdateFromJSBySelf) {
    if (CheckReactShouldAbortUpdating(incoming_data)) {
      return false;
    }
    if (CheckReactShouldComponentUpdateKey(incoming_data)) {
      return false;
    }
  }

  if (incoming_data.IsObject() && incoming_data.GetLength() > 0) {
    if ((data_.IsObject() && CheckTableShadowUpdated(data_, incoming_data)) ||
        data_.IsNil()) {
      UpdateTable(data_, incoming_data);
      data_dirty_ = true;
    }
  }

  if (incoming_property.IsObject() && incoming_property.GetLength() > 0) {
    if ((properties_.IsObject() &&
         CheckTableShadowUpdated(properties_, incoming_property)) ||
        properties_.IsNil()) {
      properties_dirty_ = true;
      ForEachLepusValue(incoming_property, [this](const lepus::Value& key,
                                                  const lepus::Value& val) {
        this->SetProperties(key.String(), val,
                            page_proxy_->GetStrictPropType());
      });
    }
  }

  // shouldn't update when both of data and properties are not changed.
  if (!data_dirty_ && !properties_dirty_ &&
      render_type != RenderType::UpdateByRenderError) {
    EXEC_EXPR_FOR_INSPECTOR({
      if (lynx::tasm::LynxEnv::GetInstance().IsTableDeepCheckEnabled()) {
        page_proxy_->element_manager()->OnComponentUselessUpdate(name_.str(),
                                                                 properties_);
      }
    });
    return false;
  }
  if (ori_render_type == RenderType::UpdateByNativeList && properties_dirty_) {
    return PreRender(RenderType::UpdateByNativeList);
  }
  return PreRender(render_type);
}

void RadonComponent::UpdateRadonComponent(RenderType render_type,
                                          const lepus::Value& incoming_property,
                                          const lepus::Value& incoming_data,
                                          const DispatchOption& option,
                                          PipelineOptions& pipeline_options) {
  LOGI("RadonComponent::UpdateRadonComponent, name: "
       << name_.str() << ", component id: " << ComponentId());
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonComponent::UpdateRadonComponent",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  SetRenderType(render_type);
  bool shouldUpdate = UpdateRadonComponentWithoutDispatch(
      render_type, incoming_property, incoming_data);

  bool forceUpdate =
      option.css_variable_changed_ || option.need_create_js_counterpart_ ||
      option.global_properties_changed_ || option.force_update_this_component ||
      option.force_diff_entire_tree_;

  if (shouldUpdate || forceUpdate) {
    Refresh(option, pipeline_options);
  }
}

void RadonComponent::SetCSSVariables(const std::string& id_selector,
                                     const lepus::Value& properties,
                                     PipelineOptions& pipeline_options) {
  set_variable_ops_.emplace_back(SetCSSVariableOp(id_selector, properties));
  DispatchOption dispatch_option(page_proxy_);
  dispatch_option.css_variable_changed_ = true;
  Refresh(dispatch_option, pipeline_options);
  if (dispatch_option.has_patched_) {
    page_proxy_->element_manager()->SetNeedsLayout();
  };
  page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
}

void RadonComponent::Refresh(const DispatchOption& option,
                             PipelineOptions& pipeline_options) {
  // Radon Compatible
  OnComponentUpdate(option);
  for (auto& slot : slots()) {
    slot.second->SetPlugCanBeMoved(true);
  }

  auto original_slots = slots_;
  // clear original slots
  radon_slots_helper_->RemoveAllSlots();
  // save original children
  auto original_radon_children = std::move(radon_children_);
  radon_children_.clear();
  RenderOption renderOption;
  if (pipeline_options.need_timestamps) {
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderStart);
  }
  RenderRadonComponentIfNeeded(renderOption);
  if (pipeline_options.need_timestamps) {
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderEnd);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kResolveStart);
    page_proxy_->element_manager()
        ->painting_context()
        ->MarkUIOperationQueueFlushTiming(
            tasm::timing::kPaintingUiOperationExecuteStart,
            pipeline_options.pipeline_id);
  }

  RadonMyersDiff(original_radon_children, option);
  if (pipeline_options.need_timestamps) {
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kResolveEnd);
  }
  /*
   * In this UpdateRadonComponent case, plugs cannot be changed, but slots
   * may be changed. We've already saved original plugs and original slots,
   * we just need to refill the original plugs to new slots.
   */
  // TODO: brothers' lifecycle
  radon_slots_helper_->ReFillSlotsAfterChildrenDiff(original_slots, option);
  ResetDispatchedStatus();
  OnReactComponentDidUpdate(option);
}

void RadonComponent::PreHandlerCSSVariable() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonComponent::PreHandlerCSSVariable",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (set_variable_ops_.empty()) {
    return;
  }

  for (auto& temp : set_variable_ops_) {
    NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                              temp.GetIdSelector());
    options.only_current_component = false;
    auto result = RadonNodeSelector::Select(this, options);
    if (result.Success()) {
      RadonNode* node = result.GetOneNode();
      auto css_variable_kv = temp.GetProperties();
      if (css_variable_kv.IsObject()) {
        ForEachLepusValue(css_variable_kv, [node](const lepus::Value& key,
                                                  const lepus::Value& val) {
          node->UpdateCSSVariableFromSetProperty(key.String(), val.String());
        });
      }
    }
  }
}

void RadonComponent::RenderRadonComponentIfNeeded(RenderOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "RadonComponent::RenderRadonComponentIfNeeded",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (radon_children_.empty()) {
    RenderRadonComponent(option);
  }
}

void RadonComponent::RenderRadonComponent(RenderOption& option) {
  if (context_) {
    lepus::Value p1(this);
    context_->CallInPauseSuppressionMode(
        "$renderComponent" + std::to_string(tid_), p1, data_, properties_,
        lepus::Value(option.recursively));
    PreHandlerCSSVariable();
  }
}

// Not first screen, native triggers updateData (list or parent component
// modifies data)
void RadonComponent::OnReactComponentRenderBase(lepus::Value& new_data,
                                                bool should_component_update) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonComponent::OnReactComponentRenderBase",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!IsReact()) {
    return;
  }

  RadonPage* page = root_node();
  if (!page) {
    return;
  }
  page_proxy_->OnReactComponentRender(
      this, page_proxy_->ProcessReactPropsForJS(properties_), new_data,
      should_component_update);
}

void RadonComponent::AdoptPlugToSlot(RadonSlot* slot,
                                     std::unique_ptr<RadonBase> plug) {
  RadonPlug* plug_to_reattach = static_cast<RadonPlug*>(plug.get());
  slot->AdoptPlug(std::move(plug));
  // re-attach plug's radon_component_ if needed
  if (plug_to_reattach->radon_component_ != radon_component_) {
    plug_to_reattach->SetAttachedComponent(this);
  }
}

void RadonComponent::AddRadonPlug(const base::String& name,
                                  std::unique_ptr<RadonBase> plug) {
  if (!plug) {
    return;
  }
  auto it = slots_.find(name);
  if (it != slots_.end()) {
    AdoptPlugToSlot(it->second, std::move(plug));
  } else {
    plugs_[name] = std::move(plug);
  }
}

void RadonComponent::RemovePlugByName(const base::String& name) {
  auto it = slots_.find(name);
  if (it != slots_.end()) {
    it->second->ReleasePlug();
  }
}

void RadonComponent::AddRadonSlot(const base::String& name, RadonSlot* slot) {
  slots_.insert_or_assign(name, slot);
  slot->radon_component_ = this;
}

void RadonComponent::ResetElementRecursively() {
  // In the Radon diff list scenario, when two VDOM components are reused for
  // Diff, any addition or deletion of a node will not trigger the corresponding
  // Component Add/Remove lifecycle. Previously, we would execute
  // EraseComponentRecord in OnComponentRemoved, but in this situation,
  // EraseComponentRecord will not be executed, which causes a destructed object
  // to remain in the component map. This leads to crashes when it is
  // subsequently used. To solve this problem, we perform an operation in the
  // ResetElementRecursively. If this RadonNode is a RadonComponent and it holds
  // a Element, it will delete the element from the component map when exec
  // ResetElementRecursively.
  if (element() != nullptr) {
    page_proxy_->element_manager()->EraseComponentRecord(ComponentStrId(),
                                                         element());
  }
  RadonNode::ResetElementRecursively();
}

void RadonComponent::OnElementRemoved(int idx) {
  if (IsRadonComponent() && page_proxy_) {
    page_proxy_->OnComponentRemoved(this);
  }
  dispatched_ = false;
}

void RadonComponent::OnElementMoved(int from_idx, int to_idx) {
  if (IsRadonComponent()) {
    page_proxy_->OnComponentMoved(this);
  }
}

void RadonComponent::DispatchChildren(const DispatchOption& option) {
  RadonBase::DispatchChildren(option);
  auto* root = static_cast<RadonPage*>(root_node());
  if (root) {
    root->CollectComponentDispatchOrder(this);
  }
}

void RadonComponent::DispatchChildrenForDiff(const DispatchOption& option) {
  RadonBase::DispatchChildrenForDiff(option);
  auto* root = static_cast<RadonPage*>(root_node());
  if (root) {
    root->CollectComponentDispatchOrder(this);
  }
}

void RadonComponent::DispatchSelf(const DispatchOption& option) {
  RadonNode::DispatchSelf(option);
}

void RadonComponent::Dispatch(const DispatchOption& option) {
  auto* root = static_cast<RadonPage*>(root_node());
  if (root == nullptr) {
    return;
  }
  // data and props are all clean.
  // No need to dispatch its children.
  bool should_update =
      data_dirty_ || properties_dirty_ || !option.class_transmit_.IsEmpty() ||
      option.css_variable_changed_ || option.global_properties_changed_ ||
      option.ssr_hydrating_;
  if (dispatched_ && !should_update) {
    DispatchSelf(option);
    return;
  }

  bool dispatched = dispatched_;
  if (!dispatched) {
    // Set component_id_ and then dispatch self
    if (component_id_ == 0) {
      GenerateAndSetComponentId();
    }
  }
  DispatchSelf(option);
  OnComponentUpdate(option);
  RenderOption render_option;
  render_option.recursively = true;
  RenderRadonComponentIfNeeded(render_option);
  DispatchSubTree(option);
  ResetDispatchedStatus();
  OnReactComponentDidUpdate(option);
}

void RadonComponent::OnDataSetChanged() {
  RadonPage* page = root_node();
  if (page) {
    auto table = lepus::Dictionary::Create();
    for (const auto& temp : data_set()) {
      table->SetValue(temp.first, temp.second);
    }
    page_proxy_->OnComponentDataSetChanged(this,
                                           lepus::Value(std::move(table)));
  }
}

void RadonComponent::OnSelectorChanged() {
  RadonPage* page = root_node();
  if (page) {
    lepus::Value data = lepus::Value();
    data.SetTable(lepus::Dictionary::Create());
    std::string class_array_string;
    for (size_t i = 0; i < classes().size(); i++) {
      class_array_string.append(classes()[i].str());
      if (i < classes().size() - 1) {
        class_array_string.append(" ");
      }
    }
    BASE_STATIC_STRING_DECL(kClassName, "className");
    BASE_STATIC_STRING_DECL(kId, "id");
    data.Table()->SetValue(kClassName, std::move(class_array_string));
    data.Table()->SetValue(kId, id_selector());
    page_proxy_->OnComponentSelectorChanged(this, data);
  }
}

void RadonComponent::DispatchForDiff(const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonComponent::DispatchForDiff",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto* root = static_cast<RadonPage*>(root_node());
  if (root == nullptr) {
    return;
  }
  RenderOption render_option;
  render_option.recursively = true;
  RenderRadonComponentIfNeeded(render_option);
  // attach plugs
  radon_slots_helper_->FillUnattachedPlugs();

  // Set component_id_ and then dispatch self
  if (!component_id_) {
    GenerateAndSetComponentId();
  }

  DispatchSelf(option);

  // update component lifecycle and then dispatch subtree
  OnComponentUpdate(option);

  DispatchChildrenForDiff(option);

  ResetDispatchedStatus();
  OnReactComponentDidUpdate(option);
}

RadonComponent* RadonComponent::GetParentComponent() {
  RadonBase* parent_node = Parent();
  while (parent_node != nullptr) {
    if (parent_node->IsRadonComponent() || parent_node->IsRadonPage()) {
      RadonComponent* parent_component =
          static_cast<RadonComponent*>(parent_node);
      return static_cast<RadonComponent*>(parent_component);
    }
    parent_node = parent_node->Parent();
  }

  return nullptr;
}

lepus::Value& RadonComponent::GetComponentInfoMap(
    const std::string& entry_name) {
  if (page_proxy_->GetEnableGlobalComponentMap()) {
    return page_proxy_->GetGlobalComponentInfoMap(
        entry_name.empty() ? GetEntryName() : entry_name);
  }
  return component_info_map_;
}

lepus::Value& RadonComponent::GetComponentPathMap(
    const std::string& entry_name) {
  if (page_proxy_->GetEnableGlobalComponentMap()) {
    return page_proxy_->GetGlobalComponentPathMap(
        entry_name.empty() ? GetEntryName() : entry_name);
  }
  return component_path_map_;
}

// Search for list in the ancestor chain and cache the result ever after.
bool RadonComponent::IsInList() {
  if (in_list_status_ != InListStatus::Unknown) {
    return in_list_status_ == InListStatus::InList;
  }
  auto* parent = Parent();
  while (parent) {
    if (parent->NodeType() == kRadonListNode) {
      in_list_status_ = InListStatus::InList;
      return true;
    }
    if (parent->NodeType() == kRadonPage) {
      in_list_status_ = InListStatus::NotInList;
      return false;
    }
    parent = parent->Parent();
  }
  // In some cases, the RadonComponent is still not connected with the root node
  // when the method IsInList is called, hence in_list_status_ should still be
  // InListStatus::Unknown.
  in_list_status_ = InListStatus::Unknown;
  return false;
}

bool RadonComponent::GetNeedElementByEntry() {
  // TODO(nihao.royal): support lazy bundle scope.
  // TODO: inject page proxy for config when doing ssr for react lynx
  return page_proxy_ == nullptr ||
         !page_proxy_->element_manager()->GetRemoveComponentElement();
}

CSSFragment* RadonComponent::GetStyleSheet() {
  auto* fragment = GetStyleSheetBase(this->attribute_holder_.get());
  OnStyleSheetReady(fragment);
  return fragment;
}

void RadonComponent::OnStyleSheetReady(CSSFragment* fragment) {
  if (!page_proxy_ || !page_proxy_->element_manager() || !fragment ||
      !fragment->HasTouchPseudoToken()) {
    return;
  }
  page_proxy_->element_manager()->UpdateTouchPseudoStatus(true);
}

void RadonComponent::OnComponentUpdate(const DispatchOption& option) {
  if ((!dispatched_ && page_proxy_->ComponentWithId(ComponentId())) ||
      option.ignore_component_lifecycle_) {
    page_proxy_->UpdateComponentInComponentMap(this);
  }
  if (option.ignore_component_lifecycle_) {
    return;
  }
  if (option.refresh_lifecycle_) {
    // refresh lifecycle should call OnComponentAdded lifecycle.
    page_proxy_->OnComponentAdded(this);
    return;
  }
  if (!dispatched_ && !page_proxy_->ComponentWithId(ComponentId())) {
    page_proxy_->OnComponentAdded(this);
  } else if (properties_dirty_) {
    if (!IsReact()) {
      page_proxy_->OnComponentPropertyChanged(this);
    }
  }
}

void RadonComponent::OnReactComponentDidUpdate(const DispatchOption& option) {
  if (IsReact() && !option.ignore_component_lifecycle_) {
    page_proxy_->OnReactComponentDidUpdate(this);
    if (!CheckReactShouldAbortRenderError(data_) && render_error_.IsObject() &&
        !render_error_.IsNil()) {
      auto catch_error = lepus::Dictionary::Create();
      BASE_STATIC_STRING_DECL(kMessage, "message");
      BASE_STATIC_STRING_DECL(kStack, "stack");
      BASE_STATIC_STRING_DECL(kName, "name");
      catch_error->SetValue(kMessage, render_error_.GetProperty(kMessage));
      catch_error->SetValue(kStack, render_error_.GetProperty(kStack));
      catch_error->SetValue(kName, BASE_STATIC_STRING(LEPUS_RENDER_ERROR));
      DispatchOption dispatch_option(page_proxy_);
      PipelineOptions pipeline_options;
      UpdateRadonComponent(RadonComponent::RenderType::UpdateByRenderError,
                           lepus::Value(), lepus::Value(), dispatch_option,
                           pipeline_options);
      page_proxy_->OnReactComponentDidCatch(
          this, lepus::Value(std::move(catch_error)));
    }
  }
}

void RadonComponent::ResetDispatchedStatus() {
  this->properties_dirty_ = false;
  this->data_dirty_ = false;
  this->dispatched_ = true;
}

// for remove component element
bool RadonComponent::ShouldRemoveComponentElement() const {
  if (radon_parent_ != nullptr && radon_parent_->NodeType() == kRadonListNode) {
    return false;
  }
  if (remove_component_element_ == BooleanProp::NotSet) {
    if (need_element_by_entry_ == BooleanProp::NotSet) {
      // remove_component_element should not be dynamically switched.
      auto* component = const_cast<RadonComponent*>(this);
      component->need_element_by_entry_ = component->GetNeedElementByEntry()
                                              ? BooleanProp::TrueValue
                                              : BooleanProp::FalseValue;
    }
    return need_element_by_entry_ == BooleanProp::FalseValue;
  }
  return remove_component_element_ == BooleanProp::TrueValue;
}

bool RadonComponent::NeedsElement() const {
  return !ShouldRemoveComponentElement() ||
         (page_proxy_ &&
          page_proxy_->element_manager()->GetEnableFiberElementForRadonDiff());
}

bool RadonComponent::NeedsExtraData() const {
  // remove extra data and need extra data are the opposite
  switch (remove_extra_data_) {
    case BooleanProp::TrueValue:
      return false;
    case BooleanProp::FalseValue:
      return true;
    case BooleanProp::NotSet: {
      // use page_config's GetEnableRemoveComponentExtraData config.
      // TODO: inject page proxy for config when doing ssr for react lynx
      return page_proxy_ == nullptr ||
             !page_proxy_->GetEnableRemoveComponentExtraData();
    }
  }
}

void RadonComponent::WillRemoveNode() {
  if (will_remove_node_has_been_called_) {
    return;
  }
  will_remove_node_has_been_called_ = true;
  for (auto& plug : plugs_) {
    if (plug.second) {
      plug.second->WillRemoveNode();
    }
  }
  for (auto& node : radon_children_) {
    if (node) {
      node->WillRemoveNode();
    }
  }
}

void RadonComponent::ModifySubTreeComponent(RadonComponent* const target) {
  // iteratively set this and this's children's radon_component_ to target
  if (!target) {
    return;
  }
  radon_component_ = target;
  for (auto& slot : slots()) {
    slot.second->SetComponent(this);
    if (!slot.second->radon_children_.empty()) {
      // modify the plug's radon_component_
      slot.second->radon_children_.front()->ModifySubTreeComponent(target);
    }
  }
  if (compile_render_) {
    for (auto& plug : plugs_) {
      // modify the plug's radon_component_.
      // only need to handle in compile render.
      if (plug.second && plug.second->component() != target) {
        plug.second->ModifySubTreeComponent(target);
      }
    }
  }
  return;
}

// for remove component element
Element* RadonComponent::TopLevelViewElement() const {
  if (ShouldRemoveComponentElement() && !IsRadonPage() &&
      !radon_children_.empty()) {
    RadonBase* first_child = radon_children_[0].get();
    if (first_child->IsRadonComponent()) {
      return static_cast<RadonComponent*>(first_child)->TopLevelViewElement();
    }
    return first_child->element();
  }
  return element();
}

void RadonComponent::SwapElement(
    const std::unique_ptr<RadonBase>& old_radon_base,
    const DispatchOption& option) {
  auto old_radon_component = static_cast<RadonComponent*>(old_radon_base.get());
  // when the parent component renders and triggers the sub component reusage,
  // we should just reuse the set_variable_op to avoid SetProperty missing
  // issue.
  set_variable_ops_ = old_radon_component->set_variable_ops_;
  remove_component_element_ = old_radon_component->remove_component_element_;
  need_element_by_entry_ = old_radon_component->need_element_by_entry_;
  RadonNode::SwapElement(old_radon_base, option);
}

void RadonComponent::RadonDiffChildren(
    const std::unique_ptr<RadonBase>& old_radon_child,
    const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonComponent::RadonDiffChildren",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  if (option.ssr_hydrating_) {
    // Hydration is attaching current nodes to nodes rendered by server side.
    // For component, it only needs to be treated as a regular node.
    // Neither info update, nor component life cycle are executed during
    // hydration.
    radon_slots_helper_->FillUnattachedPlugs();
    if (element() && element()->is_fiber_element()) {
      component_element()->SetCSSID(GetCSSId());
    }
    RadonBase::RadonDiffChildren(old_radon_child, option);
    // Component map should still be updated when hydrating.
    page_proxy_->UpdateComponentInComponentMap(this);
    return;
  }

  auto old_radon_component =
      static_cast<RadonComponent*>(old_radon_child.get());

  if (old_radon_component == nullptr ||
      old_radon_component->NodeType() != NodeType()) {
    LOGE(
        "Radon compatible error: diff radon-component with "
        "non-radon-component.");
    return;
  }
  if (option.only_swap_element_) {
    RadonReusableDiffChildren(old_radon_component, option);
    return;
  }
  if (option.refresh_lifecycle_) {
    // TODO(wangqingyu): TT should also reset when support data version
    if (IsReact()) {
      // nativeStateVersion and jsStateVersion should be reset like a new
      // created component since JS counter part are newly created
      ResetDataVersions();
    }

    // component's component_id_ should be generated like a new created
    // component when refresh lifecycle
    GenerateAndSetComponentId();
    if (NeedsElement() && element() && option.need_update_element_) {
      if (element()->is_radon_element()) {
        element()->FlushProps();
      }
    }
  } else {
    // reuse old radon component's component_id_
    component_id_ = old_radon_component->ComponentId();
  }

  LOGV("RadonComponent::RadonDiffChildren in Radon Compatible, name: "
       << name_.str() << ", component id: " << component_id_);

  // update component in component-map
  page_proxy_->UpdateComponentInComponentMap(this);

  // if use_new_component_data_ is set true, shouldn't re-use old component's
  // data_, worklet_instances_ and inner_state_.
  if (!option.use_new_component_data_) {
    // reuse old component's data
    // data cannot be changed by the component's parent component, but
    // properties may be changed
    data_ = old_radon_component->data_;
    worklet_instances_ = old_radon_component->worklet_instances_;
    inner_state_ = old_radon_component->inner_state_;
  }

  dispatched_ = true;

  bool force_update_all = option.ShouldForceUpdate();

  // check the properties of the component
  bool should_update_properties =
      GetProperties() != old_radon_component->GetProperties();
  if (should_update_properties) {
    properties_dirty_ = true;
  }
  if (should_update_properties || option.refresh_lifecycle_) {
    OnComponentUpdate(option);
  }

  bool final_should_update = false;

  if (option.refresh_lifecycle_) {
    // If should refresh lifecycle, shouldn't call PreRender.
    final_should_update = true;
  } else if (should_update_properties || force_update_all) {
    SetRenderType(RenderType::UpdateByParentComponent);
    if (NeedSavePreState(render_type_)) {
      set_pre_properties(old_radon_component->GetProperties());
      set_pre_data(data_);
    }
    final_should_update = PreRender(render_type_) || force_update_all;
  }

  if (final_should_update) {
    // need to re-render and continue diff the components' children

    // clear original slots
    radon_slots_helper_->RemoveAllSlots();
    RenderOption renderOption;
    RenderRadonComponentIfNeeded(renderOption);
    /*
     * attach new plugs
     * Q: Why need to clear original slots and then attach new plugs?
     * A: Because the plugs are created before and outside this new component.
     * The plugs are bound to this new component before the component
     * re-rendered, too. So after the new component finished re-rendering, we
     * need to re-attach these newly created plugs.
     */
    radon_slots_helper_->FillUnattachedPlugs();

    // continue diff the components' children
    RadonMyersDiff(old_radon_component->radon_children_, option);
    ResetDispatchedStatus();
    OnReactComponentDidUpdate(option);
    return;
  }

  // no need to re-render, just reuse everything from the old component, expect
  // plugs
  if (!page_proxy_->GetEnableGlobalComponentMap()) {
    component_info_map_ = old_radon_component->component_info_map_;
    component_path_map_ = old_radon_component->component_path_map_;
  }

  /*
   * Save original plugs to diff with new plugs.
   * Here although we reuse everything from the old component, we still need to
   * do diff on the plugs of the new and old component. Because the plugs depend
   * on outer component, but not this component.
   */
  NameToPlugMap original_plugs;
  radon_slots_helper_->MovePlugsFromSlots(original_plugs,
                                          old_radon_component->slots_);

  // reuse old slots
  for (auto& slot : old_radon_component->slots_) {
    AddRadonSlot(slot.first, slot.second);
  }
  // move children from old component to new component
  for (auto& child : old_radon_component->radon_children_) {
    AddChild(std::move(child));
  }
  old_radon_component->radon_children_.clear();
  // attach new plugs
  radon_slots_helper_->FillUnattachedPlugs();
  // diff old plug vs new plug
  radon_slots_helper_->DiffWithPlugs(original_plugs, option);
  // iteratively set children's radon_component_ to this
  for (auto& child : radon_children_) {
    child->NeedModifySubTreeComponent(this);
  }
  // issue: #5462
  // should not call OnReactComponentDidUpdate. remove it since 2.2.
  // leave it here in lower versions for compatibility.
  ResetDispatchedStatus();
  bool should_run_component_did_update =
      !page_proxy_->element_manager()->GetIsTargetSdkVerionHigherThan21();
  if (should_run_component_did_update) {
    OnReactComponentDidUpdate(option);
  }
}

void RadonComponent::RadonReusableDiffChildren(
    RadonComponent* old_radon_component, const DispatchOption& option) {
  // OnComponentAdded lifecycle
  if (component_id_ == 0) {
    GenerateAndSetComponentId();
  }
  // flush component to update the map of component_id -> view
  if (NeedsElement() && element() && element()->is_radon_element()) {
    element()->FlushProps();
  }
  OnComponentUpdate(option);
  // continue diff the components' children
  RadonMyersDiff(old_radon_component->radon_children_, option);
  // OnReactComponentDidUpdate lifecycle
  ResetDispatchedStatus();
  OnReactComponentDidUpdate(option);
  return;
}

const std::string& RadonComponent::GetEntryName() const {
  if (entry_name_.empty() && radon_component_ != nullptr) {
    entry_name_ = radon_component_->GetEntryName();
  }
  return entry_name_;
}

bool RadonComponent::CanBeReusedBy(const RadonBase* const radon_base) const {
  if (!RadonBase::CanBeReusedBy(radon_base)) {
    return false;
  }
  // In this case, radon_base's node_type must by kRadonComponent
  // because node_type has been checked in RadonBase::CanBeReusedBy()
  const RadonComponent* const component =
      static_cast<const RadonComponent* const>(radon_base);

  // String() == String() is true
  // String().IsEqual(String()) is false
  // the init name_ of RadonComponent is String(), so should use operator==
  // instead of IsEqual()
  return name() == component->name() &&
         remove_component_element_ == component->remove_component_element_;
}

void RadonComponent::triggerNewLifecycle(const DispatchOption& option) {
  if (dispatched_) {
    page_proxy_->OnComponentAdded(this);
  }
  RadonBase::triggerNewLifecycle(option);
  if (dispatched_) {
    OnReactComponentDidUpdate(option);
  }
}

void RadonComponent::GenerateAndSetComponentId() {
  component_id_ = page_proxy_->GetNextComponentID();
  if (element() && element()->is_fiber_element()) {
    component_element()->set_component_id(ComponentStrId());
  }
}

// Essentially a wrapper of RadonComponent.
RadonListComponent::RadonListComponent(
    PageProxy* page_proxy, int tid, CSSFragment* style_sheet,
    std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
    ComponentMould* mould, lepus::Context* context, uint32_t node_index,
    int distance_from_root, const base::String& tag_name)
    : RadonComponent(page_proxy, tid, style_sheet, style_sheet_manager, mould,
                     context, node_index, tag_name),
      distance_from_root_(distance_from_root) {}

// Change the radonListComponent's parentComponent to where it gets defined.
void RadonListComponent::SetComponent(RadonComponent* component) {
  RadonComponent* curr = component;
  for (int i = 0; curr && i < distance_from_root_; i++) {
    curr = curr->component();
  }
  RadonComponent::SetComponent(curr);
}

// Same as RadonListComponent::SetComponent().
void RadonListComponent::ModifySubTreeComponent(RadonComponent* const target) {
  if (!target) {
    return;
  }
  RadonComponent* curr = target;
  for (int i = 0; curr && i < distance_from_root_; i++) {
    curr = curr->component();
  }
  RadonComponent::ModifySubTreeComponent(curr);
}

}  // namespace tasm
}  // namespace lynx
