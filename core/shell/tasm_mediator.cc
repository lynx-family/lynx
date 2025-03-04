// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/tasm_mediator.h"

#include <utility>

#include "base/include/value/base_string.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/runtime/vm/lepus/tasks/lepus_callback_manager.h"
#include "core/shell/common/vsync_monitor.h"
#include "core/shell/lynx_actor_specialization.h"

namespace lynx {
namespace shell {

TasmMediator::TasmMediator(
    const std::shared_ptr<LynxActor<NativeFacade>>& facade_actor,
    const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr,
    const std::shared_ptr<VSyncMonitor>& vsync_monitor,
    const std::shared_ptr<LynxActor<tasm::LayoutContext>>& layout_actor,
    std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker,
    const std::shared_ptr<LynxActor<tasm::timing::TimingHandler>>& timing_actor)
    : facade_actor_(facade_actor),
      layout_actor_(layout_actor),
      timing_actor_(timing_actor),
      card_cached_data_mgr_(card_cached_data_mgr),
      vsync_monitor_(vsync_monitor),
      tasm_platform_invoker_(std::move(tasm_platform_invoker)) {}

TasmMediator::~TasmMediator() = default;

void TasmMediator::Init() {
  vsync_monitor_->BindToCurrentThread();
  vsync_monitor_->Init();
}

void TasmMediator::OnDataUpdated() {
  facade_actor_->Act([](auto& facade) { facade->OnDataUpdated(); });
}

void TasmMediator::OnTasmFinishByNative() {
  facade_actor_->Act([](auto& facade) { facade->OnTasmFinishByNative(); });
}

void TasmMediator::OnTemplateLoaded(const std::string& url) {
  facade_actor_->Act([url](auto& facade) { facade->OnTemplateLoaded(url); });
}

void TasmMediator::OnSSRHydrateFinished(const std::string& url) {
  facade_actor_->Act(
      [url](auto& facade) { facade->OnSSRHydrateFinished(url); });
}

void TasmMediator::OnErrorOccurred(base::LynxError error) {
  facade_actor_->ActAsync(
      [error = std::move(error)](auto& facade) { facade->ReportError(error); });
}
void TasmMediator::TriggerLepusngGc(base::closure func) {
  // TODO: the api will be implemented for performance in the future.
  (void)func;
}

void TasmMediator::OnDynamicComponentPerfReady(const lepus::Value& perf_info) {
  facade_actor_->Act([perf_info](auto& facade) {
    facade->OnDynamicComponentPerfReady(perf_info);
  });

  // Make sure that event send after JSPrepared.
  static constexpr const char kOnDynamicComponentPerf[] =
      "onDynamicComponentPerf";
  engine_actor_->ActAsync([this, perf_info](auto& engine) {
    auto arguments = lepus::CArray::Create();
    arguments->emplace_back(BASE_STATIC_STRING(kOnDynamicComponentPerf));
    arguments->push_back(perf_info);
    CallJSFunction("GlobalEventEmitter", "trigger",
                   lepus::Value(std::move(arguments)));
  });
}

void TasmMediator::OnConfigUpdated(const lepus::Value& data) {
  facade_actor_->Act([data](auto& facade) { facade->OnConfigUpdated(data); });
}

void TasmMediator::CallPlatformCallbackWithValue(
    const std::shared_ptr<PlatformCallBackHolder>& callback,
    const lepus::Value& value) {
  facade_actor_->Act([callback, value](auto& facade) {
    facade->InvokePlatformCallBackWithValue(callback, value);
  });
}

void TasmMediator::RemovePlatformCallback(
    const std::shared_ptr<shell::PlatformCallBackHolder>& callback) {
  facade_actor_->Act(
      [callback](auto& facade) { facade->RemovePlatformCallBack(callback); });
}

void TasmMediator::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  tasm_platform_invoker_->OnPageConfigDecoded(config);
  timing_actor_->Act(
      [enable_air = config->GetEnableLynxAir()](auto& timing_handler) mutable {
        timing_handler->SetEnableAirStrictMode(enable_air);
      });
}

void TasmMediator::SetTiming(tasm::Timing timing) {
  timing_actor_->Act(
      [timing = std::move(timing)](auto& timing_handler) mutable {
        timing_handler->SetTiming(std::move(timing));
      });
}

void TasmMediator::BindPipelineIDWithTimingFlag(
    const tasm::PipelineID& pipeline_id,
    const tasm::timing::TimingFlag& timing_flag) {
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, "Timing::BindPipelineIDWithTimingFlag",
      [&pipeline_id, timing_flag](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("pipeline_id", pipeline_id);
        ctx.event()->add_debug_annotations("timing_flag", timing_flag);
      });
  timing_actor_->Act([pipeline_id, timing_flag](auto& timing_handler) {
    timing_handler->BindPipelineIDWithTimingFlag(pipeline_id, timing_flag);
  });
}

