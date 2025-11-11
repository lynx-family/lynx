// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_SHELL_BUILDER_H_
#define CORE_SHELL_LYNX_SHELL_BUILDER_H_

#include <memory>
#include <string>

#include "core/public/layout_ctx_platform_impl.h"
#include "core/public/vsync_monitor_platform_impl.h"
#include "core/renderer/ui_wrapper/common/prop_bundle_creator_default.h"
#include "core/services/performance/performance_controller.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/tasm_platform_invoker.h"

namespace lynx {
namespace shell {
class LynxEngine;
class NativeFacade;

class LynxShellBuilder {
 public:
  LynxShellBuilder() = default;
  ~LynxShellBuilder() = default;

  LynxShellBuilder& SetNativeFacade(
      std::unique_ptr<shell::NativeFacade> native_facade);

  LynxShellBuilder& SetUseInvokeUIMethodFunction(
      bool use_invoke_ui_method_func);

  LynxShellBuilder& SetLynxEngineCreator(
      const std::function<std::unique_ptr<shell::LynxEngine>(
          std::unique_ptr<TasmMediator>)>& lynx_engine_creator);

  LynxShellBuilder& SetLynxEngineWrapper(
      shell::LynxEngineWrapper* engine_wrapper);

  LynxShellBuilder& SetPaintingContextCreator(
      const std::function<std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl>(
          LynxShell*)>& painting_context_creator);

  LynxShellBuilder& SetPaintingContextPlatformImpl(
      std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl> painting_context);

  LynxShellBuilder& SetLynxEnvConfig(tasm::LynxEnvConfig& lynx_env_config);

  LynxShellBuilder& SetEnableElementManagerVsyncMonitor(
      bool enable_element_manager_vsync_monitor);

  LynxShellBuilder& SetEnableNewAnimator(bool enable_new_animator);

  LynxShellBuilder& SetEnableNativeList(bool enable_native_list);

  LynxShellBuilder& SetLazyBundleLoader(
      const std::shared_ptr<lynx::tasm::LazyBundleLoader>& loader);

  LynxShellBuilder& SetWhiteBoard(
      const std::shared_ptr<lynx::tasm::WhiteBoard>& white_board);

  LynxShellBuilder& SetEnablePreUpdateData(bool enable_pre_update_data);

  LynxShellBuilder& SetEnableLayoutOnly(bool enable_layout_only);

  LynxShellBuilder& SetTasmLocale(const std::string& locale);

  LynxShellBuilder& SetLayoutContextPlatformImpl(
      std::unique_ptr<lynx::tasm::LayoutCtxPlatformImpl> layout_context);

  LynxShellBuilder& SetStrategy(base::ThreadStrategyForRendering strategy);

  LynxShellBuilder& SetEngineActor(
      const std::function<void(const std::shared_ptr<LynxActor<LynxEngine>>&)>&
          on_engine_actor_created);

  LynxShellBuilder& SetRuntimeActor(
      const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& runtime_actor);

  LynxShellBuilder& SetPerfControllerActor(
      const std::shared_ptr<
          LynxActor<tasm::performance::PerformanceController>>& perf_actor);

  LynxShellBuilder& SetPerformanceControllerPlatform(
      std::unique_ptr<tasm::performance::PerformanceControllerPlatformImpl>
          performance_controller_platform);

  LynxShellBuilder& SetShellOption(const ShellOption& shell_option);

  LynxShellBuilder& SetPropBundleCreator(
      const std::shared_ptr<tasm::PropBundleCreator>& creator);

  LynxShellBuilder& SetTasmPlatformInvoker(
      std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker);

  LynxShellBuilder& SetForceLayoutOnBackgroundThread(
      bool force_layout_on_background_thread);

  LynxShellBuilder& SetEnableUnifiedPipeline(bool enable_unified_pipeline);

  LynxShellBuilder& SetNativeModuleManager(
      std::unique_ptr<lynx::pub::LynxNativeModuleManager>
          native_module_factory);

  LynxShellBuilder& SetVSyncMonitorPlatformImpl(
      const std::shared_ptr<base::VSyncMonitorPlatformImpl>& monitor);

  LynxShell* build();

 private:
  std::unique_ptr<lynx::shell::LynxEngine> CreateLynxEngine(
      std::unique_ptr<TasmMediator> tasm_mediator,
      base::TaskRunnerManufactor& runners,
      const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr,
      int32_t instance_id, LynxShell* shell,
      std::unique_ptr<lynx::tasm::LayoutCtxPlatformImpl>
          platform_layout_context);
  void AttachLynxEngine(LynxShell* shell);
  //  void DetachLynxEngine()

  std::unique_ptr<shell::NativeFacade> native_facade_;

  bool use_invoke_ui_method_func_;

  std::function<std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl>(
      LynxShell*)>
      painting_context_creator_;
  std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl> painting_context_;

  tasm::LynxEnvConfig lynx_env_config_{0, 0, 1, 1};
  std::shared_ptr<lynx::tasm::LazyBundleLoader> loader_;
  std::shared_ptr<lynx::tasm::WhiteBoard> white_board_;
  std::shared_ptr<lynx::base::VSyncMonitor> element_manager_vsync_monitor_;
  bool enable_new_animator_{false};
  bool enable_native_list_{false};
  bool enable_layout_only_{true};
  bool enable_pre_update_data_{false};
  bool enable_unified_pipeline_{false};

  std::string locale_;

  std::function<std::unique_ptr<shell::LynxEngine>(
      std::unique_ptr<TasmMediator>)>
      lynx_engine_creator_;

  std::unique_ptr<lynx::tasm::LayoutCtxPlatformImpl> layout_context_;

  std::shared_ptr<tasm::PropBundleCreator> prop_bundle_creator_ =
      std::make_shared<lynx::tasm::PropBundleCreatorDefault>();

  base::ThreadStrategyForRendering strategy_;

  std::function<void(const std::shared_ptr<LynxActor<LynxEngine>>&)>
      on_engine_actor_created_;

  std::shared_ptr<LynxActor<runtime::LynxRuntime>> runtime_actor_{};
  std::shared_ptr<LynxActor<tasm::performance::PerformanceController>>
      perf_controller_actor_{};
  std::unique_ptr<tasm::performance::PerformanceControllerPlatformImpl>
      performance_controller_platform_;
  shell::LynxEngineWrapper* lynx_engine_wrapper_{nullptr};

  ShellOption shell_option_;

  std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker_;

  bool force_layout_on_background_thread_{false};
  tasm::PageOptions page_options_;

  std::unique_ptr<lynx::pub::LynxNativeModuleManager> native_module_manager_;

  std::shared_ptr<base::VSyncMonitorPlatformImpl>
      vsync_monitor_platform_impl_{};
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_SHELL_BUILDER_H_
