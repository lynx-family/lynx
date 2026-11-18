// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/flow/compositor/display_scene.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#ifdef ENABLE_SKITY
#include "clay/gfx/skity/skity_image.h"
#endif
#include "clay/gfx/style/color.h"
#include "clay/shell/common/screenshot_utils.h"
#include "clay/shell/common/shared_image_surface.h"

namespace clay {

DisplayCompositor::DisplayCompositor(
    std::unique_ptr<Surface> surface,
    Puppet<Owner::kRaster, CompositorService> compositor_service)
    : surface_(std::move(surface)),
      compositor_service_(std::move(compositor_service)) {}

DisplayCompositor::~DisplayCompositor() {
  if (surface_ && compositor_state_) {
    auto current = surface_->MakeRenderContextCurrent();
    Clear();
  }
}

void DisplayCompositor::Clear() {
  compositor_state_.reset();
  backing_ = nullptr;
  image_ = nullptr;
}

bool DisplayCompositor::Present(
    const fml::RefPtr<SharedImageBacking>& backing,
    const SurfaceFrame::SubmitInfo& submit_info,
    std::unique_ptr<CompositorState> compositor_state, GrImagePtr image) {
  if (!surface_ || !compositor_state ||
      (!compositor_state->GetDisplayScene() && !backing && !image)) {
    return false;
  }
  auto current = surface_->MakeRenderContextCurrent();
  if (!current || !current->GetResult()) {
    return false;
  }
  const bool has_scene = static_cast<bool>(compositor_state->GetDisplayScene());
  backing_ = has_scene ? nullptr : backing;
  image_ = has_scene ? nullptr : std::move(image);
  compositor_state_ = std::move(compositor_state);
  root_transform_ = surface_->GetRootTransformation();
  return PresentCurrent(submit_info);
}

bool DisplayCompositor::CanPresentImage(int64_t image_id) const {
  return compositor_state_ && compositor_state_->GetDisplayScene() &&
         compositor_state_->GetDisplayScene()->ContainsImage(image_id);
}

ScreenshotData DisplayCompositor::Screenshot(uint32_t background_color) {
  if (!compositor_state_ || !compositor_state_->GetDisplayScene() ||
      root_transform_ != surface_->GetRootTransformation()) {
    return {};
  }
  for (const auto& entry : compositor_state_->GetViewParams()) {
    if (entry.second->NeedsPlatformViewComposite()) return {};
  }
  auto current = surface_->MakeRenderContextCurrent();
  if (!current || !current->GetResult()) return {};

  TRACE_EVENT("clay", "DisplayCompositor::Screenshot");
  const auto size = compositor_state_->GetFrameSize();
  SharedImageSurface snapshot(surface_.get(), 1, false,
                              SharedImageSurface::Storage::kSnapshot);
  auto frame = snapshot.AcquireFrame(size);
  if (!frame || !frame->GetCanvas()) return {};
  auto* canvas = frame->GetCanvas();
  CANVAS_CLEAR(canvas, Color(background_color));
  auto& scene = *compositor_state_->GetDisplayScene();
  scene.Paint(canvas, surface_->GetContext());
  for (int64_t view_id : compositor_state_->GetCompositionOrder()) {
    scene.Paint(canvas, surface_->GetContext(), view_id);
  }
  if (!frame->Submit()) return {};

#ifndef ENABLE_SKITY
  auto info = SkImageInfo::MakeN32Premul(size.x, size.y);
  const auto row_bytes = info.minRowBytes();
  auto data = SkData::MakeUninitialized(info.computeByteSize(row_bytes));
  if (!data ||
      !snapshot.image()->readPixels(surface_->GetContext(), info,
                                    data->writable_data(), row_bytes, 0, 0)) {
    return {};
  }
#else
  auto pixmap = snapshot.image()->ReadPixels(surface_->GetContext());
  if (!pixmap || pixmap->RowBytes() != pixmap->Width() * 4) return {};
  auto data = skity::Data::MakeWithCopy(pixmap->Addr(),
                                        pixmap->RowBytes() * pixmap->Height());
#endif
  return {std::move(data), size};
}

bool DisplayCompositor::Present(const SurfaceFrame::SubmitInfo& submit_info) {
  if (!surface_ || !compositor_state_ ||
      root_transform_ != surface_->GetRootTransformation()) {
    return false;
  }
  auto current = surface_->MakeRenderContextCurrent();
  if (!current || !current->GetResult()) {
    return false;
  }
  return PresentCurrent(submit_info);
}

bool DisplayCompositor::PresentCurrent(
    const SurfaceFrame::SubmitInfo& submit_info) {
  if (!compositor_state_) return false;
  const auto& scene = compositor_state_->GetDisplayScene();
  const auto& size = compositor_state_->GetFrameSize();
  if (scene) {
    SharedImageSurface raster(surface_.get(), 1, false,
                              SharedImageSurface::Storage::kSnapshot);
    bool prepared =
        scene->Prepare([&](EmbedderViewSlice& recording,
                           const skity::Rect& bounds) -> GrImagePtr {
          auto frame = raster.AcquireFrame({bounds.Width(), bounds.Height()});
          if (!frame) return nullptr;
          auto* canvas = frame->GetCanvas();
          CANVAS_CLEAR(canvas, Color::kTransparent());
          CANVAS_TRANSLATE(canvas, -bounds.Left(), -bounds.Top());
          recording.render_into(canvas);
          if (!frame->Submit()) return nullptr;
          return raster.image();
        });
    if (!prepared) {
      Clear();
      return false;
    }
  }
  auto frame = surface_->AcquireFrame(size);
  if (!frame || !frame->GetCanvas()) {
    return false;
  }
  auto* context = surface_->GetContext();
  GrImagePtr image = image_;
  fml::RefPtr<SharedImageRepresentation> reader;
  if (backing_) {
    // The writer has flushed on this same context and raster task runner.
    if (!context) return false;
#ifndef ENABLE_SKITY
    auto representation = backing_->CreateSkiaRepresentation(context);
    image = representation ? representation->GetSkImage() : nullptr;
#else
    auto representation = backing_->CreateSkityRepresentation(context);
    auto texture = representation ? representation->GetSkityImage() : nullptr;
    image = texture ? texture->gr_image() : nullptr;
#endif
    if (!image) return false;
    reader = std::move(representation);
  }

  // The raster image is a complete frame. Repaint the output in full even if
  // the window's buffer age permits partial repaint.
  frame->Prepare(std::nullopt);
  auto* canvas = frame->GetCanvas();
  CANVAS_CLEAR(canvas, Color::kTransparent());
  CANVAS_SAVE(canvas);
  // The raster image already includes the root transform. Preserve the output
  // canvas's original matrix for the existing platform-view composition path.
  if (scene) {
    scene->Paint(canvas, context);
  } else {
    CANVAS_RESET_MATRIX(canvas);
#ifndef ENABLE_SKITY
    canvas->drawImage(image, 0, 0);
#else
    if (backing_ &&
        context->GetBackendType() == skity::GPUBackendType::kOpenGL) {
      CANVAS_TRANSLATE(canvas, 0, backing_->GetSize().y);
      CANVAS_SCALE(canvas, 1, -1);
    }
    canvas->DrawImage(image, 0, 0);
#endif
  }
  CANVAS_RESTORE(canvas);
  auto display_submit_info = submit_info;
  display_submit_info.buffer_damage = std::nullopt;
  display_submit_info.frame_damage = std::nullopt;
  frame->set_submit_info(display_submit_info);
  const bool submitted = compositor_service_->SubmitFrame(
      surface_->GetContextPtr(), std::move(frame),
      compositor_state_->CloneForPresentation());
  return (!reader || reader->EndRead()) && submitted;
}

}  // namespace clay
