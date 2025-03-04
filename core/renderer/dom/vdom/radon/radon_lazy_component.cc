// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/vdom/radon/radon_lazy_component.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/common/js_error_reporter.h"

namespace lynx {
namespace tasm {

uint32_t RadonLazyComponent::uid_generator_ = 0;

RadonLazyComponent::RadonLazyComponent(
    TemplateAssembler* tasm, const std::string& entry_name,
    PageProxy* page_proxy, int tid, CSSFragment* style_sheet,
    std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
    ComponentMould* mould, lepus::Context* context, uint32_t node_index,
    const lepus::Value& global_props, const base::String& tag_name)
    : RadonComponent(page_proxy, tid, style_sheet, style_sheet_manager, mould,
                     context, node_index, tag_name),
      tasm_(tasm),
      uid_(++uid_generator_) {
  node_type_ = kRadonLazyComponent;
  entry_name_ = entry_name;
  UpdateDynamicCompTopLevelVariables(mould, global_props);
}

RadonLazyComponent::RadonLazyComponent(TemplateAssembler* tasm,
                                       const std::string& entry_name,
                                       PageProxy* page_proxy, int tid,
                                       uint32_t node_index,
                                       const base::String& tag_name)
    : RadonComponent(page_proxy, tid, nullptr, nullptr, nullptr, nullptr,
                     node_index, tag_name),
      tasm_(tasm),
      uid_(++uid_generator_) {
  node_type_ = kRadonLazyComponent;
  entry_name_ = entry_name;
  SetPath(base::String());
}

RadonLazyComponent::RadonLazyComponent(const RadonLazyComponent& node,
                                       PtrLookupMap& map)
    : RadonComponent(node, map),
      need_send_event_(node.need_send_event_),
      tasm_(node.tasm_),
      uid_(node.Uid()),
      is_js_component_(node.is_js_component_) {
  node_type_ = kRadonLazyComponent;
}

void RadonLazyComponent::InitLazyComponent(
    CSSFragment* style_sheet,
    std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
    ComponentMould* mould, lepus::Context* context) {
  context_ = context;
  mould_ = mould;
  style_sheet_manager_ = std::move(style_sheet_manager);
  intrinsic_style_sheet_ = style_sheet;
  DeriveFromMould(mould);
}

void RadonLazyComponent::SetGlobalProps(const lepus::Value& global_props) {
  UpdateDynamicCompTopLevelVariables(mould_, global_props);
}

void RadonLazyComponent::UpdateDynamicCompTopLevelVariables(
    ComponentMould* data, const lepus::Value& global_props) {
  if (data == nullptr || context_ == nullptr) {
    return;
  }
  auto init_data = data->data();
  if (!init_data.IsObject()) {
    return;
  }

  ForEachLepusValue(init_data,
                    [this](const lepus::Value& key, const lepus::Value& value) {
                      context_->UpdateTopLevelVariable(key.StdString(), value);
                    });

  // update for SystemInfo
  UpdateSystemInfoToContext(GenerateSystemInfo(nullptr));
  // update for globalProps
  UpdateGlobalProps(global_props);
}

void RadonLazyComponent::UpdateSystemInfoToContext(
    const lepus::Value& system_info) {
  auto kSystemInfo_str = BASE_STATIC_STRING(kSystemInfo);
  context_->UpdateTopLevelVariable(kSystemInfo_str.str(), system_info);
  context_->SetPropertyToLynx(kSystemInfo_str, system_info);
}

void RadonLazyComponent::DeriveFromMould(ComponentMould* data) {
  // In the case of LazyComponent, DerivedFromMould maybe called after
  // setProps(Async Mode) We should merge the initProps and incoming properties.
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyComponent::DeriveFromMould");
  if (data != nullptr) {
    init_properties_ =
        lepus::Value::Clone(data->properties(), IsInLepusNGContext());
    init_data_ = data->data();

    if (!init_properties_.IsObject()) {
      init_properties_ = lepus::Value(lepus::Dictionary::Create());
    }

    if (!init_data_.IsObject()) {
      init_data_ = lepus::Value(lepus::Dictionary::Create());
    }

    lepus::Value::MergeValue(init_properties_, properties_);
    lepus::Value::MergeValue(init_data_, data_);

    data_ = init_data_;
    properties_ = init_properties_;
    ExtractExternalClass(data);
  }

  // make sure the data is table
  if (!data_.IsObject()) {
    data_ = lepus::Value(lepus::Dictionary::Create());
  }

  if (!properties_.IsObject()) {
    properties_ = lepus::Value(lepus::Dictionary::Create());
  }

  UpdateSystemInfo(GenerateSystemInfo(nullptr));
}

bool RadonLazyComponent::SetContext(TemplateAssembler* tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyComponent::SetContext");
  auto entry = tasm->FindTemplateEntry(entry_name_);
  if (entry == nullptr) {
    return false;
  }
  // For lazy component entry,
  // lazy_bundle_moulds[0] must be itself.
  auto cm_it = entry->lazy_bundle_moulds().find(0);
  if (cm_it == entry->lazy_bundle_moulds().end()) {
    return false;
  }
  ComponentMould* cm = cm_it->second.get();
  context_ = entry->GetVm().get();
  mould_ = cm;
  DeriveFromMould(cm);
  UpdateDynamicCompTopLevelVariables(cm, tasm->GetGlobalProps());
  style_sheet_.reset();
  style_sheet_manager_ = entry->GetStyleSheetManager();
  if (element() && element()->is_fiber_element()) {
    component_element()->SetComponentCSSID(cm->css_id());
    component_element()->set_style_sheet_manager(style_sheet_manager_);
  }
  intrinsic_style_sheet_ =
      entry->GetStyleSheetManager()->GetCSSStyleSheetForComponent(cm->css_id());
  SetPath(cm->path());
  if (tasm->GetPageDSL() == PackageInstanceDSL::REACT) {
    SetGetDerivedStateFromErrorProcessor(tasm->GetComponentProcessorWithName(
        path().str(), REACT_ERROR_PROCESS_LIFECYCLE, context_->name()));
  }
  SetGetDerivedStateFromPropsProcessor(tasm->GetComponentProcessorWithName(
      path().str(), REACT_PRE_PROCESS_LIFECYCLE, context_->name()));
  SetShouldComponentUpdateProcessor(tasm->GetComponentProcessorWithName(
      path().str(), REACT_SHOULD_COMPONENT_UPDATE, context_->name()));
  return true;
}

void RadonLazyComponent::RenderRadonComponent(RenderOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyComponent::RenderEntrance");
  if (!IsEmpty()) {
    lepus::Value p1(this);
    lepus::Value p4(option.recursively);
    BASE_STATIC_STRING_DECL(kRenderEntranceDynamicComponent,
                            "$renderEntranceDynamicComponent");
    context_->Call(kRenderEntranceDynamicComponent, p1, data_, properties_, p4);
  }
}

// TODO(zhoupeng.z): There is no need to pass tasm as RadonLazyComponent
// holds tasm_ itself. So does SetContext(). Fix it later.
bool RadonLazyComponent::LoadLazyBundle(const std::string& url,
                                        TemplateAssembler* tasm,
                                        const uint32_t uid) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyComponent::LoadLazyBundle");
  if (url != entry_name_ || uid != uid_) {
    return false;
  }

