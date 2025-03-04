// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_ENGINE_H_
#define CORE_SHELL_LYNX_ENGINE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/no_destructor.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/data/template_data.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/page_config.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "core/runtime/piper/js/update_data_type.h"
#include "core/shell/common/platform_call_back.h"
#include "core/shell/lynx_card_cache_data_manager.h"
#include "core/shell/tasm_operation_queue.h"

namespace lynx {
namespace shell {

class LynxEngine {
 public:
  class Delegate : public tasm::ElementManager::Delegate,
                   public tasm::TemplateAssembler::Delegate {
   public:
    Delegate() = default;
    ~Delegate() override = default;

    virtual void Init() {}
    virtual void NotifyJSUpdatePageData() = 0;
    virtual void BindPipelineIDWithTimingFlag(
        const tasm::PipelineID& pipeline_id,
        const tasm::timing::TimingFlag& timing_flag) override = 0;
    virtual void OnPipelineStart(
        const tasm::PipelineID& pipeline_id,
        const tasm::PipelineOrigin& pipeline_origin,
        tasm::timing::TimestampUs pipeline_start_timestamp) override = 0;
  };

  explicit LynxEngine(
      const std::shared_ptr<tasm::TemplateAssembler>& tasm,
      std::unique_ptr<Delegate> delegate,
      const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr,
      int32_t instance_id)
      : tasm_(tasm),
        delegate_(std::move(delegate)),
        card_cached_data_mgr_(card_cached_data_mgr),
        instance_id_(instance_id) {}
  ~LynxEngine();

  void Init();

  void LoadTemplate(const std::string& url, std::vector<uint8_t> source,
                    const std::shared_ptr<tasm::TemplateData>& template_data,
                    tasm::PipelineOptions pipeline_options,
                    const bool enable_pre_painting = false,
                    bool enable_recycle_template_bundle = false);

  void LoadTemplateBundle(
      const std::string& url, tasm::LynxTemplateBundle template_bundle,
      const std::shared_ptr<tasm::TemplateData>& template_data,
      tasm::PipelineOptions pipeline_options,
      const bool enable_pre_painting = false,
      bool enable_dump_element_tree = false);

  void LoadSSRData(std::vector<uint8_t> source,
                   const std::shared_ptr<tasm::TemplateData>& template_data,
                   tasm::PipelineOptions pipeline_options);

  void UpdateDataByParsedData(const std::shared_ptr<tasm::TemplateData>& data,
                              uint32_t native_update_data_order,
                              tasm::PipelineOptions pipeline_options);

  void ResetDataByParsedData(const std::shared_ptr<tasm::TemplateData>& data,
                             uint32_t native_update_data_order,
                             tasm::PipelineOptions pipeline_options);

  void UpdateMetaData(const std::shared_ptr<tasm::TemplateData>& data,
                      const lepus::Value& global_props,
                      uint32_t native_update_data_order,
                      tasm::PipelineOptions pipeline_options);

  void ReloadTemplate(const std::shared_ptr<tasm::TemplateData>& data,
                      const lepus::Value& global_props,
                      uint32_t native_update_data_order,
                      tasm::PipelineOptions pipeline_options);

  void UpdateConfig(const lepus::Value& config,
                    tasm::PipelineOptions pipeline_options);

  void UpdateGlobalProps(const lepus::Value& global_props,
                         tasm::PipelineOptions pipeline_options);

  void SetFontScale(float scale);

  void SetContextHasAttached();

  void SetPlatformConfig(std::string platform_config_json_string);

  void SetAnimationsPending(bool need_pending_ui_op);

  void UpdateFontScale(float scale);

  void UpdateScreenMetrics(float width, float height, float scale);

  void UpdateViewport(float width, int32_t width_mode, float height,
                      int32_t height_mode, bool need_layout = true);

  void SyncFetchLayoutResult();

  void SendAirPageEvent(const std::string& name, const lepus_value& params);

  void SendCustomEvent(const std::string& name, int32_t tag,
                       const lepus::Value& params,
                       const std::string& params_name);

  void SendTouchEvent(const std::string& name, const tasm::EventInfo& info);

  void SendGestureEvent(int tag, int gesture_id, std::string name,
                        const lepus::Value& params);

  void OnPseudoStatusChanged(int32_t id, int32_t pre_status,
                             int32_t current_status);

  void SendBubbleEvent(const std::string& name, int32_t tag,
                       lepus::DictionaryPtr dict);

  void SendGlobalEventToLepus(const std::string& name,
                              const lepus_value& params);

  void TriggerEventBus(const std::string& name, const lepus_value& params);

  void DidLoadComponentFromJS(
      tasm::LazyBundleLoader::CallBackInfo callback_info);

  void DidPreloadComponent(
      lynx::tasm::LazyBundleLoader::CallBackInfo callback_info);

  void DidLoadComponent(lynx::tasm::LazyBundleLoader::CallBackInfo);

  std::unique_ptr<lepus_value> GetCurrentData();

  lepus::Value GetPageDataByKey(const std::vector<std::string>& keys);

  tasm::ListNode* GetListNode(int32_t tag);

  void ScrollByListContainer(int32_t tag, float content_offset_x,
                             float content_offset_y, float original_x,
                             float original_y);

