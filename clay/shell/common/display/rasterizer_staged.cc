// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/service/service_manager.h"
#include "clay/shell/common/rasterizer.h"
#include "clay/shell/common/services/instrumentation_service.h"
#include "clay/shell/common/services/raster_frame_service.h"
#include "clay/ui/common/frame_timing_collector.h"
namespace clay {
bool Rasterizer::SetupStagedOutput(
    std::unique_ptr<display::RasterOutputSurface> surface,
    display::RasterBindingKey binding,
    std::shared_ptr<display::FrameMailbox> mailbox,
    std::function<void()> wake) {
  raster_frame_service_->SetProductionBinding(binding);
  ++staged_epoch_;
  if (suspended_staged_surface_ && surface &&
      suspended_staged_surface_->CanReuseContext(*surface) &&
      suspended_staged_surface_->IsValid()) {
    suspended_staged_surface_->SetConfig(surface->config());
    surface = std::move(suspended_staged_surface_);
  }
  suspended_staged_surface_.reset();
  staged_ = std::make_unique<display::StagedRasterizer>(
      std::move(surface), *this, compositor_context_->drawable_image_registry(),
      binding, std::move(mailbox), std::move(wake));
  return staged_->IsValid();
}
display::ProductionOutcome Rasterizer::RasterizeToOutputs(
    std::shared_ptr<LayerTree> tree,
    std::unique_ptr<FrameTimingsRecorder> recorder) {
  const int64_t frame_time_ms =
      (recorder ? recorder->GetVsyncTargetTime() : fml::TimePoint::Now())
          .ToEpochDelta()
          .ToMilliseconds();
  auto outcome = display::ProductionOutcome::kDeferred;
  if (recorder) {
    recorder->RecordRasterStart(fml::TimePoint::Now());
    recorder->RecordFrameTime(FrameTimingKey::kRasterStart);
    instrumentation_service_.Act([](auto& impl) {
      if (impl.GetFrameTimingCollector()->IsRecordingFirstFramePerf())
        impl.GetFrameTimingCollector()->BeginRecord(Perf::kFirstRasterCost);
    });
  }
  platform_const_service_->GetIsGPUDisabledSyncSwitch()->Execute(
      fml::SyncSwitch::Handlers().SetIfFalse([&] {
        outcome = staged_ ? staged_->RasterizeToOutputs(tree, frame_time_ms)
                          : display::ProductionOutcome::kFailed;
      }));
  const bool success = outcome == display::ProductionOutcome::kPublished;
  if (recorder) {
    recorder->RecordFrameTime(FrameTimingKey::kRasterEnd);
    recorder->RecordRasterEnd();
    if (tree) tree->AppendFrameTimings(recorder->TakeFrameTimings(), false);
    instrumentation_service_.Act(
        [timing = recorder->GetRecordedTime(), success](auto& impl) {
          impl.OnFrameRasterized(timing);
          if (impl.GetFrameTimingCollector()->IsRecordingFirstFramePerf()) {
            impl.GetFrameTimingCollector()->EndRecord(Perf::kFirstRasterCost);
            if (success)
              impl.GetFrameTimingCollector()->InsertRecord(
                  Perf::kFirstRasterEnd,
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count());
          }
        });
  }
  if (success && staged_->NeedsAnimationFrame())
    raster_frame_service_->RequestRasterFrame();
  return outcome;
}
void Rasterizer::RequestRetainedFrame() {
  if (staged_ && staged_->RequestRetainedFrame())
    raster_frame_service_->RequestContentFrame(staged_->binding());
}
bool Rasterizer::MarkDynamicSegmentsDirty(int64_t id) {
  return staged_ && staged_->MarkDynamicSegmentsDirty(id);
}
display::ProductionOutcome Rasterizer::ProducePendingContent() {
  auto outcome = display::ProductionOutcome::kDeferred;
  platform_const_service_->GetIsGPUDisabledSyncSwitch()->Execute(
      fml::SyncSwitch::Handlers().SetIfFalse([&] {
        outcome = staged_ ? staged_->ProducePendingContent()
                          : display::ProductionOutcome::kFailed;
      }));
  if (outcome == display::ProductionOutcome::kPublished &&
      staged_->NeedsAnimationFrame())
    raster_frame_service_->RequestRasterFrame();
  return outcome;
}
void Rasterizer::UpdateStagedOutputConfig(display::OutputConfig config) {
  if (staged_) staged_->SetOutputConfig(config);
}
void Rasterizer::ReleaseStagedResources(std::function<void()> drained,
                                        bool retain_production_context) {
  ++staged_epoch_;
  raster_frame_service_->SetProductionAvailable(false);
  if (!retain_production_context) suspended_staged_surface_.reset();
  if (staged_) {
    if (retain_production_context)
      suspended_staged_surface_ = staged_->Suspend(std::move(drained));
    else
      staged_->Release(std::move(drained));
    staged_.reset();
  } else if (drained)
    drained();
}
void Rasterizer::ScreenshotCompositorFrameAsync(
    std::shared_ptr<const display::CompositorFrame> frame, uint32_t background,
    std::function<void(ScreenshotData)> callback, unsigned attempt) {
  if (!staged_ || !frame || frame->result->binding != staged_->binding()) {
    callback({});
    return;
  }
  bool pending = false;
  for (const auto& entry : frame->result->outputs) {
    auto status = entry.texture_ref->ready()->Poll();
    if (status == display::CompletionStatus::kFailed) {
      callback({});
      return;
    }
    pending |= status == display::CompletionStatus::kPending;
  }
  if (pending && attempt < 250) {
    service_manager_->GetTaskRunners()
        ->SelectTaskRunner<Owner::kRaster>()
        ->PostDelayedTask(
            [weak = GetWeakPtr(), epoch = staged_epoch_,
             frame = std::move(frame), background,
             callback = std::move(callback), attempt]() mutable {
              if (weak && weak->staged_epoch_ == epoch)
                weak->ScreenshotCompositorFrameAsync(
                    std::move(frame), background, std::move(callback),
                    attempt + 1);
              else
                callback({});
            },
            fml::TimeDelta::FromMilliseconds(8));
    return;
  }
  ScreenshotData result;
  if (!pending) {
    platform_const_service_->GetIsGPUDisabledSyncSwitch()->Execute(
        fml::SyncSwitch::Handlers().SetIfFalse([&] {
          if (staged_)
            result = staged_->Screenshot(std::move(frame), background);
        }));
  }
  callback(std::move(result));
}
void Rasterizer::MakeStagedRasterSnapshot(
    std::shared_ptr<LayerTree> tree,
    std::function<void(fml::RefPtr<PaintImage>)> callback, unsigned attempt) {
  std::pair<fml::RefPtr<PaintImage>, bool> result;
  platform_const_service_->GetIsGPUDisabledSyncSwitch()->Execute(
      fml::SyncSwitch::Handlers().SetIfFalse([&] {
        if (staged_ && tree) result = staged_->SnapshotTree(*tree);
      }));
  if (result.second && attempt < 250) {
    service_manager_->GetTaskRunners()
        ->SelectTaskRunner<Owner::kRaster>()
        ->PostDelayedTask(
            [weak = GetWeakPtr(), epoch = staged_epoch_, tree = std::move(tree),
             callback = std::move(callback), attempt]() mutable {
              if (weak && weak->staged_epoch_ == epoch)
                weak->MakeStagedRasterSnapshot(
                    std::move(tree), std::move(callback), attempt + 1);
              else
                callback(nullptr);
            },
            fml::TimeDelta::FromMilliseconds(8));
  } else
    callback(std::move(result.first));
}
void Rasterizer::InvalidateStagedUploads() {
  if (staged_) staged_->InvalidateUploads();
}
void Rasterizer::OnDisplayPresented(display::RasterBindingKey binding,
                                    uint64_t layout, int64_t time) {
  if (!staged_ || staged_->binding() != binding) return;
  staged_->OnFrameSubmitted(layout);
  FireNextFrameCallbackIfPresent();
  instrumentation_service_.Act([time](auto& impl) {
    if (impl.GetFrameTimingCollector()->IsRecordingFirstFramePerf())
      impl.GetFrameTimingCollector()->InsertRecord(Perf::kFirstPresentEnd,
                                                   time);
  });
}
}  // namespace clay
