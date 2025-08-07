// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime_standalone_helper.h"

#include <utility>
#include <vector>

#include "core/base/threading/vsync_monitor.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/performance_mediator.h"
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
    std::vector<std::string> preload_js_paths,
    const std::string& bytecode_source_url, uint32_t runtime_flag,
    const lepus::Value* global_props, bool long_task_monitor_disabled) {
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

  auto delegate = std::make_unique<RuntimeMediator>(
      native_runtime_facade, nullptr, performance_actor, nullptr,
      js_task_runner, std::move(external_resource_loader));
  delegate->SetPropBundleCreator(prop_bundle_creator);
  delegate->SetWhiteBoardDelegate(white_board_delegate);
  auto* delegate_raw_ptr = delegate.get();

  auto page_options = tasm::PageOptions(instance_id);
  page_options.SetLongTaskMonitorDisabled(long_task_monitor_disabled);

  auto runtime = std::make_unique<runtime::LynxRuntime>(
      group_id, instance_id, std::move(delegate), bytecode_source_url,
      runtime_flag, page_options);
  auto runtime_actor = std::make_shared<LynxActor<runtime::LynxRuntime>>(
      std::move(runtime), js_task_runner, instance_id, true);
  delegate_raw_ptr->set_vsync_monitor(vsync_monitor, runtime_actor);
  perf_mediator_raw_ptr->SetRuntimeActor(runtime_actor);
  timing_mediator_raw_ptr->SetRuntimeActor(runtime_actor);

  on_runtime_actor_created(runtime_actor, native_runtime_facade);
  external_resource_loader_ptr->SetRuntimeActor(runtime_actor);
  white_board_delegate->SetRuntimeActor(runtime_actor);
  white_board_delegate->SetRuntimeFacadeActor(native_runtime_facade);
  const auto global_props_value = global_props
                                      ? lynx::lepus::Value::Clone(*global_props)
                                      : lynx::lepus::Value();
  runtime_actor->ActAsync(
      [module_manager, preload_js_paths = std::move(preload_js_paths),
       runtime_observer, global_props_value,
       vsync_monitor](std::unique_ptr<runtime::LynxRuntime>& runtime) mutable {
        vsync_monitor->BindToCurrentThread();
        vsync_monitor->Init();
        runtime->OnGlobalPropsUpdated(global_props_value);
        runtime->Init(module_manager, runtime_observer,
                      std::move(preload_js_paths));
      });

  return {runtime_actor, performance_actor, native_runtime_facade,
          white_board_delegate};
}

}  // namespace shell
}  // namespace lynx
