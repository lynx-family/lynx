// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TEMPLATE_ASSEMBLER_H_
#define CORE_RENDERER_TEMPLATE_ASSEMBLER_H_

#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/debug/lynx_assert.h"
#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "core/inspector/observer/inspector_lepus_observer.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/data/template_data.h"
#include "core/renderer/events/touch_event_handler.h"
#include "core/renderer/page_config.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/signal/signal_context.h"
#include "core/renderer/tasm/i18n/i18n.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/template_entry_holder.h"
#include "core/renderer/template_themed.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/runtime/bindings/lepus/event/context_proxy_in_lepus.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "core/runtime/piper/js/update_data_type.h"
#include "core/runtime/vm/lepus/lepus_global.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/services/timing_handler/timing.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
#include "core/template_bundle/template_codec/moulds.h"
#include "third_party/rapidjson/document.h"

namespace lynx {

namespace piper {
class ApiCallBack;
}  // namespace piper

namespace shell {
class PlatformCallBackHolder;
}  // namespace shell.

namespace lepus {
class Context;
}

namespace tasm {
class CSSStyleSheetManager;
class ComponentMould;
class ElementManager;
class I18n;
class LynxGetUIResult;
class WhiteBoard;
class WhiteBoardDelegate;

namespace layout {
struct CalculatedViewport;
}

static constexpr const char DEFAULT_ENTRY_NAME[] = LEPUS_DEFAULT_CONTEXT_NAME;
static constexpr const char CARD_CONFIG_STR[] = "__card_config_";
static constexpr const char CARD_CONFIG_THEME[] = "theme";

using LightComponentInfo =
    std::unordered_map<std::string, std::tuple<std::vector<int>, int32_t>>;

// AirTouchEventHandler can't be included directly in template_assembler.h
// because of rock release
// TODO(renpengcheng): when AirTouchEventHandler was deleted, delete the base
// class
class AirTouchEventHandlerBase {
 public:
  AirTouchEventHandlerBase() = default;
  virtual ~AirTouchEventHandlerBase() = default;
  // HandleTouchEvent handle touch event
  virtual void HandleTouchEvent(TemplateAssembler* tasm,
                                const std::string& page_name,
                                const std::string& name, int tag, float x,
                                float y, float client_x, float client_y,
                                float page_x, float page_y){};

  // HandleCustomEvent customEvent for example: x-element's custom event
  virtual void HandleCustomEvent(TemplateAssembler* tasm,
                                 const std::string& name, int tag,
                                 const lepus::Value& params,
                                 const std::string& pname){};

  // SendPageEvent air life function and global event
  virtual void SendPageEvent(TemplateAssembler* tasm,
                             const std::string& handler,
                             const lepus::Value& info) const {};

