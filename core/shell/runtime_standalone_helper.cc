// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime_standalone_helper.h"

#include <utility>
#include <vector>

#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/timing_handler/timing_mediator.h"
#include "core/shell/common/vsync_monitor.h"
#include "core/shell/lynx_runtime_actor_holder.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/runtime_mediator.h"

namespace lynx {
namespace shell {

InitRuntimeStandaloneResult InitRuntimeStandalone(
    const std::string& group_name, const std::string& group_id,
    std::unique_ptr<NativeFacade> native_facade_runtime,
    const std::shared_ptr<piper::InspectorRuntimeObserverNG>& runtime_observer,
    const std::shared_ptr<lynx::pub::LynxResourceLoader>& resource_loader,
    const std::shared_ptr<lynx::piper::LynxModuleManager>& module_manager,
    const std::shared_ptr<tasm::PropBundleCreator>& prop_bundle_creator,
    const std::shared_ptr<tasm::WhiteBoard>& white_board,
    const std::function<
        void(const std::shared_ptr<LynxActor<runtime::LynxRuntime>>&,
             const std::shared_ptr<LynxActor<NativeFacade>>&)>&
        on_runtime_actor_created,
    std::vector<std::string> preload_js_paths, bool enable_js_group_thread,
    bool force_reload_js_core, bool force_use_light_weight_js_engine,
    bool pending_js_task, bool enable_user_bytecode,
    const std::string& bytecode_source_url) {
  auto instance_id = lynx::shell::LynxShell::NextInstanceId();
  lynx::fml::RefPtr<lynx::fml::TaskRunner> js_task_runner =
      lynx::base::TaskRunnerManufactor::GetJSRunner(group_name);
  auto native_runtime_facade =
      std::make_shared<lynx::shell::LynxActor<lynx::shell::NativeFacade>>(
          std::move(native_facade_runtime), js_task_runner, instance_id, true);
  std::shared_ptr<VSyncMonitor> vsync_monitor =
      lynx::shell::VSyncMonitor::Create();

  auto timing_mediator =
      std::make_unique<lynx::tasm::timing::TimingMediator>(instance_id);
  timing_mediator->SetEnableJSRuntime(true);
  // TODO(huzhanbo.luc): use TimingHandler to set back actors to TimingMediator,
  // so that we can avoid raw ptr
  auto timing_mediator_raw_ptr = timing_mediator.get();

  auto timing_actor = std::make_shared<LynxActor<tasm::timing::TimingHandler>>(
      std::make_unique<tasm::timing::TimingHandler>(std::move(timing_mediator)),
      tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner(),
      instance_id);

  auto external_resource_loader =
      std::make_unique<ExternalResourceLoader>(resource_loader);
  auto* external_resource_loader_ptr = external_resource_loader.get();

  auto white_board_delegate =
      std::make_shared<tasm::WhiteBoardRuntimeDelegate>(white_board);

  auto delegate = std::make_unique<RuntimeMediator>(
      native_runtime_facade, nullptr, timing_actor, nullptr, js_task_runner,
      std::move(external_resource_loader));
  delegate->set_vsync_monitor(vsync_monitor);
  delegate->SetPropBundleCreator(prop_bundle_creator);
  delegate->SetWhiteBoardDelegate(white_board_delegate);

  auto runtime = std::make_unique<runtime::LynxRuntime>(
      group_id, instance_id, std::move(delegate), enable_user_bytecode,
      bytecode_source_url, enable_js_group_thread);
  auto runtime_actor = std::make_shared<LynxActor<runtime::LynxRuntime>>(
      std::move(runtime), js_task_runner, instance_id, true);
  vsync_monitor->set_runtime_actor(runtime_actor);
  timing_mediator_raw_ptr->SetRuntimeActor(runtime_actor);

  on_runtime_actor_created(runtime_actor, native_runtime_facade);
  external_resource_loader_ptr->SetRuntimeActor(runtime_actor);
  white_board_delegate->SetRuntimeActor(runtime_actor);
  white_board_delegate->SetRuntimeFacadeActor(native_runtime_facade);

  runtime_actor->ActAsync(
      [module_manager, preload_js_paths = std::move(preload_js_paths),
       runtime_observer, force_reload_js_core, force_use_light_weight_js_engine,
       vsync_monitor](std::unique_ptr<runtime::LynxRuntime>& runtime) mutable {
        vsync_monitor->BindToCurrentThread();
        vsync_monitor->Init();
        runtime->Init(module_manager, runtime_observer, nullptr,
                      std::move(preload_js_paths), force_reload_js_core,
                      force_use_light_weight_js_engine);
      });

  return {runtime_actor, timing_actor, native_runtime_facade,
          white_board_delegate};
}

void TriggerDestroyRuntime(
    const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& runtime_actor,
    std::string js_group_thread_name) {
  auto instance_id = runtime_actor->GetInstanceId();
  auto runtime = runtime_actor->Impl();
  if (runtime->TryToDestroy()) {
    runtime_actor->Act([instance_id](auto& runtime) {
      runtime = nullptr;
      tasm::report::FeatureCounter::Instance()->ClearAndReport(instance_id);
    });
  } else {
    // Hold LynxRuntime. It will be released when destroyed callback be
    // handled in LynxRuntime::CallJSCallback() or the delayed release
    // task time out.
    auto holder = LynxRuntimeActorHolder::GetInstance();
    holder->Hold(runtime_actor, js_group_thread_name);
    holder->PostDelayedRelease(instance_id, js_group_thread_name);
  }
}

}  // namespace shell
}  // namespace lynx
