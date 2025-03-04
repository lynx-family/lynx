// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_shell.h"

#include <utility>

#include "base/include/no_destructor.h"
#include "core/base/threading/thread_merger.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"
#include "core/runtime/piper/js/js_bundle_holder.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/feature_count/global_feature_counter.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/shell/runtime_mediator.h"
#include "core/shell/runtime_standalone_helper.h"
#include "core/shell/tasm_operation_queue_async.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace shell {

namespace {

bool DoAsyncHydration(base::ThreadStrategyForRendering strategy,
                      const ShellOption& option) {
  return !base::IsEngineAsync(strategy) &&
         !option.enable_vsync_aligned_msg_loop_ &&
         (tasm::Config::TrialAsyncHydration() ||
          option.enable_async_hydration_);
}

base::ThreadStrategyForRendering MapThreadStrategyForTemporaryAsync(
    base::ThreadStrategyForRendering main_strategy,
    bool enable_async_hydration) {
  if (enable_async_hydration) {
    return base::ToAsyncEngineStrategy(main_strategy);
  }
  return main_strategy;
}

// for async hydration.
void UnmergeToAsyncEngineIfNeeded(
    base::ThreadStrategyForRendering current_strategy,
    base::TaskRunnerManufactor& runners,
    std::shared_ptr<shell::DynamicUIOperationQueue>& ui_operation_queue,
    bool need_transfer_queue = true) {
  if (runners.GetTASMTaskRunner()->RunsTasksOnCurrentThread()) {
    fml::MessageLoopTaskQueues::GetInstance()->Unmerge(
        runners.GetUITaskRunner()->GetTaskQueueId(),
        runners.GetTASMTaskRunner()->GetTaskQueueId());
    if (need_transfer_queue) {
      ui_operation_queue->Transfer(
          base::ToAsyncEngineStrategy(current_strategy));
    }
    LOGI("Unmerge engine and ui thread, engine task runner id:"
         << runners.GetTASMTaskRunner()->GetTaskQueueId());
  }
}

}  // namespace

int32_t LynxShell::NextInstanceId() {
  static base::NoDestructor<std::atomic<int32_t>> id{0};
  return (*id)++;
}

LynxShell::LynxShell(base::ThreadStrategyForRendering strategy,
                     const ShellOption& shell_option)
    : runners_(MapThreadStrategyForTemporaryAsync(
                   strategy, DoAsyncHydration(strategy, shell_option)),
               // Multi tasm thread is necessary for auto concurrency or async
               // hydrations, beacause they have to merge the tasm runner to ui.
               shell_option.enable_multi_tasm_thread_ ||
                   DoAsyncHydration(strategy, shell_option),
               shell_option.enable_multi_layout_thread_,
               shell_option.enable_vsync_aligned_msg_loop_,
               // TODO(heshan,huangweiwu): the async_thread_cache config
               // conflicts with thread merge now.
               shell_option.enable_multi_tasm_thread_ &&
                   !shell_option.enable_auto_concurrency_ &&
                   !DoAsyncHydration(strategy, shell_option),
               shell_option.js_group_thread_name_),
      instance_id_(shell_option.instance_id_ != kUnknownInstanceId
                       ? shell_option.instance_id_
                       : NextInstanceId()),
      enable_runtime_(shell_option.enable_js_),
      tasm_operation_queue_(
          strategy == base::ThreadStrategyForRendering::ALL_ON_UI ||
                  strategy == base::ThreadStrategyForRendering::MOST_ON_TASM
              ? std::make_shared<TASMOperationQueue>()
              : std::make_shared<TASMOperationQueueAsync>()),
      ui_operation_queue_(std::make_shared<shell::DynamicUIOperationQueue>(
          strategy, runners_.GetUITaskRunner(), instance_id_)),
      enable_async_hydration_(DoAsyncHydration(strategy, shell_option)),
      current_strategy_(strategy),
      js_group_thread_name_(shell_option.js_group_thread_name_),
      enable_js_group_thread_(shell_option.enable_js_group_thread_) {
  LOGI("LynxShell create, this:" << this);
  engine_thread_switch_ = std::make_shared<EngineThreadSwitch>(
      runners_.GetUITaskRunner(), runners_.GetTASMTaskRunner(),
      ui_operation_queue_);
  if (shell_option.enable_auto_concurrency_) {
    thread_mode_manager_.ui_runner = runners_.GetUITaskRunner().get();
    thread_mode_manager_.engine_runner = runners_.GetTASMTaskRunner().get();
    thread_mode_manager_.queue = ui_operation_queue_.get();
  } else if (DoAsyncHydration(strategy, shell_option)) {
    base::ThreadMerger::Merge(runners_.GetUITaskRunner().get(),
                              runners_.GetTASMTaskRunner().get());
    ui_operation_queue_->Transfer(current_strategy_);
  }
}

LynxShell::~LynxShell() {
  LOGI("LynxShell release, this:" << this);
  Destroy();
}

