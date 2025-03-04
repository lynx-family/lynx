// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_SHELL_H_
#define CORE_SHELL_LYNX_SHELL_H_

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/lynx_actor.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/inspector/observer/inspector_runtime_observer_ng.h"
#include "core/public/lynx_resource_loader.h"
#include "core/renderer/data/template_data.h"
#include "core/renderer/ui_wrapper/common/prop_bundle_creator_default.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/services/timing_handler/timing_mediator.h"
#include "core/shell/common/vsync_monitor.h"
#include "core/shell/dynamic_ui_operation_queue.h"
#include "core/shell/engine_thread_switch.h"
#include "core/shell/layout_mediator.h"
#include "core/shell/lynx_actor_specialization.h"
#include "core/shell/lynx_card_cache_data_manager.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/native_facade.h"
#include "core/shell/native_facade_reporter.h"
#include "core/shell/tasm_mediator.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/thread_mode_auto_switch.h"

namespace lynx {

namespace air {
class AirModuleHandler;
}  // namespace air
namespace tasm {
class LynxTemplateBundle;
}  // namespace tasm
namespace piper {
class NativeModuleFactory;
}
namespace runtime {
class IRuntimeLifecycleObserver;
}

namespace shell {

struct ShellOption {
  bool enable_js_{true};
  bool enable_multi_tasm_thread_{true};
  bool enable_multi_layout_thread_{true};
  bool enable_auto_concurrency_{false};
  bool enable_js_group_thread_{false};
  bool enable_vsync_aligned_msg_loop_{false};
  bool enable_async_hydration_{false};
  int32_t instance_id_{kUnknownInstanceId};
  std::string js_group_thread_name_;
};

// support create and destroy in any thread
class LynxShell {
 public:
  virtual ~LynxShell();

  // This is needed by RuntimeStandaloneHelper to create LynxRuntime
  // outside LynxShell. Don't use this elsewhere.
  static int32_t NextInstanceId();

  virtual void InitRuntime(
      const std::string& group_id,
      const std::shared_ptr<lynx::pub::LynxResourceLoader>& resource_loader,
      const std::shared_ptr<lynx::piper::LynxModuleManager>& module_manager,
      const std::function<
          void(const std::shared_ptr<LynxActor<runtime::LynxRuntime>>&)>&
          on_runtime_actor_created,
      std::vector<std::string> preload_js_paths, bool force_reload_js_core,
      bool force_use_light_weight_js_engine = false,
      bool pending_js_task = false, bool enable_user_bytecode = false,
      const std::string& bytecode_source_url = "");

  // This method attaches a pre-created LynxRuntime to the LynxShell:
  // so only one of `AttachRuntime` and `InitRuntime` will be called
  virtual void AttachRuntime(
      std::weak_ptr<piper::LynxModuleManager> module_manager);
  void InitRuntimeWithRuntimeDisabled(
      std::shared_ptr<VSyncMonitor> vsync_monitor);

  void StartJsRuntime();

  // TODO(heshan): will be deleted after ios platform ready
  void Destroy();

  // TODO(heshan): will be deleted after ios platform ready
  bool IsDestroyed();

  void LoadTemplate(const std::string& url, std::vector<uint8_t> source,
                    const std::shared_ptr<tasm::TemplateData>& template_data,
                    const bool enable_pre_painting = false,
                    bool enable_recycle_template_bundle = false);

  void LoadTemplateBundle(
      const std::string& url, tasm::LynxTemplateBundle template_bundle,
      const std::shared_ptr<tasm::TemplateData>& template_data,
      const bool enable_pre_painting = false,
      bool enable_dump_element_tree = false);

  void MarkDirty();

  void Flush();

  void ForceFlush();

  void SetEnableUIFlush(bool enable_ui_flush);

  void SetContextHasAttached();

  void LoadSSRData(std::vector<uint8_t> source,
                   const std::shared_ptr<tasm::TemplateData>& template_data);

