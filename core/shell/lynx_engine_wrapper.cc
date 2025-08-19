// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_engine_wrapper.h"

#include "core/shell/lynx_shell.h"
namespace lynx {
namespace shell {

void LynxEngineWrapper::SetupCore(
    const std::shared_ptr<lynx::shell::LynxActor<lynx::shell::LynxEngine>>
        &engine_actor,
    const std::shared_ptr<lynx::shell::LynxActor<lynx::tasm::LayoutContext>>
        &layout_actor,
    lynx::shell::TasmMediator *tasm_mediator,
    lynx::shell::LayoutMediator *layout_mediator) {
  this->engine_actor_ = engine_actor;
  this->layout_actor_ = layout_actor;
  this->tasm_mediator_ = tasm_mediator;
  this->layout_mediator_ = layout_mediator;
  has_init_ = true;
}

void LynxEngineWrapper::BindShell(lynx::shell::LynxShell *shell) {
  shell->engine_actor_ = this->engine_actor_->TransferToNewActor(
      shell->runners_.GetTASMTaskRunner());
  shell->layout_actor_ = this->layout_actor_->TransferToNewActor(
      shell->runners_.GetLayoutTaskRunner());
  shell->tasm_mediator_ = this->tasm_mediator_;
  shell->layout_mediator_ = this->layout_mediator_;
  shell->tasm_mediator_->ResetMediatorActor(shell->layout_actor_,
                                            shell->facade_actor_,
                                            shell->perf_controller_actor_);
  auto tasm = shell->engine_actor_->Impl()->GetTasm();
  if (tasm) {
    tasm->page_proxy()
        ->element_manager()
        ->painting_context()
        ->SetUIOperationQueue(shell->ui_operation_queue_);
  }
  this->engine_actor_ = shell->engine_actor_;
  this->layout_actor_ = shell->layout_actor_;
}

void LynxEngineWrapper::DetachEngine() {}

void LynxEngineWrapper::DestroyEngine() {}

}  // namespace shell
}  // namespace lynx