void TasmMediator::OnPipelineStart(
    const tasm::PipelineID& pipeline_id,
    const tasm::PipelineOrigin& pipeline_origin,
    tasm::timing::TimestampUs pipeline_start_timestamp) {
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, "Timing::OnPipelineStart",
      [&pipeline_id, &pipeline_origin,
       pipeline_start_timestamp](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("pipeline_id", pipeline_id);
        ctx.event()->add_debug_annotations("pipeline_origin", pipeline_origin);
        ctx.event()->add_debug_annotations(
            "pipeline_start_timestamp",
            std::to_string(pipeline_start_timestamp));
      });
  timing_actor_->Act([pipeline_id, pipeline_origin,
                      pipeline_start_timestamp](auto& timing_actor) {
    timing_actor->OnPipelineStart(pipeline_id, pipeline_origin,
                                  pipeline_start_timestamp);
  });
}

void TasmMediator::ResetTimingBeforeReload(const std::string& flag) {
  // TODO(kechenglong): Temporary API, will be removed after pipelineOptions
  // finished pre-created.
  timing_actor_->Act([flag](auto& timing_handler) mutable {
    timing_handler->ResetTimingBeforeReload(flag);
  });
}

lepus::Value TasmMediator::TriggerLepusMethod(const std::string& method_name,
                                              const lepus::Value& arguments) {
  return tasm_platform_invoker_->TriggerLepusMethod(method_name, arguments);
}

void TasmMediator::TriggerLepusMethodAsync(const std::string& method_name,
                                           const lepus::Value& arguments,
                                           bool is_air) {
#if ENABLE_AIR
  if (is_air) {
    tasm_platform_invoker_->TriggerLepusMethodAsync(method_name, arguments);
    return;
  }
#endif
  facade_actor_->Act([method_name, arguments](auto& facade) {
    facade->TriggerLepusMethodAsync(method_name, arguments);
  });
}

void TasmMediator::LepusInvokeUIMethod(
    std::vector<int32_t> ui_impl_ids, const std::string& method,
    const lepus::Value& params, lepus::Context* context,
    std::unique_ptr<lepus::Value> callback_closure) {
#if ENABLE_AIR
  auto callback_manager = context->GetCallbackManager();
  auto task_id =
      callback_manager->CacheTask(context, std::move(callback_closure));
  auto value_impl = std::make_unique<pub::ValueImplLepus>(params);
  auto prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps(*value_impl);
  InvokeUIMethod(tasm::LynxGetUIResult(std::move(ui_impl_ids), 0, ""), method,
                 std::move(prop_bundle),
                 piper::ApiCallBack(static_cast<int>(task_id)));
#endif
}

