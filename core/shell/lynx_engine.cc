// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_engine.h"

#include <memory>

#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/event/event_dispatcher.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/common/jsi_object_wrapper.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/shared_data/white_board_delegate.h"
#include "core/shell/common/shell_trace_event_def.h"
#include "core/shell/layout_mediator.h"
#include "core/shell/tasm_mediator.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace shell {

namespace {

inline bool MergeCacheDataOp(lepus::Value& target,
                             const std::vector<CacheDataOp>& caches) {
  for (const auto& cache : caches) {
    DCHECK(target.IsTable() && cache.GetValue().IsTable());
    if (cache.GetType() == CacheDataType::RESET) {
      return false;
    }
    for (const auto& cache_pair : *(cache.GetValue().Table())) {
      target.Table()->SetValue(cache_pair.first, cache_pair.second);
    }
  }
  return true;
}

// ensure access on tasm thread
inline std::string& GetCoreJS() {
  static base::NoDestructor<std::string> core_js;
  return *core_js;
}

}  // namespace

LynxEngine::~LynxEngine() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_ENGINE_DESTRUCTOR);
  // TODO(heshan): now is nullptr when run unittest, in fact cannot be nullptr
  // when runtime, will remove when LynxEngine no longer be a wrapper for tasm
  if (tasm_ != nullptr) {
    tasm_->Destroy();
  }
}

void LynxEngine::Init() {
  auto& client = tasm_->page_proxy()->element_manager();
  /**
   * Init vsync_monitor here to ensure CADisplayLink on iOS platform
   * can be added to the right runloop when applying MostOnTasm or other
   * non-AllOnUI thread strategies.
   */
  if (client != nullptr && client->vsync_monitor() != nullptr) {
    client->vsync_monitor()->BindToCurrentThread();
    client->vsync_monitor()->Init();
  }
}

void LynxEngine::LoadTemplate(
    const std::string& url, std::vector<uint8_t> source,
    const std::shared_ptr<tasm::TemplateData>& template_data,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_ENGINE_LOAD_TEMPLATE, "url", url,
              INSTANCE_ID, instance_id_);
  tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(),
                                               pipeline_options);
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kFfiEnd);
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kLoadTemplateTask,
      tasm::timing::kTaskNameLynxEngineLoadTemplate);
  tasm_->LoadTemplate(url, std::move(source), template_data, pipeline_options);
}

void LynxEngine::LoadTemplateBundle(
    const std::string& url, tasm::LynxTemplateBundle template_bundle,
    const std::shared_ptr<tasm::TemplateData>& template_data,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_ENGINE_LOAD_TEMPLATE_BUNDLE, "url", url,
              INSTANCE_ID, instance_id_);
  tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(),
                                               pipeline_options);
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kFfiEnd);
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kLoadTemplateTask,
      tasm::timing::kTaskNameLynxEngineLoadTemplate);
  tasm_->LoadTemplateBundle(url, std::move(template_bundle), template_data,
                            pipeline_options);
}

void LynxEngine::LoadSSRData(
    std::vector<uint8_t> source,
    const std::shared_ptr<tasm::TemplateData>& template_data,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(),
                                               pipeline_options);
  tasm_->RenderPageWithSSRData(std::move(source), template_data,
                               pipeline_options);
}

void LynxEngine::UpdateDataByParsedData(
    const std::shared_ptr<tasm::TemplateData>& data,
    uint32_t native_update_data_order,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kUpdateDataByNativeTask,
      tasm::timing::kTaskNameLynxEngineUpdateDataByParsedData);
  tasm::UpdatePageOption update_page_option;
  update_page_option.from_native = true;
  update_page_option.native_update_data_order_ = native_update_data_order;
  tasm_->UpdateDataByPreParsedData(data, update_page_option, pipeline_options);
}

void LynxEngine::UpdateMetaData(
    const std::shared_ptr<tasm::TemplateData>& data,
    const lepus::Value& global_props, uint32_t native_update_data_order,
    LynxUpdateMode update_mode,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm::UpdatePageOption update_page_option;
  update_page_option.from_native = true;
  update_page_option.reset_page_data = (update_mode == LynxUpdateMode::RESET);
  update_page_option.native_update_data_order_ = native_update_data_order;
  tasm_->UpdateMetaData(data, global_props, update_page_option,
                        pipeline_options);
}