void LynxShell::Destroy() {
  if (is_destroyed_) {
    return;
  }
  LOGI("LynxShell Destroy, this:" << this);

  is_destroyed_ = true;

  timing_actor_->Act([](auto& timing_handler) { timing_handler = nullptr; });

  facade_actor_->Act([instance_id = instance_id_](auto& facade) {
    facade = nullptr;
    tasm::report::FeatureCounter::Instance()->ClearAndReport(instance_id);
  });

  facade_reporter_actor_->Act([](auto& facade) { facade = nullptr; });

  if (tasm::LynxEnv::GetInstance().EnableAsyncDestroyEngine()) {
    DetachEngineFromUIThread();
  }
  engine_actor_->Act([instance_id = instance_id_](auto& engine) {
    engine = nullptr;
    tasm::report::FeatureCounter::Instance()->ClearAndReport(instance_id);
  });
  layout_actor_->Act([instance_id = instance_id_](auto& layout) {
    layout = nullptr;
    tasm::report::FeatureCounter::Instance()->ClearAndReport(instance_id);
  });
  runtime_actor_->ActAsync(
      [runtime_actor = runtime_actor_,
       js_group_thread_name = js_group_thread_name_](auto& runtime) {
        TriggerDestroyRuntime(runtime_actor, js_group_thread_name);
      });

  if (enable_async_hydration_) {
    {
      std::unique_lock<std::mutex> merge_lock(tasm_merge_mutex_);
      tasm_merge_cv_.wait(merge_lock,
                          [this] { return !need_wait_for_merge_.load(); });
    }
    UnmergeToAsyncEngineIfNeeded(current_strategy_, runners_,
                                 ui_operation_queue_, false);
  }
  ui_operation_queue_->Destroy();
  tasm::report::GlobalFeatureCounter::ClearAndReport(instance_id_);
}

void LynxShell::InitRuntimeWithRuntimeDisabled(
    std::shared_ptr<VSyncMonitor> vsync_monitor) {
  DCHECK(!enable_runtime_);
  runtime_actor_ = std::make_shared<LynxActor<runtime::LynxRuntime>>(
      nullptr, nullptr, instance_id_, enable_runtime_);
  vsync_monitor->set_runtime_actor(runtime_actor_);
  tasm_mediator_->SetRuntimeActor(runtime_actor_);
  layout_mediator_->SetRuntimeActor(runtime_actor_);
  timing_mediator_->SetRuntimeActor(runtime_actor_);
}

void LynxShell::InitRuntime(
    const std::string& group_id,
    const std::shared_ptr<lynx::pub::LynxResourceLoader>& resource_loader,
    const std::shared_ptr<lynx::piper::LynxModuleManager>& module_manager,
    const std::function<
        void(const std::shared_ptr<LynxActor<runtime::LynxRuntime>>&)>&
        on_runtime_actor_created,
    std::vector<std::string> preload_js_paths, bool force_reload_js_core,
    bool force_use_light_weight_js_engine, bool pending_js_task,
    bool enable_user_code_cache, const std::string& code_cache_source_url) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxShell::InitRuntime");
#if ENABLE_TESTBENCH_RECORDER
  int64_t record_id = reinterpret_cast<int64_t>(this);
  engine_actor_->Act(
      [record_id](auto& engine) { engine->SetRecordID(record_id); });
  layout_actor_->Act(
      [record_id](auto& layout) { layout->SetRecordId(record_id); });
  module_manager->SetRecordID(record_id);
  tasm::recorder::LynxViewInitRecorder::GetInstance().RecordThreadStrategy(
      static_cast<int32_t>(current_strategy_), record_id, enable_runtime_);
#endif
  std::shared_ptr<VSyncMonitor> vsync_monitor =
      lynx::shell::VSyncMonitor::Create();
  if (!enable_runtime_) {
    InitRuntimeWithRuntimeDisabled(vsync_monitor);
    return;
  }
  fml::RefPtr<fml::TaskRunner> js_task_runner;
  js_task_runner = runners_.GetJSTaskRunner();
  auto external_resource_loader =
      std::make_unique<ExternalResourceLoader>(resource_loader);
  auto external_resource_loader_ptr = external_resource_loader.get();
  auto delegate = std::make_unique<RuntimeMediator>(
      facade_actor_, engine_actor_, timing_actor_, card_cached_data_mgr_,
      js_task_runner, std::move(external_resource_loader));
  delegate->set_vsync_monitor(vsync_monitor);
  delegate->SetPropBundleCreator(prop_bundle_creator_);
  tasm_mediator_->SetPropBundleCreator(prop_bundle_creator_);
  auto runtime = std::make_unique<runtime::LynxRuntime>(
      group_id, instance_id_, std::move(delegate), enable_user_code_cache,
      code_cache_source_url, enable_js_group_thread_);
  runtime_actor_ = std::make_shared<LynxActor<runtime::LynxRuntime>>(
      std::move(runtime), js_task_runner, instance_id_, enable_runtime_);
  vsync_monitor->set_runtime_actor(runtime_actor_);

  ConsumeModuleFactory(module_manager.get());
  OnRuntimeCreate();
  on_runtime_actor_created(runtime_actor_);
  external_resource_loader_ptr->SetRuntimeActor(runtime_actor_);
  tasm_mediator_->SetRuntimeActor(runtime_actor_);
  layout_mediator_->SetRuntimeActor(runtime_actor_);
  timing_mediator_->SetRuntimeActor(runtime_actor_);

#if ENABLE_TESTBENCH_RECORDER
  runtime_actor_->Act(
      [record_id](std::unique_ptr<runtime::LynxRuntime>& runtime) {
        runtime->SetRecordId(record_id);
      });
#endif

  start_js_runtime_task_ =
      [module_manager, preload_js_paths = std::move(preload_js_paths),
       runtime_observer = runtime_observer_,
       runtime_lifecycle_observer = runtime_lifecycle_observer_,
       force_reload_js_core, force_use_light_weight_js_engine, vsync_monitor,
       weak_js_bundle_holder = GetWeakJsBundleHolder()](
          std::unique_ptr<runtime::LynxRuntime>& runtime) mutable {
        vsync_monitor->BindToCurrentThread();
        vsync_monitor->Init();
        runtime->Init(module_manager, runtime_observer,
                      runtime_lifecycle_observer, std::move(preload_js_paths),
                      force_reload_js_core, force_use_light_weight_js_engine);
        runtime->SetJsBundleHolder(weak_js_bundle_holder);
      };

  if (!pending_js_task) {
    runtime_actor_->ActAsync(std::move(start_js_runtime_task_));
  }
}

