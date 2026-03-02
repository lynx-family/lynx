// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_shell_builder.h"

#include <memory>
#include <string>
#include <utility>

#include "core/services/performance/performance_controller.h"
#include "core/services/performance/performance_mediator.h"
#include "core/shared_data/lynx_white_board.h"
#include "core/shell/common/shell_trace_event_def.h"

namespace lynx {
namespace shell {

LynxShellBuilder& LynxShellBuilder::SetNativeFacade(
    std::unique_ptr<shell::NativeFacade> native_facade) {
  this->native_facade_ = std::move(native_facade);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetUseInvokeUIMethodFunction(
    bool use_invoke_ui_method_func) {
  this->use_invoke_ui_method_func_ = use_invoke_ui_method_func;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetPaintingContextCreator(
    const std::function<std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl>(
        LynxShell*)>& painting_context_creator) {
  this->painting_context_creator_ = painting_context_creator;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetPaintingContextPlatformImpl(
    std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl> painting_context) {
  this->painting_context_ = std::move(painting_context);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetLynxEnvConfig(
    tasm::LynxEnvConfig& lynx_env_config) {
  this->lynx_env_config_ = std::move(lynx_env_config);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetLazyBundleLoader(
    const std::shared_ptr<lynx::tasm::LazyBundleLoader>& loader) {
  this->loader_ = loader;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetEnableUnifiedPipeline(
    bool enable_unified_pipeline) {
  this->enable_unified_pipeline_ = enable_unified_pipeline;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetWhiteBoard(
    const std::shared_ptr<lynx::tasm::WhiteBoard>& white_board) {
  this->white_board_ = white_board;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetEnableElementManagerVsyncMonitor(
    bool enable_element_manager_vsync_monitor) {
  if (enable_element_manager_vsync_monitor) {
    if (this->vsync_monitor_platform_impl_) {
      this->element_manager_vsync_monitor_ =
          std::make_shared<base::VSyncMonitor>(
              this->vsync_monitor_platform_impl_,
              lynx::tasm::LynxEnv::GetInstance()
                  .EnableAnimationVsyncOnUIThread());
    } else {
      this->element_manager_vsync_monitor_ = base::VSyncMonitor::Create(
          lynx::tasm::LynxEnv::GetInstance().EnableAnimationVsyncOnUIThread());
    }
  } else {
    this->element_manager_vsync_monitor_ = nullptr;
  }
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetEnableNewAnimator(
    bool enable_new_animator) {
  this->enable_new_animator_ = enable_new_animator;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetEnableNativeList(
    bool enable_native_list) {
  this->enable_native_list_ = enable_native_list;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetEnablePreUpdateData(
    bool enable_pre_update_data) {
  this->enable_pre_update_data_ = enable_pre_update_data;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetEnableLayoutOnly(
    bool enable_layout_only) {
  this->enable_layout_only_ = enable_layout_only;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetTasmLocale(const std::string& locale) {
  this->locale_ = locale;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetLayoutContextPlatformImpl(
    std::unique_ptr<lynx::tasm::LayoutCtxPlatformImpl> layout_context) {
  this->layout_context_ = std::move(layout_context);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetStrategy(
    base::ThreadStrategyForRendering strategy) {
  this->strategy_ = strategy;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetLynxEngineWrapper(
    shell::LynxEngineWrapper* engine_wrapper) {
  lynx_engine_wrapper_ = engine_wrapper;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetRuntimeActor(
    const std::shared_ptr<LynxActor<BTSRuntime>>& runtime_actor) {
  this->runtime_actor_ = runtime_actor;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetPerfControllerActor(
    const std::shared_ptr<LynxActor<tasm::performance::PerformanceController>>&
        perf_actor) {
  this->perf_controller_actor_ = perf_actor;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetPerformanceControllerPlatform(
    std::unique_ptr<tasm::performance::PerformanceControllerPlatformImpl>
        performance_controller_platform) {
  this->performance_controller_platform_ =
      std::move(performance_controller_platform);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetShellOption(
    const ShellOption& shell_option) {
  this->shell_option_ = shell_option;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetPropBundleCreator(
    const std::shared_ptr<tasm::PropBundleCreator>& creator) {
  this->prop_bundle_creator_ = creator;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetForceLayoutOnBackgroundThread(
    bool force_layout_on_background_thread) {
  this->force_layout_on_background_thread_ = force_layout_on_background_thread;
  return *this;
}

LynxShell* LynxShellBuilder::build() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_SHELL_BUILDER_BUILD,
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("thread_strategy",
                                                   std::to_string(strategy_));
                ctx.event()->add_debug_annotations(
                    "js_group_thread_name",
                    shell_option_.js_group_thread_name_);
                ctx.event()->add_debug_annotations(
                    "enable_js_group_thread",
                    std::to_string(shell_option_.enable_js_group_thread_));
              });
  LynxShell* shell = new LynxShell(this->strategy_, this->shell_option_);
  if (this->shell_option_.instance_id_ == kUnknownInstanceId) {
    this->shell_option_.instance_id_ = shell->instance_id_;
    this->shell_option_.page_options_.SetInstanceID(shell->instance_id_);
  }

  shell->facade_actor_ = std::make_shared<LynxActor<NativeFacade>>(
      std::move(this->native_facade_), shell->runners_.GetUITaskRunner(),
      shell->instance_id_);
  if (perf_controller_actor_) {
    /**
     * In mode RuntimeStandalone, perf_controller_actor_ and perf_mediator_ will
     * not be created in advance, and perf_mediator_ will be associated with the
     * RunTime Actor, so perf_mediator_ is left blank and perf_controller_actor_
     * is assigned directly.
     */
    shell->perf_mediator_ = nullptr;
    shell->perf_controller_actor_ = perf_controller_actor_;
  } else {
    const auto enable_perf = !shell_option_.page_options_.IsEmbeddedModeOn();
    std::unique_ptr<tasm::performance::PerformanceController> perf_controller;
    if (enable_perf) {
      // create timing mediator & actor
      auto timing_mediator =
          std::make_unique<lynx::tasm::timing::TimingMediator>(
              shell->instance_id_);
      timing_mediator->SetFacadeActor(shell->facade_actor_);
      timing_mediator->SetEnableJSRuntime(this->shell_option_.enable_js_);
      shell->timing_mediator_ = timing_mediator.get();
      // create PerformanceController mediator & actor
      auto performance_mediator =
          std::make_unique<lynx::tasm::performance::PerformanceMediator>();
      shell->perf_mediator_ = performance_mediator.get();

      // Temporarily disable TimingActor in Embedded mode

      perf_controller =
          std::make_unique<tasm::performance::PerformanceController>(
              std::move(performance_mediator), std::move(timing_mediator),
              shell->instance_id_);

      perf_controller->GetTimingHandler().SetEnableJSRuntime(
          this->shell_option_.enable_js_);
      perf_controller->GetTimingHandler().SetThreadStrategy(this->strategy_);
    }
    shell->perf_controller_actor_ =
        std::make_shared<LynxActor<tasm::performance::PerformanceController>>(
            std::move(perf_controller),
            tasm::performance::PerformanceController::GetTaskRunner(),
            shell->instance_id_, enable_perf);
  }
  // Pass the `perf_controller_actor_` to the `PerformanceController`
  // object of the platform layer to establish a mapping relationship.
  if (performance_controller_platform_) {
    performance_controller_platform_->SetActor(shell->perf_controller_actor_);
    if (shell->perf_controller_actor_->Impl() != nullptr) {
      shell->perf_controller_actor_->Impl()->SetPlatformImpl(
          std::move(this->performance_controller_platform_));
    }
  }
  if (loader_ != nullptr) {
    loader_->SetPerfControllerActor(shell->perf_controller_actor_);
  }
  shell->runtime_actor_ = runtime_actor_;

  shell->engine_build_options_.lynx_env_config_ = lynx_env_config_;
  shell->engine_build_options_.lazy_bundle_loader_ = loader_;
  shell->engine_build_options_.white_board_ = white_board_;
  shell->engine_build_options_.element_manager_vsync_monitor_ =
      element_manager_vsync_monitor_;
  shell->engine_build_options_.enable_new_animator_ = enable_new_animator_;
  shell->engine_build_options_.enable_native_list_ = enable_native_list_;
  shell->engine_build_options_.enable_layout_only_ = enable_layout_only_;
  shell->engine_build_options_.enable_pre_update_data_ =
      enable_pre_update_data_;
  shell->engine_build_options_.enable_unified_pipeline_ =
      enable_unified_pipeline_;
  shell->engine_build_options_.use_invoke_ui_method_func_ =
      use_invoke_ui_method_func_;
  shell->engine_build_options_.force_layout_on_background_thread_ =
      force_layout_on_background_thread_;
  shell->engine_build_options_.locale_ = locale_;

  if (lynx_engine_wrapper_ && lynx_engine_wrapper_->HasInit()) {
    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, LYNX_SHELL_BUILDER_ATTACH_ENGINE);
    // Indicates that a reusable LynxEngine object has been obtained.
    AttachLynxEngine(shell);
    LOGI("get Engine by pool");
  } else {
    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                      LYNX_SHELL_BUILDER_CREATE_ENGINE_ACTOR);
    if (!painting_context_ && painting_context_creator_) {
      painting_context_ = painting_context_creator_(shell);
    }
    shell->BuildLynxEngine(std::move(tasm_platform_invoker_),
                           std::move(layout_context_),
                           std::move(painting_context_));
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  shell->OnLynxEngineBuilt(prop_bundle_creator_,
                           std::move(native_module_manager_));
  if (lynx_engine_wrapper_) {
    // After creating the EngineWrapper for the first time or reusing it, the
    // internal objects need to be updated.
    shell->SetBindWithEngineWrapper(true);
    lynx_engine_wrapper_->SetupCore(shell->engine_actor_, shell->layout_actor_,
                                    shell->tasm_mediator_,
                                    shell->layout_mediator_);
  }
  return shell;
}

void LynxShellBuilder::AttachLynxEngine(LynxShell* shell) {
  if (lynx_engine_wrapper_ && lynx_engine_wrapper_->HasInit()) {
    shell->SetBindWithEngineWrapper(true);
    lynx_engine_wrapper_->BindShell(shell);
  }
}

LynxShellBuilder& LynxShellBuilder::SetTasmPlatformInvoker(
    std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker) {
  this->tasm_platform_invoker_ = std::move(tasm_platform_invoker);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetNativeModuleManager(
    std::unique_ptr<lynx::pub::LynxNativeModuleManager> native_module_manager) {
  this->native_module_manager_ = std::move(native_module_manager);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetVSyncMonitorPlatformImpl(
    const std::shared_ptr<base::VSyncMonitorPlatformImpl>& monitor) {
  this->vsync_monitor_platform_impl_ = monitor;
  return *this;
}

}  // namespace shell
}  // namespace lynx