void TasmMediator::NotifyJSUpdatePageData() {
  // if there also has a "UpdateDataByJS" task pending in tasm thread, do
  // nothing,  "UpdateNativeData" will call "NotifyJSUpdatePageData" again
  runtime_actor_->ActAsync(
      [card_cached_data_mgr = card_cached_data_mgr_](auto& runtime) mutable {
        if (card_cached_data_mgr->GetTaskCount() <= 0) {
          runtime->NotifyJSUpdatePageData();
        }
      });
}

void TasmMediator::OnCardConfigDataChanged(const lepus::Value& data) {
  runtime_actor_->ActAsync(
      [safe_data = lepus_value::ShallowCopy(data)](auto& runtime) {
        runtime->OnCardConfigDataChanged(safe_data);
        runtime->NotifyJSUpdateCardConfigData();
      });
}

void TasmMediator::RequestVsync(
    uintptr_t id, base::MoveOnlyClosure<void, int64_t, int64_t> callback) {
  if (!vsync_monitor_) {
    return;
  }
  vsync_monitor_->ScheduleVSyncSecondaryCallback(id, std::move(callback));
}

std::string TasmMediator::TranslateResourceForTheme(
    const std::string& res_id, const std::string& theme_key) {
  return tasm_platform_invoker_->TranslateResourceForTheme(res_id, theme_key);
}

void TasmMediator::GetI18nResource(const std::string& channel,
                                   const std::string& fallback_url) {
  tasm_platform_invoker_->GetI18nResource(channel, fallback_url);
}

void TasmMediator::OnJSSourcePrepared(
    tasm::TasmRuntimeBundle bundle, const lepus::Value& global_props,
    const std::string& page_name, tasm::PackageInstanceDSL dsl,
    tasm::PackageInstanceBundleModuleMode bundle_module_mode,
    const std::string& url, const tasm::PipelineOptions& pipeline_options) {
  runtime_actor_->ActAsync([bundle = std::move(bundle), global_props, page_name,
                            dsl, bundle_module_mode, url,
                            pipeline_options](auto& runtime) mutable {
    runtime->OnJSSourcePrepared(std::move(bundle), global_props, page_name, dsl,
                                bundle_module_mode, url, pipeline_options);
  });
}

void TasmMediator::CallJSApiCallback(piper::ApiCallBack callback) {
  // We should use TRACE_EVENT_FLOW_BEGIN0 instead of TRACE_EVENT here, because
  // we want to trace the whole flow of the ApiCallBack, not just the begin and
  // end of the ApiCallBack.
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallJSApiCallback",
              [=](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(callback.trace_flow_id());
              });

  runtime_actor_->ActAsync(
      [callback = std::move(callback)](auto& runtime) mutable {
        runtime->CallJSApiCallback(std::move(callback));
      });
}

void TasmMediator::CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                              const lepus::Value& value,
                                              bool persist) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallJSApiCallbackWithValue",
              [=](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_terminating_flow_ids(callback.trace_flow_id());
              });

  runtime_actor_->ActAsync([callback = std::move(callback),
                            safe_value = lepus_value::ShallowCopy(value),
                            persist](auto& runtime) mutable {
    runtime->CallJSApiCallbackWithValue(std::move(callback), safe_value,
                                        persist);
  });
}

void TasmMediator::RemoveJSApiCallback(piper::ApiCallBack callback) {
  runtime_actor_->Act([callback = std::move(callback)](auto& runtime) mutable {
    runtime->EraseJSApiCallback(std::move(callback));
  });
}

void TasmMediator::CallJSFunction(const std::string& module_id,
                                  const std::string& method_id,
                                  const lepus::Value& arguments,
                                  bool force_call_despite_app_state) {
  runtime_actor_->ActAsync([module_id, method_id,
                            safe_value = lepus_value::ShallowCopy(arguments),
                            force_call_despite_app_state](auto& runtime) {
    runtime->CallJSFunction(module_id, method_id, safe_value,
                            force_call_despite_app_state);
  });
}