void LynxEngine::ResetDataByParsedData(
    const std::shared_ptr<tasm::TemplateData>& data,
    uint32_t native_update_data_order,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kUpdateDataByNativeTask,
      tasm::timing::kTaskNameLynxEngineResetDataByParsedData);
  tasm::UpdatePageOption update_page_option;
  update_page_option.from_native = true;
  update_page_option.reset_page_data = true;
  update_page_option.native_update_data_order_ = native_update_data_order;
  tasm_->UpdateDataByPreParsedData(data, update_page_option, pipeline_options);
}

void LynxEngine::ReloadTemplate(
    const std::shared_ptr<tasm::TemplateData>& data,
    const lepus::Value& global_props, uint32_t native_update_data_order,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm::UpdatePageOption update_page_option;
  update_page_option.native_update_data_order_ = native_update_data_order;
  tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(),
                                               pipeline_options);
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kFfiEnd);
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kLoadTemplateTask,
      tasm::timing::kTaskNameLynxEngineReloadTemplate);
  tasm_->ReloadTemplate(data, global_props, update_page_option,
                        pipeline_options);
}

void LynxEngine::UpdateConfig(
    const lepus::Value& config,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm_->UpdateConfig(config, false, pipeline_options);
}

void LynxEngine::UpdateGlobalProps(
    const lepus::Value& global_props,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kUpdateDataByNativeTask,
      tasm::timing::kTaskNameLynxEngineUpdateGlobalProps);
  tasm_->UpdateGlobalProps(global_props, true, pipeline_options);
}

void LynxEngine::UpdateFontScale(float scale) {
  auto& client = tasm_->page_proxy()->element_manager();
  if (client != nullptr) {
    client->UpdateFontScale(scale);
    tasm_->OnFontScaleChanged(scale);
  }
}

void LynxEngine::SetFontScale(float scale) {
  auto& client = tasm_->page_proxy()->element_manager();
  if (client != nullptr) {
    tasm_->SetFontScale(scale);
    client->UpdateFontScale(scale);
  }
}

void LynxEngine::SetContextHasAttached() {
  tasm_->page_proxy()
      ->element_manager()
      ->painting_context()
      ->SetContextHasAttached();
}

void LynxEngine::SetPlatformConfig(std::string platform_config_json_string) {
  tasm_->SetPlatformConfig(std::move(platform_config_json_string));
}

void LynxEngine::SetAnimationsPending(bool need_pending_ui_op) {
  if (need_pending_ui_op) {
    tasm_->page_proxy()->element_manager()->PauseAllAnimations();
  } else {
    tasm_->page_proxy()->element_manager()->ResumeAllAnimations();
  }
}

void LynxEngine::UpdateScreenMetrics(float width, float height,
                                     float device_pixel_ratio) {
  tasm_->OnScreenMetricsSet(width, height, device_pixel_ratio);
}

void LynxEngine::UpdateViewport(float width, int32_t width_mode, float height,
                                int32_t height_mode, bool need_layout) {
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, LYNX_ENGINE_UPDATE_VIEWPORT,
      [&](lynx::perfetto::EventContext ctx) {
        std::string view_port_info_str =
            base::FormatString("size: %.1f, %.1f; mode: %d, %d", width, height,
                               width_mode, height_mode);
        ctx.event()->add_debug_annotations("viewport", view_port_info_str);
      });
  tasm_->page_proxy()->element_manager()->UpdateViewport(
      width, static_cast<SLMeasureMode>(width_mode), height,
      static_cast<SLMeasureMode>(height_mode), need_layout);
}

void LynxEngine::SyncFetchLayoutResult() {
  auto& element_manager = tasm_->page_proxy()->element_manager();
  LayoutMediator::HandleLayoutVoluntarily(operation_queue_.get(),
                                          element_manager->catalyzer(),
                                          element_manager->GetPageOptions());
}

