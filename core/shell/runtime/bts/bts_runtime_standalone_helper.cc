// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime/bts/bts_runtime_standalone_helper.h"

#include <utility>
#include <vector>

#include "core/base/threading/vsync_monitor.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/performance_mediator.h"
#include "core/shell/common/shell_trace_event_def.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/runtime/bts/bts_runtime_mediator.h"

namespace lynx {
namespace shell {

std::unique_ptr<BTSRuntimeStandalone>
BTSRuntimeStandalone::InitRuntimeStandalone(
    const std::string& group_name, const std::string& group_id,
    std::unique_ptr<NativeFacade> native_facade_runtime,
    const std::shared_ptr<piper::InspectorRuntimeObserverNG>& runtime_observer,
    const std::shared_ptr<lynx::pub::LynxResourceLoader>& resource_loader,
    const std::shared_ptr<lynx::pub::LynxNativeModuleManager>&
        native_module_manager,
    const std::shared_ptr<tasm::PropBundleCreator>& prop_bundle_creator,
    const std::shared_ptr<tasm::WhiteBoard>& white_board,
    const std::function<void(const std::shared_ptr<LynxActor<BTSRuntime>>&,
                             const std::shared_ptr<LynxActor<NativeFacade>>&)>&
        on_runtime_actor_created,
    std::vector<std::string> preload_js_paths,
    const std::string& bytecode_source_url, uint32_t runtime_flag,
    const lepus::Value* global_props, bool debuggable,
    bool long_task_monitor_disabled) {
  auto instance_id = lynx::shell::LynxShell::NextInstanceId();
  lynx::fml::RefPtr<lynx::fml::TaskRunner> js_task_runner =
      lynx::base::TaskRunnerManufactor::GetJSRunner(group_name);
  auto native_runtime_facade =
      std::make_shared<lynx::shell::LynxActor<lynx::shell::NativeFacade>>(
          std::move(native_facade_runtime), js_task_runner, instance_id, true);
  std::shared_ptr<base::VSyncMonitor> vsync_monitor =
      base::VSyncMonitor::Create();

  auto perf_mediator =
      std::make_unique<lynx::tasm::performance::PerformanceMediator>();
  auto timing_mediator =
      std::make_unique<lynx::tasm::timing::TimingMediator>(instance_id);
  timing_mediator->SetEnableJSRuntime(true);
  // TODO(huzhanbo.luc): use TimingHandler to set back actors to TimingMediator,
  // so that we can avoid raw ptr
  auto perf_mediator_raw_ptr = perf_mediator.get();
  auto timing_mediator_raw_ptr = timing_mediator.get();

  auto performance_actor =
      std::make_shared<LynxActor<tasm::performance::PerformanceController>>(
          std::make_unique<tasm::performance::PerformanceController>(
              std::move(perf_mediator), std::move(timing_mediator),
              instance_id),
          tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner(),
          instance_id);

  auto external_resource_loader =
      std::make_unique<ExternalResourceLoader>(resource_loader);
  auto* external_resource_loader_ptr = external_resource_loader.get();

  auto white_board_delegate =
      std::make_shared<tasm::WhiteBoardRuntimeDelegate>(white_board);

  if (runtime_observer != nullptr) {
    runtime_observer->InitWhiteBoardInspector(white_board_delegate);
  }

  auto delegate = std::make_unique<BTSRuntimeMediator>(
      native_runtime_facade, nullptr, performance_actor, nullptr,
      js_task_runner, std::move(external_resource_loader));
  auto lazy_bundle_loader =
      std::make_shared<tasm::LazyBundleLoader>(resource_loader);
  auto js_bundle_proxy =
      std::make_unique<BTSRuntimeStandalone::StandaloneBundleProxy>(
          lazy_bundle_loader);
  auto js_bundle_holder =
      std::make_shared<tasm::JsBundleHolderImpl>(*js_bundle_proxy);
  delegate->SetLazyBundleLoader(lazy_bundle_loader);
  delegate->SetJsBundleHolder(js_bundle_holder);
  delegate->SetPropBundleCreator(prop_bundle_creator);
  delegate->SetWhiteBoardDelegate(white_board_delegate);
  auto* delegate_raw_ptr = delegate.get();

  auto page_options = tasm::PageOptions(instance_id);
  page_options.SetLongTaskMonitorDisabled(long_task_monitor_disabled);
  page_options.SetDebuggable(debuggable);

  auto runtime = std::make_unique<BTSRuntime>(
      group_id, instance_id, std::move(delegate), bytecode_source_url,
      runtime_flag, page_options);
  auto runtime_actor = std::make_shared<LynxActor<BTSRuntime>>(
      std::move(runtime), js_task_runner, instance_id, true);
  delegate_raw_ptr->set_vsync_monitor(vsync_monitor, runtime_actor);
  perf_mediator_raw_ptr->SetRuntimeActor(runtime_actor);
  timing_mediator_raw_ptr->SetRuntimeActor(runtime_actor);
  lazy_bundle_loader->SetEngineActor(nullptr);

  on_runtime_actor_created(runtime_actor, native_runtime_facade);
  external_resource_loader_ptr->SetRuntimeActor(runtime_actor);
  white_board_delegate->SetRuntimeActor(runtime_actor);
  white_board_delegate->SetRuntimeFacadeActor(native_runtime_facade);
  const auto global_props_value = global_props
                                      ? lynx::lepus::Value::Clone(*global_props)
                                      : lynx::lepus::Value();
  runtime_actor->ActAsync(
      [native_module_manager, preload_js_paths = std::move(preload_js_paths),
       runtime_observer, global_props_value, vsync_monitor,
       weak_js_bundle_holder = std::weak_ptr<piper::JsBundleHolder>(
           js_bundle_holder)](std::unique_ptr<BTSRuntime>& runtime) mutable {
        vsync_monitor->BindToCurrentThread();
        vsync_monitor->Init();
        runtime->OnGlobalPropsUpdated(global_props_value);
        runtime->Init(native_module_manager, runtime_observer,
                      std::move(preload_js_paths));
        runtime->SetJsBundleHolder(weak_js_bundle_holder);
      });
  return std::unique_ptr<BTSRuntimeStandalone>(new BTSRuntimeStandalone(
      group_name, instance_id, runtime_actor, performance_actor,
      native_runtime_facade, white_board_delegate, lazy_bundle_loader,
      std::move(js_bundle_proxy), js_bundle_holder));
}

void BTSRuntimeStandalone::EvaluateScript(std::string url, std::string script) {
  uint64_t trace_flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, EVALUATE_SCRIPT_STANDALONE,
              [&url, trace_flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("url", url);
                ctx.event()->add_flow_ids(trace_flow_id);
              });
  runtime_actor_->Act([url = std::move(url), script = std::move(script),
                       trace_flow_id](auto& runtime) mutable {
    runtime->EvaluateScriptStandalone(std::move(url), std::move(script),
                                      trace_flow_id);
  });
}

void BTSRuntimeStandalone::EvaluateScript(
    std::string url, lynx::tasm::LynxTemplateBundle* bundle,
    std::string js_file) {
  auto js_content = bundle->GetJsBundle().GetJsContent(js_file);
  if (!js_content.has_value()) {
    return;
  }
  auto js_content_val = js_content->get();
  if (js_content_val.IsError()) {
    return;
  }
  auto buffer = js_content_val.GetBuffer();

  if (!buffer || !buffer->data()) {
    return;
  }

  const auto length = buffer->size();
  const auto* data = reinterpret_cast<const char*>(buffer->data());

  uint64_t trace_flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, EVALUATE_SCRIPT_STANDALONE,
              [&url, trace_flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("url", url);
                ctx.event()->add_flow_ids(trace_flow_id);
              });
  runtime_actor_->Act([url = std::move(url), script = std::string(data, length),
                       trace_flow_id](auto& runtime) mutable {
    runtime->EvaluateScriptStandalone(std::move(url), std::move(script),
                                      trace_flow_id);
  });
}