  // SendComponentEvent send Component related lifecycle event
  virtual void SendComponentEvent(TemplateAssembler* tasm,
                                  const std::string& event_name,
                                  const int component_id,
                                  const lepus::Value& params,
                                  const std::string& param_name){};
  // Only for the situation when child component needs to send message to parent
  virtual size_t TriggerComponentEvent(TemplateAssembler* tasm,
                                       const std::string& event_name,
                                       const lepus::Value& data) = 0;
};

class TemplateAssembler final
    : public std::enable_shared_from_this<TemplateAssembler>,
      public TemplateEntryHolder,
      public TemplateBinaryReader::PageConfigger,
      public PageProxy::TasmDelegate,
      public lepus::Context::Delegate {
 public:
  class Delegate : public runtime::ContextProxy::Delegate {
   public:
    Delegate() = default;
    ~Delegate() override = default;

    virtual void OnDataUpdated() = 0;
    virtual void OnTasmFinishByNative() = 0;
    virtual void OnTemplateLoaded(const std::string& url) = 0;
    virtual void OnSSRHydrateFinished(const std::string& url) = 0;
    virtual void OnErrorOccurred(base::LynxError error) = 0;
    virtual void TriggerLepusngGc(base::closure func) = 0;
    virtual void OnDynamicComponentPerfReady(const lepus::Value& perf_info) = 0;
    virtual void OnConfigUpdated(const lepus::Value& data) = 0;
    virtual void OnPageConfigDecoded(
        const std::shared_ptr<tasm::PageConfig>& config) = 0;

    // recycle a complete bundle
    virtual void OnTemplateBundleReady(LynxTemplateBundle bundle) {}

    // sometimes the bundle in the runtime entry is incomplete, because of the
    // existence of lazy decoding, so a recycler is required to complete the
    // greedy decode first
    virtual void RecycleTemplateBundle(
        std::unique_ptr<tasm::LynxBinaryRecyclerDelegate> recycler) {}

    // synchronous
    virtual std::string TranslateResourceForTheme(
        const std::string& res_id, const std::string& theme_key) = 0;

    virtual void GetI18nResource(const std::string& key,
                                 const std::string& fallback_url) = 0;
    virtual void SetTiming(tasm::Timing timing) = 0;

    virtual void BindPipelineIDWithTimingFlag(
        const tasm::PipelineID& pipeline_id,
        const tasm::timing::TimingFlag& timing_flag) = 0;
    virtual void OnPipelineStart(
        const tasm::PipelineID& pipeline_id,
        const tasm::PipelineOrigin& pipeline_origin,
        tasm::timing::TimestampUs pipeline_start_timestamp) = 0;
    virtual void ResetTimingBeforeReload(const std::string& flag) = 0;

    virtual void CallJSApiCallback(piper::ApiCallBack callback) = 0;
    virtual void CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                            const lepus::Value& value,
                                            bool persist = false) = 0;
    virtual void RemoveJSApiCallback(piper::ApiCallBack callback) = 0;
    virtual void CallPlatformCallbackWithValue(
        const std::shared_ptr<shell::PlatformCallBackHolder>&,
        const lepus::Value& value) = 0;
    virtual void RemovePlatformCallback(
        const std::shared_ptr<shell::PlatformCallBackHolder>& callback) = 0;
    virtual void CallJSFunction(const std::string& module_id,
                                const std::string& method_id,
                                const lepus::Value& arguments,
                                bool force_call_despite_app_state = false) = 0;
    virtual void OnDataUpdatedByNative(tasm::TemplateData data,
                                       const bool reset) = 0;
    virtual void OnJSAppReload(tasm::TemplateData data,
                               const PipelineOptions& pipeline_options) = 0;
    virtual void OnGlobalPropsUpdated(const lepus::Value& props) = 0;
    virtual void OnLifecycleEvent(const lepus::Value& args) = 0;
    virtual void PrintMsgToJS(const std::string& level,
                              const std::string& msg) = 0;
    virtual void OnI18nResourceChanged(const std::string& res) = 0;
    virtual void RequestVsync(
        uintptr_t id,
        base::MoveOnlyClosure<void, int64_t, int64_t> callback) = 0;
    virtual lepus::Value TriggerLepusMethod(const std::string& method_name,
                                            const lepus::Value& arguments) = 0;
    virtual void TriggerLepusMethodAsync(const std::string& method_name,
                                         const lepus::Value& arguments,
                                         bool is_air) = 0;
    virtual void InvokeUIMethod(LynxGetUIResult ui_result,
                                const std::string& method,
                                std::unique_ptr<tasm::PropBundle> params,
                                piper::ApiCallBack callback) = 0;

    // air-runtime methods
    virtual void LepusInvokeUIMethod(
        std::vector<int32_t> ui_impl_ids, const std::string& method,
        const lepus::Value& params, lepus::Context* context,
        std::unique_ptr<lepus::Value> callback_closure) = 0;

    virtual void OnJSSourcePrepared(
        tasm::TasmRuntimeBundle bundle, const lepus::Value& global_props,
        const std::string& page_name, tasm::PackageInstanceDSL dsl,
        tasm::PackageInstanceBundleModuleMode bundle_module_mode,
        const std::string& url, const PipelineOptions& pipeline_options) = 0;
    virtual void OnComponentDecoded(tasm::TasmRuntimeBundle bundle) = 0;
    virtual void OnCardConfigDataChanged(const lepus::Value& data) = 0;

    virtual fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() = 0;
  };

