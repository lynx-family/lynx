// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor.h"

#include <utility>

#include "clay/gfx/shared_image/shared_image_backing.h"
#ifdef ENABLE_SKITY
#include "clay/gfx/skity/skity_image.h"
#endif
#include "clay/gfx/style/color.h"

namespace clay {

DisplayCompositor::DisplayCompositor(
    std::unique_ptr<Surface> surface,
    Puppet<Owner::kRaster, CompositorService> compositor_service)
    : surface_(std::move(surface)),
      compositor_service_(std::move(compositor_service)) {}

DisplayCompositor::~DisplayCompositor() = default;

bool DisplayCompositor::Present(
    const fml::RefPtr<SharedImageBacking>& backing,
    const SurfaceFrame::SubmitInfo& submit_info,
    std::unique_ptr<CompositorState> compositor_state) {
  if (!backing || !surface_ || !compositor_state) {
    return false;
  }
  auto current = surface_->MakeRenderContextCurrent();
  if (!current || !current->GetResult()) {
    return false;
  }
  auto frame = surface_->AcquireFrame(compositor_state->GetFrameSize());
  if (!frame || !frame->GetCanvas()) {
    return false;
  }
  // The writer has flushed on this same context and raster task runner.
  auto* context = surface_->GetContext();
  if (!context) return false;
#ifndef ENABLE_SKITY
  auto reader = backing->CreateSkiaRepresentation(context);
  auto image = reader ? reader->GetSkImage() : nullptr;
#else
  auto reader = backing->CreateSkityRepresentation(context);
  auto texture = reader ? reader->GetSkityImage() : nullptr;
  auto image = texture ? texture->gr_image() : nullptr;
#endif
  if (!image) return false;

  // The raster image is a complete frame. Repaint the output in full even if
  // the window's buffer age permits partial repaint.
  frame->Prepare(std::nullopt);
  auto* canvas = frame->GetCanvas();
  CANVAS_CLEAR(canvas, Color::kTransparent());
  CANVAS_SAVE(canvas);
  // The raster image already includes the root transform. Preserve the output
  // canvas's original matrix for the existing platform-view composition path.
  CANVAS_RESET_MATRIX(canvas);
#ifndef ENABLE_SKITY
  canvas->drawImage(image, 0, 0);
#else
  if (context->GetBackendType() == skity::GPUBackendType::kOpenGL) {
    CANVAS_TRANSLATE(canvas, 0, backing->GetSize().y);
    CANVAS_SCALE(canvas, 1, -1);
  }
  canvas->DrawImage(image, 0, 0);
#endif
  CANVAS_RESTORE(canvas);
  auto display_submit_info = submit_info;
  display_submit_info.buffer_damage = std::nullopt;
  display_submit_info.frame_damage = std::nullopt;
  frame->set_submit_info(display_submit_info);
  const bool submitted = compositor_service_->SubmitFrame(
      surface_->GetContextPtr(), std::move(frame), std::move(compositor_state));
  return reader->EndRead() && submitted;
}

}  // namespace clay