void LynxShell::AttachRuntime(
    std::weak_ptr<piper::LynxModuleManager> module_manager) {
  auto lock_module_manager = module_manager.lock();
  DCHECK(lock_module_manager);
  if (lock_module_manager) {
    ConsumeModuleFactory(lock_module_manager.get());
  }
  OnRuntimeCreate();

  // TODO(huzhanbo.luc): support TestBench
  tasm_mediator_->SetPropBundleCreator(prop_bundle_creator_);
  tasm_mediator_->SetRuntimeActor(runtime_actor_);
  layout_mediator_->SetRuntimeActor(runtime_actor_);
  // timing_mediator_ will be handled inside RuntimeMediator::AttachToLynxShell

  runtime_actor_->ActAsync(
      [facade_actor = facade_actor_, engine_actor = engine_actor_,
       card_cached_data_mgr = card_cached_data_mgr_,
       runtime_lifecycle_observer = runtime_lifecycle_observer_,
       weak_js_bundle_holder = GetWeakJsBundleHolder()](auto& runtime) mutable {
        runtime->AdoptRuntimeLifecycleObserver(runtime_lifecycle_observer);
        runtime->SetJsBundleHolder(weak_js_bundle_holder);
        static_cast<RuntimeMediator*>(runtime->GetDelegate())
            ->AttachToLynxShell(std::move(facade_actor),
                                std::move(engine_actor),
                                std::move(card_cached_data_mgr));
      });
}

void LynxShell::StartJsRuntime() {
  if (!is_destroyed_ && start_js_runtime_task_ != nullptr) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxShell::StartJsRuntime");
    runtime_actor_->ActAsync(std::move(start_js_runtime_task_));
  }
}

bool LynxShell::IsDestroyed() { return is_destroyed_; }

void LynxShell::LoadTemplate(
    const std::string& url, std::vector<uint8_t> source,
    const std::shared_ptr<tasm::TemplateData>& template_data,
    const bool enable_pre_painting, bool enable_recycle_template_bundle) {
  tasm::PipelineOptions pipeline_options;
  pipeline_options.need_timestamps = true;
  pipeline_options.pipeline_origin = tasm::timing::kLoadBundle;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  bool need_to_merge_back = false;
  if (hydration_pending_ && enable_async_hydration_) {
    LOGI("Do async hydration for ssr, url=" << url);
    UnmergeToAsyncEngineIfNeeded(current_strategy_, runners_,
                                 ui_operation_queue_);
    need_to_merge_back = true;
    need_wait_for_merge_ = true;
  }

  EnsureTemplateDataThreadSafe(template_data);
  hydration_pending_ = false;
  timing_actor_->ActAsync([url](auto& timing) { timing->SetURL(url); });
  engine_actor_->Act([url, source = std::move(source), template_data,
                      pipeline_options = std::move(pipeline_options),
                      enable_pre_painting, enable_recycle_template_bundle,
                      need_to_merge_back,
                      tasm_runner = runners_.GetTASMTaskRunner().get(),
                      weak_ui_queue =
                          std::weak_ptr<shell::DynamicUIOperationQueue>(
                              ui_operation_queue_),
                      current_strategy = current_strategy_,
                      &need_wait_for_merge = need_wait_for_merge_,
                      &tasm_merge_cv = tasm_merge_cv_](auto& engine) mutable {
    engine->LoadTemplate(url, std::move(source), template_data,
                         std::move(pipeline_options), enable_pre_painting,
                         enable_recycle_template_bundle);
    if (need_to_merge_back) {
      // FIXME(heshan,zhixuan,liting):After each engine_actor's task is
      // completed, the afterInvoke() is executed.Within this
      // function, the flush() of the ui_queue is called on the TASM
      // thread, while the transfer() of the ui_queue is called on the UI
      // thread at the same time.That can result in mutiple threads to
      // reading and writing to the same variable.
      //
      // To avoid this issue,
      // tasks are now posted by tasm_runner to avoid running flush() and
      // Transfer() at the same time.
      //
      // We need refactor this code in the future.
      tasm_runner->PostTask([tasm_runner, weak_ui_queue, current_strategy,
                             &need_wait_for_merge, &tasm_merge_cv]() {
        fml::MessageLoopTaskQueues::GetInstance()->Merge(
            base::UIThread::GetRunner()->GetTaskQueueId(),
            tasm_runner->GetTaskQueueId());
        need_wait_for_merge = false;
        tasm_merge_cv.notify_all();
        LOGI(
            "Engine thread is merged backed to ui thread, engine task runner "
            "id::"
            << tasm_runner->GetTaskQueueId());
        base::UIThread::GetRunner()->PostEmergencyTask([weak_ui_queue,
                                                        current_strategy] {
          if (auto ui_queue = weak_ui_queue.lock()) {
            ui_queue->Transfer(current_strategy);
            LOGI("UI op queue back to thread strategy:" << current_strategy);
          }
        });
      });
    }
  });
}