  class Scope {
   public:
    explicit Scope(TemplateAssembler* tasm);
    ~Scope();

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&&) = delete;
    Scope& operator=(Scope&&) = delete;

   private:
    bool scoped_ = false;
  };

  TemplateAssembler(Delegate& delegate, std::unique_ptr<ElementManager> client,
                    int32_t instance_id);
  ~TemplateAssembler() override;

  void Init(fml::RefPtr<fml::TaskRunner> tasm_runner);

  void LoadTemplate(const std::string& url, std::vector<uint8_t> source,
                    const std::shared_ptr<TemplateData>& template_data,
                    PipelineOptions& pipeline_options,
                    const bool enable_pre_painting = false,
                    bool enable_recycle_template_bundle = false);

  void LoadTemplateBundle(const std::string& url,
                          LynxTemplateBundle template_bundle,
                          const std::shared_ptr<TemplateData>& template_data,
                          PipelineOptions& pipeline_options,
                          const bool enable_pre_painting = false,
                          bool enable_dump_element_tree = false);

  // Diff the entire tree using the new template_data.
  // Refresh the card and component's lifecycle like a new loaded template.
  // No need to decode and set page config.
  void ReloadTemplate(const std::shared_ptr<TemplateData>& template_data,
                      UpdatePageOption& update_page_option,
                      PipelineOptions& pipeline_options);

  void ReloadTemplate(const std::shared_ptr<TemplateData>& template_data,
                      const lepus::Value& global_props,
                      UpdatePageOption& update_page_option,
                      PipelineOptions& pipeline_options);

  // used in lynx.reload() api for FE.
  void ReloadFromJS(const runtime::UpdateDataTask& task,
                    PipelineOptions& pipeline_options);

  void AddFont(const lepus::Value& font);

  // Render page with page data that rendered on server side.
  void RenderPageWithSSRData(std::vector<uint8_t> data,
                             const std::shared_ptr<TemplateData>& template_data,
                             PipelineOptions& pipeline_options);

  void DidPreloadComponent(LazyBundleLoader::CallBackInfo callback_info);

  void DidLoadComponent(LazyBundleLoader::CallBackInfo callback_info,
                        PipelineOptions& pipeline_options);

  void LoadComponentWithCallbackInfo(
      LazyBundleLoader::CallBackInfo callback_info,
      PipelineOptions& pipeline_options);

  void ReportRuntimeReady();
  void ReportError(int32_t error_code, const std::string& msg,
                   base::LynxErrorLevel level = base::LynxErrorLevel::Error);
  void ReportError(base::LynxError error) override;

  void ReportGCTimingEvent(const char* start, const char* end) override;

  fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() override;

  void ExecuteDataProcessor(TemplateData& data);

  void UpdateGlobalProps(const lepus::Value& data, bool need_render,
                         PipelineOptions& pipeline_options);

  void SendTouchEvent(const std::string& name, const EventInfo& info);
  void SendCustomEvent(const std::string& name, int tag,
                       const lepus::Value& params, const std::string& pname);

  void SendGestureEvent(int tag, int gesture_id, const std::string& name,
                        const lepus::Value& params);

  void OnPseudoStatusChanged(int32_t id, uint32_t pre_status,
                             uint32_t current_status);

  // Just send `onLazyBundleEvent` globalEvent.
  // for history version compatibility, not to break.
  void SendLazyBundleGlobalEvent(const std::string& url,
                                 const lepus::Value& err);

  // Send bindEvent via element id
  void SendLazyBundleBindEvent(const std::string& url,
                               const std::string& event_name,
                               const lepus::Value& msg, int imp_id);

  void SendBubbleEvent(const std::string& name, int tag,
                       lepus::DictionaryPtr dict);

  BASE_EXPORT_FOR_DEVTOOL void SetLepusObserver(
      const std::shared_ptr<lepus::InspectorLepusObserver>& observer);

