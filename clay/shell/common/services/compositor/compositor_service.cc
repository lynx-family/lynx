// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_COMPOSITOR_COMPOSITOR_SERVICE_IMPL_CC_
#define CLAY_SHELL_COMMON_COMPOSITOR_COMPOSITOR_SERVICE_IMPL_CC_

#include "clay/shell/common/services/compositor/compositor_service.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/trace/native/trace_event.h"
#include "clay/flow/surface.h"
#include "clay/flow/view_slicer.h"
#include "clay/shell/common/rasterizer.h"
#include "clay/shell/common/services/compositor/platform_overlay_service.h"

namespace clay {

namespace {
constexpr int64_t kPlatformPresentWaitTimeoutMs = 3000;
}  // namespace

bool CompositorService::SubmitFrame(
    clay::GrContext* context, std::unique_ptr<SurfaceFrame> background_frame,
    std::unique_ptr<CompositorState> compositor_state) {
  TRACE_EVENT("clay", "CompositorService::SubmitFrame");

  if (compositor_state->GetCompositionOrder().empty() &&
      !had_hybrid_composited_) {
    had_hybrid_composited_ = false;
    return background_frame->Submit();
  }

  had_hybrid_composited_ = true;

  bool did_encode = true;
  std::unordered_map<int64_t, OverlayData> platform_overlays;
  std::vector<std::pair<SurfaceFrame::SubmitCallback, SurfaceFrame::SubmitInfo>>
      submit_infos;
  submit_infos.reserve(compositor_state->GetCompositionOrder().size() + 1);

  std::vector<OverlayData> overlay_render_requests = SliceViews(
      background_frame->GetCanvas(), compositor_state->GetCompositionOrder(),
      compositor_state->GetSlices(), compositor_state->GetViewParams());

  // background frame must come first since it's the "current" surface
  background_frame->set_submit_info({.present_with_transaction = true});
  did_encode &= background_frame->Encode();
  submit_infos.push_back(background_frame->PrepareSubmit());

  CreateMissingSurfaces(overlay_render_requests.size(), context);

  for (OverlayData& overlay_data : overlay_render_requests) {
    const skity::Rect& overlay_rect = overlay_data.rect;
    auto& slices = compositor_state->GetSlices();
    auto slice_it = slices.find(overlay_data.view_id);

    CompositorSurface& compositor_surface = GetCompositorSurface();
    overlay_data.overlay = compositor_surface.platform_overlay;
    compositor_surface.platform_overlay->PrepareSurface(overlay_data);
    std::unique_ptr<SurfaceFrame> frame =
        compositor_surface.surface->AcquireFrame(
            {overlay_rect.Width(), overlay_rect.Height()});

    // If frame is null, AcquireFrame already printed out an error message.
    if (!frame) {
      continue;
    }
    frame->Prepare(std::make_optional<skity::Rect>(
        {0, 0, overlay_rect.Width(), overlay_rect.Height()}));
    clay::GrCanvas* overlay_canvas = frame->GetCanvas();
    int restore_count = CANVAS_GET_SAVE_COUNT(overlay_canvas);
    CANVAS_SAVE(overlay_canvas);
    CANVAS_CLIP_RECT(
        overlay_canvas,
        skity::Rect::MakeWH(overlay_rect.Width(), overlay_rect.Height()));
    CANVAS_CLEAR(overlay_canvas, clay::Color::kTransparent());
    CANVAS_TRANSLATE(overlay_canvas, -overlay_rect.X(), -overlay_rect.Y());

    if (slice_it != slices.end() && slice_it->second) {
      slice_it->second->render_into(overlay_canvas);
    }
    CANVAS_RESTORE_TO_COUNT(overlay_canvas, restore_count);

    frame->set_submit_info({.present_with_transaction = true});
    did_encode &= frame->Encode();

    platform_overlays[overlay_data.view_id] = overlay_data;
    submit_infos.emplace_back(frame->PrepareSubmit());
  }

  std::vector<std::shared_ptr<PlatformOverlay>> unused_overlays =
      RemoveUnusedSurfaces();
  RecycleSurfaces();

  PresentFrame present_frame{
      .overlays = std::move(platform_overlays),
      .compositor_params = std::move(compositor_state->GetViewParams()),
      .composite_order = std::move(compositor_state->GetCompositionOrder()),
      .unused_overlays = std::move(unused_overlays),
      .submit_infos = std::move(submit_infos),
  };

#if OS_IOS
  const bool needs_platform_present_sync =
      !present_frame.composite_order.empty();
#else
  const bool needs_platform_present_sync = false;
#endif

  std::shared_ptr<fml::AutoResetWaitableEvent> present_latch;
  if (needs_platform_present_sync && platform_task_runner_ &&
      !platform_task_runner_->RunsTasksOnCurrentThread()) {
    present_latch = std::make_shared<fml::AutoResetWaitableEvent>();
  }

  presenter_service_.Act([did_encode, present_frame = std::move(present_frame),
                          present_latch](auto& impl) mutable {
    // We do `did_encode` check here because present_frame needs to be
    // destructed in Platform thread
    if (did_encode) {
      impl.Present(present_frame);
    }
    if (present_latch) {
      present_latch->Signal();
    }
  });

  if (present_latch) {
    present_latch->WaitWithTimeout(
        fml::TimeDelta::FromMilliseconds(kPlatformPresentWaitTimeoutMs));
  }

  return did_encode;
}

// |clay::Service|
void CompositorService::OnInit(clay::ServiceManager& service_manager,
                               const clay::RasterServiceContext& ctx) {
  presenter_service_ = service_manager.GetService<PresenterService>();
  overlay_service_ = service_manager.GetService<PlatformOverlayService>();
  platform_task_runner_ = service_manager.GetTaskRunners()
                              ->SelectTaskRunner<clay::Owner::kPlatform>();
  raster_task_runner_ = service_manager.GetTaskRunners()
                            ->SelectTaskRunner<clay::Owner::kRaster>();
}
// |clay::Service|
void CompositorService::OnDestroy() {
  presenter_service_ = nullptr;
  overlay_service_ = nullptr;
  platform_task_runner_ = nullptr;
  raster_task_runner_ = nullptr;
  compositor_surfaces_.clear();
}

void CompositorService::CreateMissingSurfaces(size_t required_surfaces,
                                              clay::GrContext* context) {
  if (required_surfaces <= compositor_surfaces_.size()) {
    return;
  }

  compositor_surfaces_.reserve(required_surfaces);
  auto created_surfaces = overlay_service_->CreatePlatformOverlay(
      required_surfaces - compositor_surfaces_.size());
  for (auto& surface : created_surfaces) {
    compositor_surfaces_.emplace_back(CompositorSurface{
        .platform_overlay = surface,
        .surface = surface->GetOutputSurface()->CreateGPUSurface(context)});
  }
}

CompositorSurface& CompositorService::GetCompositorSurface() {
  CompositorSurface& result = compositor_surfaces_[available_layer_index_];
  available_layer_index_++;
  return result;
}

std::vector<std::shared_ptr<PlatformOverlay>>
CompositorService::RemoveUnusedSurfaces() {
  std::vector<std::shared_ptr<PlatformOverlay>> results;
  for (size_t i = available_layer_index_; i < compositor_surfaces_.size();
       i++) {
    results.push_back(compositor_surfaces_[i].platform_overlay);
  }
  // Leave at least one overlay layer, to work around cases where scrolling
  // platform views under an app bar continually adds and removes an
  // overlay layer. This logic could be removed if
  // https://github.com/flutter/flutter/issues/150646 is fixed.
  static constexpr size_t kLeakLayerCount = 1;
  size_t erase_offset = std::max(available_layer_index_, kLeakLayerCount);
  if (erase_offset < compositor_surfaces_.size()) {
    compositor_surfaces_.erase(compositor_surfaces_.begin() + erase_offset,
                               compositor_surfaces_.end());
  }
  return results;
}

void CompositorService::RecycleSurfaces() { available_layer_index_ = 0; }

std::shared_ptr<CompositorService> CompositorService::Create() {
  return std::make_shared<CompositorService>();
}

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_COMPOSITOR_COMPOSITOR_SERVICE_IMPL_CC_