void LynxShell::LoadTemplateBundle(
    const std::string& url, tasm::LynxTemplateBundle template_bundle,
    const std::shared_ptr<tasm::TemplateData>& template_data,
    const bool enable_pre_painting, bool enable_dump_element_tree) {
  tasm::PipelineOptions pipeline_options;
  pipeline_options.need_timestamps = true;
  pipeline_options.pipeline_origin = tasm::timing::kLoadBundle;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  EnsureTemplateDataThreadSafe(template_data);
  engine_actor_->Act(
      [url, template_bundle = std::move(template_bundle), template_data,
       pipeline_options = std::move(pipeline_options), enable_pre_painting,
       enable_dump_element_tree](auto& engine) mutable {
        engine->LoadTemplateBundle(url, std::move(template_bundle),
                                   template_data, std::move(pipeline_options),
                                   enable_pre_painting,
                                   enable_dump_element_tree);
      });
}

void LynxShell::MarkDirty() {
  if (is_destroyed_) {
    return;
  }
  ui_operation_queue_->MarkDirty();
}

void LynxShell::Flush() {
  if (is_destroyed_) {
    return;
  }
  ui_operation_queue_->Flush();
}

void LynxShell::ForceFlush() {
  if (is_destroyed_) {
    return;
  }
  ui_operation_queue_->ForceFlush();
}

void LynxShell::SetEnableUIFlush(bool enable_ui_flush) {
  if (is_destroyed_) {
    return;
  }
  ui_operation_queue_->SetEnableFlush(enable_ui_flush);
  SetAnimationsPending(!enable_ui_flush);
}

void LynxShell::SetContextHasAttached() {
  engine_actor_->Act([](auto& engine) { engine->SetContextHasAttached(); });
};

void LynxShell::LoadSSRData(
    std::vector<uint8_t> source,
    const std::shared_ptr<tasm::TemplateData>& template_data) {
  tasm::PipelineOptions pipeline_options;
  pipeline_options.need_timestamps = true;
  // TODO(kechenglong): should find a better pipeline_origin name?
  pipeline_options.pipeline_origin = tasm::timing::kLoadSSRData;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);
  hydration_pending_ = true;
  EnsureTemplateDataThreadSafe(template_data);
  engine_actor_->Act(
      [source = std::move(source), template_data,
       pipeline_options = std::move(pipeline_options)](auto& engine) mutable {
        engine->LoadSSRData(std::move(source), template_data,
                            std::move(pipeline_options));
      });
}

void LynxShell::UpdateDataByParsedData(
    const std::shared_ptr<tasm::TemplateData>& data) {
  tasm::PipelineOptions pipeline_options;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  EnsureTemplateDataThreadSafe(data);
  auto order = ui_operation_queue_->UpdateNativeUpdateDataOrder();
  engine_actor_->Act([data, order,
                      pipeline_options =
                          std::move(pipeline_options)](auto& engine) mutable {
    engine->UpdateDataByParsedData(data, order, std::move(pipeline_options));
  });
}

void LynxShell::UpdateMetaData(const std::shared_ptr<tasm::TemplateData>& data,
                               const lepus::Value& global_props) {
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordUpdateMetaData(
      data, global_props, reinterpret_cast<int64_t>(this));
#endif
  tasm::PipelineOptions pipeline_options;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);
  EnsureTemplateDataThreadSafe(data);
  EnsureGlobalPropsThreadSafe(global_props);
  auto order = ui_operation_queue_->UpdateNativeUpdateDataOrder();
  engine_actor_->Act(
      [data, global_props, order,
       pipeline_options = std::move(pipeline_options)](auto& engine) {
        engine->UpdateMetaData(data, global_props, order,
                               std::move(pipeline_options));
      });
}

void LynxShell::ResetDataByParsedData(
    const std::shared_ptr<tasm::TemplateData>& data) {
  tasm::PipelineOptions pipeline_options;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  EnsureTemplateDataThreadSafe(data);
  auto order = ui_operation_queue_->UpdateNativeUpdateDataOrder();
  engine_actor_->Act(
      [data, order,
       pipeline_options = std::move(pipeline_options)](auto& engine) {
        engine->ResetDataByParsedData(data, order, std::move(pipeline_options));
      });
}

void LynxShell::SetSessionStorageItem(
    const std::string& key, const std::shared_ptr<tasm::TemplateData>& data) {
  engine_actor_->Act([key, data](auto& engine) {
    engine->SetClientSessionStorage(std::move(key), data->GetValue());
  });
}

void LynxShell::GetSessionStorageItem(
    const std::string& key, std::unique_ptr<shell::PlatformCallBack> callback) {
  auto callback_holder = facade_actor_->ActSync(
      [callback = std::move(callback)](auto& facade) mutable {
        return facade->CreatePlatformCallBackHolder(std::move(callback));
      });
  engine_actor_->Act([key, callback_holder = std::move(callback_holder)](
                         auto& engine) mutable {
    engine->GetClientSessionStorage(key, std::move(callback_holder));
  });
}

int32_t LynxShell::SubscribeSessionStorage(
    const std::string& key, std::unique_ptr<shell::PlatformCallBack> callback) {
  auto callback_holder = facade_actor_->ActSync(
      [callback = std::move(callback)](auto& facade) mutable {
        return facade->CreatePlatformCallBackHolder(std::move(callback));
      });
  int32_t id = callback_holder->id();
  engine_actor_->Act([key, callback_holder = std::move(callback_holder)](
                         auto& engine) mutable {
    engine->SubscribeClientSessionStorage(key, std::move(callback_holder));
  });
  return id;
}

void LynxShell::UnSubscribeSessionStorage(const std::string& key,
                                          double callback_id) {
  engine_actor_->Act([key, callback_id](auto& engine) {
    engine->UnsubscribeClientSessionStorage(std::move(key), callback_id);
  });
}