void TasmMediator::OnJSAppReload(
    tasm::TemplateData data, const tasm::PipelineOptions& pipeline_options) {
  runtime_actor_->ActAsync(
      [data = std::move(data), pipeline_options](auto& runtime) mutable {
        runtime->OnAppReload(std::move(data), pipeline_options);
      });
}

void TasmMediator::OnLifecycleEvent(const lepus::Value& args) {
  runtime::MessageEvent event(runtime::kMessageEventTypeOnLifecycleEvent,
                              runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext, args);
  DispatchMessageEvent(std::move(event));
}

void TasmMediator::OnDataUpdatedByNative(tasm::TemplateData data,
                                         const bool reset) {
  // if the incoming value is read_only, it's unnecessary to clone.
  card_cached_data_mgr_->AddCardCacheData(
      std::move(data), reset ? CacheDataType::RESET : CacheDataType::UPDATE);
  NotifyJSUpdatePageData();
}

void TasmMediator::PrintMsgToJS(const std::string& level,
                                const std::string& msg) {
  runtime_actor_->ActAsync([level, msg](auto& runtime) {
    runtime->ConsoleLogWithLevel(level, msg);
  });
}

void TasmMediator::OnI18nResourceChanged(const std::string& msg) {
  runtime_actor_->ActAsync(
      [msg](auto& runtime) { runtime->I18nResourceChanged(msg); });
}

void TasmMediator::OnComponentDecoded(tasm::TasmRuntimeBundle bundle) {
  runtime_actor_->ActAsync([bundle = std::move(bundle)](auto& runtime) mutable {
    runtime->OnComponentDecoded(std::move(bundle));
  });
}

fml::RefPtr<fml::TaskRunner> TasmMediator::GetLepusTimedTaskRunner() {
  return engine_actor_->GetRunner();
}

// delegate for class element manager
void TasmMediator::DispatchLayoutUpdates(const tasm::PipelineOptions& options) {
  layout_actor_->Act(
      [options](auto& layout) { layout->DispatchLayoutUpdates(options); });
}

std::unordered_map<int32_t, tasm::LayoutInfoArray>
TasmMediator::GetSubTreeLayoutInfo(int32_t root_id, tasm::Viewport viewport) {
  return layout_actor_->ActSync([root_id, viewport](auto& layout) {
    return layout->GetSubTreeLayoutInfo(root_id, viewport);
  });
}

void TasmMediator::SetEnableLayout() {
  layout_actor_->Act([](auto& layout) { layout->SetEnableLayout(); });
}

void TasmMediator::SetRootOnLayout(int32_t id) {
  layout_actor_->ActLite([id](auto& layout) { layout->SetRoot(id); });
}

void TasmMediator::OnUpdateDataWithoutChange() {
  facade_actor_->Act([](auto& facade) { facade->OnUpdateDataWithoutChange(); });
}

void TasmMediator::OnUpdateViewport(float width, int width_mode, float height,
                                    int height_mode, bool need_layout) {
  layout_actor_->Act([width, width_mode, height, height_mode,
                      need_layout](auto& layout) {
    layout->UpdateViewport(width, width_mode, height, height_mode, need_layout);
  });
}

void TasmMediator::UpdateLynxEnvForLayoutThread(tasm::LynxEnvConfig env) {
  layout_actor_->ActLite(
      [env](auto& layout) { layout->UpdateLynxEnvForLayoutThread(env); });
}

// delegate for class element
void TasmMediator::CreateLayoutNode(int32_t id, const base::String& tag) {
  layout_actor_->ActLite(
      [id, tag](auto& layout) { layout->CreateLayoutNode(id, tag); });
}

void TasmMediator::UpdateLayoutNodeFontSize(int32_t id,
                                            double cur_node_font_size,
                                            double root_node_font_size,
                                            double font_scale) {
  layout_actor_->ActLite(
      [id, cur_node_font_size, root_node_font_size, font_scale](auto& layout) {
        layout->UpdateLayoutNodeFontSize(id, cur_node_font_size,
                                         root_node_font_size, font_scale);
      });
}