void BTSRuntimeStandalone::SetPresetData(lepus::Value data) {
  runtime_actor_->Act([data = std::move(data)](auto& runtime) mutable {
    runtime->OnSetPresetData(std::move(data));
  });
}

void BTSRuntimeStandalone::SetSessionStorageItem(
    const std::string& key, const std::shared_ptr<tasm::TemplateData>& data) {
  runtime_actor_->Act(
      [key, data, delegate = white_board_delegate_](auto& runtime) mutable {
        delegate->SetSessionStorageItem(std::move(key), data->GetValue());
      });
}

void BTSRuntimeStandalone::SetSessionStorageItem(const std::string& key,
                                                 const lepus::Value value) {
  runtime_actor_->Act(
      [key, data = std::move(value),
       delegate = white_board_delegate_](auto& runtime) mutable {
        delegate->SetSessionStorageItem(std::move(key), std::move(data));
      });
}

void BTSRuntimeStandalone::GetSessionStorageItem(
    const std::string& key, std::unique_ptr<PlatformCallBack> callback) {
  runtime_actor_->Act(
      [key, callback = std::move(callback), facade = native_runtime_facade_,
       delegate = white_board_delegate_](auto& runtime) mutable {
        auto callback_holder = facade->ActSync(
            [callback = std::move(callback)](auto& facade) mutable {
              return facade->CreatePlatformCallBackHolder(std::move(callback));
            });

        auto value = delegate->GetSessionStorageItem(key);
        delegate->CallPlatformCallbackWithValue(callback_holder, value);
      });
}