  lepus::Value& GetComponentInfoMap(const std::string& entry_name) override;
  lepus::Value& GetComponentPathMap(const std::string& entry_name) override;
  bool SupportComponentJS() const override { return support_component_js_; }

  bool destroyed() { return destroyed_; }

#if ENABLE_TESTBENCH_RECORDER
  void SetRecordID(int64_t record_id);
  int64_t GetRecordID() const;
#endif

  TemplateData GenerateTemplateDataPostedToJs(const TemplateData& value);

  void UpdateMetaData(const std::shared_ptr<TemplateData>& template_data,
                      const lepus::Value& global_props,
                      UpdatePageOption& update_page_option,
                      PipelineOptions& pipeline_options);

  // Non-threadsafe
  void UpdateDataByPreParsedData(
      const std::shared_ptr<TemplateData>& template_data,
      UpdatePageOption& update_page_option, PipelineOptions& pipeline_options);
  // Threadsafe

  void UpdateDataByJS(const runtime::UpdateDataTask& task,
                      PipelineOptions& pipeline_options);

  const lepus::Value GetGlobalProps() const {
    return lepus::Value::ShallowCopy(global_props_);
  }

  PageProxy* page_proxy() { return &page_proxy_; }
  lepus::Context* context(const std::string entry_name) {
    return FindEntry(entry_name)->GetVm().get();
  }

  const std::shared_ptr<lepus::Context>& getLepusContext(
      const std::string& entry_name) {
    const auto& entry = FindEntry(entry_name);
    return entry->GetVm();
  }

  std::unordered_map<int, std::shared_ptr<ComponentMould>>& component_moulds(
      const std::string& entry_name) {
    return FindEntry(entry_name)->component_moulds();
  }

  std::pair<ComponentMould*, std::string> FindComponentMould(
      const std::string& entry_name, const std::string& component_name,
      int tid);

  std::unordered_map<int, std::shared_ptr<DynamicComponentMould>>&
  lazy_bundle_moulds(const std::string& entry_name) {
    return FindEntry(entry_name)->lazy_bundle_moulds();
  }

  const std::unordered_map<std::string, int>& component_name_to_id(
      const std::string& entry_name) {
    return FindEntry(entry_name)->component_name_to_id();
  }

  std::shared_ptr<CSSStyleSheetManager> style_sheet_manager(
      const std::string& entry_name) {
    return FindEntry(entry_name)->GetStyleSheetManager();
  }

  const std::unordered_map<int, std::shared_ptr<PageMould>>& page_moulds();
  void Destroy();

  Delegate& GetDelegate() { return delegate_; }

  std::unique_ptr<lepus::Value> GetCurrentData();
  lepus::Value GetPageDataByKey(const std::vector<std::string>& keys);

  void UpdateComponentData(const runtime::UpdateDataTask& task,
                           PipelineOptions& pipeline_options);

  void SelectComponent(const std::string& component_id, const std::string&,
                       const bool single, piper::ApiCallBack callback);

  void ElementAnimate(const std::string& component_id,
                      const std::string& id_selector, const lepus::Value& args);

  void GetComponentContextDataAsync(const std::string& component_id,
                                    const std::string& key,
                                    piper::ApiCallBack callback);
  void TriggerComponentEvent(const std::string& event_name,
                             const lepus::Value& msg);

  void LepusInvokeUIMethod(std::vector<int32_t> ui_impl_ids,
                           const std::string& method,
                           const lepus::Value& params, lepus::Context* context,
                           std::unique_ptr<lepus::Value> callback_closure);

  void CallJSFunctionInLepusEvent(const std::string& component_id,
                                  const std::string& name,
                                  const lepus::Value& params);

  void TriggerLepusGlobalEvent(const std::string& event_name,
                               const lepus::Value& msg);
  void TriggerWorkletFunction(std::string component_id,
                              std::string worklet_module_name,
                              std::string method_name, lepus::Value args,
                              piper::ApiCallBack callback);
  lepus::Value TriggerLepusBridge(const std::string& event_name,
                                  const lepus::Value& msg);
  void TriggerLepusBridgeAsync(const std::string& method_name,
                               const lepus::Value& arguments,
                               bool is_air = false);
  void InvokeLepusCallback(const int32_t callback_id,
                           const std::string& entry_name,
                           const lepus::Value& data);