void LynxEngine::SendEvent(int32_t target_id, fml::RefPtr<event::Event> event) {
  auto target =
      tasm_->page_proxy()->element_manager()->node_manager()->Get(target_id);
  if (target) {
    event::EventDispatcher::DispatchEvent(*target, std::move(event));
  }
}

void LynxEngine::SendAirPageEvent(const std::string& name,
                                  const lepus_value& params) {}

void LynxEngine::SendCustomEvent(const std::string& name, int32_t tag,
                                 const lepus::Value& params,
                                 const std::string& params_name) {
  tasm_->SendCustomEvent(name, tag, params, params_name);
}

void LynxEngine::SendTouchEvent(const std::string& name,
                                const tasm::EventInfo& info) {
  tasm_->SendTouchEvent(name, info);
}

void LynxEngine::SendGestureEvent(int tag, int gesture_id, std::string name,
                                  const lepus::Value& params) {
  tasm_->SendGestureEvent(tag, gesture_id, name, params);
}

void LynxEngine::OnPseudoStatusChanged(int32_t id, int32_t pre_status,
                                       int32_t current_status) {
  tasm_->OnPseudoStatusChanged(id, pre_status, current_status);
}

void LynxEngine::StartEventGenerate(const lepus::Value& event_params) {
  tasm_->StartEventGenerate(event_params);
}

void LynxEngine::StartEventCapture(int64_t event_id) {
  tasm_->StartEventCapture(event_id);
}

void LynxEngine::StartEventBubble(int64_t event_id) {
  tasm_->StartEventBubble(event_id);
}

void LynxEngine::StartEventFire(bool is_stop, int64_t event_id) {
  tasm_->StartEventFire(is_stop, event_id);
}

void LynxEngine::SendBubbleEvent(const std::string& name, int32_t tag,
                                 lepus::DictionaryPtr dict) {
  tasm_->SendBubbleEvent(name, tag, dict);
}

void LynxEngine::SendGlobalEventToLepus(const std::string& name,
                                        const lepus_value& params) {
  tasm_->SendGlobalEventToLepus(name, params);
}

void LynxEngine::TriggerEventBus(const std::string& name,
                                 const lepus_value& params) {
  tasm_->TriggerEventBus(name, params);
}

void LynxEngine::DidLoadComponentFromJS(
    tasm::LazyBundleLoader::CallBackInfo callback_info) {
  auto pipeline_options = std::make_shared<tasm::PipelineOptions>();
  if (delegate_) {
    delegate_->OnPipelineStart(pipeline_options->pipeline_id,
                               pipeline_options->pipeline_origin,
                               pipeline_options->pipeline_start_timestamp);
  }
  tasm_->LoadComponentWithCallbackInfo(std::move(callback_info),
                                       pipeline_options);
}

void LynxEngine::DidLoadComponent(
    lynx::tasm::LazyBundleLoader::CallBackInfo callback_info) {
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kNativeFuncTask,
      tasm::timing::kTaskNameLynxEngineDidLoadComponent,
      callback_info.component_url);
  auto pipeline_options = std::make_shared<tasm::PipelineOptions>();
  if (delegate_) {
    delegate_->OnPipelineStart(pipeline_options->pipeline_id,
                               pipeline_options->pipeline_origin,
                               pipeline_options->pipeline_start_timestamp);
  }
  tasm_->DidLoadComponent(std::move(callback_info), pipeline_options);
}

void LynxEngine::DidFetchBundle(
    lynx::tasm::LazyBundleLoader::CallBackInfo callback_info) {
  tasm_->DidFetchBundle(std::move(callback_info));
}

std::unique_ptr<lepus_value> LynxEngine::GetCurrentData() {
  return tasm_->GetCurrentData();
}

lepus::Value LynxEngine::GetPageDataByKey(
    const std::vector<std::string>& keys) {
  return tasm_->GetPageDataByKey(keys);
}

