// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PAGE_PROXY_H_
#define CORE_RENDERER_PAGE_PROXY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/base_component.h"
#include "core/renderer/page_config.h"
#include "core/renderer/template_themed.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/services/ssr/client/ssr_data_update_manager.h"
#include "core/services/ssr/client/ssr_hydrate_info.h"

namespace lynx {
namespace lepus {
class Value;
}
namespace tasm {
class RadonComponent;
class RadonLazyComponent;
class RadonPage;
class RadonNode;
class RadonComponent;
class ElementManager;
class TouchEventHandler;
class LynxGetUIResult;
class TemplateAssembler;
struct NodeSelectOptions;
struct NodeSelectRoot;

struct UpdatePageOption {
  lepus::Value ToLepusValue() const;

  // Update data or reset data from native.
  // from_native would be false if the data is updated from JS.
  bool from_native = true;

  // Clear current data and update with the new given data.
  // Used in ResetData and ResetDataAndRefreshLifecycle by now.
  bool reset_page_data = false;

  // Update data first time in loadTemplate.
  bool update_first_time = false;

  // Refresh the card and component's lifecycle like a new loaded template.
  // Used only in ReloadTemplate by now.
  bool reload_template = false;

  // used in UpdateGlobalProps
  bool global_props_changed = false;

  // used in lynx.reload() api
  bool reload_from_js = false;

  // This variable records the order of native update data. Used for syncFlush
  // only.
  uint32_t native_update_data_order_ = 0;
};

enum class PrePaintingStage : uint8_t {
  // We can trigger lifecycle if and only if PrePaintingStage == kPrePaintingOFF
  kPrePaintingOFF = 0,
  // Start pre painting, this stage should be set if and only if
  // enable_pre_painting is set to true in loadTemplate
  kStartPrePainting,
  // This stage should be set when user call updateData from native after pre
  // painting.
  kStartUpdatePage,
};

class PageProxy {
 public:
  class TasmDelegate {
   public:
    TasmDelegate() = default;
    virtual ~TasmDelegate() = default;

    virtual lepus::Value &GetComponentInfoMap(
        const std::string &entry_name) = 0;
    virtual lepus::Value &GetComponentPathMap(
        const std::string &entry_name) = 0;
    virtual bool SupportComponentJS() const = 0;
  };

  PageProxy(TasmDelegate *tasm_delegate,
            std::unique_ptr<ElementManager> client_ptr,
            runtime::ContextProxy::Delegate *context_proxy_delegate);

  virtual ~PageProxy() = default;

  void Destroy();

  bool is_dry_run_ = false;

  PageProxy(const PageProxy &) = delete;
  PageProxy(PageProxy &&) = delete;

  PageProxy &operator=(const PageProxy &) = delete;
  PageProxy &operator=(PageProxy &&) = delete;

  // used in ReloadTemplate, call old components' unmount lifecycle.
  void RemoveOldComponentBeforeReload();

  // Runtime Lifecycle
  void OnComponentAdded(RadonComponent *node);
  void OnComponentRemoved(RadonComponent *node);
  void OnComponentMoved(RadonComponent *node);

  void OnComponentPropertyChanged(RadonComponent *node);
  void OnComponentDataSetChanged(RadonComponent *node,
                                 const lepus::Value &data_set);
  void OnComponentSelectorChanged(RadonComponent *node,
                                  const lepus::Value &instance);
  // for react
  void OnReactComponentCreated(RadonComponent *component,
                               const lepus::Value &props,
                               const lepus::Value &data,
                               const std::string &parent_id);
  void OnReactComponentRender(RadonComponent *component,
                              const lepus::Value &props,
                              const lepus::Value &data,
                              bool should_component_update);
  void OnReactComponentDidUpdate(RadonComponent *component);
  void OnReactComponentDidCatch(RadonComponent *component,
                                const lepus::Value &error);

  void OnReactComponentUnmount(RadonComponent *component);

  void OnReactCardDidUpdate();
  void OnReactCardRender(const lepus::Value &data,
                         bool should_component_update);

  bool UpdateGlobalProps(const lepus::Value &table, bool should_render,
                         PipelineOptions &pipeline_options);

  lepus::Value GetGlobalPropsFromTasm() const;

  void UpdateComponentData(const std::string &id, const lepus::Value &table,
                           PipelineOptions &pipeline_options);

  bool UpdateGlobalDataInternal(const lepus_value &value,
                                const UpdatePageOption &update_page_option,
                                PipelineOptions &pipeline_options);

  const lepus::Value GetComponentContextDataByKey(const std::string &id,
                                                  const std::string &key);

  bool UpdateConfig(const lepus::Value &config, lepus::Value &out,
                    bool to_refresh, PipelineOptions &pipeline_options);

  std::unique_ptr<lepus::Value> GetData();

  lepus::Value GetDataByKey(const std::vector<std::string> &keys);

  bool OnLazyBundleLoadedSuccess(TemplateAssembler *tasm,
                                 const std::string &url, uint32_t uid,
                                 int &impl_id);

  // get impl id of failed components and try to render fallback
  bool OnLazyBundleLoadedFailed(uint32_t uid, int &impl_id);