  void InvokeLepusComponentCallback(const int64_t callback_id,
                                    const std::string& entry_name,
                                    const lepus::Value& data);

  void GetDecodedJSSource(
      std::unordered_map<std::string, std::string>& js_source);

  std::shared_ptr<TemplateEntry> QueryComponent(const std::string& url);

  void SendAirPageEvent(const std::string& event, const lepus::Value& value);
  void RenderTemplateForAir(const std::shared_ptr<TemplateEntry>& card,
                            const lepus::Value& data,
                            PipelineOptions& pipeline_options);
  void SendAirComponentEvent(const std::string& event_name,
                             const int component_id, const lepus::Value& params,
                             const std::string& param_name);
  // air-runtime methods
  void InvokeAirCallback(int64_t id, const std::string& entry_name,
                         const lepus::Value& data);

  SignalContext* GetSignalContext() { return &signal_context_; }

  // TODO: make this protected
 public:
  void SetPageConfig(const std::shared_ptr<PageConfig>& config) override;

  void SetEnableLayoutOnly(bool enable_layout_only) {
    LOGI("Lynx Set Enable Layout Only: " << enable_layout_only
                                         << " from LynxView, "
                                         << " this:" << this);
    page_proxy_.SetTasmEnableLayoutOnly(enable_layout_only);
  }

  void OnPageConfigDecoded(const std::shared_ptr<PageConfig>& config);

  PackageInstanceDSL GetPageDSL() {
    return page_config_ ? page_config_->GetDSL() : PackageInstanceDSL::TT;
  }

  PackageInstanceBundleModuleMode GetBundleModuleMode() {
    return page_config_ ? page_config_->GetBundleModuleMode()
                        : PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE;
  }

  std::shared_ptr<PageConfig> GetPageConfig() override { return page_config_; }

  void SetPageConfigClient() {
    // add for global config
    if (page_proxy_.element_manager()) {
      page_proxy_.element_manager()->SetConfig(GetPageConfig());
    }
  }

  inline bool EnableLynxAir() {
    return page_config_ && (page_config_->GetLynxAirMode() ==
                            CompileOptionAirMode::AIR_MODE_STRICT);
  }

  bool ShouldPostDataToJs() const {
    // currently, only air&air_fiber mode should not post data to js
    return !(page_config_ && (page_config_->GetLynxAirMode() ==
                                  CompileOptionAirMode::AIR_MODE_STRICT ||
                              page_config_->GetLynxAirMode() ==
                                  CompileOptionAirMode::AIR_MODE_FIBER));
  }

  const lepus::Value& GetDefaultProcessor() { return default_processor_; }
  const std::unordered_map<std::string, lepus::Value>& GetProcessorMap() const {
    return processor_with_name_;
  }
  const lepus::Value& GetProcessorWithName(const std::string& name) {
    return processor_with_name_[name];
  }
  const lepus::Value& GetComponentProcessorWithName(
      const std::string& component_path, const std::string& name,
      const std::string& entry_name) {
    return component_processor_with_name_[entry_name.empty()
                                              ? DEFAULT_ENTRY_NAME
                                              : entry_name][component_path]
                                         [name];
  }

  void SetDefaultProcessor(const lepus::Value& processor) {
    default_processor_ = processor;
  }
  void SetProcessorWithName(const lepus::Value& processor,
                            const std::string& name) {
    processor_with_name_[name] = processor;
  }
  void SetComponentProcessorWithName(const lepus::Value& processor,
                                     const std::string& name,
                                     const std::string& component_path,
                                     const std::string& entry_name) {
    component_processor_with_name_[entry_name.empty() ? DEFAULT_ENTRY_NAME
                                                      : entry_name]
                                  [component_path][name] = processor;
  }

  void SetLazyBundleLoader(std::shared_ptr<LazyBundleLoader> loader) {
    component_loader_ = loader;
  }

  void SetLocale(const std::string& locale) { locale_ = locale; }