void LynxShell::ReloadTemplate(const std::shared_ptr<tasm::TemplateData>& data,
                               const lepus::Value& global_props) {
  tasm::PipelineOptions pipeline_options;
  pipeline_options.need_timestamps = true;
  // TODO(kechenglong): should find a better pipeline_origin name?
  pipeline_options.pipeline_origin = tasm::timing::kReloadBundle;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  EnsureTemplateDataThreadSafe(data);
  auto order = ui_operation_queue_->UpdateNativeUpdateDataOrder();
  engine_actor_->Act(
      [data, global_props, order,
       pipeline_options = std::move(pipeline_options)](auto& engine) {
        engine->ReloadTemplate(data, global_props, order,
                               std::move(pipeline_options));
      });
}

void LynxShell::UpdateConfig(const lepus::Value& config) {
  tasm::PipelineOptions pipeline_options;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  engine_actor_->Act(
      [config, pipeline_options = std::move(pipeline_options)](auto& engine) {
        engine->UpdateConfig(config, std::move(pipeline_options));
      });
}

void LynxShell::UpdateGlobalProps(const lepus::Value& global_props) {
  tasm::PipelineOptions pipeline_options;
  OnPipelineStart(pipeline_options.pipeline_id,
                  pipeline_options.pipeline_origin,
                  pipeline_options.pipeline_start_timestamp);
  auto global_props_thread_safe = EnsureGlobalPropsThreadSafe(global_props);
  engine_actor_->Act(
      [global_props_thread_safe,
       pipeline_options = std::move(pipeline_options)](auto& engine) {
        engine->UpdateGlobalProps(global_props_thread_safe,
                                  std::move(pipeline_options));
      });
}

void LynxShell::UpdateViewport(float width, int32_t width_mode, float height,
                               int32_t height_mode, bool need_layout) {
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::LynxViewInitRecorder::GetInstance().RecordViewPort(
      height_mode, width_mode, height, width, height, width,
      tasm::Config::pixelRatio(), reinterpret_cast<int64_t>(this));
#endif
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, "LynxShell.UpdateViewport",
      [&](lynx::perfetto::EventContext ctx) {
        std::string view_port_info_str =
            base::FormatString("size: %.1f, %.1f; mode: %d, %d", width, height,
                               width_mode, height_mode);
        ctx.event()->add_debug_annotations("viewport", view_port_info_str);
      });
  engine_actor_->Act([width, width_mode, height, height_mode,
                      need_layout](auto& engine) {
    engine->UpdateViewport(width, width_mode, height, height_mode, need_layout);
  });
}

void LynxShell::TriggerLayout() {
  layout_actor_->Act([](auto& layout) { layout->Layout(); });
}

void LynxShell::UpdateScreenMetrics(float width, float height, float scale) {
  engine_actor_->Act([width, height, scale](auto& engine) {
    engine->UpdateScreenMetrics(width, height, scale);
  });
}

void LynxShell::UpdateFontScale(float scale) {
  engine_actor_->Act([scale](auto& engine) { engine->UpdateFontScale(scale); });
}

void LynxShell::SetFontScale(float scale) {
  engine_actor_->Act([scale](auto& engine) { engine->SetFontScale(scale); });
}

void LynxShell::SetPlatformConfig(std::string platform_config_json_string) {
  engine_actor_->Act([json_string = std::move(platform_config_json_string)](
                         auto& engine) mutable {
    engine->SetPlatformConfig(std::move(json_string));
  });
}

void LynxShell::SyncFetchLayoutResult() {
  engine_actor_->Act([](auto& engine) { engine->SyncFetchLayoutResult(); });
}

