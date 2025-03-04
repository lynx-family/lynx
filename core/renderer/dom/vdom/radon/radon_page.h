// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_PAGE_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_PAGE_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/css/css_fragment.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/page_proxy.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/template_bundle/template_codec/moulds.h"

namespace lynx {
namespace tasm {

class RadonPage : public RadonComponent {
 public:
  RadonPage(PageProxy *proxy, int tid, CSSFragment *style_sheet,
            std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
            PageMould *mould, lepus::Context *context);
  virtual ~RadonPage();

  void UpdateComponentData(const std::string &id, const lepus::Value &table,
                           PipelineOptions &pipeline_options);
  bool UpdatePage(const lepus::Value &table,
                  const UpdatePageOption &update_page_option,
                  PipelineOptions &pipeline_options);
#if ENABLE_TRACE_PERFETTO
  std::string ConcatUpdateDataInfo(const RadonComponent *comp,
                                   const lepus::Value &table) const;
#endif
  std::string ConcatenateTableKeys(const lepus::Value &table) const;
  void DeriveFromMould(ComponentMould *data) override;

  virtual void DispatchSelf(const DispatchOption &) override;
  virtual void Dispatch(const DispatchOption &) override;
  virtual void DispatchForDiff(const DispatchOption &) override;
  // for remove component element
  virtual bool NeedsElement() const override { return true; }
  virtual bool UpdateConfig(const lepus::Value &config, bool to_refresh,
                            PipelineOptions &pipeline_options);
  void UpdateSystemInfo(const lynx::lepus::Value &config);
  void Refresh(const DispatchOption &,
               PipelineOptions &pipeline_options) override;
  void SetCSSVariables(const std::string &component_id,
                       const std::string &id_selector,
                       const lepus::Value &properties,
                       PipelineOptions &pipeline_options);

  virtual bool IsPageForBaseComponent() const override { return true; }
  virtual CSSFragment *GetStyleSheetBase(AttributeHolder *holder) override;

  bool NeedsExtraData() const override;

  std::unique_ptr<lepus::Value> GetPageData();

  lepus::Value GetPageDataByKey(const std::vector<std::string> &keys);

  inline void SetEnableSavePageData(bool enable) {
    enable_save_page_data_ = enable;
  }

  inline void SetEnableCheckDataWhenUpdatePage(bool option) {
    enable_check_data_when_update_page_ = option;
  }

  bool RefreshWithGlobalProps(const lepus::Value &table, bool should_render,
                              PipelineOptions &pipeline_options);

  RadonComponent *GetComponent(const std::string &comp_id);

  virtual const std::string &GetEntryName() const override;

  PageProxy *proxy_ = nullptr;
  std::vector<RadonComponent *> radon_component_dispatch_order_;

  void TriggerComponentLifecycleUpdate(const std::string name);
  void ResetComponentDispatchOrder();
  void CollectComponentDispatchOrder(RadonBase *radon_node);

  RadonPage(const RadonPage &) = delete;
  RadonPage(RadonPage &&) = delete;

  RadonPage &operator=(const RadonPage &) = delete;
  RadonPage &operator=(RadonPage &&) = delete;

  lepus::Value OnScreenMetricsSet(const lepus::Value &input);
  void SetScreenMetricsOverrider(const lepus::Value &overrider);

  // Bind the page logic to the page reconstructed from ssr data
  void Hydrate(PipelineOptions &pipeline_options);

 protected:
  virtual fml::RefPtr<Element> CreateFiberElement() override;
  void OnReactComponentDidUpdate(const DispatchOption &option) override;
  virtual void triggerNewLifecycle(const DispatchOption &option) override;

 private:
  bool UpdatePageData(const std::string &name, const lepus::Value &value,
                      const bool update_top_var = false);
  bool ResetPageData();
  bool PrePageRender(const lepus::Value &data,
                     const UpdatePageOption &update_page_option);
  bool PrePageRenderReact(const lepus::Value &data,
                          const UpdatePageOption &update_page_option);
  bool PrePageRenderTT(const lepus::Value &data,
                       const UpdatePageOption &update_page_option);
  bool ForcePreprocessPageData(const lepus::Value &updated_data,
                               lepus::Value &merged_data);
  bool ShouldKeepPageData();
  bool enable_save_page_data_{false};
  bool enable_check_data_when_update_page_{true};
  lepus::Value get_override_screen_metrics_function_;

 private:
  void AttachSSRPageElement(RadonPage *ssr_page);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_PAGE_H_
