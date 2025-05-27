// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/page_proxy.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_node_info.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/dom/selector/fiber_element_selector.h"
#include "core/renderer/dom/vdom/radon/node_path_info.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_lazy_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/services/ssr/client/ssr_client_utils.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

#if ENABLE_AIR
#include "core/renderer/dom/air/air_element/air_page_element.h"
#endif

namespace lynx {
namespace tasm {

lepus::Value UpdatePageOption::ToLepusValue() const {
  auto dict = lepus::Dictionary::Create();
  // When performing native data updates, the UpdatePageOption should also be
  // passed as a parameter to the LepusRuntime. Therefore, the ToLepusValue
  // function has been added to the UpdatePageOption. Currently, only
  // resetPageData and reloadTemplate will be used in Fiber, so only these two
  // parameters will be passed. If other parameters are needed in the future,
  // they will be added accordingly.
  constexpr const static char kResetPageData[] = "resetPageData";
  constexpr const static char kReloadFromJS[] = "reloadFromJS";
  constexpr const static char kReloadTemplate[] = "reloadTemplate";

  // Clear current data and update with the new given data.
  // Used in ResetData and ResetDataAndRefreshLifecycle by now.
  dict->SetValue(BASE_STATIC_STRING(kResetPageData), reset_page_data);

  // Indicate that this reloadTemplate operation was initiated by the JS API
  // lynx.reload().
  dict->SetValue(BASE_STATIC_STRING(kReloadFromJS), reload_from_js);

  // Refresh the card and component's lifecycle like a new loaded template.
  // Used only in ReloadTemplate by now.
  dict->SetValue(BASE_STATIC_STRING(kReloadTemplate), reload_template);

  // a flag for indicating current native updateData order
  dict->SetValue(BASE_STATIC_STRING(kNativeUpdateDataOrder),
                 native_update_data_order_);

  return lepus::Value(std::move(dict));
}

const lepus::Value PageProxy::GetComponentContextDataByKey(
    const std::string &id, const std::string &key) {
  // TODO: Radon support.
  if (radon_page_) {
    //    return radon_page_->GetComponentContextDataByKey(id, key);
  }
  return lepus::Value();
}

bool PageProxy::UpdateConfig(const lepus::Value &config, lepus::Value &out,
                             bool to_refresh,
                             PipelineOptions &pipeline_options) {
  if (!config.IsObject()) {
    LOGE("config not table");
    return false;
  }
  auto configSrc = config.Table();
  if (configSrc->size() == 0) {
    out = lepus_value(false);
    return true;
  }

  if (!config_.IsObject()) {
    config_ = lynx::lepus::Value(lynx::lepus::Dictionary::Create());
  }

  // Config value should be deep cloned If Config is already const.
  if (config_.IsTable() && config_.Table()->IsConst()) {
    config_ = lynx::lepus::Value::Clone(config_);
  }
  for (const auto &[key, value] : *configSrc) {
    config_.SetProperty(key, value);
  }

  auto cfgToJs = lynx::lepus::Dictionary::Create();
  cfgToJs->SetValue(BASE_STATIC_STRING(CARD_CONFIG_STR), config_);

  if (radon_page_) {
    if (themed_.hasTransConfig) {
      UpdateThemedTransMapsBeforePageUpdated();
    }
    radon_page_->UpdateConfig(config, to_refresh, pipeline_options);
  }
  out = lepus_value(std::move(cfgToJs));
  return true;
}

std::unique_ptr<lepus::Value> PageProxy::GetData() {
  if (radon_page_) {
    return radon_page_->GetPageData();
  }
#if ENABLE_AIR
  else if (element_manager()->AirRoot()) {
    return std::make_unique<lepus::Value>(
        lepus::Value::Clone(element_manager()->AirRoot()->GetData()));
  }
#endif
  return nullptr;
}

lepus::Value PageProxy::GetDataByKey(const std::vector<std::string> &keys) {
  if (radon_page_) {
    return radon_page_->GetPageDataByKey(keys);
  }
#if ENABLE_AIR
  else if (element_manager()->AirRoot()) {
    return element_manager()->AirRoot()->GetPageDataByKey(keys);
  }
#endif
  return lepus::Value();
}

void PageProxy::UpdateThemedTransMapsBeforePageUpdated() {
  int tid = 0;
  auto &mapRef = themed_.currentTransMap;
  mapRef = nullptr;
  themed_.hasAnyCurRes = themed_.hasAnyFallback = false;

  if (radon_page_) {
    tid = radon_page_->tid();
  }

  auto it = themed_.pageTransMaps.find(tid);
  if (it == themed_.pageTransMaps.end() || it->second == nullptr) {
    return;
  }

  mapRef = it->second;
  auto mapSize = mapRef->size();
  for (unsigned int i = 0; i < mapSize; ++i) {
    auto &transMap = mapRef->at(i);
    transMap.currentRes_ = nullptr;
    transMap.curFallbackRes_ = nullptr;
  }

  for (unsigned int i = 0; i < mapSize; ++i) {
    auto &transMap = mapRef->at(i);

    if (config_.IsTable()) {
      auto theme =
          config_.Table()->GetValue(BASE_STATIC_STRING(CARD_CONFIG_THEME));
      if (theme.IsTable()) {
        for (const auto &item : *theme.Table()) {
          if (transMap.name_ != item.first.str()) {
            continue;
          }
          if (item.second.IsString()) {
            auto it = transMap.resMap_.find(item.second.StdString());
            if (it != transMap.resMap_.end()) {
              transMap.currentRes_ = it->second;
              themed_.hasAnyCurRes = true;
              break;
            }
          }
        }
      }
    }

    if (transMap.currentRes_ == nullptr && !transMap.default_.empty()) {
      auto it = transMap.resMap_.find(transMap.default_);
      if (it != transMap.resMap_.end()) {
        transMap.currentRes_ = it->second;
        themed_.hasAnyCurRes = true;
      }
    }

    if (transMap.curFallbackRes_ == nullptr && !transMap.fallback_.empty()) {
      auto it = transMap.resMap_.find(transMap.fallback_);
      if (it != transMap.resMap_.end()) {
        transMap.curFallbackRes_ = it->second;
        themed_.hasAnyFallback = true;
      }
    }
  }
}

// used in ReloadTemplate, call old components' unmount lifecycle.
void PageProxy::RemoveOldComponentBeforeReload() {
  if (!radon_page_) {
    return;
  }
  for (auto &child : radon_page_->radon_children_) {
    child->OnComponentRemovedInPostOrder();
  }
}

bool PageProxy::UpdateGlobalProps(const lepus::Value &table, bool should_render,
                                  PipelineOptions &pipeline_options) {
  if (radon_page_) {
    global_props_ = table;
    return radon_page_->RefreshWithGlobalProps(table, should_render,
                                               pipeline_options);
  }
#if ENABLE_AIR
  else if (element_manager()->AirRoot()) {
    return element_manager()->AirRoot()->RefreshWithGlobalProps(table,
                                                                should_render);
  }
#endif
  return false;
}

lepus::Value PageProxy::GetGlobalPropsFromTasm() const { return global_props_; }

void PageProxy::UpdateComponentData(const std::string &id,
                                    const lepus::Value &table,
                                    PipelineOptions &pipeline_options) {
  if (radon_page_) {
    radon_page_->UpdateComponentData(id, table, pipeline_options);
  }
}

std::vector<std::string> PageProxy::SelectComponent(
    const std::string &comp_id, const std::string &id_selector,
    const bool single) const {
  std::vector<std::string> result;
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            id_selector);
  options.first_only = single;
  options.only_current_component = false;
  options.component_only = true;