  bool UpdateConfig(const lepus::Value& config, bool noticeDelegate,
                    PipelineOptions& pipeline_options);

  std::string TranslateResourceForTheme(const std::string& res_id,
                                        const std::string& theme_key) {
    std::string result;
    if (res_id.empty()) {
      return result;
    }

    if (page_proxy_.themed().hasAnyCurRes &&
        page_proxy_.themed().currentTransMap) {
      if (InnerTranslateResourceForTheme(result, res_id, theme_key, false)) {
        return result;
      }
    }

    result = delegate_.TranslateResourceForTheme(res_id, theme_key);
    if (!result.empty()) {
      return result;
    }

    if (page_proxy_.themed().hasAnyFallback &&
        page_proxy_.themed().currentTransMap) {
      if (InnerTranslateResourceForTheme(result, res_id, theme_key, true)) {
        return result;
      }
    }

    return "";
  }

  lepus::Value GetI18nResources(const lepus::Value& locale,
                                const lepus::Value& channel,
                                const lepus::Value& fallback_url);

  void UpdateI18nResource(const std::string& key, const std::string& new_data);

  void updateLocale(const lepus::Value& locale, const lepus::Value& channel);

  void ReFlushPage();

  void FilterI18nResource(const lepus::Value& channel,
                          const lepus::Value& locale,
                          const lepus::Value& reserve_keys);

  void OnFontScaleChanged(float scale);

  void OnI18nResourceChanged(const std::string& new_data);

  void OnI18nResourceFailed();

  void SetFontScale(float scale);

  void SetPlatformConfig(std::string platform_config_json_string);

  void OnScreenMetricsSet(float width, float height);

  void SendFontScaleChanged(float scale);

  void SendGlobalEvent(const std::string& event, const lepus::Value& value);

  void UpdateViewport(float width, int32_t width_mode, float height,
                      int32_t height_mode);

  void OnLazyBundlePerfReady(const lepus::Value& perf_info);

  std::string GetTargetUrl(const std::string& current,
                           const std::string& target);

  std::shared_ptr<TemplateEntry> RequireTemplateEntry(
      RadonLazyComponent* lazy_bundle, const std::string& url,
      const lepus::Value& callback = lepus::Value());

  void OnDynamicJSSourcePrepared(const std::string& component_url);

  const std::string& TargetSdkVersion() override { return target_sdk_version_; }

  // print js console log
  // level: log, warn, error, info, debug
  void PrintMsgToJS(const std::string& level, const std::string& msg) override;

  bool UseLepusNG();

  void SetCSSVariables(const std::string& component_id,
                       const std::string& id_selector,
                       const lepus::Value& properties,
                       PipelineOptions& pipeline_options);

  void SetNativeProps(const NodeSelectRoot& root,
                      const tasm::NodeSelectOptions& options,
                      const lepus::Value& native_props,
                      PipelineOptions& pipeline_options);

  void SetLepusEventListener(const std::string& name,
                             const lepus::Value& listener);
  void RemoveLepusEventListener(const std::string& name);

  void SendGlobalEventToLepus(const std::string& name,
                              const lepus_value& params);

  void TriggerEventBus(const std::string& name, const lepus_value& params);

  void RenderToBinary(
      base::MoveOnlyClosure<void, RadonNode*, tasm::TemplateAssembler*>);

  // ssr server only
  bool LoadTemplateForSSRRuntime(std::vector<uint8_t> source);

  struct Themed& Themed() override;

  void SetThemed(const Themed::PageTransMaps& page_trans_maps);

  // For fiber
  void CallLepusMethod(const std::string& method_name, lepus::Value args,
                       const piper::ApiCallBack& callback,
                       uint64_t trace_flow_id);

  void PreloadLazyBundles(const std::vector<std::string>& urls);

  void SetWhiteBoard(const std::shared_ptr<WhiteBoard>& white_board);

  const std::shared_ptr<WhiteBoardDelegate>& GetWhiteBoardDelegate() {
    return white_board_delegate_;
  }

