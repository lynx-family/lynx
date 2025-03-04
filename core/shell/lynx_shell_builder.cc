// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_shell_builder.h"

#include <memory>
#include <string>
#include <utility>

#include "core/services/event_report/event_tracker_platform_impl.h"

namespace lynx {
namespace shell {

LynxShellBuilder& LynxShellBuilder::SetNativeFacade(
    std::unique_ptr<shell::NativeFacade> native_facade) {
  this->native_facade_ = std::move(native_facade);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetNativeFacadeReporter(
    std::unique_ptr<shell::NativeFacadeReporter> native_facade_async) {
  this->native_facade_reporter_ = std::move(native_facade_async);
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetUseInvokeUIMethodFunction(
    bool use_invoke_ui_method_func) {
  this->use_invoke_ui_method_func_ = use_invoke_ui_method_func;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetLynxEngineCreator(
    const std::function<std::unique_ptr<shell::LynxEngine>(
        std::unique_ptr<TasmMediator>)>& lynx_engine_creator) {
  this->lynx_engine_creator_ = lynx_engine_creator;
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

LynxShellBuilder& LynxShellBuilder::SetEnableDiffWithoutLayout(
    bool enable_diff_without_layout) {
  this->enable_diff_without_layout_ = enable_diff_without_layout;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetLazyBundleLoader(
    const std::shared_ptr<lynx::tasm::LazyBundleLoader>& loader) {
  this->loader_ = loader;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetWhiteBoard(
    const std::shared_ptr<lynx::tasm::WhiteBoard>& white_board) {
  this->white_board_ = white_board;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetEnableElementManagerVsyncMonitor(
    bool enable_element_manager_vsync_monitor) {
  this->element_manager_vsync_monitor_ =
      enable_element_manager_vsync_monitor ? VSyncMonitor::Create() : nullptr;
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

LynxShellBuilder& LynxShellBuilder::SetEngineActor(
    const std::function<void(const std::shared_ptr<LynxActor<LynxEngine>>&)>&
        on_engine_actor_created) {
  this->on_engine_actor_created_ = on_engine_actor_created;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetRuntimeActor(
    const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& runtime_actor) {
  this->runtime_actor_ = runtime_actor;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetTimingActor(
    const std::shared_ptr<LynxActor<tasm::timing::TimingHandler>>&
        timing_actor) {
  this->timing_actor_ = timing_actor;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetTimingCollectorPlatform(
    const std::shared_ptr<tasm::timing::TimingCollectorPlatformImpl>&
        timing_collector_platform) {
  this->timing_collector_platform_ = timing_collector_platform;
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

LynxShell* LynxShellBuilder::build() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxShell::Create");

  // for auto concurrency, force using MULTI_THREADS by default.
  if (this->shell_option_.enable_auto_concurrency_) {
    this->strategy_ = base::ThreadStrategyForRendering::MULTI_THREADS;
  }

  LynxShell* shell = new LynxShell(this->strategy_, this->shell_option_);
  shell->facade_actor_ = std::make_shared<LynxActor<NativeFacade>>(
      std::move(this->native_facade_), shell->runners_.GetUITaskRunner(),
      shell->instance_id_);
  shell->facade_reporter_actor_ =
      std::make_shared<LynxActor<NativeFacadeReporter>>(
          std::move(this->native_facade_reporter_),
          lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner(),
          shell->instance_id_);

  if (timing_actor_) {
    shell->timing_mediator_ = nullptr;
    shell->timing_actor_ = timing_actor_;
  } else {
    // create timing mediator & actor
    auto timing_mediator = std::make_unique<lynx::tasm::timing::TimingMediator>(
        shell->instance_id_);
    timing_mediator->SetFacadeActor(shell->facade_actor_);
    timing_mediator->SetFacadeReporterActor(shell->facade_reporter_actor_);
    timing_mediator->SetEnableJSRuntime(this->shell_option_.enable_js_);
    shell->timing_mediator_ = timing_mediator.get();

    shell->timing_actor_ =
        std::make_shared<LynxActor<tasm::timing::TimingHandler>>(
            std::make_unique<tasm::timing::TimingHandler>(
                std::move(timing_mediator)),
            tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner(),
            shell->instance_id_);
    shell->timing_actor_->Impl()->SetEnableJSRuntime(
        this->shell_option_.enable_js_);
    shell->timing_actor_->Impl()->SetThreadStrategy(this->strategy_);
  }

  // create layout actor
  auto layout_mediator = std::make_unique<lynx::shell::LayoutMediator>(
      shell->tasm_operation_queue_);
  shell->layout_mediator_ = layout_mediator.get();
  if (layout_context_) {
    layout_context_->SetLynxShell(shell);
  }
  shell->layout_actor_ = std::make_shared<LynxActor<tasm::LayoutContext>>(
      std::make_unique<lynx::tasm::LayoutContext>(
          std::move(layout_mediator), std::move(this->layout_context_),
          this->lynx_env_config_, shell->instance_id_),
      shell->runners_.GetLayoutTaskRunner(), shell->instance_id_);

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    "LynxShell::Create::CreateEngineActor");
  // create engine actor
  auto vsync_monitor = VSyncMonitor::Create();
  auto tasm_mediator = std::make_unique<TasmMediator>(
      shell->facade_actor_, shell->card_cached_data_mgr_, vsync_monitor,
      shell->layout_actor_, std::move(tasm_platform_invoker_),
      shell->timing_actor_);
  shell->tasm_mediator_ = tasm_mediator.get();
  shell->engine_actor_ = std::make_shared<LynxActor<LynxEngine>>(
      CreateLynxEngine(std::move(tasm_mediator), shell->runners_,
                       shell->card_cached_data_mgr_, shell->instance_id_,
                       shell),
      shell->runners_.GetTASMTaskRunner(), shell->instance_id_);
  this->on_engine_actor_created_(shell->engine_actor_);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  shell->tasm_mediator_->SetEngineActor(shell->engine_actor_);
  if (shell->timing_mediator_) {
    shell->timing_mediator_->SetEngineActor(shell->engine_actor_);
  }
  // after set shell members
  shell->engine_actor_->Impl()->SetOperationQueue(shell->tasm_operation_queue_);
  shell->layout_actor_->Impl()->SetRequestLayoutCallback(
      [layout_actor = shell->layout_actor_]() {
        layout_actor->Act([](auto& layout) { layout->Layout(); });
      });
  shell->prop_bundle_creator_ = prop_bundle_creator_;
  auto tasm = shell->engine_actor_->Impl()->GetTasm();
  // @note(lipin): avoid crash when lynx_shell_unittest
  if (tasm != nullptr) {
    shell->ui_operation_queue_->SetErrorCallback(
        [facade_actor = shell->facade_actor_](base::LynxError error) {
          facade_actor->Act([error = std::move(error)](auto& facade) mutable {
            facade->ReportError(error);
          });
        });
    auto& element_manager = tasm->page_proxy()->element_manager();
    element_manager->SetEnableNewAnimatorRadon(enable_new_animator_);
    element_manager->SetEnableNativeListFromShell(enable_native_list_);
    element_manager->SetPropBundleCreator(prop_bundle_creator_);
    element_manager->SetThreadStrategy(this->strategy_);
    if (element_manager->vsync_monitor()) {
      element_manager->vsync_monitor()->BindTaskRunner(
          shell->runners_.GetTASMTaskRunner());
    }
    element_manager->painting_context()->SetUIOperationQueue(
        shell->ui_operation_queue_);
    element_manager->painting_context()->impl()->SetInstanceId(
        shell->instance_id_);
    if (!timing_collector_platform_) {
      timing_collector_platform_ =
          std::make_shared<tasm::timing::TimingCollectorPlatformImpl>();
    }
    timing_collector_platform_->SetTimingActor(shell->timing_actor_);
    element_manager->painting_context()->SetTimingCollectorPlatform(
        std::move(timing_collector_platform_));
    shell->layout_mediator_->Init(
        shell->engine_actor_, shell->facade_actor_, shell->timing_actor_,
        element_manager->node_manager(), element_manager->air_node_manager(),
        element_manager->catalyzer());
    // @note(tangyongjie): avoid crash when lynx_shell_builder_unittest
    if (vsync_monitor) {
      vsync_monitor->BindTaskRunner(shell->runners_.GetTASMTaskRunner());
      shell->engine_actor_->Act([](auto& engine) { engine->Init(); });
    }

    auto painting_context = element_manager->painting_context();
    if (use_invoke_ui_method_func_) {
      shell::InvokeUIMethodFunction invoke_ui_method_func =
          [painting_context](lynx::tasm::LynxGetUIResult ui_result,
                             const std::string& method,
                             std::unique_ptr<lynx::tasm::PropBundle> params,
                             lynx::piper::ApiCallBack callback) {
            painting_context->InvokeUIMethod(ui_result.UiImplIds()[0], method,
                                             std::move(params), callback.id());
          };
      shell->tasm_mediator_->SetInvokeUIMethodFunction(
          std::move(invoke_ui_method_func));
    }
  }

  shell->runtime_actor_ = runtime_actor_;
  return shell;
}

std::unique_ptr<lynx::shell::LynxEngine> LynxShellBuilder::CreateLynxEngine(
    std::unique_ptr<TasmMediator> tasm_mediator,
    base::TaskRunnerManufactor& runners,
    const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr,
    int32_t instance_id, LynxShell* shell) {
  // lynx_engine_creator_ is nullptr by default, it is used only for
  // lynx_shell_unitests.
  if (this->lynx_engine_creator_ != nullptr) {
    return this->lynx_engine_creator_(std::move(tasm_mediator));
  }

  if (painting_context_ == nullptr) {
    painting_context_ = painting_context_creator_(shell);
  }
  auto element_manager = std::make_unique<lynx::tasm::ElementManager>(
      std::move(painting_context_), tasm_mediator.get(), this->lynx_env_config_,
      instance_id, this->element_manager_vsync_monitor_,
      this->enable_diff_without_layout_);
  auto tasm = std::make_shared<lynx::tasm::TemplateAssembler>(
      *tasm_mediator, std::move(element_manager), instance_id);
  tasm->SetEnableLayoutOnly(this->enable_layout_only_);
  tasm->Init(runners.GetTASMTaskRunner());
  if (this->loader_ != nullptr) {
    tasm->SetLazyBundleLoader(this->loader_);
  }

  if (this->white_board_ != nullptr) {
    tasm->SetWhiteBoard(this->white_board_);
  }

  if (!this->locale_.empty()) {
    tasm->SetLocale(this->locale_);
  }
  tasm->EnablePreUpdateData(this->enable_pre_update_data_);

  auto lynx_engine = std::make_unique<lynx::shell::LynxEngine>(
      tasm, std::move(tasm_mediator), card_cached_data_mgr, instance_id);
  return lynx_engine;
}

LynxShellBuilder& LynxShellBuilder::SetTasmPlatformInvoker(
    std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker) {
  this->tasm_platform_invoker_ = std::move(tasm_platform_invoker);
  return *this;
}

}  // namespace shell
}  // namespace lynx
