// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime_mediator.h"

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/template_assembler.h"
#include "core/services/timing_handler/timing_mediator.h"
#include "core/shared_data/white_board_delegate.h"
#include "core/shell/common/vsync_monitor.h"

#if ENABLE_TESTBENCH_RECORDER
#include "core/services/recorder/testbench_base_recorder.h"
#endif

namespace lynx {
namespace shell {

void RuntimeMediator::AttachToLynxShell(
    const std::shared_ptr<LynxActor<NativeFacade>>& facade_actor,
    const std::shared_ptr<LynxActor<LynxEngine>>& engine_actor,
    const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr) {
  // attach LynxShell's actor to RuntimeMediator, so the Mediator is fully
  // functional.
  facade_actor_ = facade_actor;
  engine_actor_ = engine_actor;
  // TODO(chenyouhui): Use LynxResourceLoader directly.
  external_resource_loader_->SetEngineActor(engine_actor);
  card_cached_data_mgr_ = card_cached_data_mgr;

  // attach NativeFacadeActor to TimingActor, so the TmingHandler is fully
  // functional.
  timing_actor_->Act([facade_actor](auto& timing) {
    static_cast<lynx::tasm::timing::TimingMediator*>(timing->GetDelegate())
        ->SetFacadeActor(facade_actor);
  });

  runtime_standalone_mode_ = false;
}

void RuntimeMediator::UpdateDataByJS(runtime::UpdateDataTask task) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "UpdateDataByJS not supported on runtime standalone mode");
    return;
  }
  card_cached_data_mgr_->IncrementTaskCount();
  engine_actor_->ActAsync([task = std::move(task)](auto& engine) mutable {
    engine->UpdateDataByJS(std::move(task));
  });
}

void RuntimeMediator::UpdateBatchedDataByJS(
    std::vector<runtime::UpdateDataTask> tasks, uint64_t update_task_id) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "UpdateBatchedDataByJS not supported on runtime standalone mode");
    return;
  }
  card_cached_data_mgr_->IncrementTaskCount();
  engine_actor_->ActAsync(
      [tasks = std::move(tasks),
       update_task_id = update_task_id](auto& engine) mutable {
        engine->UpdateBatchedDataByJS(std::move(tasks), update_task_id);
      });
}

std::vector<shell::CacheDataOp> RuntimeMediator::FetchUpdatedCardData() {
  if (runtime_standalone_mode_) {
    // There are no Cached Data in standalone mode
    return {};
  }
  return card_cached_data_mgr_->ObtainCardCacheData();
}

std::string RuntimeMediator::GetLynxJSAsset(const std::string& name) {
  std::string resource = LoadJSSource(name);
  if (resource.empty()) {
    LOGE("GetLynxJSAsset failed, the source_url is: " << name);
  }
  return resource;
}

piper::JsContent RuntimeMediator::GetJSContentFromExternal(
    const std::string& bundle_name, const std::string& name, long timeout) {
  LOGE("GetJSContent with externalResourceLoader: " << name);
  auto info = external_resource_loader_->LoadScript(name, timeout);
  std::string external_resource_content("");
  piper::JsContent::Type type(piper::JsContent::Type::ERROR);
  if (info.Success()) {
    external_resource_content = std::string(info.data.begin(), info.data.end());
    type = piper::JsContent::Type::SOURCE;
  } else {
    external_resource_content = info.err_msg;
  }
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TestBenchBaseRecorder::GetInstance().RecordScripts(
      name.c_str(), external_resource_content.c_str());
  return {std::move(external_resource_content), type};
#else
  return {std::move(external_resource_content), type};
#endif
}

void RuntimeMediator::GetComponentContextDataAsync(
    const std::string& component_id, const std::string& key,
    piper::ApiCallBack callback) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "GetComponentContextDataAsync not supported on runtime standalone "
        "mode");
    return;
  }
  engine_actor_->ActAsync([component_id, key, callback](auto& engine) {
    engine->GetComponentContextDataAsync(component_id, key, callback);
  });
}

bool RuntimeMediator::LoadDynamicComponentFromJS(
    const std::string& url, const piper::ApiCallBack& callback,
    const std::vector<std::string>& ids) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "LoadDynamicComponentFromJS not supported on runtime standalone mode");
    return true;
  }
  external_resource_loader_->LoadLazyBundle(url, callback.id(), ids);
  return false;
}

void RuntimeMediator::LoadScriptAsync(const std::string& url,
                                      piper::ApiCallBack callback) {
  external_resource_loader_->LoadScriptAsync(url, callback.id());
}

void RuntimeMediator::AddFont(const lepus::Value& font,
                              const piper::ApiCallBack& callback) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "AddFont not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync(
      [font, callback](const std::unique_ptr<LynxEngine>& engine) {
        engine->AddFont(font, std::move(callback));
      });
}

void RuntimeMediator::OnRuntimeReady() {
  DCHECK(!runtime_standalone_mode_);
  facade_actor_->ActAsync([](auto& facade) { facade->OnRuntimeReady(); });
}