double BTSRuntimeStandalone::SubscribeSessionStorage(
    const std::string& key, std::unique_ptr<PlatformCallBack> callback) {
  auto callback_holder = native_runtime_facade_->ActSync(
      [callback = std::move(callback)](auto& facade) mutable {
        return facade->CreatePlatformCallBackHolder(std::move(callback));
      });

  double callback_id = callback_holder->id();

  runtime_actor_->Act(
      [key, callback_holder = std::move(callback_holder),
       delegate = white_board_delegate_](auto& runtime) mutable {
        delegate->SubScribeClientSessionStorage(std::move(key),
                                                std::move(callback_holder));
      });
  return callback_id;
}

void BTSRuntimeStandalone::UnSubscribeSessionStorage(const std::string& key,
                                                     double callback_id) {
  runtime_actor_->Act([key, callback_id, delegate = white_board_delegate_](
                          auto& runtime) mutable {
    delegate->UnsubscribeClientSessionStorage(std::move(key), callback_id);
  });
}

void BTSRuntimeStandalone::TransitionToFullRuntime() {
  runtime_actor_->Act(
      [](auto& runtime) { runtime->TransitionToFullRuntime(); });
}

void BTSRuntimeStandalone::DestroyRuntime() {
  perf_controller_actor_->Act(
      [instance_id = perf_controller_actor_->GetInstanceId()](auto& facade) {
        facade = nullptr;
        lynx::tasm::report::FeatureCounter::Instance()->ClearAndReport(
            instance_id);
      });

  native_runtime_facade_->Act(
      [instance_id = perf_controller_actor_->GetInstanceId()](auto& facade) {
        facade = nullptr;
        lynx::tasm::report::FeatureCounter::Instance()->ClearAndReport(
            instance_id);
      });

  runtime_actor_->ActAsync([runtime_actor = runtime_actor_,
                            js_group_thread_name = group_name_](auto& runtime) {
    lynx::shell::LynxShell::TriggerDestroyRuntime(runtime_actor,
                                                  js_group_thread_name);
  });
}

}  // namespace shell
}  // namespace lynx