  void UpdateData(const std::string& data);

  void UpdateDataByParsedData(const std::shared_ptr<tasm::TemplateData>& data);

  void ResetDataByParsedData(const std::shared_ptr<tasm::TemplateData>& data);

  void ReloadTemplate(const std::shared_ptr<tasm::TemplateData>& data,
                      const lepus::Value& global_props = lepus::Value());

  void SetSessionStorageItem(const std::string& key,
                             const std::shared_ptr<tasm::TemplateData>& data);
  void GetSessionStorageItem(const std::string& key,
                             std::unique_ptr<PlatformCallBack> callback);

  int32_t SubscribeSessionStorage(const std::string& key,
                                  std::unique_ptr<PlatformCallBack> callback);

  void UnSubscribeSessionStorage(const std::string& key, double callback_id);

  void UpdateConfig(const lepus::Value& config);

  void UpdateGlobalProps(const lepus::Value& global_props);

  void UpdateMetaData(const std::shared_ptr<tasm::TemplateData>& data,
                      const lepus::Value& global_props);

  void UpdateScreenMetrics(float width, float height, float scale);

  void UpdateFontScale(float scale);

  void SetFontScale(float scale);

  void SetPlatformConfig(std::string platform_config_json_string);

  void UpdateViewport(float width, int32_t width_mode, float height,
                      int32_t height_mode, bool need_layout = true);

  void TriggerLayout();

  void SyncFetchLayoutResult();

  void LayoutImmediatelyWithUpdatedViewport(float width, int32_t width_mode,
                                            float height, int32_t height_mode);

  void SendCustomEvent(const std::string& name, int32_t tag,
                       const lepus::Value& params,
                       const std::string& params_name);

  void SendGestureEvent(int32_t tag, int32_t gesture_id, std::string name,
                        const lepus::Value& params);

  void SendTouchEvent(const std::string& name, int32_t tag, float x, float y,
                      float client_x, float client_y, float page_x,
                      float page_y);

  void OnPseudoStatusChanged(int32_t id, int32_t pre_status,
                             int32_t current_status);

  void SendBubbleEvent(const std::string& name, int32_t tag,
                       lepus::DictionaryPtr dict);

  void SendGlobalEventToLepus(const std::string& name,
                              const lepus_value& params);

  void SendSsrGlobalEvent(const std::string& name, const lepus_value& params);

  void TriggerEventBus(const std::string& name, const lepus_value& params);

  // synchronous
  std::unique_ptr<lepus_value> GetCurrentData();

  const lepus::Value GetPageDataByKey(std::vector<std::string> keys);

  tasm::ListNode* GetListNode(int32_t tag);

  // list methods
  void RenderListChild(int32_t tag, uint32_t index, int64_t operation_id);

  void UpdateListChild(int32_t tag, uint32_t sign, uint32_t index,
                       int64_t operation_id);

  void RemoveListChild(int32_t tag, uint32_t sign);

  int32_t ObtainListChild(int32_t tag, uint32_t index, int64_t operation_id,
                          bool enable_reuse_notification);

  void RecycleListChild(int32_t tag, uint32_t sign);

  void ScrollByListContainer(int32_t tag, float offset_x, float offset_y,
                             float original_x, float original_y);

  void ScrollToPosition(int32_t tag, int index, float offset, int align,
                        bool smooth);

  void ScrollStopped(int32_t tag);

  void AssembleListPlatformInfo(
      int32_t tag, base::MoveOnlyClosure<void, tasm::ListNode*> assembler);

  void LoadListNode(int32_t tag, uint32_t index, int64_t operationId,
                    bool enable_reuse_notification);

  void EnqueueListNode(int32_t tag, uint32_t component_tag);

  void OnEnterForeground();

  void OnEnterBackground();

  void UpdateI18nResource(const std::string& key, const std::string& new_data);