void LynxShell::LayoutImmediatelyWithUpdatedViewport(float width,
                                                     int32_t width_mode,
                                                     float height,
                                                     int32_t height_mode) {
  DCHECK(runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
  DCHECK(runners_.GetTASMTaskRunner()->RunsTasksOnCurrentThread());
  DCHECK(!runners_.GetLayoutTaskRunner()->RunsTasksOnCurrentThread());

  auto layout_runner = runners_.GetLayoutTaskRunner();
  auto layout_loop = layout_runner->GetLoop();

  auto ui_loop = runners_.GetUITaskRunner()->GetLoop();

  // TODO(heshan): Merge with the similar logic of EngineThreadSwitch
  layout_runner->Bind(ui_loop);
  tasm_operation_queue_->SetAppendPendingTaskNeededDuringFlush(true);

  UpdateViewport(width, width_mode, height, height_mode);

  TriggerLayout();

  layout_runner->UnBind();
  tasm_operation_queue_->SetAppendPendingTaskNeededDuringFlush(false);

  layout_loop->PostTask(
      [layout_runner, layout_loop]() { layout_runner->Bind(layout_loop); },
      fml::TimePoint::Now(), fml::TaskSourceGrade::kEmergency);
}

void LynxShell::SendCustomEvent(const std::string& name, int32_t tag,
                                const lepus::Value& params,
                                const std::string& params_name) {
  engine_actor_->Act([name, tag, params, params_name](auto& engine) {
    engine->SendCustomEvent(name, tag, params, params_name);
  });
}

void LynxShell::SendGestureEvent(int32_t tag, int32_t gesture_id,
                                 std::string name, const lepus::Value& params) {
  engine_actor_->Act([tag, gesture_id, name, params](auto& engine) {
    engine->SendGestureEvent(tag, gesture_id, name, params);
  });
}

void LynxShell::SendTouchEvent(const std::string& name, int32_t tag, float x,
                               float y, float client_x, float client_y,
                               float page_x, float page_y) {
  tasm::EventInfo info(tag, x, y, client_x, client_y, page_x, page_y);
  engine_actor_->Act([name, info = std::move(info)](auto& engine) mutable {
    (void)engine->SendTouchEvent(name, info);
  });
}

void LynxShell::OnPseudoStatusChanged(int32_t id, int32_t pre_status,
                                      int32_t current_status) {
  engine_actor_->Act([id, pre_status, current_status](auto& engine) {
    (void)engine->OnPseudoStatusChanged(id, pre_status, current_status);
  });
}

void LynxShell::SendBubbleEvent(const std::string& name, int32_t tag,
                                lepus::DictionaryPtr dict) {
  engine_actor_->Act([name, tag, &dict](auto& engine) {
    engine->SendBubbleEvent(name, tag, dict);
  });
}

void LynxShell::SendSsrGlobalEvent(const std::string& name,
                                   const lepus_value& params) {
  runtime_actor_->ActAsync([name, params](auto& runtime) {
    runtime->SendSsrGlobalEvent(name, params);
  });
}

void LynxShell::SendGlobalEventToLepus(const std::string& name,
                                       const lepus_value& params) {
  engine_actor_->Act([name, params](auto& engine) {
    engine->SendGlobalEventToLepus(name, params);
  });
}

void LynxShell::TriggerEventBus(const std::string& name,
                                const lepus_value& params) {
  engine_actor_->Act(
      [name, params](auto& engine) { engine->TriggerEventBus(name, params); });
}

std::unique_ptr<lepus_value> LynxShell::GetCurrentData() {
  return engine_actor_->ActSync(
      [](auto& engine) { return engine->GetCurrentData(); });
}

const lepus::Value LynxShell::GetPageDataByKey(std::vector<std::string> keys) {
  return engine_actor_->ActSync([keys = std::move(keys)](auto& engine) mutable {
    return engine->GetPageDataByKey(std::move(keys));
  });
}

// TODO(heshan): change this method to be private
tasm::ListNode* LynxShell::GetListNode(int32_t tag) {
  // ensure on engine thread
  DCHECK(engine_actor_->CanRunNow());

  return engine_actor_->Impl()->GetListNode(tag);
}

void LynxShell::RenderListChild(int32_t tag, uint32_t index,
                                int64_t operation_id) {
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  auto* list_node = GetListNode(tag);
  if (list_node != nullptr) {
    list_node->RenderComponentAtIndex(index, operation_id);
  }
}

void LynxShell::UpdateListChild(int32_t tag, uint32_t sign, uint32_t index,
                                int64_t operation_id) {
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  auto* list_node = GetListNode(tag);
  if (list_node != nullptr) {
    list_node->UpdateComponent(sign, index, operation_id);
  }
}

void LynxShell::RemoveListChild(int32_t tag, uint32_t sign) {
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  auto* list_node = GetListNode(tag);
  if (list_node != nullptr) {
    list_node->RemoveComponent(sign);
  }
}

int32_t LynxShell::ObtainListChild(int32_t tag, uint32_t index,
                                   int64_t operation_id,
                                   bool enable_reuse_notification) {
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  auto* list_node = GetListNode(tag);
  if (list_node == nullptr) {
    return -1;
  }
  return list_node->ComponentAtIndex(index, operation_id,
                                     enable_reuse_notification);
}

void LynxShell::RecycleListChild(int32_t tag, uint32_t sign) {
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  auto* list_node = GetListNode(tag);
  if (list_node != nullptr) {
    list_node->EnqueueComponent(sign);
  }
}

void LynxShell::ScrollByListContainer(int32_t tag, float offset_x,
                                      float offset_y, float original_x,
                                      float original_y) {
  engine_actor_->Act(
      [tag, offset_x, offset_y, original_x, original_y](auto& engine) {
        engine->ScrollByListContainer(tag, offset_x, offset_y, original_x,
                                      original_y);
      });
}

void LynxShell::ScrollToPosition(int32_t tag, int index, float offset,
                                 int align, bool smooth) {
  engine_actor_->Act([tag, index, offset, align, smooth](auto& engine) {
    engine->ScrollToPosition(tag, index, offset, align, smooth);
  });
}

void LynxShell::ScrollStopped(int32_t tag) {
  engine_actor_->Act([tag](auto& engine) { engine->ScrollStopped(tag); });
}

void LynxShell::AssembleListPlatformInfo(
    int32_t tag, base::MoveOnlyClosure<void, tasm::ListNode*> assembler) {
  ThreadModeAutoSwitch auto_switch(thread_mode_manager_);

  auto* list_node = GetListNode(tag);
  if (list_node != nullptr) {
    assembler(list_node);
  }
}

// TODO(heshan): For auto concurrency, add a sub-queue to process async tasks of
// the list. This will prevent dirty data caused by client update data
// triggering list rendering.
void LynxShell::LoadListNode(int32_t tag, uint32_t index, int64_t operationId,
                             bool enable_reuse_notification) {
  engine_actor_->ActAsync([tag, index, operationId,
                           enable_reuse_notification](auto& engine) {
    tasm::ListNode* listNode = engine->GetListNode(tag);
    if (listNode) {
      listNode->ComponentAtIndex(index, operationId, enable_reuse_notification);
    }
  });
}

void LynxShell::EnqueueListNode(int32_t tag, uint32_t component_tag) {
  engine_actor_->ActAsync([tag, component_tag](auto& engine) {
    tasm::ListNode* listNode = engine->GetListNode(tag);
    if (listNode) {
      listNode->EnqueueComponent(component_tag);
    }
  });
}

void LynxShell::OnEnterForeground() {
  if (IsDestroyed()) {
    return;
  }
  if (app_state_ == AppState::kForeground) {
    return;
  }
  app_state_ = AppState::kForeground;
// TODO(liukeang): remove macro
#if ENABLE_AIR
  if (!enable_runtime_) {
    engine_actor_->Act([](auto& engine) {
      engine->SendAirPageEvent("onShow", lepus_value());
    });
    return;
  }
#endif
  runtime::MessageEvent event(runtime::kMessageEventTypeOnAppEnterForeground,
                              runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value());
  tasm_mediator_->DispatchMessageEvent(std::move(event));
}

void LynxShell::OnEnterBackground() {
  if (IsDestroyed()) {
    return;
  }
  if (app_state_ == AppState::kBackground) {
    return;
  }
  app_state_ = AppState::kBackground;
#if ENABLE_AIR
  if (!enable_runtime_) {
    engine_actor_->Act([](auto& engine) {
      engine->SendAirPageEvent("onHide", lepus_value());
    });
    return;
  }
#endif
  runtime::MessageEvent event(runtime::kMessageEventTypeOnAppEnterBackground,
                              runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value());
  tasm_mediator_->DispatchMessageEvent(std::move(event));
}

void LynxShell::UpdateI18nResource(const std::string& key,
                                   const std::string& new_data) {
  engine_actor_->Act([key, new_data](auto& engine) {
    engine->UpdateI18nResource(key, new_data);
  });
}

std::unordered_map<std::string, std::string> LynxShell::GetAllJsSource() {
  return engine_actor_->ActSync(
      [](auto& engine) { return engine->GetAllJsSource(); });
}

std::shared_ptr<tasm::TemplateAssembler> LynxShell::GetTasm() {
  return engine_actor_->Impl()->GetTasm();
}

tasm::LynxGetUIResult LynxShell::GetLynxUI(
    const tasm::NodeSelectRoot& root, const tasm::NodeSelectOptions& options) {
  return engine_actor_->Impl()->GetLynxUI(root, options);
}

void LynxShell::RunOnTasmThread(std::function<void(void)>&& task) {
  engine_actor_->Act([task = std::move(task)](auto& engine) { task(); });
}

void LynxShell::AttachEngineToUIThread() {
  if (engine_thread_switch_) {
    switch (current_strategy_) {
      case base::ThreadStrategyForRendering::MOST_ON_TASM:
        current_strategy_ = base::ThreadStrategyForRendering::ALL_ON_UI;
        break;
      case base::ThreadStrategyForRendering::MULTI_THREADS:
        current_strategy_ = base::ThreadStrategyForRendering::PART_ON_LAYOUT;
        break;
      default:
        return;
    }
    engine_thread_switch_->AttachEngineToUIThread();
    OnThreadStrategyUpdated();
  }
}

void LynxShell::DetachEngineFromUIThread() {
  if (engine_thread_switch_) {
    switch (current_strategy_) {
      case base::ThreadStrategyForRendering::ALL_ON_UI:
        current_strategy_ = base::ThreadStrategyForRendering::MOST_ON_TASM;
        break;
      case base::ThreadStrategyForRendering::PART_ON_LAYOUT:
        current_strategy_ = base::ThreadStrategyForRendering::MULTI_THREADS;
        break;
      default:
        return;
    }

    if (!engine_thread_switch_->HasSetEngineLoop()) {
      engine_thread_switch_->SetEngineLoop(runners_.GetTASMLoop());
    }
    engine_thread_switch_->DetachEngineFromUIThread();
    OnThreadStrategyUpdated();
  }
}

void LynxShell::OnThreadStrategyUpdated() {
  runners_.OnThreadStrategyUpdated(current_strategy_);
  timing_actor_->Act(
      [current_strategy = current_strategy_](auto& timing_handler) {
        timing_handler->SetThreadStrategy(current_strategy);
      });
  engine_actor_->Act([current_strategy = current_strategy_](auto& engine) {
    engine->GetTasm()->page_proxy()->element_manager()->SetThreadStrategy(
        current_strategy);
  });
}

uint32_t LynxShell::ThreadStrategy() { return current_strategy_; }

void LynxShell::EnsureTemplateDataThreadSafe(
    const std::shared_ptr<tasm::TemplateData>& template_data) {
  // need clone template data if consumed by tasm thread
  if (template_data != nullptr && !(engine_actor_->CanRunNow())) {
    template_data->CloneValue();
  }
}

lepus::Value LynxShell::EnsureGlobalPropsThreadSafe(
    const lepus::Value& global_props) {
  // need clone global props if consumed by tasm thread
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxShell::EnsureGlobalPropsThreadSafe");
  if (!(engine_actor_->CanRunNow())) {
    return lynx::lepus::Value::Clone(global_props);
  } else {
    return global_props;
  }
}

void LynxShell::PreloadLazyBundles(std::vector<std::string> urls) {
  engine_actor_->Act([urls = std::move(urls)](auto& engine) {
    engine->PreloadLazyBundles(urls);
  });
}

void LynxShell::RegisterLazyBundle(std::string url,
                                   tasm::LynxTemplateBundle bundle) {
  engine_actor_->Act(
      [url = std::move(url), bundle = std::move(bundle)](auto& engine) mutable {
        engine->InsertLynxTemplateBundle(url, std::move(bundle));
      });
}

void LynxShell::SetEnableBytecode(bool enable,
                                  std::string bytecode_source_url) {
  runtime_actor_->ActAsync([enable, bytecode_source_url = std::move(
                                        bytecode_source_url)](auto& runtime) {
    runtime->SetEnableBytecode(enable, bytecode_source_url);
  });
}

void LynxShell::SetAnimationsPending(bool need_pending_ui_op) {
  engine_actor_->ActLite([need_pending_ui_op](auto& engine) {
    engine->SetAnimationsPending(need_pending_ui_op);
  });
}

void LynxShell::DispatchMessageEvent(runtime::MessageEvent event) {
  if (event.IsSendingToUIThread()) {
    facade_actor_->Act([event = std::move(event)](auto& facade) mutable {
      facade->OnReceiveMessageEvent(std::move(event));
    });
  } else if (event.IsSendingToCoreThread()) {
    engine_actor_->Act([event = std::move(event)](auto& engine) mutable {
      engine->OnReceiveMessageEvent(std::move(event));
    });
  } else if (event.IsSendingToJSThread()) {
    runtime_actor_->Act([event = std::move(event)](auto& runtime) mutable {
      runtime->OnReceiveMessageEvent(std::move(event));
    });
  }
}

// Timing related function.
void LynxShell::SetTiming(uint64_t us_timestamp,
                          tasm::timing::TimestampKey timing_key,
                          tasm::PipelineID pipeline_id) const {
  timing_actor_->Act(
      [us_timestamp, timing_key = std::move(timing_key),
       pipeline_id = std::move(pipeline_id)](auto& timing_handler) mutable {
        timing_handler->SetTiming(timing_key, us_timestamp, pipeline_id);
      });
}

BASE_EXPORT_FOR_DEVTOOL const lepus::Value LynxShell::GetAllTimingInfo() const {
  return timing_actor_->ActSync([](auto& timing_handler) {
    auto all_timing_info = timing_handler->GetAllTimingInfo();
    lepus::Value lepus_all_info =
        pub::ValueUtils::ConvertValueToLepusValue(*all_timing_info);

    return lepus_all_info;
  });
}

void LynxShell::SetSSRTimingData(std::string url, uint64_t data_size) const {
  timing_actor_->Act([url = std::move(url), data_size](auto& timing_actor) {
    timing_actor->SetSSRTimingData(url, data_size);
  });
}

void LynxShell::ClearAllTimingInfo() const {
  timing_actor_->Act(
      [](auto& timing_handler) { timing_handler->ClearAllTimingInfo(); });
}

void LynxShell::OnPipelineStart(
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
                      pipeline_start_timestamp](auto& timing_handler) {
    timing_handler->OnPipelineStart(pipeline_id, pipeline_origin,
                                    pipeline_start_timestamp);
  });
}

