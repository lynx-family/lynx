// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TASM_MEDIATOR_H_
#define CORE_SHELL_TASM_MEDIATOR_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/lynx_actor.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/services/performance/performance_controller.h"
#include "core/shell/lynx_card_cache_data_manager.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/native_facade.h"
#include "core/shell/tasm_platform_invoker.h"

namespace lynx {

namespace base {
class VSyncMonitor;
}

namespace shell {

using InvokeUIMethodFunction =
    base::MoveOnlyClosure<void, tasm::LynxGetUIResult, const std::string&,
                          fml::RefPtr<tasm::PropBundle>, piper::ApiCallBack>;

// ensure run on tasm thread, lifecycle manage by LynxEngine
class TasmMediator : public LynxEngine::Delegate {
 public:
  TasmMediator(
      const std::shared_ptr<LynxActor<NativeFacade>>& facade_actor,
      const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr,
      const std::shared_ptr<LynxActor<tasm::LayoutContext>>& layout_actor,
      std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker,
      const std::shared_ptr<
          LynxActor<tasm::performance::PerformanceController>>& perf_actor);

  ~TasmMediator() override;

  void SetRuntimeActor(
      const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& actor) {
    runtime_actor_ = actor;
  }

  void SetEngineActor(const std::shared_ptr<LynxActor<LynxEngine>>& actor) {
    engine_actor_ = actor;
  }

  void SetInvokeUIMethodFunction(InvokeUIMethodFunction func) {
    invoke_ui_method_func_ = std::move(func);
  }

  void ResetMediatorActor(
      const std::shared_ptr<LynxActor<tasm::LayoutContext>>& layout_actor,
      const std::shared_ptr<LynxActor<NativeFacade>>& facade_actor,
      const std::shared_ptr<
          LynxActor<tasm::performance::PerformanceController>>& perf_actor);

  void OnDataUpdated() override;

  void OnTasmFinishByNative() override;

  void OnRunPipelineFinished() override;

  void OnTemplateLoaded(const std::string& url) override;

  void OnPageUpdated(bool is_first_screen) override;

  void OnSSRHydrateFinished(const std::string& url) override;

  void OnErrorOccurred(base::LynxError error) override;
  void TriggerLepusngGc(base::closure func) override;

  void OnDynamicComponentPerfReady(const lepus::Value& perf_info) override;

  void OnConfigUpdated(const lepus::Value& data) override;

  void OnPageConfigDecoded(
      const std::shared_ptr<tasm::PageConfig>& config) override;

  void NotifyJSUpdatePageData() override;

  void OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) override;

  void InvokeResponsePromiseCallback(base::closure closure) override;

  void RecycleTemplateBundle(
      std::unique_ptr<tasm::LynxBinaryRecyclerDelegate> recycler) override;

  // synchronous
  std::string TranslateResourceForTheme(const std::string& res_id,
                                        const std::string& theme_key) override;

  void GetI18nResource(const std::string& channel,
                       const std::string& fallback_url) override;

  void OnJSSourcePrepared(
      tasm::TasmRuntimeBundle bundle, const lepus::Value& global_props,
      const std::string& page_name, tasm::PackageInstanceDSL dsl,
      tasm::PackageInstanceBundleModuleMode bundle_module_mode,
      const std::string& url,
      const std::shared_ptr<tasm::PipelineOptions>& pipeline_options) override;

  void CallJSApiCallback(piper::ApiCallBack callback) override;

  void CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                  const lepus::Value& value,
                                  bool persist = false) override;

  void RemoveJSApiCallback(piper::ApiCallBack callback) override;

  void CallPlatformCallbackWithValue(
      const std::shared_ptr<PlatformCallBackHolder>& callback,
      const lepus::Value& value) override;

  void RemovePlatformCallback(
      const std::shared_ptr<PlatformCallBackHolder>& callback) override;
  void CallJSFunction(const std::string& module_id,
                      const std::string& method_id,
                      const lepus::Value& arguments) override;

  lepus::Value TriggerLepusMethod(const std::string& method_id,
                                  const lepus::Value& arguments) override;

  void TriggerLepusMethodAsync(const std::string& method_name,
                               const lepus::Value& arguments,
                               bool is_air = false) override;

  void SendNativeCustomEvent(const std::string& name, int tag,
                             const lepus::Value& param_value,
                             const std::string& param_name) override;

  void OnDataUpdatedByNative(tasm::TemplateData data,
                             const bool reset) override;

  void OnJSAppReload(
      tasm::TemplateData data,
      const std::shared_ptr<tasm::PipelineOptions>& pipeline_options) override;

  void OnLifecycleEvent(const lepus::Value& data) override;

  void OnI18nResourceChanged(const std::string& res) override;

  void OnComponentDecoded(tasm::TasmRuntimeBundle bundle) override;

  fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() override;

  void OnCardConfigDataChanged(const lepus::Value& data) override;

  void ReportElementMemoryInfo(int64_t memory_size_bytes,
                               int element_count) override;

  void OnRuntimeGC(
      std::unordered_map<std::string, std::string> mem_info) override;

  void SetTiming(tasm::Timing timing) override;

  virtual void BindPipelineIDWithTimingFlag(
      const tasm::PipelineID& pipeline_id,
      const tasm::timing::TimingFlag& timing_flag) override;

  virtual void OnPipelineStart(
      const tasm::PipelineID& pipeline_id,
      const tasm::PipelineOrigin& pipeline_origin,
      tasm::timing::TimestampUs pipeline_start_timestamp) override;

