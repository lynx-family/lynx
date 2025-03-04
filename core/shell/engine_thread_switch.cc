// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/engine_thread_switch.h"

namespace lynx {
namespace shell {

EngineThreadSwitch::EngineThreadSwitch(
    const fml::RefPtr<fml::TaskRunner>& ui_runner,
    const fml::RefPtr<fml::TaskRunner>& engine_runner,
    const std::shared_ptr<shell::DynamicUIOperationQueue>& queue)
    : ui_runner_(ui_runner),
      engine_runner_(engine_runner),
      ui_loop_(ui_runner_->GetLoop()),
      engine_loop_(engine_runner_->GetLoop()),
      queue_(queue) {}

void EngineThreadSwitch::AttachEngineToUIThread() {
  DCHECK(ui_runner_->RunsTasksOnCurrentThread());
  if (engine_runner_->GetLoop() == ui_loop_) {
    LOGE(
        "EngineThreadSwitch::AttachEngineToUIThread failed because engine "
        "runner is running on ui thread now");
    return;
  }
  {
    std::unique_lock<std::mutex> lock{mutex_};
    detaching_process_cv_.wait(lock,
                               [this] { return !is_in_detaching_process_; });
  }
  engine_runner_->Bind(ui_loop_);
  queue_->Transfer(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
  LOGI("EngineThreadSwitch::AttachEngineToUIThread succeeded");
}

void EngineThreadSwitch::DetachEngineFromUIThread() {
  DCHECK(ui_runner_->RunsTasksOnCurrentThread());
  if (engine_runner_->GetLoop() == engine_loop_) {
    LOGE(
        "EngineThreadSwitch::DetachEngineFromUIThread failed because engine "
        "runner is running on a background engine thread now");
    return;
  }
  {
    std::lock_guard lock(mutex_);
    is_in_detaching_process_ = true;
  }
  engine_runner_->UnBind();
  queue_->Transfer(base::ThreadStrategyForRendering::MULTI_THREADS);
  engine_loop_->PostTask(
      [self = shared_from_this()]() {
        self->engine_runner_->Bind(self->engine_loop_);
        LOGI("EngineThreadSwitch::DetachEngineFromUIThread succeeded");
        {
          std::lock_guard lock(self->mutex_);
          self->is_in_detaching_process_ = false;
        }
        self->detaching_process_cv_.notify_one();
      },
      fml::TimePoint::Now(), fml::TaskSourceGrade::kEmergency);
}

bool EngineThreadSwitch::HasSetEngineLoop() { return engine_loop_ != ui_loop_; }

void EngineThreadSwitch::SetEngineLoop(
    const fml::RefPtr<fml::MessageLoopImpl>& engine_loop) {
  engine_loop_ = std::move(engine_loop);
}

}  // namespace shell
}  // namespace lynx