  // invoke lepus closure
  lepus::Value TriggerLepusClosure(const lepus::Value& closure,
                                   const lepus::Value& param);

  inline void EnablePreUpdateData(bool enable) {
    enable_pre_update_data_ = enable;
  }

  void SyncAndroidPackageExternalPath(const std::string& path);

  bool EnableFiberArch() {
    return page_config_ && page_config_->GetEnableFiberArch();
  }

  void OnReceiveMessageEvent(runtime::MessageEvent event);

  ContextProxyInLepus* GetContextProxy(runtime::ContextProxy::Type type);

  lepus::Value GetCustomSection(const std::string& key);

  inline int32_t GetInstanceId() { return instance_id_; }

  void SetSupportComponentJS(bool support) override {
    support_component_js_ = support;
  }
  void SetTargetSdkVersion(const std::string& targetSdkVersion) override {
    target_sdk_version_ = targetSdkVersion;
  }
  void SetDefaultLepusNG(bool value) { default_use_lepus_ng_ = value; }

 private:
  friend class TemplateBinaryReader;
  friend class TemplateBinaryReaderSSR;
  friend class TemplateEntry;

  bool EnableDataProcessorOnJs() {
    // TODO(songshourui.null): Currently, JS DataProcessor is only supported in
    // fiber mode. Support for JS DataProcessor in all scenarios will be added
    // in the future.
    return EnableFiberArch() && page_config_ &&
           page_config_->GetEnableDataProcessorOnJs();
  }

  void DidComponentLoaded(
      const std::shared_ptr<TemplateEntry>& component_entry);

  // Build TemplateEntry for lazy bundle
  bool BuildComponentEntryInternal(
      const std::shared_ptr<TemplateEntry>& entry, const std::string& url,
      const base::MoveOnlyClosure<bool, const std::shared_ptr<TemplateEntry>&>&
          entry_initializer);

  // try to construct a entry with preloaded resources
  std::shared_ptr<TemplateEntry> BuildTemplateEntryFromPreload(
      const std::string& url);

  // internal method to send a lazy bundle request by loader
  std::shared_ptr<TemplateEntry> RequestTemplateEntryInternal(
      std::unique_ptr<LazyBundleLifecycleOption> lifecycle_option,
      RadonLazyComponent* lazy_bundle);

  void UpdateGlobalPropsToContext(const lepus::Value& props);

  void LoadTemplateInternal(
      const std::string& url,
      const std::shared_ptr<TemplateData>& template_data,
      PipelineOptions& pipeline_options,
      base::MoveOnlyClosure<bool, const std::shared_ptr<TemplateEntry>&>
          entry_initializer);

  bool OnLoadTemplate(PipelineOptions& pipeline_options);
  void DidLoadTemplate();

  void OnDecodeTemplate();
  void DidDecodeTemplate(const std::shared_ptr<TemplateData>& template_data,
                         const std::shared_ptr<TemplateEntry>& entry,
                         bool post_js, const PipelineOptions& pipeline_options);

  void OnVMExecute();
  void DidVMExecute();

  TemplateData OnRenderTemplate(
      const std::shared_ptr<TemplateData>& template_data,
      const std::shared_ptr<TemplateEntry>& card, bool post_js,
      PipelineOptions& pipeline_options);
  void RenderTemplate(const std::shared_ptr<TemplateEntry>& card,
                      const TemplateData& data,
                      PipelineOptions& pipeline_options);
  void UpdateTemplate(const TemplateData& data,
                      const UpdatePageOption& update_page_option,
                      PipelineOptions& pipeline_options);
  void DidRenderTemplate(PipelineOptions& pipeline_options);
  void RenderTemplateForFiber(const std::shared_ptr<TemplateEntry>& card,
                              const TemplateData& data,
                              PipelineOptions& pipeline_options);

  void OnDataUpdatedByNative(TemplateData value, const bool reset = false);

  void OnJSPrepared(const std::string& url,
                    const PipelineOptions& pipeline_options);
  void NotifyGlobalPropsChanged(const lepus::Value& value);

