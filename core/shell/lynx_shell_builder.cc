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
  this->element_manager_vsync_monitor_ = enable_element_manager_vsync_monitor
                                             ? base::VSyncMonitor::Create()
                                             : nullptr;
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

LynxShellBuilder& LynxShellBuilder::SetLynxEngineWrapper(
    shell::LynxEngineWrapper* engine_wrapper) {
  lynx_engine_wrapper_ = engine_wrapper;
  return *this;
}

LynxShellBuilder& LynxShellBuilder::SetRuntimeActor(
    const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& runtime_actor) {
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_SHELL_BUILDER_BUILD);

  // for auto concurrency, force using MULTI_THREADS by default.
  if (this->shell_option_.enable_auto_concurrency_) {
    this->strategy_ = base::ThreadStrategyForRendering::MULTI_THREADS;
  }

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
    shell->perf_controller_actor_->Impl()->SetPlatformImpl(
        std::move(this->performance_controller_platform_));
  }
  if (loader_ != nullptr) {
    loader_->SetPerfControllerActor(shell->perf_controller_actor_);
  }

  if (lynx_engine_wrapper_ && lynx_engine_wrapper_->HasInit()) {
    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, LYNX_SHELL_BUILDER_ATTACH_ENGINE);
    // Indicates that a reusable LynxEngine object has been obtained.
    AttachLynxEngine(shell);
    LOGI("get Engine by pool");
  } else {
    // create layout actor
    std::unique_ptr<LayoutMediator> layout_mediator;

    if (force_layout_on_background_thread_) {
      shell->layout_result_manager_ = std::make_shared<LayoutResultManager>();

      layout_mediator = std::make_unique<lynx::shell::LayoutMediator>(
          shell->layout_result_manager_);
    } else {
      layout_mediator = std::make_unique<lynx::shell::LayoutMediator>(
          shell->tasm_operation_queue_);
    }

    layout_mediator->SetPageOptions(shell_option_.page_options_);
    shell->layout_mediator_ = layout_mediator.get();
    if (layout_context_) {
      layout_context_->SetLynxShell(shell);
    }
    shell->layout_actor_ = std::make_shared<LynxActor<tasm::LayoutContext>>(
        std::make_unique<lynx::tasm::LayoutContext>(
            std::move(layout_mediator),
            (!shell_option_.page_options_.IsLayoutInElementModeOn()
                 ? std::move(this->layout_context_)
                 : nullptr),
            this->lynx_env_config_, shell_option_.page_options_),
        shell->runners_.GetLayoutTaskRunner(), shell->instance_id_);

    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                      LYNX_SHELL_BUILDER_CREATE_ENGINE_ACTOR);
    // create engine actor
    auto tasm_mediator = std::make_unique<TasmMediator>(
        shell->facade_actor_, shell->card_cached_data_mgr_,
        shell->layout_actor_, std::move(tasm_platform_invoker_),
        shell->perf_controller_actor_);
    tasm_mediator->SetPageOptions(shell_option_.page_options_);
    shell->tasm_mediator_ = tasm_mediator.get();
    shell->engine_actor_ = std::make_shared<LynxActor<LynxEngine>>(
        CreateLynxEngine(std::move(tasm_mediator), shell->runners_,
                         shell->card_cached_data_mgr_, shell->instance_id_,
                         shell,
                         (shell_option_.page_options_.IsLayoutInElementModeOn()
                              ? std::move(this->layout_context_)
                              : nullptr)),
        shell->runners_.GetTASMTaskRunner(), shell->instance_id_);
  }

  this->on_engine_actor_created_(shell->engine_actor_);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  shell->tasm_mediator_->SetEngineActor(shell->engine_actor_);
  if (shell->perf_mediator_) {
    shell->perf_mediator_->SetEngineActor(shell->engine_actor_);
  }
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
    element_manager->painting_context()->SetPerfActor(
        shell->perf_controller_actor_);
    shell->layout_mediator_->Init(
        shell->engine_actor_, shell->facade_actor_,
        shell->perf_controller_actor_, element_manager->node_manager(),
        element_manager->air_node_manager(), element_manager->catalyzer());
    // @note(tangyongjie): avoid crash when lynx_shell_builder_unittest
    shell->engine_actor_->ActLite([](auto& engine) { engine->Init(); });

    auto painting_context = element_manager->painting_context();
    if (use_invoke_ui_method_func_) {
      shell::InvokeUIMethodFunction invoke_ui_method_func =
          [painting_context](lynx::tasm::LynxGetUIResult ui_result,
                             const std::string& method,
                             fml::RefPtr<lynx::tasm::PropBundle> params,
                             lynx::piper::ApiCallBack callback) {
            painting_context->InvokeUIMethod(ui_result.UiImplIds()[0], method,
                                             std::move(params), callback.id());
          };
      shell->tasm_mediator_->SetInvokeUIMethodFunction(
          std::move(invoke_ui_method_func));
    }
  }

  shell->runtime_actor_ = runtime_actor_;
  shell->SetPageOptions(shell_option_.page_options_);
  if (lynx_engine_wrapper_) {
    // After creating the EngineWrapper for the first time or reusing it, the
    // internal objects need to be updated.
    lynx_engine_wrapper_->SetupCore(shell->engine_actor_, shell->layout_actor_,
                                    shell->tasm_mediator_,
                                    shell->layout_mediator_);
  }
  return shell;
}