tasm::ListNode* LynxEngine::GetListNode(int32_t tag) {
  // client maybe nullptr
  if (tasm_ == nullptr) {
    return nullptr;
  }
  auto& client = tasm_->page_proxy()->element_manager();
  if (client == nullptr) {
    return nullptr;
  }
  lynx::tasm::Element* element = client->node_manager()->Get(tag);
  if (element == nullptr) {
    return nullptr;
  }
  return element->GetListNode();
}

std::list<int32_t> LynxEngine::GetAncestorElements(int32_t tag) {
  // client maybe nullptr
  if (tasm_ == nullptr) {
    return {};
  }
  auto& client = tasm_->page_proxy()->element_manager();
  if (client == nullptr) {
    return {};
  }
  std::list<int32_t> elements;
  lynx::tasm::Element* element = client->node_manager()->Get(tag);
  while (element != nullptr) {
    elements.emplace_back(element->impl_id());
    element = element->parent();
  }
  return elements;
}

// (TODO)fangzhou.fz: Putting these list-related methods here directly is
// inappropriate.
void LynxEngine::ScrollByListContainer(int32_t tag, float content_offset_x,
                                       float content_offset_y, float original_x,
                                       float original_y) {
  if (tasm_ == nullptr) {
    return;
  }
  auto& client = tasm_->page_proxy()->element_manager();
  if (client == nullptr) {
    return;
  }
  lynx::tasm::Element* element = client->node_manager()->Get(tag);
  if (element != nullptr) {
    element->ScrollByListContainer(content_offset_x, content_offset_y,
                                   original_x, original_y);
  }
}

void LynxEngine::ScrollToPosition(int32_t tag, int index, float offset,
                                  int align, bool smooth) {
  if (tasm_ == nullptr) {
    return;
  }
  auto& client = tasm_->page_proxy()->element_manager();
  if (client == nullptr) {
    return;
  }
  lynx::tasm::Element* element = client->node_manager()->Get(tag);
  if (element != nullptr) {
    element->ScrollToPosition(index, offset, align, smooth);
  }
}

void LynxEngine::ScrollStopped(int32_t tag) {
  if (tasm_ == nullptr) {
    return;
  }
  auto& client = tasm_->page_proxy()->element_manager();
  if (client == nullptr) {
    return;
  }
  lynx::tasm::Element* element = client->node_manager()->Get(tag);
  if (element != nullptr) {
    element->ScrollStopped();
  }
}

std::unordered_map<std::string, std::string> LynxEngine::GetAllJsSource() {
  std::unordered_map<std::string, std::string> source;
  tasm_->GetDecodedJSSource(source);
  source.emplace("core.js", GetCoreJS());
  return source;
}

void LynxEngine::UpdateDataByJS(runtime::UpdateDataTask task) {
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kUpdateDataByJSTask,
      tasm::timing::kTaskNameLynxEngineUpdateDataByJS);
  auto& pipeline_options = task.pipeline_options_;
  tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(),
                                               pipeline_options);
  auto cached_page_data = card_cached_data_mgr_->GetCardCacheData();
  if (MergeCacheDataOp(task.data_, cached_page_data)) {
    tasm_->UpdateDataByJS(task, pipeline_options);
  }
  card_cached_data_mgr_->DecrementTaskCount();
  if (!cached_page_data.empty()) {
    delegate_->NotifyJSUpdatePageData();
  }
  delegate_->CallJSApiCallback(task.callback_);
}

void LynxEngine::UpdateBatchedDataByJS(
    std::vector<runtime::UpdateDataTask> tasks, uint64_t update_task_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_BATCHED_UPDATE_DATA,
              [=](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_terminating_flow_ids(update_task_id);
              });
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kUpdateDataByJSTask,
      tasm::timing::kTaskNameLynxEngineUpdateBatchedDataByJS);
  auto cached_page_data = card_cached_data_mgr_->GetCardCacheData();
  for (auto& task : tasks) {
    auto& pipeline_options = task.pipeline_options_;
    tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(),
                                                 pipeline_options);
    if (task.is_card_) {
      if (MergeCacheDataOp(task.data_, cached_page_data)) {
        tasm_->UpdateDataByJS(task, pipeline_options);
      }
      delegate_->CallJSApiCallback(task.callback_);
    } else {
      tasm_->UpdateComponentData(task, task.pipeline_options_);
    }
  }

  card_cached_data_mgr_->DecrementTaskCount();
  if (!cached_page_data.empty()) {
    delegate_->NotifyJSUpdatePageData();
  }
}

