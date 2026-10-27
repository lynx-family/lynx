// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_COMPOSITOR_COMPOSITOR_SERVICE_IMPL_CC_
#define CLAY_SHELL_COMMON_COMPOSITOR_COMPOSITOR_SERVICE_IMPL_CC_

#include "clay/shell/common/services/compositor/compositor_service.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/flow/surface.h"
#include "clay/flow/view_slicer.h"
#include "clay/shell/common/rasterizer.h"
#include "clay/shell/common/services/compositor/platform_overlay_service.h"

namespace clay {

bool CompositorService::SubmitFrame(
    clay::GrContext* context, std::unique_ptr<SurfaceFrame> background_frame,
    std::unique_ptr<CompositorState> compositor_state) {
  TRACE_EVENT("clay", "FlutterPlatformViewsController::SubmitFrame");

  if (compositor_state->GetCompositionOrder().empty() &&
      !had_hybrid_composited_) {
    had_hybrid_composited_ = false;
    return background_frame->Submit();
  }

  had_hybrid_composited_ = true;

  // TODO(haoyoufeng.aji) support external overlay views
  bool did_encode = true;
  std::unordered_map<int64_t, OverlayData> platform_overlays;
  std::vector<std::pair<SurfaceFrame::SubmitCallback, SurfaceFrame::SubmitInfo>>
      submit_infos;
  submit_infos.reserve(compositor_state->GetCompositionOrder().size() + 1);
  std::unordered_map<int64_t, skity::Rect> view_rects;

  for (int64_t view_id : compositor_state->GetCompositionOrder()) {
    view_rects[view_id] =
        compositor_state->GetViewParams()[view_id]->finalBoundingRect();
  }

  std::unordered_map<int64_t, skity::Rect> overlay_layers = SliceViews(
      background_frame->GetCanvas(), compositor_state->GetCompositionOrder(),
      compositor_state->GetSlices(), view_rects);

  // background frame must come first since it's the "current" surface
  background_frame->set_submit_info({.present_with_transaction = true});
  did_encode &= background_frame->Encode();
  submit_infos.push_back(background_frame->PrepareSubmit());

  size_t required_overlay_layers = 0;
  for (int64_t view_id : compositor_state->GetCompositionOrder()) {
    std::unordered_map<int64_t, skity::Rect>::const_iterator overlay =
        overlay_layers.find(view_id);
    if (overlay == overlay_layers.end()) {
      continue;
    }
    required_overlay_layers++;
  }

  CreateMissingSurfaces(required_overlay_layers, context);

  int64_t overlay_id = 0;
  for (int64_t view_id : compositor_state->GetCompositionOrder()) {
    std::unordered_map<int64_t, skity::Rect>::const_iterator it =
        overlay_layers.find(view_id);
    if (it == overlay_layers.end()) {
      continue;
    }
    auto& [_, overlay_rect] = *it;
    CompositorSurface& compositor_surface = GetCompositorSurface();

    std::unique_ptr<SurfaceFrame> frame =
        compositor_surface.surface->AcquireFrame(
            compositor_state->GetFrameSize());

    // If frame is null, AcquireFrame already printed out an error message.
    if (!frame) {
      continue;
    }
    clay::GrCanvas* overlay_canvas = frame->GetCanvas();
    int restore_count = CANVAS_GET_SAVE_COUNT(overlay_canvas);
    CANVAS_SAVE(overlay_canvas);
    CANVAS_CLIP_RECT(overlay_canvas, overlay_rect);
    CANVAS_CLEAR(overlay_canvas, clay::Color::kTransparent());
    CANVAS_TRANSLATE(overlay_canvas, -overlay_rect.X(), -overlay_rect.Y());
    compositor_state->GetSlices()[view_id]->render_into(overlay_canvas);
    CANVAS_RESTORE_TO_COUNT(overlay_canvas, restore_count);

    frame->set_submit_info({.present_with_transaction = true});
    did_encode &= frame->Encode();

    platform_overlays[view_id] = OverlayData{
        .rect = overlay_rect,                           //
        .view_id = view_id,                             //
        .overlay_id = overlay_id,                       //
        .overlay = compositor_surface.platform_overlay  //
    };
    submit_infos.emplace_back(frame->PrepareSubmit());
    overlay_id++;
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

  presenter_service_.Act([did_encode, present_frame = std::move(present_frame)](
                             auto& impl) mutable {
    // We do `did_encode` check here because present_frame needs to be
    // destructed in Platform thread
    if (did_encode) {
      impl.Present(present_frame);
    }
  });

  return did_encode;
}

// |clay::Service|
void CompositorService::OnInit(clay::ServiceManager& service_manager,
                               const clay::RasterServiceContext& ctx) {
  presenter_service_ = service_manager.GetService<PresenterService>();
  overlay_service_ = service_manager.GetService<PlatformOverlayService>();
  raster_task_runner_ = service_manager.GetTaskRunners()
                            ->SelectTaskRunner<clay::Owner::kRaster>();
}
// |clay::Service|
void CompositorService::OnDestroy() {
  presenter_service_ = nullptr;
  overlay_service_ = nullptr;
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