void LynxShellBuilder::AttachLynxEngine(LynxShell* shell) {
  if (lynx_engine_wrapper_ && lynx_engine_wrapper_->HasInit()) {
    lynx_engine_wrapper_->BindShell(shell);
  }
}

std::unique_ptr<lynx::shell::LynxEngine> LynxShellBuilder::CreateLynxEngine(
    std::unique_ptr<TasmMediator> tasm_mediator,
    base::TaskRunnerManufactor& runners,
    const std::shared_ptr<LynxCardCacheDataManager>& card_cached_data_mgr,
    int32_t instance_id, LynxShell* shell,
    std::unique_ptr<lynx::tasm::LayoutCtxPlatformImpl>
        platform_layout_context) {
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
      std::move(platform_layout_context));
  // Currently, tasm_mediator serves as the implementation of both
  // TemplateAssembler::Delegate and TemplateAssembler::LayoutScheduler,
  // so here passes *tasm_mediator twice.
  // TODO(chennengshi) : We may refactor LayoutScheduler's implementation as a
  // new instance rather than tasm_mediator when LayoutScheduler is more
  // complex.
  auto tasm = std::make_unique<lynx::tasm::TemplateAssembler>(
      *tasm_mediator, std::move(element_manager), *tasm_mediator, instance_id,
      this->enable_unified_pipeline_);
  tasm->SetEnableLayoutOnly(this->enable_layout_only_);
  if (this->loader_ != nullptr) {
    tasm->SetLazyBundleLoader(this->loader_);
  }

  if (this->white_board_ == nullptr) {
    this->white_board_ = std::make_shared<tasm::WhiteBoard>();
  }
  tasm->SetWhiteBoard(this->white_board_);

  if (!this->locale_.empty()) {
    tasm->SetLocale(this->locale_);
  }
  tasm->EnablePreUpdateData(this->enable_pre_update_data_);

  auto lynx_engine = std::make_unique<lynx::shell::LynxEngine>(
      std::move(tasm), std::move(tasm_mediator), card_cached_data_mgr,
      instance_id);
  return lynx_engine;
}

LynxShellBuilder& LynxShellBuilder::SetTasmPlatformInvoker(
    std::unique_ptr<TasmPlatformInvoker> tasm_platform_invoker) {
  this->tasm_platform_invoker_ = std::move(tasm_platform_invoker);
  return *this;
}

}  // namespace shell
}  // namespace lynx