void RuntimeMediator::OnErrorOccurred(base::LynxError error) {
  facade_actor_->ActAsync(
      [error = std::move(error)](auto& facade) { facade->ReportError(error); });
}

void RuntimeMediator::OnModuleMethodInvoked(const std::string& module,
                                            const std::string& method,
                                            int32_t code) {
  facade_actor_->ActAsync([module, method, code](auto& facade) {
    facade->OnModuleMethodInvoked(module, method, code);
  });
}

void RuntimeMediator::UpdateComponentData(runtime::UpdateDataTask task) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "UpdateComponentData not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([task = std::move(task)](auto& engine) mutable {
    engine->UpdateComponentData(std::move(task));
  });
}

void RuntimeMediator::SelectComponent(const std::string& component_id,
                                      const std::string& id_selector,
                                      const bool single,
                                      piper::ApiCallBack callBack) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "SelectComponent not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync(
      [component_id, id_selector, single, callBack](auto& engine) {
        engine->SelectComponent(component_id, id_selector, single, callBack);
      });
}

void RuntimeMediator::InvokeUIMethod(tasm::NodeSelectRoot root,
                                     tasm::NodeSelectOptions options,
                                     std::string method,
                                     std::unique_ptr<tasm::PropBundle> params,
                                     piper::ApiCallBack callback) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "InvokeUIMethod not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([root = std::move(root), options = std::move(options),
                           method = std::move(method),
                           params = std::move(params),
                           callback](auto& engine) mutable {
    engine->InvokeUIMethod(root, options, method, std::move(params), callback);
  });
}

void RuntimeMediator::GetPathInfo(tasm::NodeSelectRoot root,
                                  tasm::NodeSelectOptions options,
                                  piper::ApiCallBack call_back) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "GetPathInfo not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([root = std::move(root), options = std::move(options),
                           call_back](auto& engine) {
    engine->GetPathInfo(root, options, call_back);
  });
}

void RuntimeMediator::GetFields(tasm::NodeSelectRoot root,
                                tasm::NodeSelectOptions options,
                                std::vector<std::string> fields,
                                piper::ApiCallBack call_back) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "GetFields not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([root = std::move(root), options = std::move(options),
                           fields = std::move(fields),
                           call_back](auto& engine) {
    engine->GetFields(root, options, fields, call_back);
  });
}

void RuntimeMediator::ElementAnimate(const std::string& component_id,
                                     const std::string& id_selector,
                                     const lepus::Value& args) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "ElementAnimate not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([component_id, id_selector, args](auto& engine) {
    engine->ElementAnimate(component_id, id_selector, args);
  });
}

void RuntimeMediator::OnCoreJSUpdated(std::string core_js) {
  // TODO(huzhanbo.luc): support devtool
  if (runtime_standalone_mode_) {
    return;
  }
  engine_actor_->ActAsync([core_js = std::move(core_js)](auto& engine) mutable {
    engine->UpdateCoreJS(std::move(core_js));
  });
}

void RuntimeMediator::TriggerComponentEvent(const std::string& event_name,
                                            const lepus::Value& msg) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "TriggerComponentEvent not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([event_name, msg](auto& engine) {
    engine->TriggerComponentEvent(event_name, msg);
  });
}

void RuntimeMediator::TriggerLepusGlobalEvent(const std::string& event_name,
                                              const lepus::Value& msg) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "TriggerLepusGlobalEvent not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([event_name, msg](auto& engine) {
    engine->TriggerLepusGlobalEvent(event_name, msg);
  });
}

void RuntimeMediator::InvokeLepusComponentCallback(
    const int64_t callback_id, const std::string& entry_name,
    const lepus::Value& data) {
  DCHECK(!runtime_standalone_mode_);
  engine_actor_->ActAsync([callback_id, entry_name, data](auto& engine) {
    engine->InvokeLepusComponentCallback(callback_id, entry_name, data);
  });
}

void RuntimeMediator::TriggerWorkletFunction(std::string component_id,
                                             std::string worklet_module_name,
                                             std::string method_name,
                                             lepus::Value args,
                                             piper::ApiCallBack callback) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "TriggerWorkletFunction not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync(
      [component_id = std::move(component_id),
       worklet_module_name = std::move(worklet_module_name),
       method_name = std::move(method_name), args = std::move(args),
       callback = std::move(callback)](auto& engine) mutable {
        engine->TriggerWorkletFunction(
            std::move(component_id), std::move(worklet_module_name),
            std::move(method_name), std::move(args), std::move(callback));
      });
}

void RuntimeMediator::RunOnJSThread(base::closure closure) {
  return js_runner_->PostTask(std::move(closure));
}

void RuntimeMediator::RunOnJSThreadWhenIdle(base::closure closure) {
  return js_runner_->PostIdleTask(std::move(closure));
}

void RuntimeMediator::SetCSSVariables(const std::string& component_id,
                                      const std::string& id_selector,
                                      const lepus::Value& properties,
                                      tasm::PipelineOptions pipeline_options) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "SetCSSVariables not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync(
      [component_id, id_selector, properties,
       pipeline_options = std::move(pipeline_options)](auto& engine) {
        engine->SetCSSVariables(component_id, id_selector, properties,
                                std::move(pipeline_options));
      });
}