  if (radon_page_) {
    auto unary_op = [](RadonBase *base) {
      return std::to_string(static_cast<RadonComponent *>(base)->ComponentId());
    };
    RadonComponent *component = radon_page_->GetComponent(comp_id);
    if (component == nullptr) {
      return result;
    }
    auto components = RadonNodeSelector::Select(component, options).nodes;
    std::transform(components.begin(), components.end(),
                   std::back_inserter(result), unary_op);
  } else if (client_ && client_->GetEnableFiberArch()) {
    auto unary_op = [](FiberElement *base) {
      return static_cast<ComponentElement *>(base)->component_id().str();
    };
    FiberElement *component =
        static_cast<FiberElement *>(element_manager()->GetComponent(comp_id));
    if (component == nullptr) {
      return result;
    }
    auto components = FiberElementSelector::Select(component, options).nodes;
    std::transform(components.begin(), components.end(),
                   std::back_inserter(result), unary_op);
  }
  return result;
}

std::vector<Element *> PageProxy::SelectElements(
    const NodeSelectRoot &root, const NodeSelectOptions &options) const {
  std::vector<Element *> targets;
  if (radon_page_) {
    auto unary_op = [](RadonBase *base) {
      return base->IsRadonComponent()
                 ? static_cast<RadonComponent *>(base)->TopLevelViewElement()
                 : base->element();
    };

    auto bases =
        RadonNodeSelector::Select(radon_page_.get(), root, options).nodes;
    std::transform(bases.begin(), bases.end(), std::back_inserter(targets),
                   unary_op);
  } else if (client_ && client_->GetEnableFiberArch()) {
    auto unary_op = [](FiberElement *base) {
      return static_cast<Element *>(base);
    };
    auto elements =
        FiberElementSelector::Select(element_manager(), root, options).nodes;
    std::transform(elements.begin(), elements.end(),
                   std::back_inserter(targets), unary_op);
  }
  return targets;
}

/*
 * Returns: vector of id of selected elements. returns empty when no node found.
 *
 * Args:
 * component_id: id of parent component given by user which we should search
 *               inside.
 * selector: selector or ref id. Currently ID, Class, ElementType and Descendant
 *           selectors are supported.
 * by_ref_id: if selector parameter is a ref id.
 * first_only: only return the first node satisfying the selector given.
 */
LynxGetUIResult PageProxy::GetLynxUI(const NodeSelectRoot &root,
                                     const NodeSelectOptions &options) const {
  if (radon_page_) {
    auto select_result =
        RadonNodeSelector::Select(radon_page_.get(), root, options);
    return select_result.PackageLynxGetUIResult();
  } else if (client_ && client_->GetEnableFiberArch()) {
    auto select_result =
        FiberElementSelector::Select(element_manager(), root, options);
    return select_result.PackageLynxGetUIResult();
  }
  return LynxGetUIResult({}, LynxGetUIResult::NODE_NOT_FOUND,
                         options.NodeIdentifierMessage());
}

void PageProxy::UpdateInLoadTemplate(const lepus::Value &data,
                                     const UpdatePageOption &update_page_option,
                                     PipelineOptions &pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "UpdateInLoadTemplate");
  // TODO(huzhanbo.luc): check if we can remove this
  if (Page()) {
    /*
     * Mock DIFF_ROOT_CREATE event in radon page here to ensure
     * that PerfCollector gets enough events to pass threshold
     * and call OnFirstLoadPerfReady.
     */
    Page()->UpdatePage(data, update_page_option, pipeline_options);
  } else {
    LOGE("LoadTemplate UpdateData page is null");
  }
}

void PageProxy::ForceUpdate(const UpdatePageOption &update_page_option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "ForceUpdate");
  lepus::Value data = lepus::Value(lepus::Dictionary::Create());
  PipelineOptions pipeline_options;
  UpdateInLoadTemplate(data, update_page_option, pipeline_options);
}

bool PageProxy::OnLazyBundleLoadedFailed(uint32_t uid, int &impl_id) {
  if (HasRadonPage()) {
    for (auto iter = empty_component_map_.begin();
         iter != empty_component_map_.end(); ++iter) {
      if (iter->second->Uid() == uid) {
        impl_id = iter->second->ImplId();
        // try to render fallback if failed.
        return iter->second->RenderFallback();
      }
    }
  }
  return false;
}