void TasmMediator::InsertLayoutNode(int32_t parent_id, int32_t child_id,
                                    int index) {
  layout_actor_->ActLite([parent_id, child_id, index](auto& layout) {
    layout->InsertLayoutNode(parent_id, child_id, index);
  });
}

void TasmMediator::SendAnimationEvent(const char* type, int tag,
                                      const lepus::Value& dict) {
  engine_actor_->ActLite([arguments = dict, tag, type](auto& engine) {
    engine->SendCustomEvent(type, tag, arguments, "params");
  });
}

void TasmMediator::SendNativeCustomEvent(const std::string& name, int tag,
                                         const lepus::Value& param_value,
                                         const std::string& param_name) {
  engine_actor_->ActLite([name, tag, param_value, param_name](auto& engine) {
    engine->SendCustomEvent(name, tag, param_value, param_name);
  });
};

void TasmMediator::RemoveLayoutNodeAtIndex(int32_t parent_id, int index) {
  layout_actor_->ActLite([parent_id, index](auto& layout) {
    layout->RemoveLayoutNodeAtIndex(parent_id, index);
  });
}

void TasmMediator::MoveLayoutNode(int32_t parent_id, int32_t child_id,
                                  int from_index, int to_index) {
  layout_actor_->ActLite(
      [parent_id, child_id, from_index, to_index](auto& layout) {
        layout->MoveLayoutNode(parent_id, child_id, from_index, to_index);
      });
}

void TasmMediator::InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id,
                                          int32_t ref_id) {
  layout_actor_->ActLite([parent_id, child_id, ref_id](auto& layout) {
    layout->InsertLayoutNodeBefore(parent_id, child_id, ref_id);
  });
}

void TasmMediator::RemoveLayoutNode(int32_t parent_id, int32_t child_id) {
  layout_actor_->ActLite([parent_id, child_id](auto& layout) {
    layout->RemoveLayoutNode(parent_id, child_id);
  });
}
void TasmMediator::DestroyLayoutNode(int32_t id) {
  layout_actor_->ActLite([id](auto& layout) { layout->DestroyLayoutNode(id); });
}

void TasmMediator::UpdateLayoutNodeStyle(int32_t id, tasm::CSSPropertyID css_id,
                                         const tasm::CSSValue& value) {
  layout_actor_->ActLite([id, css_id, value](auto& layout) {
    layout->UpdateLayoutNodeStyle(id, css_id, value);
  });
}

void TasmMediator::ResetLayoutNodeStyle(int32_t id,
                                        tasm::CSSPropertyID css_id) {
  layout_actor_->ActLite(
      [id, css_id](auto& layout) { layout->ResetLayoutNodeStyle(id, css_id); });
}

void TasmMediator::UpdateLayoutNodeAttribute(int32_t id,
                                             starlight::LayoutAttribute key,
                                             const lepus::Value& value) {
  // The value passed in here may be a JSValue.
  // Using JSValue in the Layout thread may cause potential UAF issues.
  // The purpose of calling lepus_value::ShallowCopy is
  // to get a cloned object that is not a JSValue.
  // This is a temporary solution. The long-term solution should be
  // to save the attribute with LepusValue during the
  // renderer_functions.SetAttributeTo process.
  layout_actor_->ActLite(
      [id, key, safe_value = lepus_value::ShallowCopy(value)](auto& layout) {
        layout->UpdateLayoutNodeAttribute(id, key, safe_value);
      });
}

void TasmMediator::SetFontFaces(const tasm::CSSFontFaceRuleMap& fontfaces) {
  layout_actor_->Act(
      [fontfaces](auto& layout) { layout->SetFontFaces(fontfaces); });
}

void TasmMediator::UpdateLayoutNodeByBundle(
    int32_t id, std::unique_ptr<tasm::LayoutBundle> bundle) {
  layout_actor_->ActLite(
      [id, bundle = std::move(bundle)](auto& layout) mutable {
        layout->UpdateLayoutNodeByBundle(id, std::move(bundle));
      });
}

