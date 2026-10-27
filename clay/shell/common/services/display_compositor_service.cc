// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/display_compositor_service.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/common/service/service_manager.h"
#include "clay/fml/logging.h"
namespace clay {
std::shared_ptr<DisplayCompositorService> DisplayCompositorService::Create() {
  return std::make_shared<DisplayCompositorService>();
}
void DisplayCompositorService::OnInit(ServiceManager& manager,
                                      const RasterServiceContext&) {
  runner_ = manager.GetTaskRunners()->SelectTaskRunner<Owner::kRaster>();
  platform_runner_ =
      manager.GetTaskRunners()->SelectTaskRunner<Owner::kPlatform>();
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  clock_ = manager.GetService<DisplayFrameService>();
}
void DisplayCompositorService::OnDestroy() {
  Close();
  clock_ = nullptr;
}
void DisplayCompositorService::SetBinding(
    display::RasterBindingKey binding,
    std::shared_ptr<display::FrameMailbox> mailbox) {
  ++*error_epoch_;
  if (mailbox_) mailbox_->Close();
  mailbox_ = std::move(mailbox);
  retained_frame_.reset();
  builder_.SetBinding(binding);
}
void DisplayCompositorService::SetOutputConfig(display::OutputConfig config) {
  config_ = config;
  if (retained_frame_ && retained_frame_->result->config != config)
    retained_frame_.reset();
  builder_.SetOutputConfig(config);
  if (compositor_) compositor_->InvalidateCandidate();
}
void DisplayCompositorService::NotifyLayoutSubmitted(uint64_t version) {
  builder_.NotifyLayoutSubmitted(version);
}
void DisplayCompositorService::Attach(
    TargetFactory factory, display::WindowKey window,
    display::DisplayCompositor::Presented presented,
    std::function<void()> request_retained, Snapshot snapshot) {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  Detach();
  factory_ = std::move(factory);
  window_ = window;
  presented_ = std::move(presented);
  request_retained_ = std::move(request_retained);
  snapshot_ = std::move(snapshot);
  if (!hybrid_composition_active_) StartTarget();
}
void DisplayCompositorService::StartTarget() {
  if (!factory_) return;
  auto target = factory_();
  if (!target) {
    FML_LOG(ERROR) << "Could not create the Display output.";
    return;
  }
  compositor_ = std::make_shared<display::DisplayCompositor>(
      runner_, std::move(target), window_,
      [clock = clock_] {
        clock.Act([](auto& impl) { impl.RequestDisplay(); });
      },
      presented_, [this](auto error) { ReportError(error); }, snapshot_);
  clock_.Act([weak = std::weak_ptr<display::DisplayCompositor>(compositor_)](
                 auto& impl) {
    impl.SetCompose([weak](auto deadline) {
      auto compositor = weak.lock();
      return compositor && compositor->Compose(deadline);
    });
  });
  compositor_->SetEnabled(enabled_);
  // Restore the Display-owned snapshot first. A newer pending Raster result
  // replaces it in Drain without resetting the producer or rebuilding the UI.
  if (retained_frame_ && retained_frame_->result->config == config_) {
    bool reusable = std::all_of(
        retained_frame_->result->outputs.begin(),
        retained_frame_->result->outputs.end(), [](const auto& entry) {
          return entry.texture_ref &&
                 !entry.texture_ref->resource()->IsRetired();
        });
    if (reusable)
      compositor_->Accept(std::make_shared<const display::CompositorFrame>(
          display::CompositorFrame{retained_frame_->frame_id, window_,
                                   retained_frame_->result}));
  }
  retained_frame_.reset();
  Drain();
  if (!compositor_->latest() && request_retained_) request_retained_();
}
void DisplayCompositorService::Detach() {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  StopTarget();
  factory_ = nullptr;
  presented_ = nullptr;
  request_retained_ = nullptr;
  snapshot_.reset();
  window_ = {};
}
void DisplayCompositorService::StopTarget() {
  if (compositor_) {
    clock_.Act([](auto& impl) { impl.SetCompose(nullptr); });
    retained_frame_ = compositor_->latest();
    compositor_->Stop();
    compositor_.reset();
  }
}
void DisplayCompositorService::Close() {
  ++*error_epoch_;
  Detach();
  retained_frame_.reset();
  if (mailbox_) mailbox_->Close();
  mailbox_.reset();
  builder_.Reset();
  hybrid_composition_active_ = false;
}
void DisplayCompositorService::SetEnabled(bool enabled) {
  enabled_ = enabled;
  clock_.Act([enabled](auto& impl) { impl.SetActive(enabled); });
  if (compositor_) compositor_->SetEnabled(enabled);
}
void DisplayCompositorService::SetHybridCompositionActive(bool active) {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (hybrid_composition_active_ == active) return;
  hybrid_composition_active_ = active;
  if (active) {
    ++*error_epoch_;
    // Flush staged work before the legacy path draws to the same output.
    StopTarget();
    retained_frame_.reset();
    if (mailbox_) {
      mailbox_->TakeLatest();
      mailbox_->TakeFailure();
    }
  } else {
    StartTarget();
  }
}
void DisplayCompositorService::ReportError(display::PipelineError error) {
  if (!error_) return;
  // A staged failure queued before switching paths must not recover the
  // replacement Hybrid output on Platform.
  platform_runner_->PostTask([handler = error_, epoch = error_epoch_,
                              expected = error_epoch_->load(), error] {
    if (epoch->load() == expected) handler(error);
  });
}
void DisplayCompositorService::Drain() {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (!mailbox_) return;
  auto error = mailbox_->TakeFailure();
  if (hybrid_composition_active_) {
    mailbox_->TakeLatest();
    return;
  }
  if (error != display::PipelineError::kNone) {
    ReportError(error);
    return;
  }
  // A detached consumer has not taken ownership. Keep one replaceable latest
  // publication in the mailbox until Attach can consume it.
  if (!compositor_) return;
  auto frame = builder_.Build(mailbox_->TakeLatest(), window_);
  if (!frame) return;
  compositor_->Accept(std::move(frame));
}
}  // namespace clay