void LynxShell::BindLynxEngineToUIThread() {
  DCHECK(!thread_mode_auto_switch_);

  // TODO(heshan): use TaskRunner::Bind instead.
  thread_mode_auto_switch_ =
      std::make_unique<ThreadModeAutoSwitch>(thread_mode_manager_);
}

void LynxShell::UnbindLynxEngineFromUIThread() {
  DCHECK(thread_mode_auto_switch_);

  // TODO(heshan): use TaskRunner::Unbind instead.
  thread_mode_auto_switch_ = nullptr;
}

void LynxShell::SetInspectorElementObserver(
    const std::shared_ptr<tasm::InspectorElementObserver>&
        inspector_element_observer) {
  engine_actor_->Act([inspector_element_observer](auto& engine) {
    engine->SetInspectorElementObserver(inspector_element_observer);
  });
}

void LynxShell::SetHierarchyObserver(
    const std::shared_ptr<tasm::HierarchyObserver>& hierarchy_observer) {
  layout_actor_->Act([hierarchy_observer](auto& layout) {
    layout->SetHierarchyObserver(hierarchy_observer);
  });
}

void LynxShell::RegisterRuntimeLifecycleObserver(
    const std::shared_ptr<lynx::runtime::IRuntimeLifecycleObserver>&
        runtime_lifecycle_observer,
    base::MoveOnlyClosure<void, fml::RefPtr<fml::TaskRunner>> on_complete) {
  if (runtime_lifecycle_observer) {
    runtime_lifecycle_observer_ = runtime_lifecycle_observer;
    if (on_complete) {
      on_complete(runners_.GetJSTaskRunner());
    }
  }
}

void LynxShell::RegisterModuleFactory(
    std::unique_ptr<lynx::piper::NativeModuleFactory> module_factory) {
  module_factories_.push_back(std::move(module_factory));
}

void LynxShell::OnRuntimeCreate() {
  if (runtime_lifecycle_observer_) {
    DCHECK(runtime_actor_);
    runtime_actor_->Act([observer = runtime_lifecycle_observer_,
                         runtime_actor = runtime_actor_](auto& runtime) {
      observer->OnRuntimeCreate(runtime->GetVSyncObserver());
    });
  }
}

void LynxShell::ConsumeModuleFactory(piper::LynxModuleManager* module_manager) {
  if (module_manager) {
    for (auto& module_factory : module_factories_) {
      if (module_factory) {
        module_manager->SetModuleFactory(std::move(module_factory));
      }
    }
    module_factories_.clear();
  }
}

std::weak_ptr<piper::JsBundleHolder> LynxShell::GetWeakJsBundleHolder() {
  auto tasm = GetTasm();
  return tasm ? tasm->GetJsBundleHolder() : nullptr;
}

}  // namespace shell
}  // namespace lynx