void LynxEngine::TriggerComponentEvent(const std::string& event_name,
                                       const lepus::Value& msg) {
  tasm_->TriggerComponentEvent(event_name, msg);
}

void LynxEngine::TriggerLepusGlobalEvent(const std::string& event_name,
                                         const lepus::Value& msg) {
  tasm_->TriggerLepusGlobalEvent(event_name, msg);
}

void LynxEngine::TriggerWorkletFunction(std::string component_id,
                                        std::string worklet_module_name,
                                        std::string method_name,
                                        lepus::Value args,
                                        piper::ApiCallBack callback) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, ENGINE_TRIGGER_WORKLET_FUNC,
      [flow_id = callback.trace_flow_id()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_flow_ids(flow_id);
      });
  tasm_->TriggerWorkletFunction(
      std::move(component_id), std::move(worklet_module_name),
      std::move(method_name), std::move(args), std::move(callback));
}

// TODO(zhangqun.29): Merge the calls of three scenes
void LynxEngine::InvokeLepusCallback(const int32_t callback_id,
                                     const std::string& entry_name,
                                     const lepus::Value& data,
                                     bool from_mts_module) {
  if (tasm_->page_proxy()->element_manager()->IsAirModeFiberEnabled()) {
    tasm_->InvokeCacheLepusCallback(callback_id, entry_name,
                                    std::vector<lepus::Value>{data});
    return;
  }
  if (from_mts_module) {
    std::vector<lepus::Value> params_vec;
    if (data.IsArrayOrJSArray()) {
      size_t size = data.Array()->size();
      for (size_t i = 0; i < size; ++i) {
        params_vec.push_back(data.GetProperty(static_cast<int>(i)));
      }
    } else {
      params_vec.push_back(data);
    }
    tasm_->InvokeCacheLepusCallback(callback_id, entry_name, params_vec);
    return;
  }
  tasm_->InvokeLepusCallback(callback_id, entry_name, data);
}

void LynxEngine::InvokeLepusComponentCallback(const int64_t callback_id,
                                              const std::string& entry_name,
                                              const lepus::Value& data) {
  tasm_->InvokeLepusComponentCallback(callback_id, entry_name, data);
}

void LynxEngine::UpdateComponentData(runtime::UpdateDataTask task) {
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kUpdateDataByJSTask,
      tasm::timing::kTaskNameLynxEngineUpdateComponentData);
  auto& pipeline_options = task.pipeline_options_;
  tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(),
                                               pipeline_options);
  tasm_->UpdateComponentData(task, task.pipeline_options_);
}

void LynxEngine::SelectComponent(const std::string& component_id,
                                 const std::string& id_selector,
                                 const bool single,
                                 piper::ApiCallBack callBack) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, ENGINE_SELECT_COMPONENT,
      [flow_id = callBack.trace_flow_id()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_flow_ids(flow_id);
      });
  tasm_->SelectComponent(component_id, id_selector, single, callBack);
}

void LynxEngine::ElementAnimate(const std::string& component_id,
                                const std::string& id_selector,
                                const lepus::Value& args) {
  tasm_->ElementAnimate(component_id, id_selector, args);
}

void LynxEngine::ElementAnimateV2(const std::string& component_id,
                                  const std::string& id_selector,
                                  const lepus::Value& args) {
  tasm_->ElementAnimateV2(component_id, id_selector, args);
}

void LynxEngine::GetComponentContextDataAsync(const std::string& component_id,
                                              const std::string& key,
                                              piper::ApiCallBack callback) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, ENGINE_GET_COMPONENT_CONTEXT_DATA_ASYNC,
      [flow_id = callback.trace_flow_id()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_flow_ids(flow_id);
      });
  tasm_->GetComponentContextDataAsync(component_id, key, callback);
}

void LynxEngine::UpdateCoreJS(std::string core_js) {
  GetCoreJS().assign(std::move(core_js));
}