  void RequestVsync(
      uintptr_t id,
      base::MoveOnlyClosure<void, int64_t, int64_t> callback) override;
  // delegate for class element manager
  void DispatchLayoutUpdates(
      const std::shared_ptr<tasm::PipelineOptions>& options) override;
  std::unordered_map<int32_t, tasm::LayoutInfoArray> GetSubTreeLayoutInfo(
      int32_t root_id, tasm::Viewport viewport = tasm::Viewport{}) override;

  void SetRootOnLayout(int32_t id) override;
  void OnUpdateDataWithoutChange() override;
  void OnUpdateViewport(float width, int width_mode, float height,
                        int height_mode, bool need_layout) override;
  void UpdateLynxEnvForLayoutThread(tasm::LynxEnvConfig env) override;

  // delegate for class element
  void CreateLayoutNode(int32_t id, const base::String& tag) override;
  void UpdateLayoutNodeFontSize(int32_t id, double cur_node_font_size,
                                double root_node_font_size,
                                double font_scale) override;
  void InsertLayoutNode(int32_t parent_id, int32_t child_id,
                        int index) override;
  void SendAnimationEvent(const std::string& type, int tag,
                          const lepus::Value& dict) override;
  void RemoveLayoutNodeAtIndex(int32_t parent_id, int index) override;
  void MoveLayoutNode(int32_t parent_id, int32_t child_id, int from_index,
                      int to_index) override;
  void InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id,
                              int32_t ref_id) override;
  void RemoveLayoutNode(int32_t parent_id, int32_t child_id) override;
  void DestroyLayoutNode(int32_t id) override;
  void UpdateLayoutNodeStyle(int32_t id, tasm::CSSPropertyID css_id,
                             const tasm::CSSValue& value) override;
  void ResetLayoutNodeStyle(int32_t id, tasm::CSSPropertyID css_id) override;
  void UpdateLayoutNodeAttribute(int32_t id, starlight::LayoutAttribute key,
                                 const lepus::Value& value) override;
  void SetFontFaces(const tasm::CSSFontFaceRuleMap& fontfaces) override;

  void UpdateLayoutNodeByBundle(
      int32_t id, std::unique_ptr<tasm::LayoutBundle> bundle) override;

  void UpdateLayoutNodeProps(
      int32_t id, const fml::RefPtr<tasm::PropBundle>& props) override;
  void MarkLayoutDirty(int32_t id) override;
  void AttachLayoutNodeType(
      int32_t id, const base::String& tag, bool allow_inline,
      const fml::RefPtr<tasm::PropBundle>& props) override;

  void InvokeUIMethod(tasm::LynxGetUIResult ui_result,
                      const std::string& method,
                      fml::RefPtr<tasm::PropBundle> params,
                      piper::ApiCallBack callback) override;

  void SetPageConfigForLayoutThread(
      const std::shared_ptr<tasm::PageConfig>& config) override;
  void LepusInvokeUIMethod(
      std::vector<int32_t> ui_impl_ids, const std::string& method,
      const lepus::Value& params, lepus::Context* context,
      std::unique_ptr<lepus::Value> callback_closure) override;

  void RequestLayout(
      const std::shared_ptr<tasm::PipelineOptions>& options) override;

  event::DispatchEventResult DispatchMessageEvent(
      runtime::MessageEvent event) override;

  TasmMediator(const TasmMediator&) = delete;
  TasmMediator& operator=(const TasmMediator&) = delete;
  TasmMediator(TasmMediator&&) = delete;
  TasmMediator& operator=(TasmMediator&&) = delete;

  void SetPropBundleCreator(
      const std::shared_ptr<tasm::PropBundleCreator>& creator) {
    prop_bundle_creator_ = creator;
  }

  void OnGlobalPropsUpdated(const lepus::Value& props) override;

  virtual void OnEventCapture(long target_id, bool is_catch,
                              int64_t event_id) override;

  virtual void OnEventBubble(long target_id, bool is_catch,
                             int64_t event_id) override;

  virtual void OnEventFire(long target_id, bool is_stop,
                           int64_t event_id) override;

  virtual void OnLynxEvent(const lepus::Value& event_detail) override;

  void SetPageOptions(const tasm::PageOptions& options) {
    page_options_ = options;
  }

 private:
  void InitVSyncMonitorIfNeeded();
  bool IsEmbeddedModeOn() const { return page_options_.IsEmbeddedModeOn(); }
  std::shared_ptr<LynxActor<NativeFacade>> facade_actor_;

  std::shared_ptr<LynxActor<runtime::LynxRuntime>> runtime_actor_;
  std::shared_ptr<LynxActor<tasm::LayoutContext>> layout_actor_;
  std::shared_ptr<LynxActor<LynxEngine>> engine_actor_;
  std::shared_ptr<LynxActor<tasm::performance::PerformanceController>>
      perf_actor_;

  std::shared_ptr<LynxCardCacheDataManager> card_cached_data_mgr_;

  InvokeUIMethodFunction invoke_ui_method_func_;

  // vsync monitor.
  // TODO(songshourui.null): Provide requesAnimationFrame capability to
  // ElementWorklet later by this vsync_monitor_;
  std::shared_ptr<base::VSyncMonitor> vsync_monitor_;

  std::shared_ptr<tasm::PropBundleCreator> prop_bundle_creator_;

  std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker_;

  tasm::PageOptions page_options_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_TASM_MEDIATOR_H_
