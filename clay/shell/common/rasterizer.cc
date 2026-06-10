// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/rasterizer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/make_copyable.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/common/service/service_manager.h"
#include "clay/flow/compositor/compositor_state.h"
#include "clay/flow/compositor_context.h"
#include "clay/flow/frame_timings.h"
#include "clay/fml/base64.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/shell/common/serialization_callbacks.h"
#include "clay/ui/common/frame_timing_collector.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/resource/gpu_resource_cache.h"

namespace clay {

// The rasterizer will tell Skia to purge cached resources that have not been
// used within this interval.
#ifndef ENABLE_SKITY
static constexpr std::chrono::milliseconds kSkiaCleanupExpiration(15000);
#endif  // ENABLE_SKITY

Rasterizer::Rasterizer(std::shared_ptr<clay::ServiceManager> service_manager)
    : service_manager_(service_manager),
      platform_const_service_(
          service_manager->GetService<PlatformConstService>()),
      raster_frame_service_(service_manager->GetService<RasterFrameService>()),
      instrumentation_service_(
          service_manager->GetService<InstrumentationService>()),
      compositor_service_(service_manager->GetService<CompositorService>()),
      compositor_context_(std::make_unique<clay::CompositorContext>(*this)),
      user_override_resource_cache_bytes_(false),
      raster_time_(std::make_shared<FixedRefreshRateStopwatch>()),
      frame_total_time_(std::make_shared<FixedRefreshRateStopwatch>()),
      weak_factory_(this) {
  FML_DCHECK(compositor_context_);
}

Rasterizer::~Rasterizer() {
  // Some gpu objects (like GrDirectContext lives with PageView) may be unrefed
  // after last `Drain()` in `Rasterizer::Teardown()`. So drain again.
  if (unref_queue_) {
    unref_queue_->Drain();
    // Initiate StartAutoPendingDrain after the Rasterizer destructor.
    // This is necessary because some objects containing GPUObject types are
    // reset after the Rasterizer is destructed. For instance, the LayerTree
    // member of the RasterService is reset post-destruction. Additionally, this
    // step accounts for Images that are referenced across multiple pages,
    // ensuring that all associated resources are properly managed and released.
    unref_queue_->StartAutoPendingDrain();
  }
}

fml::WeakPtr<Rasterizer> Rasterizer::GetWeakPtr() const {
  return weak_factory_.GetWeakPtr();
}

void Rasterizer::Setup(std::unique_ptr<Surface> surface) {
  surface_ = std::move(surface);
  last_ignore_raster_cache_ = false;

  if (max_cache_bytes_.has_value()) {
    SetResourceCacheMaxBytes(max_cache_bytes_.value(),
                             user_override_resource_cache_bytes_);
  }

  auto context_switch = surface_->MakeRenderContextCurrent();
  if (context_switch->GetResult()) {
    compositor_context_->OnGrContextCreated();
  }
}

void Rasterizer::Teardown() {
  if (surface_) {
    auto context_switch = surface_->MakeRenderContextCurrent();
    if (context_switch->GetResult()) {
      compositor_context_->OnGrContextDestroyed();
      if ([[maybe_unused]] auto* context = surface_->GetContext()) {
#ifndef ENABLE_SKITY
        context->purgeUnlockedResources(/*scratchResourcesOnly=*/false);
#endif  // ENABLE_SKITY
        if (unref_queue_) {
          unref_queue_->Drain();
        }
      }
    }
    surface_.reset();
  }

  last_layer_tree_.reset();
  last_ignore_raster_cache_ = false;
}

void Rasterizer::CleanForRecycle() {
  user_override_resource_cache_bytes_ = false;
  last_layer_tree_.reset();
  last_ignore_raster_cache_ = false;
  compositor_context_ = std::make_unique<clay::CompositorContext>(*this);
  if (unref_queue_) {
    unref_queue_->Drain();
    unref_queue_->StartAutoPendingDrain();
  }
}

void Rasterizer::NotifyLowMemoryWarning() const {
  if (!surface_) {
    FML_DLOG(INFO)
        << "Rasterizer::NotifyLowMemoryWarning called with no surface.";
    return;
  }
  auto context = surface_->GetContext();
  if (!context) {
    FML_DLOG(INFO)
        << "Rasterizer::NotifyLowMemoryWarning called with no GrContext.";
    return;
  }
  auto context_switch = surface_->MakeRenderContextCurrent();
  if (!context_switch->GetResult()) {
    return;
  }
#ifndef ENABLE_SKITY
  context->performDeferredCleanup(std::chrono::milliseconds(0));
#endif  // ENABLE_SKITY
  compositor_context_->raster_cache().Clear();
}

std::shared_ptr<DrawableImageRegistry> Rasterizer::GetDrawableImageRegistry() {
  return compositor_context_->drawable_image_registry();
}

clay::LayerTree* Rasterizer::GetLastLayerTree() {
  return last_layer_tree_.get();
}

RasterStatus Rasterizer::DrawLastLayerTree(
    std::unique_ptr<FrameTimingsRecorder> frame_timings_recorder) {
  if (!last_layer_tree_ || !surface_) {
    return RasterStatus::kFailed;
  }
  RasterStatus raster_status = RasterStatus::kFailed;
  if (!last_layer_tree_->DoAnimations()) {
    raster_frame_service_->RequestRasterFrame();
  }
  raster_status = DrawToSurface(*frame_timings_recorder, *last_layer_tree_);

  if (raster_status == RasterStatus::kSuccess) {
    frame_timings_recorder->RecordLastDrawVsyncTime(
        frame_timings_recorder->GetVsyncStartTime());
    frame_timings_recorder->RecordVsyncSequenceId(
        frame_timings_recorder->GetVsyncSequenceId());
  }
  return raster_status;
}

RasterStatus Rasterizer::Draw(std::shared_ptr<LayerTree> layer_tree,
                              std::unique_ptr<FrameTimingsRecorder> recorder,
                              LayerTreeDiscardCallback discard_callback,
                              bool report_instrumentation) {
  TRACE_EVENT("clay", "GPURasterizer::Draw");
  fml::ScopedCleanupClosure instrumentation;
  if (report_instrumentation) {
    instrumentation.SetClosure([this] {
      instrumentation_service_.Act([](auto& impl) {
        impl.GetFrameTimingCollector()
            ->InsertFocusChangedUntilFirstRasterFinish();
        impl.GetFrameTimingCollector()->SetReceivedFocusTime(
            0, clay::FocusManager::Direction::kUnknown);
      });
    });
  }

  FML_DCHECK(service_manager_->GetTaskRunners()
                 ->SelectTaskRunner<clay::Owner::kRaster>()
                 ->RunsTasksOnCurrentThread());

  RasterStatus raster_status = RasterStatus::kFailed;
  if (discard_callback(*layer_tree.get())) {
    raster_status = RasterStatus::kDiscarded;
  } else {
    raster_status = DoDraw(std::move(recorder), std::move(layer_tree));
  }

  return raster_status;
}

fml::RefPtr<PaintImage> Rasterizer::MakeRasterSnapshot(
    std::unique_ptr<LayerTree> layer_tree) {
  return TakeScreenshot(std::move(layer_tree), surface_.get(),
                        compositor_context_.get());
}

fml::RefPtr<PaintImage> Rasterizer::MakeRasterSnapshot(GrPicturePtr picture,
                                                       skity::Vec2 size) {
  return TakeScreenshot(picture, size);
}

fml::Milliseconds Rasterizer::GetFrameBudget() const {
  return platform_const_service_->GetFrameBudget();
}

RasterStatus Rasterizer::DoDraw(
    std::unique_ptr<FrameTimingsRecorder> frame_timings_recorder,
    std::shared_ptr<clay::LayerTree> layer_tree) {
  TRACE_EVENT_WITH_FRAME_NUMBER(frame_timings_recorder, "clay",
                                "Rasterizer::DoDraw");
  FML_DCHECK(service_manager_->GetTaskRunners()
                 ->SelectTaskRunner<clay::Owner::kRaster>()
                 ->RunsTasksOnCurrentThread());
  if (!layer_tree || !surface_) {
    return RasterStatus::kFailed;
  }

  PersistentCache* persistent_cache = PersistentCache::GetCacheForProcess();
  persistent_cache->ResetStoredNewShaders();

  bool need_next_animation_frame = false;
  if (layer_tree->HasAnimations()) {
    if (last_layer_tree_ != layer_tree) {
      layer_tree->MergeAnimations(last_layer_tree_.get());
    }
    need_next_animation_frame = !layer_tree->DoAnimations();
  }

  RasterStatus raster_status =
      DrawToSurface(*frame_timings_recorder, *layer_tree);

  // In order to release the `SkiaGPUObject`s held by the `LayerTree`s in time.
  fml::ScopedCleanupClosure unref_queue_drain([unref_queue = unref_queue_] {
    if (unref_queue) {
      unref_queue->Drain();
    }
  });

  if (raster_status == RasterStatus::kSuccess) {
    if (last_layer_tree_ != layer_tree && !!last_layer_tree_) {
      last_layer_tree_->ResetServiceManagerForAnimation();
    }
    last_layer_tree_ = std::move(layer_tree);
  } else if (raster_status == RasterStatus::kDiscarded ||
             raster_status == RasterStatus::kFailed) {
    return raster_status;
  }
#ifndef ENABLE_SKITY
  if (persistent_cache->IsDumpingSkp() &&
      persistent_cache->StoredNewShaders()) {
    auto screenshot = ScreenshotLastLayerTree(
        ScreenshotData::ScreenshotType::SkiaPicture, false);
    persistent_cache->DumpSkp(*screenshot.data);
  }
#endif  // ENABLE_SKITY

  // TODO(liyuqian): in Fuchsia, the rasterization doesn't finish when
  // Rasterizer::DoDraw finishes. Future work is needed to adapt the timestamp
  // for Fuchsia to capture SceneUpdateContext::ExecutePaintTasks.
  instrumentation_service_.Act(
      [time = frame_timings_recorder->GetRecordedTime()](auto& impl) {
        impl.OnFrameRasterized(time);
      });

  if (need_next_animation_frame) {
    raster_frame_service_->RequestRasterFrame();
  }

// SceneDisplayLag events are disabled on Fuchsia.
// see: https://github.com/flutter/flutter/issues/56598
#if !defined(OS_FUCHSIA)
#if FLUTTER_TIMELINE_ENABLED
  const fml::TimePoint raster_finish_time =
      frame_timings_recorder->GetRasterEndTime();
  fml::TimePoint frame_target_time =
      frame_timings_recorder->GetVsyncTargetTime();
  if (raster_finish_time > frame_target_time) {
    fml::TimePoint latest_frame_target_time =
        delegate_.GetLatestFrameTargetTime();
    const auto frame_budget_millis = delegate_.GetFrameBudget().count();
    if (latest_frame_target_time < raster_finish_time) {
      latest_frame_target_time =
          latest_frame_target_time +
          fml::TimeDelta::FromMillisecondsF(frame_budget_millis);
    }
    const auto frame_lag =
        (latest_frame_target_time - frame_target_time).ToMillisecondsF();
    const int vsync_transitions_missed = round(frame_lag / frame_budget_millis);
    fml::tracing::TraceEventAsyncComplete(
        "clay",                       // category
        "SceneDisplayLag",            // name
        raster_finish_time,           // begin_time
        latest_frame_target_time,     // end_time
        "frame_target_time",          // arg_key_1
        frame_target_time,            // arg_val_1
        "current_frame_target_time",  // arg_key_2
        latest_frame_target_time,     // arg_val_2
        "vsync_transitions_missed",   // arg_key_3
        vsync_transitions_missed      // arg_val_3
    );                                // NOLINT
  }
#endif
#endif

  return raster_status;
}

RasterStatus Rasterizer::DrawToSurface(
    FrameTimingsRecorder& frame_timings_recorder, clay::LayerTree& layer_tree) {
  TRACE_EVENT("clay", "Rasterizer::DrawToSurface");
  FML_DCHECK(surface_);

  instrumentation_service_.Act([raster_time = raster_time_,
                                now = fml::TimePoint::Now()](auto& impl) {
    if (impl.GetFrameTimingCollector()->IsRecordingFirstFramePerf()) {
      impl.GetFrameTimingCollector()->BeginRecord(clay::Perf::kFirstRasterCost);
    } else {
      raster_time->Start(now);
    }
  });

  RasterStatus raster_status;
  if (surface_->AllowsDrawingWhenGpuDisabled()) {
    raster_status = DrawToSurfaceUnsafe(frame_timings_recorder, layer_tree);
  } else {
    platform_const_service_->GetIsGPUDisabledSyncSwitch()->Execute(
        fml::SyncSwitch::Handlers()
            .SetIfTrue([&] { raster_status = RasterStatus::kDiscarded; })
            .SetIfFalse([&] {
              raster_status =
                  DrawToSurfaceUnsafe(frame_timings_recorder, layer_tree);
            }));
  }

  layer_tree.AppendFrameTimings(frame_timings_recorder.TakeFrameTimings(),
                                true);
  return raster_status;
}

/// Unsafe because it assumes we have access to the GPU which isn't the case
/// when iOS is backgrounded, for example.
/// \see Rasterizer::DrawToSurface
RasterStatus Rasterizer::DrawToSurfaceUnsafe(
    FrameTimingsRecorder& frame_timings_recorder, clay::LayerTree& layer_tree) {
  FML_DCHECK(surface_);

  ScopedTimingRecorder scoped_draw_timing(frame_timings_recorder,
                                          FrameTimingKey::kDoDrawStart,
                                          FrameTimingKey::kDoDrawEnd);

  compositor_context_->ui_time().SetLapTime(
      frame_timings_recorder.GetBuildDuration());

  std::unique_ptr<SurfaceFrame> frame = nullptr;
  {
    TRACE_EVENT("clay", "GPURasterizer::AcquireFrame");
    ScopedTimingRecorder scoped_acquire_frame_timing(
        frame_timings_recorder, FrameTimingKey::kAcquireFrameStart,
        FrameTimingKey::kAcquireFrameEnd);
    frame = surface_->AcquireFrame(layer_tree.frame_size());
    scoped_acquire_frame_timing.MarkRecordEnd();
  }
  if (frame == nullptr) {
    return RasterStatus::kFailed;
  }
  auto compositor_state =
      std::make_unique<CompositorState>(layer_tree.frame_size());

  instrumentation_service_.Act(
      [supports_partial_repaint =
           frame->framebuffer_info().supports_partial_repaint](auto& impl) {
        if (impl.GetFrameTimingCollector()->IsRecordingFirstFramePerf()) {
          impl.GetFrameTimingCollector()->InsertRecord(
              clay::Perf::kEnablePartialRepaint, supports_partial_repaint);
        }
      });

  auto compositor_frame = compositor_context_->AcquireFrame(
      surface_->GetContext(),             // skia GrContext
      frame->GetCanvas(),                 // root surface canvas
      compositor_state.get(),             // compositor state
      surface_->GetRootTransformation(),  // root surface transformation
      true,                               // instrumentation enabled
      frame->framebuffer_info()
          .supports_readback  // surface supports pixel reads
  );

  if (compositor_frame) {
    compositor_context_->raster_cache().BeginFrame();
    frame_timings_recorder.RecordRasterStart(fml::TimePoint::Now());

    std::unique_ptr<FrameDamage> damage;
    // when leaf layer tracing is enabled we wish to repaint the whole frame
    // for accurate performance metrics.
    if (frame->framebuffer_info().supports_partial_repaint &&
        !layer_tree.is_leaf_layer_tracing_enabled()) {
      // FIXME(haoyoufeng.aji) : partial repaint with platform view present
      bool force_full_repaint = true;

      damage = std::make_unique<FrameDamage>();
      if (frame->framebuffer_info().existing_damage && !force_full_repaint) {
        damage->SetPreviousLayerTree(last_layer_tree_.get());
        damage->AddAdditionalDamage(*frame->framebuffer_info().existing_damage);
        damage->SetClipAlignment(
            frame->framebuffer_info().horizontal_clip_alignment,
            frame->framebuffer_info().vertical_clip_alignment);
      }
    }

    bool should_low_memory_usage = false;
    if (render_settings_ && render_settings_->ShouldLowMemoryUsage()) {
      should_low_memory_usage = true;
    }
    if (last_memory_strategy_ && !should_low_memory_usage) {
      compositor_context_->raster_cache().set_needs_build_all_caches(true);
    } else {
      compositor_context_->raster_cache().set_needs_build_all_caches(false);
    }
    last_memory_strategy_ = should_low_memory_usage;
#if defined(OS_ANDROID) || defined(OS_IOS)
    // Mobile Clay bypasses raster cache intentionally; avoid retaining large
    // cached display lists on memory-sensitive devices.
    bool ignore_raster_cache = true;
#else
    bool ignore_raster_cache =
        should_low_memory_usage || !surface_->EnableRasterCache() ||
        (render_settings_ && render_settings_->IgnoreRasterCache());
#endif

    if (ignore_raster_cache && !last_ignore_raster_cache_) {
      // Clear cache manually before bypassing it to avoid errors in
      // raster_cache.EndFrame().
      compositor_context_->raster_cache().Clear();
    }
    last_ignore_raster_cache_ = ignore_raster_cache;
    {
      ScopedTimingRecorder scoped_raster_timing(frame_timings_recorder,
                                                FrameTimingKey::kRasterStart,
                                                FrameTimingKey::kRasterEnd);
      compositor_frame->Raster(
          layer_tree, ignore_raster_cache, damage.get(),
          Color::kTransparent().Value(), [&] {
            frame->Prepare(damage ? damage->GetBufferDamage() : std::nullopt);
          });
      scoped_raster_timing.MarkRecordEnd();
    }
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    instrumentation_service_.Act([timestamp](auto& impl) {
      if (impl.GetFrameTimingCollector()->IsRecordingFirstFramePerf()) {
        impl.GetFrameTimingCollector()->InsertRecord(
            clay::Perf::kFirstRasterEnd, timestamp);
        impl.GetFrameTimingCollector()->EndRecord(clay::Perf::kFirstRasterCost);
      }
    });

    SurfaceFrame::SubmitInfo submit_info;
    // TODO (https://github.com/flutter/flutter/issues/105596):  // NOLINT
    // this can be in the past and might need to get snapped to future as this
    // frame could have been resubmitted. `presentation_time` on `submit_info`
    // is not set in this case.
    const auto presentation_time = frame_timings_recorder.GetVsyncTargetTime();
    if (presentation_time > fml::TimePoint::Now()) {
      submit_info.presentation_time = presentation_time;
    }
    if (damage) {
      submit_info.frame_damage = damage->GetFrameDamage();
      submit_info.buffer_damage = damage->GetBufferDamage();
    }

    frame->set_submit_info(submit_info);
    {
      TRACE_EVENT("clay", "GPURasterizer::SubmitFrame");
      ScopedTimingRecorder scoped_submit_frame_timing(
          frame_timings_recorder, FrameTimingKey::kSubmitFrameStart,
          FrameTimingKey::kSubmitFrameEnd);
      compositor_service_->SubmitFrame(surface_->GetContext(), std::move(frame),
                                       std::move(compositor_state));
      scoped_submit_frame_timing.MarkRecordEnd();
    }
    compositor_context_->raster_cache().EndFrame();
    frame_timings_recorder.RecordRasterEnd(
        &compositor_context_->raster_cache());

    FireNextFrameCallbackIfPresent();

    if (unref_queue_) {
      unref_queue_->Drain();
    }

    fml::TimePoint now = fml::TimePoint::Now();
    timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
    instrumentation_service_.Act(
        [raster_time = raster_time_, frame_total_time = frame_total_time_, now,
         lap_time = now - frame_timings_recorder.GetBuildStartTime(),
         sync_compositor =
             platform_const_service_->GetSettings().enable_sync_compositor,
         timestamp](auto& impl) {
          if (impl.GetFrameTimingCollector()->IsRecordingFirstFramePerf() &&
              !sync_compositor) {
            impl.GetFrameTimingCollector()->InsertRecord(
                clay::Perf::kFirstPresentEnd, timestamp);
          } else {
            raster_time->Stop(now);
            impl.GetFrameTimingCollector()->InsertRasterRecord(*raster_time);
            frame_total_time->SetLapTime(lap_time);
            impl.GetFrameTimingCollector()->InsertFrameTotalCostRecord(
                *frame_total_time);
          }
        });
    if (surface_->GetContext()) {
#ifndef ENABLE_SKITY
      surface_->GetContext()->performDeferredCleanup(kSkiaCleanupExpiration);
#endif  // ENABLE_SKITY
    }
    if (auto* info = compositor_context_->GetRasterCacheInfo()) {
      instrumentation_service_.Act([raster_cache_info = *info](auto& impl) {
        impl.UpdateRasterCacheInfo(raster_cache_info);
      });
    }
    scoped_draw_timing.MarkRecordEnd();
    return RasterStatus::kSuccess;
  }

  return RasterStatus::kFailed;
}

ScreenshotData Rasterizer::ScreenshotLastLayerTree(
    ScreenshotData::ScreenshotType type, bool base64_encode,
    uint32_t background_color) {
  auto* layer_tree = GetLastLayerTree();
  return TakeScreenshotWithBase64(layer_tree, type, surface_.get(),
                                  compositor_context_.get(), base64_encode,
                                  background_color);
}

void Rasterizer::AddNextFrameCallback(const fml::closure& callback) {
  next_frame_callbacks_.push_back(callback);
}

void Rasterizer::FireNextFrameCallbackIfPresent() {
  if (next_frame_callbacks_.empty()) {
    return;
  }
  // It is safe for callbacks to register new callbacks.
  std::vector<fml::closure> callbacks;
  callbacks.swap(next_frame_callbacks_);
  for (auto& cb : callbacks) {
    if (cb) {
      cb();
    }
  }
}

void Rasterizer::SetResourceCacheMaxBytes(size_t max_bytes, bool from_user) {
  user_override_resource_cache_bytes_ |= from_user;

  if (!from_user && user_override_resource_cache_bytes_) {
    // We should not update the setting here if a user has explicitly set a
    // value for this over the flutter/skia channel.
    return;
  }

  max_cache_bytes_ = max_bytes;
  if (!surface_) {
    return;
  }

#ifndef ENABLE_SKITY
  GrDirectContext* context = surface_->GetContext();
  if (context) {
    auto context_switch = surface_->MakeRenderContextCurrent();
    if (!context_switch->GetResult()) {
      return;
    }
    context->setResourceCacheLimit(max_bytes);
  }
#else
// TODO(zhangzhijian.123): Since the function
// 'skity::GPUContext::SetResourceCacheLimit()' is unstable, so do nothing
// now. We'll fix this issue later.
#endif  // ENABLE_SKITY
}

std::optional<size_t> Rasterizer::GetResourceCacheMaxBytes() const {
  if (!surface_) {
    return std::nullopt;
  }
#ifndef ENABLE_SKITY
  GrDirectContext* context = surface_->GetContext();
  if (context) {
    return context->getResourceCacheLimit();
  }
#endif  // ENABLE_SKITY
  return std::nullopt;
}

void Rasterizer::SetRenderSettings(
    fml::RefPtr<clay::RenderSettings> render_settings) {
  render_settings_ = render_settings;
}
}  // namespace clay