  void OnLazyBundleLoadedFromJS(const std::string &url,
                                const std::vector<std::string> &ids,
                                PipelineOptions &pipeline_options);

  void UpdateInLoadTemplate(const lepus::Value &data,
                            const UpdatePageOption &update_page_option,
                            PipelineOptions &pipeline_options);
  void ForceUpdate(const UpdatePageOption &update_page_option);

  void SetRadonPage(RadonPage *page);
  bool HasRadonPage() const { return radon_page_ != nullptr; }
  bool HasSSRRadonPage() const { return ssr_radon_page_ != nullptr; }
  RadonComponent *ComponentWithId(int component_id);
  Element *ComponentElementWithStrId(const std::string &id);
  Element *GetPageElement();

  RadonPage *Page() { return radon_page_.get(); }
  RadonPage *SSRPage() { return ssr_radon_page_.get(); }
  void ResetSSRPage();

  void set_is_updating_config(bool value) { is_updating_config_ = value; }
  bool is_updating_config() const { return is_updating_config_; }

  const std::unique_ptr<ElementManager> &element_manager() const {
    return client_;
  }

  void SetCSSVariables(const std::string &component_id,
                       const std::string &id_selector,
                       const lepus::Value &properties,
                       PipelineOptions &pipeline_options);

  std::vector<std::string> SelectComponent(const std::string &comp_id,
                                           const std::string &id_selector,
                                           const bool single) const;

  /**
   * Select elements using given options.
   * @param root root node of the select operation.
   * @param options options of node selecting.
   * @return result of selected elements.
   */
  LynxGetUIResult GetLynxUI(const NodeSelectRoot &root,
                            const NodeSelectOptions &options) const;

  // Returns an Element vector for the given selector.
  //  comp_id is the root component's id.
  //    * If id selector, must begin with '#'.
  //    * If class selector, must begin with '.'.
  //    * Others are considered to be tag selector.
  //  selector is the target elements' selector.
  //  single determines whether to find all elements that contains the selector.
  //  current_component current_component determines whether to search only in
  //  the current component.
  std::vector<Element *> SelectElements(const NodeSelectRoot &root,
                                        const NodeSelectOptions &options) const;

  Themed &themed() { return themed_; }

  lepus::Value GetConfig() { return config_; };

  bool GetEnableSavePageData() { return client_->GetEnableSavePageData(); }

  bool GetEnableComponentNullProp() {
    return client_->GetEnableComponentNullProp();
  }

  bool GetEnableCheckDataWhenUpdatePage() {
    return client_->GetEnableCheckDataWhenUpdatePage();
  }

  bool GetListNewArchitecture() { return client_->GetListNewArchitecture(); }

  bool GetListRemoveComponent() { return client_->GetListRemoveComponent(); }

  bool GetEnableReloadLifecycle() {
    return client_->GetEnableReloadLifecycle();
  }

  // get if enable new gesture
  bool GetEnableNewGesture() { return client_->GetEnableNewGesture(); }

  bool GetListEnableMoveOperation() {
    return client_->GetListEnableMoveOperation();
  }

  bool GetListEnablePlug() { return client_->GetListEnablePlug(); }

  bool GetStrictPropType() { return client_->GetStrictPropType(); }

  void SetTasmEnableLayoutOnly(bool enable_layout_only);

  void UpdateComponentInComponentMap(RadonComponent *component);

  bool GetRemoveCSSScopeEnabled() { return remove_css_scope_enabled_; }

  void SetRemoveCSSScopeEnabled(bool remove_css_scope_enabled) {
    remove_css_scope_enabled_ = remove_css_scope_enabled;
  }

  bool GetPageElementEnabled() { return page_element_enable_; }

  void SetPageElementEnabled(bool page_element_enable) {
    page_element_enable_ = page_element_enable;
  }

  PrePaintingStage GetPrePaintingStage() { return pre_painting_stage_; }

  void SetPrePaintingStage(PrePaintingStage pre_painting_stage) {
    pre_painting_stage_ = pre_painting_stage;
  }

  bool GetEnableReactOnlyPropsId() const {
    return client_->GetEnableReactOnlyPropsId();
  }

  bool GetEnableGlobalComponentMap() const {
    return client_->GetEnableGlobalComponentMap();
  }

  bool GetEnableRemoveComponentExtraData() const {
    return client_->GetEnableRemoveComponentExtraData();
  }

  lepus::Value ProcessReactPropsForJS(const lepus::Value &props) const;

  lepus::Value ProcessInitDataForJS(const lepus::Value &data);

  void FireComponentLifecycleEvent(const std::string name,
                                   RadonComponent *component,
                                   const lepus::Value &data = lepus::Value());

  bool GetComponentLifecycleAlignWithWebview() {
    return client_->GetEnableComponentLifecycleAlignWebview();
  }

  lepus::Value GetPathInfo(const NodeSelectRoot &root,
                           const NodeSelectOptions &options);

  lepus::Value GetFields(const NodeSelectRoot &root,
                         const tasm::NodeSelectOptions &options,
                         const std::vector<std::string> &fields);