void LynxEngine::UpdateI18nResource(const std::string& key,
                                    const std::string& new_data) {
  tasm_->UpdateI18nResource(key, new_data);
}

void LynxEngine::Flush() {
  if (tasm_ != nullptr &&
      !tasm_->GetPageOptions()
           .IsEmbeddedModeOn()) {  // for unittest, judge null
    tasm_->page_proxy()->element_manager()->painting_context()->Flush();
  }
}

tasm::TemplateAssembler* LynxEngine::GetTasm() { return tasm_.get(); }

void LynxEngine::SetCSSVariables(
    const std::string& component_id, const std::string& id_selector,
    const lepus::Value& properties,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm_->SetCSSVariables(component_id, id_selector, properties,
                         pipeline_options);
}

void LynxEngine::SetNativeProps(
    const tasm::NodeSelectRoot& root, const tasm::NodeSelectOptions& options,
    const lepus::Value& native_props,
    std::shared_ptr<tasm::PipelineOptions> pipeline_options) {
  tasm_->SetNativeProps(root, options, native_props, pipeline_options);
}

void LynxEngine::ReloadFromJS(runtime::UpdateDataTask task) {
  auto& pipeline_options = task.pipeline_options_;
  tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(),
                                               pipeline_options);
  tasm_->ReloadFromJS(task, pipeline_options);
  delegate_->CallJSApiCallback(task.callback_);
}

void LynxEngine::AddFont(const lepus::Value& font,
                         piper::ApiCallBack callback) {
  tasm_->AddFont(font);
  delegate_->CallJSApiCallback(std::move(callback));
}

void LynxEngine::FetchBundle(
    const std::string& bundle_url,
    const std::shared_ptr<runtime::ResponsePromise<tasm::BundleResourceInfo>>&
        response_promise) {
  tasm_->FetchBundle(bundle_url, response_promise);
}

void LynxEngine::InvokeUIMethod(const tasm::NodeSelectRoot& root,
                                const tasm::NodeSelectOptions& options,
                                const std::string& method,
                                fml::RefPtr<tasm::PropBundle> params,
                                piper::ApiCallBack callback) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, ENGINE_INVOKE_UI_METHOD,
      [flow_id = callback.trace_flow_id()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_flow_ids(flow_id);
      });
  auto result = tasm_->page_proxy()->GetLynxUI(root, options);
  if (!result.Success()) {
    delegate_->CallJSApiCallbackWithValue(callback,
                                          result.StatusAsLepusValue());
    return;
  }

  delegate_->InvokeUIMethod(std::move(result), method, std::move(params),
                            callback);
}

void LynxEngine::InvokeUIMethod(const tasm::NodeSelectRoot& root,
                                const tasm::NodeSelectOptions& options,
                                const std::string& method,
                                const pub::Value& params,
                                piper::ApiCallBack callback) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, ENGINE_INVOKE_UI_METHOD,
      [flow_id = callback.trace_flow_id()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_flow_ids(flow_id);
      });
  auto result = tasm_->page_proxy()->GetLynxUI(root, options);
  if (!result.Success()) {
    delegate_->CallJSApiCallbackWithValue(callback,
                                          result.StatusAsLepusValue());
    return;
  }

  delegate_->InvokeUIMethod(std::move(result), method, params, callback);
}

void LynxEngine::GetPathInfo(const tasm::NodeSelectRoot& root,
                             const tasm::NodeSelectOptions& options,
                             piper::ApiCallBack call_back) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ENGINE_GET_PATH_INFO,
              [&call_back](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(call_back.trace_flow_id());
              });
  auto result = tasm_->page_proxy()->GetPathInfo(root, options);
  delegate_->CallJSApiCallbackWithValue(call_back, result);
}

void LynxEngine::GetFields(const tasm::NodeSelectRoot& root,
                           const tasm::NodeSelectOptions& options,
                           const std::vector<std::string>& fields,
                           piper::ApiCallBack call_back) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ENGINE_GET_FIELDS,
              [&call_back](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(call_back.trace_flow_id());
              });
  auto result = tasm_->page_proxy()->GetFields(root, options, fields);
  delegate_->CallJSApiCallbackWithValue(call_back, result);
}