void PageProxy::OnLazyBundleLoadedFromJS(const std::string &url,
                                         const std::vector<std::string> &ids,
                                         PipelineOptions &pipeline_options) {
  if (!HasRadonPage() || ids.empty()) {
    return;
  }

  bool need_dispatch = false;
  for (const auto &comp_id : ids) {
    NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                              comp_id);
    options.first_only = false;
    options.only_current_component = false;
    options.component_only = true;
    auto components =
        RadonNodeSelector::Select(radon_page_.get(), options).nodes;
    for (auto component : components) {
      if (component && component->IsRadonLazyComponent()) {
        auto lazy_bundle = static_cast<RadonLazyComponent *>(component);
        need_dispatch = lazy_bundle->LoadLazyBundleFromJS(url) || need_dispatch;
      }
    }
  }

  if (need_dispatch) {
    // TODO(kechenglong): SetNeedsLayout if and only if needed.
    element_manager()->SetNeedsLayout();
    element_manager()->OnPatchFinish(pipeline_options);
    element_manager()->painting_context()->Flush();
  }
}

bool PageProxy::OnLazyBundleLoadedSuccess(TemplateAssembler *tasm,
                                          const std::string &url, uint32_t uid,
                                          int &impl_id) {
  if (HasRadonPage()) {
    for (auto iter = empty_component_map_.begin();
         iter != empty_component_map_.end(); ++iter) {
      auto *component = iter->second;
      if (component && component->LoadLazyBundle(url, tasm, uid)) {
        impl_id = component->ImplId();
        empty_component_map_.erase(iter);
        return true;
      }
    }
  }
  return false;
}

PageProxy::PageProxy(PageProxy::TasmDelegate *tasm_delegate,
                     std::unique_ptr<ElementManager> client_ptr,
                     runtime::ContextProxy::Delegate *context_proxy_delegate)
    : tasm_delegate_(tasm_delegate),
      context_proxy_delegate_(context_proxy_delegate),
      client_(std::move(client_ptr)) {
  // LynxEnv's get function has internal lock,
  // so we get once here and use for multi times.
  enable_feature_report_ =
      LynxEnv::GetInstance().EnableGlobalFeatureSwitchStatistic();
}

void PageProxy::SetRadonPage(RadonPage *page) {
  ResetComponentId();
  radon_page_.reset(page);
  if (themed_.hasTransConfig) {
    UpdateThemedTransMapsBeforePageUpdated();
  }
  if (radon_page_) {
    PipelineOptions pipeline_options;
    radon_page_->UpdateConfig(config_, false, pipeline_options);
  }
}

void PageProxy::ResetHydrateInfo() { hydrate_info_ = SSRHydrateInfo{}; }

void PageProxy::ResetComponentId() { component_id_generator_ = 1; }

uint32_t PageProxy::GetNextComponentID() { return component_id_generator_++; }

RadonComponent *PageProxy::ComponentWithId(int component_id) {
  auto iter = component_map_.find(component_id);
  if (iter == component_map_.end()) {
    return nullptr;
  }
  return iter->second;
}

Element *PageProxy::ComponentElementWithStrId(const std::string &id) {
  return element_manager()->GetComponent(id);
}

Element *PageProxy::GetPageElement() {
  if (element_manager()) {
    return element_manager()->root();
  }
  return nullptr;
}

void PageProxy::UpdateComponentInComponentMap(RadonComponent *component) {
  AdoptComponent(component);
}

void PageProxy::SetCSSVariables(const std::string &component_id,
                                const std::string &id_selector,
                                const lepus::Value &properties,
                                PipelineOptions &pipeline_options) {
  if (Page() && !element_manager()->GetEnableFiberElementForRadonDiff()) {
    Page()->SetCSSVariables(component_id, id_selector, properties,
                            pipeline_options);
  } else if ((client_ && client_->GetEnableFiberArch()) ||
             (Page() &&
              element_manager()->GetEnableFiberElementForRadonDiff())) {
    NodeSelectRoot root = NodeSelectRoot::ByComponentId(component_id);
    NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                              id_selector);
    options.only_current_component = false;
    auto result =
        FiberElementSelector::Select(element_manager(), root, options);
    if (result.Success()) {
      FiberElement *node = result.GetOneNode();
      node->UpdateCSSVariable(properties);
    }
  }
}

void PageProxy::Destroy() {
  destroyed_ = true;
  radon_page_.reset(nullptr);
  if (ssr_radon_page_ != nullptr) {
    ssr_radon_page_.reset(nullptr);
  }
  client_.reset(nullptr);
}

bool PageProxy::UpdateGlobalDataInternal(
    const lepus_value &value, const UpdatePageOption &update_page_option,
    PipelineOptions &pipeline_options) {
  if (Page()) {
    return Page()->UpdatePage(value, update_page_option, pipeline_options);
  }
#if ENABLE_AIR
  else if (element_manager()->AirRoot()) {
    return element_manager()->AirRoot()->UpdatePageData(
        value, update_page_option, pipeline_options);
  }
#endif
  return false;
}