  bool InnerTranslateResourceForTheme(std::string& ret,
                                      const std::string& res_id,
                                      const std::string& theme_key,
                                      bool isFinalFallback);
  bool FromBinary(const std::shared_ptr<TemplateEntry>& entry,
                  std::vector<uint8_t> source, bool is_card = true);

  bool UpdateGlobalDataInternal(const lepus_value& value,
                                const UpdatePageOption& update_page_option,
                                PipelineOptions& pipeline_options);
  void EnsureTouchEventHandler();

  void EnsureAirTouchEventHandler();

  void SetPageConfigRadonMode() const;

  // Insert data inplace to parameter, return if the page data should be read
  // only.
  TemplateData ProcessTemplateData(
      const std::shared_ptr<TemplateData>& template_data, bool first_screen);
  TemplateData ProcessTemplateDataForFiber(
      const std::shared_ptr<TemplateData>& template_data, bool first_screen);
  TemplateData ProcessTemplateDataForRadon(
      const std::shared_ptr<TemplateData>& template_data, bool first_screen);

  // SSR and Hydration related methods.
  void UpdateGlobalPropsWithDefaultProps(PipelineOptions& pipeline_options);

  // merge with preserved data if needed
  TemplateData ProcessInitData(
      const std::shared_ptr<TemplateData>& init_template_data);

  void ClearCacheData() { cache_data_.clear(); }

  void DumpElementTree(const std::shared_ptr<TemplateEntry>& card);

  lepus::Value TryToGetElementCache(const std::shared_ptr<TemplateEntry>& card,
                                    const ElementManager& manager);

  void OnNativeAppReady();

  bool default_use_lepus_ng_ = false;

  PageProxy page_proxy_;

  static thread_local TemplateAssembler* curr_;

  bool support_component_js_;
  std::string target_sdk_version_;
  bool can_use_snapshot_;
  bool template_loaded_;

  using PerfTime = long long;
  PerfTime actual_fmp_start_;
  PerfTime actual_fmp_end_;

  Delegate& delegate_;
  I18n i18n;

  std::unique_ptr<TouchEventHandler> touch_event_handler_;

  std::unique_ptr<AirTouchEventHandlerBase> air_touch_event_handler_;

  std::atomic<bool> has_load_page_;
  //  std::string page_name_;
  std::shared_ptr<PageConfig> page_config_;

  std::string platform_config_json_string_;

  const int32_t instance_id_;
  bool destroyed_;
  lepus::Value default_processor_;
  std::unordered_map<std::string, lepus::Value> processor_with_name_;
  // key: [0]entry_name -> [1]component_path -> [3]processor_name
  // value: processor
  typedef std::unordered_map<
      std::string,
      std::unordered_map<std::string,
                         std::unordered_map<std::string, lepus::Value>>>
      ComponentProcessorMap;
  ComponentProcessorMap component_processor_with_name_;

  lepus::Value global_props_;  // cache globalProps
  std::string url_;
  size_t source_size_;
  bool is_loading_template_;
  float font_scale_;
  std::unordered_map<std::string, lepus::Value> lepus_event_listeners_;

  std::shared_ptr<lepus::InspectorLepusObserver> lepus_observer_;

  std::shared_ptr<LazyBundleLoader> component_loader_;
  std::string locale_;

  TemplateAssembler(const TemplateAssembler&) = delete;
  TemplateAssembler& operator=(const TemplateAssembler&) = delete;

  ALLOW_UNUSED_TYPE int64_t record_id_ = 0;

  // enable updateData before loadTemplate
  bool enable_pre_update_data_{false};
  // data updated before loadTemplate
  std::vector<std::shared_ptr<TemplateData>> cache_data_;

  bool pre_painting_{false};

  std::string android_package_external_path;

  std::shared_ptr<WhiteBoardDelegate> white_board_delegate_{nullptr};

  std::unique_ptr<ContextProxyInLepus>
      context_proxy_vector_[static_cast<int32_t>(
          runtime::ContextProxy::Type::kUnknown)] = {nullptr};

  SignalContext signal_context_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TEMPLATE_ASSEMBLER_H_