  lepus::Value OnScreenMetricsSet(const lepus::Value &input);

  const std::unordered_map<int, RadonComponent *> &GetComponentMap() const {
    return component_map_;
  };

  std::unordered_map<int, RadonComponent *> &GetComponentMap() {
    return component_map_;
  };

  // SSR and Hydration related methods.
  lepus::Value GetDefaultPageData() const { return default_page_data_; }

  // SSR and Hydration related methods.
  lepus::Value GetDefaultGlobalProps() const { return default_global_props_; }

  // Initialization data for ssr service.
  void UpdateInitDataForSSRServer(const lepus::Value &page_data,
                                  const lepus::Value &system_info);

  // Hydration may be triggered immediately after lepus rendered the page if no
  // js constructor is triggered. And otherwise hydration will be triggered when
  // all pending js tasks are done.
  void HydrateOnFirstScreenIfPossible(TemplateAssembler *tasm,
                                      PipelineOptions &pipeline_options);

  // When the data used for server side rendering is the same with the current
  // client side page data. It can be assumed that the page rendered on client
  // side is identical as the one rendered on server. Therefore diff operation
  // can be skipped in this case.
  bool HydrateDataIdenticalAsSSR() const {
    return hydrate_info_.hydrate_data_identical_as_ssr_;
  }

  void RenderToBinary(const base::MoveOnlyClosure<void, RadonNode *,
                                                  tasm::TemplateAssembler *> &,
                      tasm::TemplateAssembler *);
  bool IsServerSideRendering();

  void RenderWithSSRData(TemplateAssembler *tasm, const lepus::Value &data,
                         const lepus::Value &injected_data, int32_t instance_id,
                         PipelineOptions &pipeline_options);

  void RenderWithSSRData(SSRHydrateInfo info, std::string global_event_script,
                         int32_t instance_id,
                         PipelineOptions &pipeline_options);

  void UpdateDataForSsr(std::vector<base::String> &keys_to_be_updated,
                        const lepus::Value &data,
                        PipelineOptions &pipeline_options);
  void DiffHydrationData(const lepus::Value &data);

  void ResetHydrateInfo();

  uint32_t GetNextComponentID();

  void ResetComponentId();

  bool CheckComponentExists(int component_id) const;

  bool EnableFeatureReport() const { return enable_feature_report_; };

  lepus::Value &GetGlobalComponentInfoMap(const std::string &entry_name);
  lepus::Value &GetGlobalComponentPathMap(const std::string &entry_name);

  bool EraseFromEmptyComponentMap(RadonComponent *component);
  bool InsertEmptyComponent(RadonComponent *component);

  bool IsWaitingSSRHydrate() {
    return hydrate_info_.waiting_for_hydrating_ || HasSSRRadonPage();
  }

  void OnSsrScriptReady(std::string script);

 private:
  TasmDelegate *tasm_delegate_{nullptr};

  void UpdateThemedTransMapsBeforePageUpdated();
  friend class TouchEventHandler;

  bool NeedSendTTComponentLifecycle(RadonComponent *node) const;
  bool IsReact() const;

  std::string GetParentComponentId(RadonComponent *component) const;

  void AdoptComponent(RadonComponent *component);
  bool EraseComponent(RadonComponent *component);

  using EmptyComponentMap = std::unordered_map<int, RadonLazyComponent *>;

  bool destroyed_{false};
  bool enable_feature_report_{false};
  runtime::ContextProxy::Delegate *context_proxy_delegate_ = nullptr;

  /* Be CAREFUL when you adjust the order of the declaration of following data
   * members. Make sure that the dtor of `client_` will be called after the
   * dtors of the `radon_page_` and `differentiator_` being called.
   *
   * During the dtor of `RadonNode`, the `element_` (which is an `Element`, a
   * data member of the `RadonNode`) needs to remove itself from the
   * `node_manager_ held by `client_`.
   *
   * Differentiator will use raw pointer of ElementManager, so `differentiator_`
   * should be released before `client_`.
   */
  std::unique_ptr<ElementManager> client_;

  // Hold component's element, use component id as key
  std::unordered_map<int, RadonComponent *> component_map_;
  EmptyComponentMap empty_component_map_;
  std::unique_ptr<RadonPage> radon_page_;
  lepus::Value global_props_;

  bool is_updating_config_ = false;
  bool remove_css_scope_enabled_{false};
  bool page_element_enable_{false};
  // In pre painting stage, we will not trigger any lifecycle.
  PrePaintingStage pre_painting_stage_{PrePaintingStage::kPrePaintingOFF};

  lepus::Value config_;  // cache the config
  Themed themed_;

  lepus::Value default_page_data_;
  lepus::Value default_global_props_;

  // A page constructed with server side rendering output.
  // It will be destroyed once the page get hydrated.
  std::unique_ptr<RadonPage> ssr_radon_page_;

  SSRHydrateInfo hydrate_info_;

  std::unique_ptr<ssr::SsrDataUpdateManager> ssr_data_update_manager_;

  // component id is self-increasing
  uint32_t component_id_generator_{1};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_PAGE_PROXY_H_
