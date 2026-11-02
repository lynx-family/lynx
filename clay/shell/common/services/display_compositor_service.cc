// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/display_compositor_service.h"

#include <memory>
#include <utility>

#include "clay/common/service/service_manager.h"
namespace clay {
std::shared_ptr<DisplayCompositorService> DisplayCompositorService::Create() {
  return std::make_shared<DisplayCompositorService>();
}
void DisplayCompositorService::OnInit(ServiceManager& manager,
                                      const PlatformServiceContext&) {
  runner_ = manager.GetTaskRunners()->SelectTaskRunner<Owner::kPlatform>();
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  clock_ = manager.GetService<DisplayFrameService>();
}
void DisplayCompositorService::OnDestroy() {
  if (compositor_) {
    compositor_->Stop();
    compositor_.reset();
  }
  if (mailbox_) mailbox_->Close();
  mailbox_.reset();
  builder_.Reset();
}
void DisplayCompositorService::SetBinding(
    display::RasterBindingKey binding,
    std::shared_ptr<display::FrameMailbox> mailbox) {
  if (mailbox_) mailbox_->Close();
  mailbox_ = std::move(mailbox);
  builder_.SetBinding(binding);
}
void DisplayCompositorService::SetOutputConfig(display::OutputConfig config) {
  builder_.SetOutputConfig(config);
  if (compositor_) compositor_->InvalidateCandidate();
}
void DisplayCompositorService::NotifyLayoutSubmitted(uint64_t version) {
  builder_.NotifyLayoutSubmitted(version);
}
void DisplayCompositorService::Attach(
    std::unique_ptr<display::DisplayTarget> target, display::WindowKey window,
    display::DisplayCompositor::Presented presented) {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  Detach();
  window_ = window;
  compositor_ = std::make_shared<display::DisplayCompositor>(
      runner_, std::move(target), window,
      [weak = GetWeakPtr()] {
        if (weak) weak->clock_->RequestDisplay();
      },
      std::move(presented), error_);
  clock_->SetCompose([weak = std::weak_ptr<display::DisplayCompositor>(
                          compositor_)](auto deadline) {
    auto compositor = weak.lock();
    return compositor && compositor->Compose(deadline);
  });
  compositor_->SetEnabled(enabled_);
  // Publications produced while detached remain pending in the mailbox. A
  // compatible reattach requests fresh Raster output rather than retaining
  // consumed inputs solely for replay to another window generation.
  Drain();
}
void DisplayCompositorService::Detach() {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  clock_->SetCompose(nullptr);
  if (compositor_) {
    compositor_->Stop();
    compositor_.reset();
  }
  window_ = {};
}
void DisplayCompositorService::Close() {
  Detach();
  if (mailbox_) mailbox_->Close();
  mailbox_.reset();
  builder_.Reset();
}
void DisplayCompositorService::SetEnabled(bool enabled) {
  enabled_ = enabled;
  clock_->SetActive(enabled);
  if (compositor_) compositor_->SetEnabled(enabled);
}
std::function<void()> DisplayCompositorService::GetWakeCallback() {
  return [runner = runner_, weak = GetWeakPtr()] {
    runner->PostTask([weak] {
      if (weak) weak->Drain();
    });
  };
}
void DisplayCompositorService::Drain() {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (!mailbox_) return;
  auto error = mailbox_->TakeFailure();
  if (error != display::PipelineError::kNone) {
    if (error_) error_(error);
    return;
  }
  // A detached consumer has not taken ownership. Keep one replaceable latest
  // publication in the mailbox until Attach can consume it.
  if (!compositor_) return;
  auto publication = mailbox_->TakeLatest();
  if (!publication || !builder_.SetScene(std::move(publication.scene)) ||
      !builder_.Accept(std::move(publication.result)))
    return;
  auto frame = builder_.Build(window_);
  if (!frame) return;
  compositor_->Accept(std::move(frame));
}
std::shared_ptr<const display::CompositorFrame>
DisplayCompositorService::CurrentFrame() const {
  return compositor_ ? compositor_->current() : nullptr;
}
}  // namespace clay
