// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_SHELL_BUILDER_H_
#define CORE_SHELL_LYNX_SHELL_BUILDER_H_

#include <memory>
#include <string>

#include "core/renderer/ui_wrapper/common/prop_bundle_creator_default.h"
#include "core/services/timing_handler/timing_collector_platform_impl.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/native_facade_reporter.h"
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

  LynxShellBuilder& SetNativeFacadeReporter(
      std::unique_ptr<shell::NativeFacadeReporter> native_facade_reporter);

  LynxShellBuilder& SetUseInvokeUIMethodFunction(
      bool use_invoke_ui_method_func);

  LynxShellBuilder& SetLynxEngineCreator(
      const std::function<std::unique_ptr<shell::LynxEngine>(
          std::unique_ptr<TasmMediator>)>& lynx_engine_creator);

  LynxShellBuilder& SetPaintingContextCreator(
      const std::function<std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl>(
          LynxShell*)>& painting_context_creator);

  LynxShellBuilder& SetPaintingContextPlatformImpl(
      std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl> painting_context);

  LynxShellBuilder& SetLynxEnvConfig(tasm::LynxEnvConfig& lynx_env_config);

  LynxShellBuilder& SetEnableDiffWithoutLayout(bool enable_diff_without_layout);

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

  LynxShellBuilder& SetTimingActor(
      const std::shared_ptr<LynxActor<tasm::timing::TimingHandler>>&
          timing_actor);

  LynxShellBuilder& SetTimingCollectorPlatform(
      const std::shared_ptr<tasm::timing::TimingCollectorPlatformImpl>&
          timing_collector_platform);

  LynxShellBuilder& SetShellOption(const ShellOption& shell_option);

  LynxShellBuilder& SetPropBundleCreator(
      const std::shared_ptr<tasm::PropBundleCreator>& creator);

  LynxShellBuilder& SetTasmPlatformInvoker(
      std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker);

  LynxShell* build();

 private:
  std::unique_ptr<lynx::shell::LynxEngine> CreateLynxEngine(
      std::unique_ptr<TasmMediator> tasm_mediator,
      base::TaskRunnerManufactor& runners,
      const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr,
      int32_t instance_id, LynxShell* shell);

  std::unique_ptr<shell::NativeFacade> native_facade_;
  // TODO(nihao.royal): The timing_mediator uses this to send performanceEntry
  // to the reporter thread. This should be refactored after native_facade_ can
  // send events to the async thread.
  std::unique_ptr<shell::NativeFacadeReporter> native_facade_reporter_;

  bool use_invoke_ui_method_func_;

  std::function<std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl>(
      LynxShell*)>
      painting_context_creator_;
  std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl> painting_context_;

  tasm::LynxEnvConfig lynx_env_config_{0, 0, 1, 1};
  std::shared_ptr<lynx::tasm::LazyBundleLoader> loader_;
  std::shared_ptr<lynx::tasm::WhiteBoard> white_board_;
  std::shared_ptr<lynx::shell::VSyncMonitor> element_manager_vsync_monitor_;
  bool enable_new_animator_{false};
  bool enable_native_list_{false};
  bool enable_layout_only_{true};
  bool enable_pre_update_data_{false};
  bool enable_diff_without_layout_{false};
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
  std::shared_ptr<LynxActor<tasm::timing::TimingHandler>> timing_actor_{};
  std::shared_ptr<tasm::timing::TimingCollectorPlatformImpl>
      timing_collector_platform_{};

  ShellOption shell_option_;

  std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_SHELL_BUILDER_H_