void TasmMediator::UpdateLayoutNodeProps(
    int32_t id, const std::shared_ptr<tasm::PropBundle>& props) {
  layout_actor_->ActLite(
      [id, props](auto& layout) { layout->UpdateLayoutNodeProps(id, props); });
}

void TasmMediator::MarkLayoutDirty(int32_t id) {
  layout_actor_->ActLite([id](auto& layout) { layout->MarkDirty(id); });
}

void TasmMediator::AttachLayoutNodeType(
    int32_t id, const base::String& tag, bool allow_inline,
    const std::shared_ptr<tasm::PropBundle>& props) {
  layout_actor_->ActLite([id, tag, allow_inline, props](auto& layout) {
    layout->AttachLayoutNodeType(id, tag, allow_inline, props);
  });
}

void TasmMediator::InvokeUIMethod(tasm::LynxGetUIResult ui_result,
                                  const std::string& method,
                                  std::unique_ptr<tasm::PropBundle> params,
                                  piper::ApiCallBack callback) {
  if (invoke_ui_method_func_ != nullptr) {
    invoke_ui_method_func_(std::move(ui_result), method, std::move(params),
                           std::move(callback));
    return;
  }
  facade_actor_->Act([ui_result = std::move(ui_result), method,
                      params = std::move(params),
                      callback = std::move(callback)](auto& facade) mutable {
    facade->InvokeUIMethod(ui_result, method, std::move(params),
                           std::move(callback));
  });
}

void TasmMediator::SetPageConfigForLayoutThread(
    const std::shared_ptr<tasm::PageConfig>& config) {
  layout_actor_->Act(
      [config](auto& layout) { layout->SetPageConfigForLayoutThread(config); });
}

void TasmMediator::OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) {
  facade_actor_->ActAsync([bundle = std::move(bundle)](auto& facade) mutable {
    facade->OnTemplateBundleReady(std::move(bundle));
  });
}

void TasmMediator::RecycleTemplateBundle(
    std::unique_ptr<tasm::LynxBinaryRecyclerDelegate> recycler) {
  // post a task to async thread, which will greedy decode the incomplete bundle
  // and then recycle the bundle
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [weak_actor = std::weak_ptr<LynxActor<NativeFacade>>(facade_actor_),
       recycler = std::move(recycler)]() mutable {
        recycler->CompleteDecode();
        auto facade_actor = weak_actor.lock();
        if (facade_actor) {
          facade_actor->ActAsync(
              [bundle = recycler->GetCompleteTemplateBundle()](
                  auto& facade) mutable {
                facade->OnTemplateBundleReady(std::move(bundle));
              });
        }
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
}

event::DispatchEventResult TasmMediator::DispatchMessageEvent(
    runtime::MessageEvent event) {
  auto copy_event = runtime::MessageEvent::ShallowCopy(event);
  if (event.IsSendingToJSThread()) {
    runtime_actor_->Act(
        [message_event = std::move(copy_event)](auto& runtime) mutable {
          runtime->OnReceiveMessageEvent(std::move(message_event));
        });
  } else if (event.IsSendingToUIThread()) {
    facade_actor_->Act(
        [message_event = std::move(copy_event)](auto& facade) mutable {
          facade->OnReceiveMessageEvent(std::move(message_event));
        });
  } else {
    return event::DispatchEventResult::kCanceledBeforeDispatch;
  }
  return event::DispatchEventResult::kNotCanceled;
}

void TasmMediator::OnGlobalPropsUpdated(const lepus::Value& props) {
  runtime_actor_->Act(
      [props = lepus::Value::ShallowCopy(props)](auto& runtime) {
        runtime->OnGlobalPropsUpdated(props);
      });
}

}  // namespace shell
}  // namespace lynx