  if (!IsEmpty()) {
    LOGE("Unexpected RadonLazyComponent: "
         << entry_name_ << ", id: " << ComponentId()
         << ", unempty component was reloaded incorrectly.");
    return false;
  }

  return LoadLazyBundleInternal();
};

bool RadonLazyComponent::LoadLazyBundleInternal() {
  if (!SetContext(tasm_)) {
    LOGE("Unexpected RadonLazyComponent: "
         << entry_name_ << ", id: " << ComponentId()
         << ", empty component fails to be set context.");
    return false;
  }

  SetLazyBundleState(
      LazyBundleState::STATE_SUCCESS,
      lazy_bundle::ConstructSuccessMessageForMTS(entry_name_, true));

  DispatchForRender();
  fallback_.reset(nullptr);
  return true;
}

bool RadonLazyComponent::LoadLazyBundleFromJS(const std::string& url) {
  if (!is_js_component_) {
    constexpr char kErrorMsg[] =
        "The entry name of target lazy component cannot be rewritten by js.";
    constexpr char kSuggestion[] =
        "Please review your code: the lazy components whose url is set by "
        "JS should not have the \"is\" attribute.";
    auto error = lynx::base::LynxError(
        error::E_LAZY_BUNDLE_LOAD_BAD_RESPONSE, std::string(kErrorMsg),
        std::string(kSuggestion), base::LynxErrorLevel::Error);
    common::FormatErrorUrl(error, entry_name_);
    error.AddContextInfo("component_url_from_js", url);
    lynx::base::ErrorStorage::GetInstance().SetError(std::move(error));
    return false;
  }

  entry_name_ = url;
  return LoadLazyBundleInternal();
}

void RadonLazyComponent::DispatchForRender() {
  PreRender(RenderType::FirstRender);
  dispatched_ = false;
  DispatchOption option(page_proxy_);
  // radon diff
  DispatchForDiff(option);
}

const std::string& RadonLazyComponent::GetEntryName() const {
  return entry_name_;
}

lepus::Value& RadonLazyComponent::GetComponentInfoMap(
    const std::string& entry_name) {
  if (IsEmpty()) {
    // lazy-bundle is loaded failed.
    return component_info_map_;
  }
  return RadonComponent::GetComponentInfoMap(entry_name);
}

lepus::Value& RadonLazyComponent::GetComponentPathMap(
    const std::string& entry_name) {
  if (IsEmpty()) {
    // lazy-bundle is loaded failed.
    return component_path_map_;
  }
  return RadonComponent::GetComponentPathMap(entry_name);
}

bool RadonLazyComponent::UpdateGlobalProps(const lepus::Value& table) {
  if (!table.IsObject()) {
    return false;
  }
  auto global_props = table.ToLepusValue(true);
  RadonComponent::UpdateGlobalProps(global_props);
  auto kGlobalPropsKey_str = BASE_STATIC_STRING(kGlobalPropsKey);
  context_->UpdateTopLevelVariable(kGlobalPropsKey_str.str(), global_props);
  context_->SetPropertyToLynx(kGlobalPropsKey_str, global_props);
  return true;
}

bool RadonLazyComponent::CanBeReusedBy(
    const RadonBase* const radon_base) const {
  if (!RadonComponent::CanBeReusedBy(radon_base)) {
    return false;
  }
  // In this case, radon_base's node_type must by kRadonLazyComponent
  // because node_type has been checked in RadonBase::CanBeReusedBy()
  const RadonLazyComponent* const component =
      static_cast<const RadonLazyComponent* const>(radon_base);

  if (is_js_component_ && component->is_js_component_) {
    return true;
  }

  return entry_name_ == component->GetEntryName() &&
         IsEmpty() == component->IsEmpty() &&
         remove_component_element_ == component->remove_component_element_;
}

void RadonLazyComponent::SetProperties(const base::String& key,
                                       const lepus::Value& value,
                                       bool strict_prop_type) {
  auto properties = value.ToLepusValue(true);
  RadonComponent::SetProperties(key, properties, strict_prop_type);
}

void RadonLazyComponent::SetData(const base::String& key,
                                 const lepus::Value& value) {
  auto data = value.ToLepusValue(true);
  RadonComponent::SetData(key, data);
}

bool RadonLazyComponent::NeedsExtraData() const {
  // remove lazy component's extra data
  // only when component "enableRemoveComponentExtraData: true"
  if (remove_extra_data_ == BooleanProp::TrueValue) {
    return false;
  }
  return true;
}

RadonLazyComponent* RadonLazyComponent::CreateRadonLazyComponent(
    TemplateAssembler* tasm, const std::string& url, const base::String& name,
    int tid, uint32_t index) {
  auto* comp =
      new RadonLazyComponent(tasm, url, tasm->page_proxy(), tid, index);
  auto entry = tasm->RequireTemplateEntry(comp, url);
  if (entry != nullptr) {
    auto cm_it = entry->lazy_bundle_moulds().find(0);
    if (cm_it != entry->lazy_bundle_moulds().end()) {
      ComponentMould* cm = cm_it->second.get();
      auto context = entry->GetVm().get();
      comp->InitLazyComponent(nullptr, entry->GetStyleSheetManager(), cm,
                              context);
      comp->SetPath(cm->path());
      comp->SetGlobalProps(tasm->GetGlobalProps());

      if (comp->GetDSL() == PackageInstanceDSL::REACT) {
        comp->SetGetDerivedStateFromErrorProcessor(
            tasm->GetComponentProcessorWithName(comp->path().str(),
                                                REACT_ERROR_PROCESS_LIFECYCLE,
                                                context->name()));
      }

      comp->SetGetDerivedStateFromPropsProcessor(
          tasm->GetComponentProcessorWithName(comp->path().str(),
                                              REACT_PRE_PROCESS_LIFECYCLE,
                                              context->name()));
      comp->SetShouldComponentUpdateProcessor(
          tasm->GetComponentProcessorWithName(comp->path().str(),
                                              REACT_SHOULD_COMPONENT_UPDATE,
                                              context->name()));
    }
  }
  comp->SetName(name);
  comp->SetDSL(tasm->GetPageDSL());
  return comp;
}

void RadonLazyComponent::OnComponentAdopted() {
  if (need_send_event_ && state_ != LazyBundleState::STATE_UNKNOWN && tasm_) {
    tasm_->SendLazyBundleBindEvent(entry_name_,
                                   state_ == LazyBundleState::STATE_FAIL
                                       ? lazy_bundle::kEventFail
                                       : lazy_bundle::kEventSuccess,
                                   event_msg_, ImplId());
    need_send_event_ = state_ == LazyBundleState::STATE_FAIL;
  }
}

void RadonLazyComponent::SwapElement(
    const std::unique_ptr<RadonBase>& old_radon_base,
    const DispatchOption& option) {
  auto old_radon_component =
      static_cast<RadonLazyComponent*>(old_radon_base.get());
  // Prevent sending events on every render
  this->need_send_event_ = old_radon_component->need_send_event_;

  if (is_js_component_) {
    entry_name_ = old_radon_component->entry_name_;
    SetContext(tasm_);
  }

  RadonComponent::SwapElement(old_radon_base, option);
}

void RadonLazyComponent::CreateAndAdoptFallback(
    std::unique_ptr<RadonPlug> plug) {
  auto fallback_slot = new RadonSlot(plug->plug_name());
  AddChild(std::unique_ptr<RadonBase>(fallback_slot));
  AdoptPlugToSlot(fallback_slot, std::move(plug));
}

void RadonLazyComponent::AddFallback(std::unique_ptr<RadonPlug> fallback) {
  // lazy component loaded successfully does not need fallback
  if (!fallback || !IsEmpty()) {
    return;
  }

  switch (state_) {
    case LazyBundleState::STATE_UNKNOWN:
      fallback_ = std::move(fallback);
      break;
    case LazyBundleState::STATE_FAIL:
      CreateAndAdoptFallback(std::move(fallback));
      break;
    default:
      break;
  }
}

bool RadonLazyComponent::RenderFallback() {
  if (fallback_) {
    CreateAndAdoptFallback(std::move(fallback_));
    DispatchForRender();
    return true;
  }
  return false;
}

}  // namespace tasm
}  // namespace lynx