tasm::LynxGetUIResult LynxEngine::GetLynxUI(
    const tasm::NodeSelectRoot& root, const tasm::NodeSelectOptions& options) {
  return tasm_->page_proxy()->GetLynxUI(root, options);
}

void LynxEngine::SetInspectorElementObserver(
    const std::shared_ptr<tasm::InspectorElementObserver>&
        inspector_element_observer) {
  tasm_->page_proxy()->element_manager()->SetInspectorElementObserver(
      inspector_element_observer);
}

void LynxEngine::CallLepusMethod(const std::string& method_name,
                                 lepus::Value args,
                                 const piper::ApiCallBack& callback) {
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kNativeFuncTask,
      tasm::timing::kTaskNameLynxEngineCallLepusMethod, method_name);
  tasm_->CallLepusMethod(method_name, std::move(args), callback);
}

void LynxEngine::GetJSSessionStorage(const std::string& key,
                                     const piper::ApiCallBack& callback) {
  auto white_board_delegate = tasm_->GetWhiteBoardDelegate();
  if (white_board_delegate) {
    TRACE_EVENT(
        LYNX_TRACE_CATEGORY, ENGINE_GET_JS_SESSION_STORAGE,
        [flow_id = callback.trace_flow_id()](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_flow_ids(flow_id);
        });
    auto value = white_board_delegate->GetSessionStorageItem(key);
    delegate_->CallJSApiCallbackWithValue(callback, value);
  }
}

void LynxEngine::SubscribeJSSessionStorage(const std::string& key,
                                           double listener_id,
                                           const piper::ApiCallBack& callback) {
  auto white_board_delegate = tasm_->GetWhiteBoardDelegate();
  if (white_board_delegate) {
    white_board_delegate->SubscribeJSSessionStorage(key, listener_id, callback);
  }
}

void LynxEngine::SetClientSessionStorage(const std::string& key,
                                         const lepus::Value& value) {
  auto white_board_delegate = tasm_->GetWhiteBoardDelegate();
  if (white_board_delegate) {
    white_board_delegate->SetSessionStorageItem(key, value);
  }
}

void LynxEngine::GetClientSessionStorage(
    const std::string& key,
    const std::shared_ptr<shell::PlatformCallBackHolder>& callback) {
  auto white_board_delegate = tasm_->GetWhiteBoardDelegate();
  if (white_board_delegate) {
    auto value = white_board_delegate->GetSessionStorageItem(key);
    delegate_->CallPlatformCallbackWithValue(callback, value);
  }
}

void LynxEngine::SubscribeClientSessionStorage(
    const std::string& key,
    const std::shared_ptr<shell::PlatformCallBackHolder>& callback) {
  auto white_board_delegate = tasm_->GetWhiteBoardDelegate();
  if (white_board_delegate) {
    white_board_delegate->SubScribeClientSessionStorage(key,
                                                        std::move(callback));
  }
}

void LynxEngine::UnsubscribeClientSessionStorage(const std::string& key,
                                                 double callback_id) {
  auto white_board_delegate = tasm_->GetWhiteBoardDelegate();
  if (white_board_delegate) {
    white_board_delegate->UnsubscribeClientSessionStorage(key, callback_id);
  }
}

#if ENABLE_TESTBENCH_RECORDER
void LynxEngine::SetRecordID(int64_t record_id) {
  tasm_->SetRecordID(record_id);
}
#endif

void LynxEngine::PreloadLazyBundles(const std::vector<std::string>& urls) {
  tasm_->PreloadLazyBundles(urls);
}

void LynxEngine::InsertLynxTemplateBundle(
    const std::string& url, lynx::tasm::LynxTemplateBundle bundle) {
  tasm_->InsertLynxTemplateBundle(url, std::move(bundle));
}

void LynxEngine::OnReceiveMessageEvent(
    fml::RefPtr<runtime::MessageEvent> event) {
  tasm_->OnReceiveMessageEvent(std::move(event));
}

}  // namespace shell
}  // namespace lynx