  void ScrollToPosition(int32_t tag, int index, float offset, int align,
                        bool smooth);

  void ScrollStopped(int32_t tag);

  std::unordered_map<std::string, std::string> GetAllJsSource();

  void UpdateDataByJS(runtime::UpdateDataTask task);

  void UpdateBatchedDataByJS(std::vector<runtime::UpdateDataTask> tasks,
                             uint64_t update_task_id);

  void TriggerComponentEvent(const std::string& event_name,
                             const lepus::Value& msg);

  void TriggerLepusGlobalEvent(const std::string& event_name,
                               const lepus::Value& msg);
  void TriggerWorkletFunction(std::string component_id,
                              std::string worklet_module_name,
                              std::string method_name, lepus::Value args,
                              piper::ApiCallBack callback);

  void CallJSFunctionInLepusEvent(const std::string& component_id,
                                  const std::string& name,
                                  const lepus::Value& params);

  void InvokeLepusCallback(const int32_t callback_id,
                           const std::string& entry_name,
                           const lepus::Value& data);

  void InvokeLepusComponentCallback(const int64_t callback_id,
                                    const std::string& entry_name,
                                    const lepus::Value& data);

  void UpdateComponentData(runtime::UpdateDataTask task);

  void SelectComponent(const std::string& component_id,
                       const std::string& id_selector, const bool single,
                       piper::ApiCallBack callBack);

  void ElementAnimate(const std::string& component_id,
                      const std::string& id_selector, const lepus::Value& args);

  void UpdateCoreJS(std::string core_js);

  void GetComponentContextDataAsync(const std::string& component_id,
                                    const std::string& key,
                                    piper::ApiCallBack callBack);

  std::shared_ptr<tasm::TemplateAssembler> GetTasm();

  void SetCSSVariables(const std::string& component_id,
                       const std::string& id_selector,
                       const lepus::Value& properties,
                       tasm::PipelineOptions pipeline_options);

  void SetNativeProps(const tasm::NodeSelectRoot& root,
                      const tasm::NodeSelectOptions& options,
                      const lepus::Value& native_props,
                      tasm::PipelineOptions pipeline_options);

  void ReloadFromJS(runtime::UpdateDataTask task);

  void AddFont(const lepus::Value& font, piper::ApiCallBack callback);

#if ENABLE_TESTBENCH_RECORDER
  void SetRecordID(int64_t record_id);
#endif

  void UpdateI18nResource(const std::string& key, const std::string& new_data);

  void Flush();

  void InvokeUIMethod(const tasm::NodeSelectRoot& root,
                      const tasm::NodeSelectOptions& options,
                      const std::string& method,
                      std::unique_ptr<tasm::PropBundle> params,
                      piper::ApiCallBack callback);

  void GetPathInfo(const tasm::NodeSelectRoot& root,
                   const tasm::NodeSelectOptions& options,
                   piper::ApiCallBack call_back);

  void GetFields(const tasm::NodeSelectRoot& root,
                 const tasm::NodeSelectOptions& options,
                 const std::vector<std::string>& fields,
                 piper::ApiCallBack call_back);

  tasm::LynxGetUIResult GetLynxUI(const tasm::NodeSelectRoot& root,
                                  const tasm::NodeSelectOptions& options);

  void SetInspectorElementObserver(
      const std::shared_ptr<tasm::InspectorElementObserver>&
          inspector_element_observer);

  // For Fiber
  void CallLepusMethod(const std::string& method_name, lepus::Value args,
                       const piper::ApiCallBack& callback,
                       uint64_t trace_flow_id);

  inline void SetOperationQueue(
      const std::shared_ptr<TASMOperationQueue>& tasm_operation_queue) {
    operation_queue_ = tasm_operation_queue;
  }

  void PreloadLazyBundles(const std::vector<std::string>& urls);

  void InsertLynxTemplateBundle(const std::string& url,
                                lynx::tasm::LynxTemplateBundle bundle);

  void OnReceiveMessageEvent(runtime::MessageEvent event);

  void GetJSSessionStorage(const std::string&,
                           const piper::ApiCallBack& callback);

  void SubscribeJSSessionStorage(const std::string&, double listener_id,
                                 const piper::ApiCallBack& callback);

  void SetClientSessionStorage(const std::string& key,
                               const lepus::Value& data);

  void GetClientSessionStorage(
      const std::string& key,
      const std::shared_ptr<shell::PlatformCallBackHolder>& callback);

  void SubscribeClientSessionStorage(
      const std::string& key,
      const std::shared_ptr<shell::PlatformCallBackHolder>& callback);

  void UnsubscribeClientSessionStorage(const std::string& key,
                                       double callback_id);

 private:
  std::shared_ptr<tasm::TemplateAssembler> tasm_;
  std::unique_ptr<Delegate> delegate_;
  std::shared_ptr<LynxCardCacheDataManager> card_cached_data_mgr_;
  // tasm thread and layout thread is same one
  // when strategy is {ALL_ON_UI, MOST_ON_TASM}
  std::shared_ptr<TASMOperationQueue> operation_queue_;
  const int32_t instance_id_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_ENGINE_H_