void RuntimeMediator::SetNativeProps(tasm::NodeSelectRoot root,
                                     const tasm::NodeSelectOptions& options,
                                     const lepus::Value& native_props,
                                     tasm::PipelineOptions pipeline_options) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "SetNativeProps not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync(
      [root = std::move(root), options, native_props,
       pipeline_options = std::move(pipeline_options)](auto& engine) {
        engine->SetNativeProps(root, options, native_props,
                               std::move(pipeline_options));
      });
}

void RuntimeMediator::ReloadFromJS(runtime::UpdateDataTask task) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "ReloadFromJS not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([task = std::move(task)](auto& engine) mutable {
    engine->ReloadFromJS(std::move(task));
  });
}

void RuntimeMediator::SetTiming(tasm::Timing timing) {
  timing_actor_->Act(
      [timing = std::move(timing)](auto& timing_handler) mutable {
        timing_handler->SetTiming(std::move(timing));
      });
}

void RuntimeMediator::SetTimingWithTimingFlag(
    const tasm::timing::TimingFlag& timing_flag,
    const std::string& timestamp_key, tasm::timing::TimestampUs timestamp) {
  timing_actor_->Act(
      [timing_flag, timestamp_key, timestamp](auto& timing_handler) {
        timing_handler->SetTimingWithTimingFlag(timing_flag, timestamp_key,
                                                timestamp);
      });
}

void RuntimeMediator::FlushJSBTiming(piper::NativeModuleInfo timing) {
  if (runtime_standalone_mode_) {
    // TODO(huzhanbo.luc): support JSB Timing
    return;
  }
  facade_actor_->ActAsync([timing = std::move(timing)](auto& facade) mutable {
    facade->FlushJSBTiming(std::move(timing));
  });
}

void RuntimeMediator::OnPipelineStart(
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

void RuntimeMediator::BindPipelineIDWithTimingFlag(
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

void RuntimeMediator::CallLepusMethod(const std::string& method_name,
                                      lepus::Value args,
                                      const piper::ApiCallBack& callback,
                                      uint64_t trace_flow_id) {
  if (runtime_standalone_mode_) {
    REPORT_JSI_NATIVE_EXCEPTION(
        "CallLepusMethod not supported on runtime standalone mode");
    return;
  }
  engine_actor_->ActAsync([method_name, args = std::move(args), callback,
                           trace_flow_id](auto& engine) mutable {
    engine->CallLepusMethod(method_name, std::move(args), callback,
                            trace_flow_id);
  });
}

event::DispatchEventResult RuntimeMediator::DispatchMessageEvent(
    runtime::MessageEvent event) {
  if (runtime_standalone_mode_) {
    // In standalone mode, runtime don't have other target, reject event message
    // here.
    return event::DispatchEventResult::kCanceledByEventHandler;
  }
  auto copy_event = runtime::MessageEvent::ShallowCopy(event);
  if (event.IsSendingToCoreThread()) {
    engine_actor_->Act(
        [message_event = std::move(copy_event)](auto& engine) mutable {
          engine->OnReceiveMessageEvent(std::move(message_event));
        });
  } else if (event.IsSendingToUIThread()) {
    facade_actor_->Act(
        [message_event = std::move(copy_event)](auto& facade) mutable {
          facade->OnReceiveMessageEvent(std::move(message_event));
        });
  }
  return event::DispatchEventResult::kNotCanceled;
}

std::string RuntimeMediator::LoadJSSource(const std::string& name) {
  auto result = external_resource_loader_->LoadJSSource(name);
  std::string str(result.begin(), result.end());
  return str;
}

void RuntimeMediator::AddEventListenersToWhiteBoard(
    runtime::ContextProxy* js_context_proxy) {
  if (white_board_delegate_) {
    white_board_delegate_->AddEventListeners(js_context_proxy);
  }
}

void RuntimeMediator::GetSessionStorageItem(
    const std::string& key, const piper::ApiCallBack& callback) {
  if (runtime_standalone_mode_) {
    if (white_board_delegate_) {
      auto value = white_board_delegate_->GetSessionStorageItem(key);
      white_board_delegate_->CallJSApiCallbackWithValue(callback, value);
    }
    return;
  }
  engine_actor_->Act([key, callback](auto& engine) {
    engine->GetJSSessionStorage(key, callback);
  });
}

void RuntimeMediator::SubscribeSessionStorage(
    const std::string& key, double listener_id,
    const piper::ApiCallBack& callback) {
  if (runtime_standalone_mode_) {
    if (white_board_delegate_) {
      white_board_delegate_->SubscribeJSSessionStorage(key, listener_id,
                                                       callback);
    }
    return;
  }
  engine_actor_->Act([key, listener_id, callback](auto& engine) {
    engine->SubscribeJSSessionStorage(key, listener_id, callback);
  });
}

}  // namespace shell
}  // namespace lynx