void PageProxy::OnComponentPropertyChanged(RadonComponent *node) {
  if (NeedSendTTComponentLifecycle(node) &&
      tasm_delegate_->SupportComponentJS() &&
      context_proxy_delegate_ != nullptr) {
    auto args = lepus::CArray::Create();
    args->emplace_back(node->ComponentStrId());
    // properties must copy first to avoid to be marked const.
    args->emplace_back(lepus_value::ShallowCopy(node->GetProperties()));
    runtime::MessageEvent event(
        runtime::kMessageEventTypeOnComponentPropertiesChanged,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext, lepus::Value(args));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnComponentDataSetChanged(RadonComponent *node,
                                          const lepus::Value &data_set) {
  if (NeedSendTTComponentLifecycle(node) &&
      tasm_delegate_->SupportComponentJS() &&
      context_proxy_delegate_ != nullptr) {
    auto args = lepus::CArray::Create();
    args->emplace_back(node->ComponentStrId());
    // data_set must copy first to avoid to be marked const.
    args->emplace_back(lepus_value::ShallowCopy(data_set));
    runtime::MessageEvent event(
        runtime::kMessageEventTypeOnComponentDataSetChanged,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext, lepus::Value(std::move(args)));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnComponentSelectorChanged(RadonComponent *node,
                                           const lepus::Value &instance) {
  if (NeedSendTTComponentLifecycle(node) &&
      context_proxy_delegate_ != nullptr &&
      tasm_delegate_->SupportComponentJS()) {
    auto args = lepus::CArray::Create();
    args->emplace_back(node->ComponentStrId());
    // instance should be ShallowCopy first to avoid to be marked const.
    args->emplace_back(lepus_value::ShallowCopy(instance));
    runtime::MessageEvent event(
        runtime::kMessageEventTypeOnComponentSelectorChanged,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext, lepus::Value(std::move(args)));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnComponentAdded(RadonComponent *node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnComponentAdded");
  AdoptComponent(node);

  lepus::Value data = ProcessInitDataForJS(node->GetData());
  if (IsReact()) {
    if (context_proxy_delegate_ != nullptr) {
      lepus::Value props = ProcessReactPropsForJS(node->GetProperties());
      auto *comp = node->GetParentComponent();
      if (comp) {
        OnReactComponentCreated(node, props, data, comp->ComponentStrId());
      }
    }
    return;
  }

  FireComponentLifecycleEvent(kComponentCreated, node, data);
  OnComponentPropertyChanged(node);
  if (radon_page_) {
    RadonComponent *component = static_cast<RadonComponent *>(node);
    component->OnDataSetChanged();
    component->OnSelectorChanged();
  }
  if (!GetComponentLifecycleAlignWithWebview()) {
    FireComponentLifecycleEvent(kComponentAttached, node);
    FireComponentLifecycleEvent(kComponentReady, node);
  }
}

void PageProxy::OnComponentRemoved(RadonComponent *node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnComponentRemoved");
  if (destroyed_) {
    return;
  }
  if (!node->IsEmpty() && !CheckComponentExists(node->ComponentId())) {
    return;
  }

  // Erase component from component_map_/empty_component_map_, if failed,
  // return.
  if (!EraseComponent(node)) {
    return;
  }

  if (IsReact()) {
    OnReactComponentUnmount(node);
    return;
  }
  FireComponentLifecycleEvent(kComponentDetached, node);
}

void PageProxy::OnComponentMoved(RadonComponent *node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnComponentMoved");
  if (!CheckComponentExists(node->ComponentId())) {
    LOGF("component doesn't exist in OnComponentMoved");
    return;
  }
  if (NeedSendTTComponentLifecycle(node)) {
    FireComponentLifecycleEvent(kComponentMoved, node);
  }
}

void PageProxy::OnReactComponentCreated(RadonComponent *component,
                                        const lepus::Value &props,
                                        const lepus::Value &data,
                                        const std::string &parent_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnReactComponentCreated");
  if (!component->IsEmpty() &&
      pre_painting_stage_ == PrePaintingStage::kPrePaintingOFF &&
      context_proxy_delegate_ != nullptr &&
      tasm_delegate_->SupportComponentJS()) {
    // The update callback of js constructor will be forced even if there is
    // nothing to be updated, when a react lynx page is trying to find a timing
    // to hydrate itself. It has to know when the js constructors is done.
    auto args = lepus::CArray::Create();
    args->reserve(7);
    args->emplace_back(component->GetEntryName());
    args->emplace_back(component->path());
    args->emplace_back(component->ComponentStrId());
    // props and data should be ShallowCopy first to avoid to be marked const.
    args->emplace_back(lepus_value::ShallowCopy(props));
    args->emplace_back(lepus_value::ShallowCopy(data));
    args->emplace_back(parent_id);
    args->emplace_back(HasSSRRadonPage());
    runtime::MessageEvent event(
        runtime::kMessageEventTypeOnReactComponentCreated,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext, lepus::Value(std::move(args)));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnReactComponentRender(RadonComponent *component,
                                       const lepus::Value &props,
                                       const lepus::Value &data,
                                       bool should_component_update) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnReactComponentRender");
  if (!component->IsEmpty() &&
      pre_painting_stage_ == PrePaintingStage::kPrePaintingOFF &&
      context_proxy_delegate_ != nullptr &&
      tasm_delegate_->SupportComponentJS()) {
    auto args = lepus::CArray::Create();
    args->emplace_back(component->ComponentStrId());
    // props and data should be ShallowCopy first to avoid to be marked const.
    args->emplace_back(lepus_value::ShallowCopy(props));
    args->emplace_back(lepus_value::ShallowCopy(data));
    args->emplace_back(should_component_update);
    runtime::MessageEvent event(
        runtime::kMessageEventTypeOnReactComponentRender,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext, lepus::Value(std::move(args)));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnReactComponentDidUpdate(RadonComponent *component) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnReactComponentDidUpdate");
  if (!component->IsEmpty() &&
      pre_painting_stage_ == PrePaintingStage::kPrePaintingOFF &&
      context_proxy_delegate_ != nullptr &&
      tasm_delegate_->SupportComponentJS()) {
    runtime::MessageEvent event(
        runtime::kMessageEventTypeOnReactComponentDidUpdate,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext,
        lepus::Value(component->ComponentStrId()));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnReactComponentDidCatch(RadonComponent *component,
                                         const lepus::Value &error) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnReactComponentDidCatch");
  if (!component->IsEmpty() &&
      pre_painting_stage_ == PrePaintingStage::kPrePaintingOFF &&
      context_proxy_delegate_ != nullptr &&
      tasm_delegate_->SupportComponentJS()) {
    auto args = lepus::CArray::Create();
    args->emplace_back(component->ComponentStrId());
    // error should be ShallowCopy first to avoid to be marked const.
    args->emplace_back(lepus_value::ShallowCopy(error));
    runtime::MessageEvent event(
        runtime::kMessageEventTypeOnReactComponentDidCatch,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext, lepus::Value(std::move(args)));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnReactComponentUnmount(RadonComponent *component) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnReactComponentUnmount");
  if (!component->IsEmpty() &&
      pre_painting_stage_ == PrePaintingStage::kPrePaintingOFF &&
      context_proxy_delegate_ != nullptr &&
      tasm_delegate_->SupportComponentJS()) {
    runtime::MessageEvent event(
        runtime::kMessageEventTypeOnReactComponentUnmount,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext,
        lepus::Value(component->ComponentStrId()));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnReactCardRender(const lepus::Value &data,
                                  bool should_component_update) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnReactCardRender");
  if (pre_painting_stage_ != PrePaintingStage::kStartPrePainting &&
      context_proxy_delegate_ != nullptr &&
      tasm_delegate_->SupportComponentJS()) {
    // The update callback of js constructor will be forced even if there is
    // nothing to be updated, when a react lynx page is trying to find a timing
    // to hydrate itself. It has to know when the js constructors is done.
    auto args = lepus::CArray::Create();
    // data should be ShallowCopy first to avoid to be marked const.
    args->emplace_back(lepus_value::ShallowCopy(data));
    args->emplace_back(should_component_update);
    args->emplace_back(HasSSRRadonPage());
    runtime::MessageEvent event(runtime::kMessageEventTypeOnReactCardRender,
                                runtime::ContextProxy::Type::kCoreContext,
                                runtime::ContextProxy::Type::kJSContext,
                                lepus::Value(std::move(args)));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

void PageProxy::OnReactCardDidUpdate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnReactCardDidUpdate");
  if (pre_painting_stage_ == PrePaintingStage::kPrePaintingOFF &&
      context_proxy_delegate_ && tasm_delegate_->SupportComponentJS()) {
    runtime::MessageEvent event(runtime::KMessageEventTypeOnReactCardDidUpdate,
                                runtime::ContextProxy::Type::kCoreContext,
                                runtime::ContextProxy::Type::kJSContext,
                                lepus::Value());
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

lepus::Value PageProxy::ProcessReactPropsForJS(
    const lepus::Value &props) const {
  lepus::Value ret = props;
  BASE_STATIC_STRING_DECL(kPropsId, "propsId");
  const auto props_id = props.GetProperty(kPropsId);
  // in ReactLynx@1.0, we could not get propsId here in first screen
  // thus we should check if we have propsId or not
  // - If we got propsId, we could only pass propsId and a flag to JS thread
  //   JS thread will use a propsMap to get correct props
  //   for more details and how it works, see: #5778
  // - Otherwise, we just pass all props to JS thread
  if (GetEnableReactOnlyPropsId() && props_id.IsString()) {
    ret = lepus::Value(lepus::Dictionary::Create());
    BASE_STATIC_STRING_DECL(kOnlyPropsId, "$$onlyPropsId");
    ret.SetProperty(kPropsId, props_id);
    ret.SetProperty(kOnlyPropsId, lepus::Value(true));
  }
  return ret;
}

lepus::Value PageProxy::ProcessInitDataForJS(const lepus::Value &data) {
  if (!client_->GetEnableReduceInitDataCopy()) {
    return data;
  }

  if (IsReact()) {
    return data;
  }

  // For ttml, only copy Object.keys(data) to JS thread
  auto array = lepus::CArray::Create();
  ForEachLepusValue(
      data, [&array](const lepus::Value &key, const lepus::Value &value) {
        if (value.IsNil() || value.IsUndefined()) {
          // ignore null and undefined
          return;
        }
        array->push_back(key);
      });
  return lepus::Value(std::move(array));
}

void PageProxy::FireComponentLifecycleEvent(const std::string name,
                                            RadonComponent *component,
                                            const lepus::Value &data) {
  if (!component->IsEmpty() && context_proxy_delegate_ != nullptr &&
      pre_painting_stage_ == PrePaintingStage::kPrePaintingOFF &&
      tasm_delegate_->SupportComponentJS()) {
    std::string id = component->ComponentStrId();
    std::string entry_name = component->GetEntryName();
    std::string parent_id;
    if (name != kComponentDetached) {
      parent_id = GetParentComponentId(component);
    }
    auto args = lepus::CArray::Create();
    args->reserve(6);
    args->emplace_back(name);
    args->emplace_back(id);
    args->emplace_back(parent_id);
    args->emplace_back(component->path());
    args->emplace_back(entry_name);
    if (data.IsNil()) {
      lepus::Value undefined_value;
      undefined_value.SetUndefined();
      args->emplace_back(std::move(undefined_value));
    } else {
      // data must copy first to avoid to be marked const.
      args->emplace_back(lepus_value::ShallowCopy(data));
    }

    runtime::MessageEvent event(runtime::kMessageEventTypeOnComponentActivity,
                                runtime::ContextProxy::Type::kCoreContext,
                                runtime::ContextProxy::Type::kJSContext,
                                lepus::Value(std::move(args)));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

bool PageProxy::CheckComponentExists(int component_id) const {
  return component_map_.find(component_id) != component_map_.end();
}

std::string PageProxy::GetParentComponentId(RadonComponent *component) const {
  if (radon_page_) {
    RadonComponent *parent_component =
        static_cast<RadonComponent *>(component->GetParentComponent());
    if (!parent_component) {
      // in multi-layer slot, the parent_component may be nullptr
      LOGI("parent_component is nullptr in PageProxy::GetParentComponentId");
      return "";
    }
    if (parent_component->IsRadonComponent()) {
      return parent_component->ComponentStrId();
    } else if (parent_component->IsRadonPage()) {
      return PAGE_ID;
    } else {
      LOGF(
          "parent_component is not radon component or radon page in "
          "PageProxy::GetParentComponentId");
      return "";
    }
  }
  return "";
}

void PageProxy::AdoptComponent(RadonComponent *component) {
  if (!component) {
    LOGE("component is NULL in AdoptComponent" << this);
    return;
  }
  if (!component->ComponentId()) {
    LOGE("component's Id is not zero in AdoptComponent" << this);
    return;
  }

  if (component->IsRadonLazyComponent()) {
    // cast to RadonLazyComponent
    auto *lazy_bundle = static_cast<RadonLazyComponent *>(component);
    if (lazy_bundle->IsEmpty()) {
      // hold RadonLazyComponent raw ptr
      empty_component_map_[component->ComponentId()] = lazy_bundle;
    }
    lazy_bundle->OnComponentAdopted();
  }

  if (!component->IsEmpty()) {
    // hold RadonComponent raw ptr
    component_map_[component->ComponentId()] = component;
    // hold component's element raw ptr
    element_manager()->RecordComponent(component->ComponentStrId(),
                                       component->element());
  }
}

bool PageProxy::EraseFromEmptyComponentMap(RadonComponent *component) {
  if (component->IsEmpty()) {
    // cast to RadonLazyComponent
    auto *lazy_bundle = static_cast<RadonLazyComponent *>(component);

    // check if hold RadonLazyComponent raw ptr
    auto iter = empty_component_map_.find(component->ComponentId());
    if (iter != empty_component_map_.end() && iter->second == lazy_bundle) {
      empty_component_map_.erase(iter);
      return true;
    }
  }
  return false;
}

bool PageProxy::InsertEmptyComponent(RadonComponent *component) {
  if (component->IsEmpty()) {
    // cast to RadonLazyComponent
    auto *lazy_bundle = static_cast<RadonLazyComponent *>(component);
    empty_component_map_.emplace(component->ComponentId(), lazy_bundle);
    return true;
  }
  return false;
}

bool PageProxy::EraseComponent(RadonComponent *component) {
  if (!component) {
    LOGE("component is NULL in EraseComponent" << this);
    return false;
  }

  if (!EraseFromEmptyComponentMap(component)) {
    if (!component->IsEmpty()) {
      // check if hold RadonComponent raw ptr
      auto iter = component_map_.find(component->ComponentId());
      if (iter != component_map_.end() && iter->second == component) {
        component_map_.erase(iter);
        element_manager()->EraseComponentRecord(component->ComponentStrId(),
                                                component->element());
      } else {
        return false;
      }
    }
  }
  return true;
}

bool PageProxy::NeedSendTTComponentLifecycle(RadonComponent *node) const {
  if (IsReact() || node->IsEmpty() ||
      pre_painting_stage_ != PrePaintingStage::kPrePaintingOFF) {
    return false;
  }
  if (!CheckComponentExists(node->ComponentId())) {
    LOGI("component doesn't exist in PageProxy::NeedSendTTComponentLifecycle");
    return false;
  }
  return true;
}

bool PageProxy::IsReact() const {
  if (radon_page_ && radon_page_->IsReact()) {
    return true;
  }
  return false;
}

void PageProxy::SetTasmEnableLayoutOnly(bool enable_layout_only) {
  client_->SetEnableLayoutOnly(enable_layout_only);
}

lepus::Value PageProxy::GetPathInfo(const NodeSelectRoot &root,
                                    const NodeSelectOptions &options) {
  auto package_data = [](lepus::Value &&status, lepus::Value &&data) {
    auto result_dict = lepus::Dictionary::Create();
    BASE_STATIC_STRING_DECL(kStatus, "status");
    BASE_STATIC_STRING_DECL(kData, "data");
    result_dict->SetValue(kStatus, std::move(status));
    result_dict->SetValue(kData, std::move(data));
    return lepus::Value(std::move(result_dict));
  };

  LOGI("GetPathInfo by root: " << root.ToPrettyString()
                               << ", node: " << options.ToString());
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "PageProxy::GetPathInfo",
              [&](lynx::perfetto::EventContext ctx) {
                std::string info = std::string("root: ")
                                       .append(root.ToPrettyString())
                                       .append(", node: ")
                                       .append(options.ToString());
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("Info");
                debug->set_string_value(info);
              });

  if (radon_page_) {
    auto result = RadonNodeSelector::Select(radon_page_.get(), root, options);
    auto ui_result = result.PackageLynxGetUIResult();
    if (!ui_result.Success()) {
      return package_data(ui_result.StatusAsLepusValue(),
                          options.first_only
                              ? lepus::Value()
                              : lepus::Value(lepus::CArray::Create()));
    }

    lepus::Value path_result;
    if (!options.first_only) {
      auto result_array = lepus::CArray::Create();
      for (auto &node : result.nodes) {
        if (node && node->element()) {
          auto path = RadonPathInfo::PathToRoot(node);
          result_array->emplace_back(RadonPathInfo::GetNodesInfo(path));
        }
      }
      path_result.SetArray(std::move(result_array));
    } else {
      auto path = RadonPathInfo::PathToRoot(result.GetOneNode());
      path_result = RadonPathInfo::GetNodesInfo(path);
    }

    return package_data(ui_result.StatusAsLepusValue(), std::move(path_result));
  } else if (client_ && client_->GetEnableFiberArch()) {
    auto result =
        FiberElementSelector::Select(element_manager(), root, options);
    auto ui_result = result.PackageLynxGetUIResult();
    if (!ui_result.Success()) {
      return package_data(ui_result.StatusAsLepusValue(),
                          options.first_only
                              ? lepus::Value()
                              : lepus::Value(lepus::CArray::Create()));
    }

    lepus::Value path_result;
    if (!options.first_only) {
      auto result_array = lepus::CArray::Create();
      for (auto &node : result.nodes) {
        if (node) {
          auto path = FiberNodeInfo::PathToRoot(node);
          result_array->emplace_back(FiberNodeInfo::GetNodesInfo(
              path, {"tag", "id", "dataSet", "index", "class"}));
        }
      }
      path_result.SetArray(std::move(result_array));
    } else {
      auto path = FiberNodeInfo::PathToRoot(result.GetOneNode());
      path_result = FiberNodeInfo::GetNodesInfo(
          path, {"tag", "id", "dataSet", "index", "class"});
    }

    return package_data(ui_result.StatusAsLepusValue(), std::move(path_result));
  }
  return package_data(LynxGetUIResult::UnknownError(
                          "PageProxy::GetPathInfo: radon page not found")
                          .StatusAsLepusValue(),
                      lepus::Value());
}

lepus::Value PageProxy::GetFields(const NodeSelectRoot &root,
                                  const tasm::NodeSelectOptions &options,
                                  const std::vector<std::string> &fields) {
  auto package_data = [](lepus::Value &&status, lepus::Value &&data) {
    auto result_dict = lepus::Dictionary::Create();
    BASE_STATIC_STRING_DECL(kStatus, "status");
    BASE_STATIC_STRING_DECL(kData, "data");
    result_dict->SetValue(kStatus, std::move(status));
    result_dict->SetValue(kData, std::move(data));
    return lepus::Value(std::move(result_dict));
  };

  LOGI("GetFields by root: " << root.ToPrettyString()
                             << ", node: " << options.ToString());
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "PageProxy::GetFields",
              [&](lynx::perfetto::EventContext ctx) {
                std::string info = std::string("root: ")
                                       .append(root.ToPrettyString())
                                       .append(", node: ")
                                       .append(options.ToString());
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("Info");
                debug->set_string_value(info);
              });

  if (radon_page_) {
    auto result = RadonNodeSelector::Select(radon_page_.get(), root, options);
    auto ui_result = result.PackageLynxGetUIResult();
    if (!ui_result.Success()) {
      return package_data(ui_result.StatusAsLepusValue(),
                          options.first_only
                              ? lepus::Value()
                              : lepus::Value(lepus::CArray::Create()));
    }

    lepus::Value fields_result;
    if (!options.first_only) {
      auto result_array = lepus::CArray::Create();
      for (auto &node : result.nodes) {
        if (node && node->element()) {
          result_array->emplace_back(RadonPathInfo::GetNodeInfo(node, fields));
        }
      }
      fields_result.SetArray(std::move(result_array));
    } else {
      fields_result = RadonPathInfo::GetNodeInfo(result.GetOneNode(), fields);
    }
    return package_data(ui_result.StatusAsLepusValue(),
                        std::move(fields_result));
  } else if (client_ && client_->GetEnableFiberArch()) {
    auto result =
        FiberElementSelector::Select(element_manager(), root, options);
    auto ui_result = result.PackageLynxGetUIResult();
    if (!ui_result.Success()) {
      return package_data(ui_result.StatusAsLepusValue(),
                          options.first_only
                              ? lepus::Value()
                              : lepus::Value(lepus::CArray::Create()));
    }

    lepus::Value fields_result;
    if (!options.first_only) {
      auto result_array = lepus::CArray::Create();
      for (auto &node : result.nodes) {
        if (node) {
          result_array->emplace_back(FiberNodeInfo::GetNodeInfo(node, fields));
        }
      }
      fields_result.SetArray(std::move(result_array));
    } else {
      fields_result = FiberNodeInfo::GetNodeInfo(result.GetOneNode(), fields);
    }
    return package_data(ui_result.StatusAsLepusValue(),
                        std::move(fields_result));
  }
  return package_data(LynxGetUIResult::UnknownError(
                          "PageProxy::GetPathInfo: radon page not found")
                          .StatusAsLepusValue(),
                      lepus::Value());
}

lepus::Value PageProxy::OnScreenMetricsSet(const lepus::Value &input) {
  if (radon_page_) {
    return radon_page_->OnScreenMetricsSet(input);
  }
  return lepus::Value();
}

lepus::Value &PageProxy::GetGlobalComponentInfoMap(
    const std::string &entry_name) {
  return tasm_delegate_->GetComponentInfoMap(entry_name);
}

lepus::Value &PageProxy::GetGlobalComponentPathMap(
    const std::string &entry_name) {
  return tasm_delegate_->GetComponentPathMap(entry_name);
}

void PageProxy::HydrateOnFirstScreenIfPossible(
    TemplateAssembler *tasm, PipelineOptions &pipeline_options) {
  // radon ttml hydrate
  if (!IsReact() && HasSSRRadonPage() && HasRadonPage()) {
    radon_page_->Hydrate(pipeline_options);
    return;
  }

  // fiber hydrate?
  if (hydrate_info_.waiting_for_hydrating_) {
    // find list item ids.
    auto list_ids = lepus::CArray::Create();
    // List element hydrate
    for (const auto &element : hydrate_info_.list_node_ref_) {
      static_cast<ListElement *>(element.get())->Hydrate();
      list_ids->emplace_back(element->impl_id());
    }

    // Apply css sheet manager. Make
    auto css_manager =
        tasm->FindEntry(DEFAULT_ENTRY_NAME)->GetStyleSheetManager();
    if (element_manager()->GetPageElement()) {
      element_manager()->GetPageElement()->ResetSheetRecursively(css_manager);
    }

    // Call the function to hydrate in lepus.
    // Bind fiber elements with lepus by custom_hydrate_info_.
    BASE_STATIC_STRING_DECL(kSsrHydrate, "ssrHydrate");
    tasm->FindEntry(DEFAULT_ENTRY_NAME)
        ->GetVm()
        ->Call(kSsrHydrate, lepus::Value(hydrate_info_.custom_hydrate_info_),
               lepus::Value(list_ids));

    ResetHydrateInfo();

    // TODO(liting.src): Compatibility code! Removed later.
    // Due to the special process of SSR, kLayoutEnd, kUIOperationFlushEnd, and
    // kPaintEnd are not triggered during hydrate. This prevents the timing API
    // being triggered in JavaScript during hydration.Therefore, adding this
    // compatibility code here is necessary.
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLayoutEnd);
    tasm::TimingCollector::Instance()->Mark(
        tasm::timing::kLayoutUiOperationExecuteEnd);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kPaintEnd);
  }
}

void PageProxy::RenderToBinary(
    const base::MoveOnlyClosure<void, RadonNode *, tasm::TemplateAssembler *>
        &binarizer,
    tasm::TemplateAssembler *template_assembler) {
  binarizer(radon_page_.get(), template_assembler);
}

bool PageProxy::IsServerSideRendering() {
#if BUILD_SSR_SERVER_RUNTIME
  return true;
#else
  return false;
#endif
}

void PageProxy::UpdateDataForSsr(std::vector<base::String> &keys_updated,
                                 const lepus::Value &dict,
                                 PipelineOptions &pipeline_options) {
  if (ssr_data_update_manager_ == nullptr) {
    LOGE("ssr data update manager is nullptr");
    return;
  }

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "UpdateDataForSsr");

  // update placeholders in DOM and fresh UI
  ssr_data_update_manager_->UpdateDomIfUpdated(keys_updated, dict);

  // TODO(kechenglong): SetNeedsLayout if and only if needed.
  element_manager()->SetNeedsLayout();
  element_manager()->OnPatchFinish(pipeline_options);

  // update placeholders in script and re-execute script
  const lepus::Value &script_value =
      ssr_data_update_manager_->GetScriptIfUpdated(keys_updated, dict);
  if (!script_value.IsEmpty()) {
    std::string script_str = script_value.ToString();
    OnSsrScriptReady(std::move(script_str));
  }

  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void PageProxy::RenderWithSSRData(TemplateAssembler *tasm,
                                  const lepus::Value &ssr_out_value,
                                  const lepus::Value &injected_data,
                                  int32_t instance_id,
                                  PipelineOptions &pipeline_options) {
  DispatchOption option(this);
  // Reset the status of hydration.
  hydrate_info_ = SSRHydrateInfo{};

  // All the information of the dom will be constructed by ssr re-constructor.
  ssr_radon_page_ = std::make_unique<RadonPage>(
      this, 1, nullptr, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
      nullptr, nullptr);

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "SSR::CreateDom");

  tasm::TimingCollector::Instance()->Mark(tasm::timing::kCreateVDomStartSSR);

  ssr_data_update_manager_ = std::make_unique<ssr::SsrDataUpdateManager>();

  ssr::SSRRenderUtils::ReconstructDom(
      ssr_out_value, this, ssr_radon_page_.get(), injected_data,
      ssr_data_update_manager_.get(),
      [this](lepus::Value page_data, lepus::Value global_props) {
        default_page_data_ = page_data;
        default_global_props_ = page_data;
      });

  tasm::TimingCollector::Instance()->Mark(tasm::timing::kCreateVDomEndSSR);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // ssr radon page dispatch.
  DispatchOption dispatch_option(this);
  dispatch_option.need_update_element_ = true;

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "SSR::Dispatch");

  tasm::TimingCollector::Instance()->Mark(tasm::timing::kDispatchStartSSR);

  element_manager()->painting_context()->MarkUIOperationQueueFlushTiming(
      tasm::timing::kPaintingUiOperationExecuteStart,
      pipeline_options.pipeline_id);

  ssr_radon_page_->Dispatch(dispatch_option);

  tasm::TimingCollector::Instance()->Mark(tasm::timing::kDispatchEndSSR);

  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  pipeline_options.is_first_screen = true;
  // TODO(kechenglong): SetNeedsLayout if and only if needed.
  element_manager()->SetNeedsLayout();
  element_manager()->OnPatchFinish(pipeline_options);

  ssr::SSRRenderUtils::ProcessSsrScriptIfNeeded(
      ssr_out_value, this, injected_data, ssr_data_update_manager_.get());
}

void PageProxy::RenderWithSSRData(SSRHydrateInfo info,
                                  std::string global_event_script,
                                  int32_t instance_id,
                                  PipelineOptions &pipeline_options) {
  hydrate_info_ = std::move(info);

  pipeline_options.is_first_screen = true;
  element_manager()->OnPatchFinish(pipeline_options);

  // script
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SSR::ProcessScript");
  if (!global_event_script.empty()) {
    OnSsrScriptReady(std::move(global_event_script));
  }
}

void PageProxy::UpdateInitDataForSSRServer(const lepus::Value &page_data,
                                           const lepus::Value &system_info) {
  default_page_data_ = page_data;
  if (HasRadonPage()) {
    radon_page_->UpdateSystemInfo(system_info);
  }
}

void PageProxy::DiffHydrationData(const lepus::Value &data) {
  if (HasSSRRadonPage()) {
    hydrate_info_.hydrate_data_identical_as_ssr_ =
        data.IsEqual(default_page_data_);
  }
}

void PageProxy::ResetSSRPage() {
  ssr_radon_page_.reset();
  ssr_data_update_manager_.reset();
}

void PageProxy::OnSsrScriptReady(std::string script) {
  if (context_proxy_delegate_) {
    runtime::MessageEvent event(runtime::kMessageEventTypeOnSsrScriptReady,
                                runtime::ContextProxy::Type::kCoreContext,
                                runtime::ContextProxy::Type::kJSContext,
                                lepus::Value(std::move(script)));
    context_proxy_delegate_->DispatchMessageEvent(std::move(event));
  }
}

}  // namespace tasm
}  // namespace lynx