  // TODO(heshan):will be deleted, pass when ReportError
  std::unordered_map<std::string, std::string> GetAllJsSource();

  // TODO(heshan):will be deleted when js thread ready
  BASE_EXPORT_FOR_DEVTOOL std::shared_ptr<tasm::TemplateAssembler> GetTasm();

  BASE_EXPORT_FOR_DEVTOOL void SetInspectorElementObserver(
      const std::shared_ptr<tasm::InspectorElementObserver>&
          inspector_element_observer);

  BASE_EXPORT_FOR_DEVTOOL void SetInspectorRuntimeObserver(
      const std::shared_ptr<piper::InspectorRuntimeObserverNG>& observer) {
    runtime_observer_ = observer;
  }

  BASE_EXPORT_FOR_DEVTOOL bool IsRuntimeEnabled() { return enable_runtime_; }

  BASE_EXPORT_FOR_DEVTOOL void SetHierarchyObserver(
      const std::shared_ptr<tasm::HierarchyObserver>& hierarchy_observer);

  int32_t GetInstanceId() { return instance_id_; }

  std::shared_ptr<LynxActor<NativeFacade>> GetFacadeActor() {
    return facade_actor_;
  }

  std::shared_ptr<LynxActor<runtime::LynxRuntime>> GetRuntimeActor() {
    return runtime_actor_;
  }

  std::shared_ptr<LynxActor<shell::LynxEngine>> GetEngineActor() {
    return engine_actor_;
  }

  base::TaskRunnerManufactor* GetRunners() { return &runners_; }

  void RunOnTasmThread(std::function<void(void)>&& task);

  tasm::LynxGetUIResult GetLynxUI(const tasm::NodeSelectRoot& root,
                                  const tasm::NodeSelectOptions& options);

  /**
   * @brief get  thread strategy
   * @return one of ThreadStrategyForRendering
   */
  uint32_t ThreadStrategy();

  void PreloadLazyBundles(std::vector<std::string> urls);

  /**
   * register a dynamic component with template bundle and url
   */
  void RegisterLazyBundle(std::string url,
                          tasm::LynxTemplateBundle template_bundle);

  void SetEnableBytecode(bool enable, std::string bytecode_source_url);

  void SetAnimationsPending(bool need_pending_ui_op);
  /**
   * Dispatch MessageEvent from platform, currently only dispatching
   * MessageEvent from DevTool.
   */
  void DispatchMessageEvent(runtime::MessageEvent event);

  // Timing related function.
  void SetTiming(uint64_t us_timestamp, tasm::timing::TimestampKey timing_key,
                 tasm::PipelineID pipeline_id) const;

  const lepus::Value GetAllTimingInfo() const;

  // TODO(kechenglong): should find a better way to set SSR timing data?
  void SetSSRTimingData(std::string url, uint64_t data_size) const;

  // TODO(kechenglong): Temporary API, will be removed after pipelineOptions
  // finished pre-created.
  void ClearAllTimingInfo() const;

  void OnPipelineStart(const tasm::PipelineID& pipeline_id,
                       const tasm::PipelineOrigin& pipeline_origin,
                       tasm::timing::TimestampUs pipeline_start_timestamp);

  // TODO(heshan): The temporarily added API will be removed
  // once the overall design for dynamically switching thread modes is
  // implemented.
  void BindLynxEngineToUIThread();

  // TODO(heshan): The temporarily added API will be removed
  // once the overall design for dynamically switching thread modes is
  // implemented.
  void UnbindLynxEngineFromUIThread();

  void AttachEngineToUIThread();

  void DetachEngineFromUIThread();

  void RegisterRuntimeLifecycleObserver(
      const std::shared_ptr<lynx::runtime::IRuntimeLifecycleObserver>&
          runtime_lifecycle_observer,
      base::MoveOnlyClosure<void, fml::RefPtr<fml::TaskRunner>> on_complete);
  void RegisterModuleFactory(
      std::unique_ptr<lynx::piper::NativeModuleFactory> module_factory);

 protected:
  // use for record app state
  enum class AppState { kUnknown, kForeground, kBackground };

  friend class LynxShellBuilder;
  explicit LynxShell(base::ThreadStrategyForRendering strategy,
                     const ShellOption& shell_option);

  void EnsureTemplateDataThreadSafe(
      const std::shared_ptr<tasm::TemplateData>& template_data);

  void OnThreadStrategyUpdated();

  lepus::Value EnsureGlobalPropsThreadSafe(const lepus::Value& global_props);

  void OnRuntimeCreate();

  void ConsumeModuleFactory(piper::LynxModuleManager* module_manager);

  std::atomic_bool is_destroyed_{false};

  std::shared_ptr<LynxActor<NativeFacade>>
      facade_actor_;  // on platform UI runner

  std::shared_ptr<LynxActor<NativeFacadeReporter>>
      facade_reporter_actor_;  // on platform Reporter runner

  std::shared_ptr<LynxActor<LynxEngine>> engine_actor_;  // on TASM runner

  std::shared_ptr<LynxActor<runtime::LynxRuntime>>
      runtime_actor_;  // on JS runner
  std::shared_ptr<LynxActor<tasm::LayoutContext>>
      layout_actor_;  // on Layout runner

  base::TaskRunnerManufactor runners_;

  // TODO(heshan):will move to delegate of LynxRuntime
  std::shared_ptr<piper::InspectorRuntimeObserverNG> runtime_observer_;

  const int32_t instance_id_;

  bool enable_runtime_ = true;

  std::shared_ptr<LynxCardCacheDataManager> card_cached_data_mgr_ =
      std::make_shared<LynxCardCacheDataManager>();
  std::shared_ptr<TASMOperationQueue> tasm_operation_queue_;
  std::shared_ptr<shell::DynamicUIOperationQueue> ui_operation_queue_;
  TasmMediator* tasm_mediator_;                    // NOT OWNED
  LayoutMediator* layout_mediator_;                // NOT OWNED
  tasm::timing::TimingMediator* timing_mediator_;  // NOT OWNED

  std::function<void(std::unique_ptr<runtime::LynxRuntime>&)>
      start_js_runtime_task_;

  ThreadModeManager thread_mode_manager_;
  // A SSR page will be rendered when LoadSSRData is called.
  // A ssr page will be further hydrated when a load template is called.
  bool hydration_pending_{false};
  bool enable_async_hydration_{false};

  base::ThreadStrategyForRendering current_strategy_;

  std::string js_group_thread_name_;
  bool enable_js_group_thread_;
  std::condition_variable tasm_merge_cv_;
  std::mutex tasm_merge_mutex_;
  std::atomic_bool need_wait_for_merge_{false};
  std::shared_ptr<tasm::PropBundleCreator> prop_bundle_creator_ =
      std::make_shared<tasm::PropBundleCreatorDefault>();
  AppState app_state_{AppState::kUnknown};
  std::shared_ptr<LynxActor<tasm::timing::TimingHandler>>
      timing_actor_;  // on timing runner

  std::unique_ptr<ThreadModeAutoSwitch> thread_mode_auto_switch_;
  std::shared_ptr<EngineThreadSwitch> engine_thread_switch_;

  std::shared_ptr<lynx::runtime::IRuntimeLifecycleObserver>
      runtime_lifecycle_observer_;
  // Only ref module_manager;
  std::weak_ptr<lynx::piper::LynxModuleManager> weak_module_manager_;
  // Managed by Runtime, only keep those before runtime create.
  std::vector<std::unique_ptr<lynx::piper::NativeModuleFactory>>
      module_factories_;

 private:
  std::weak_ptr<piper::JsBundleHolder> GetWeakJsBundleHolder();
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_SHELL_H_
