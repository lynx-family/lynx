// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/screenshot_utils.h"

#include <utility>

#ifndef ENABLE_SKITY
#include "clay/flow/layers/offscreen_surface.h"
#endif  // ENABLE_SKITY
#include <cstdlib>

#include "clay/fml/base64.h"
#include "clay/fml/logging.h"
#include "clay/gfx/paint_image_skity.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/shell/common/serialization_callbacks.h"

namespace clay {

#ifndef ENABLE_SKITY
static sk_sp<SkData> ScreenshotLayerTreeAsPicture(
    clay::LayerTree* tree, clay::CompositorContext& compositor_context,
    uint32_t background_color) {
  FML_DCHECK(tree != nullptr);
  SkPictureRecorder recorder;
  recorder.beginRecording(
      SkRect::MakeWH(tree->frame_size().x, tree->frame_size().y));

  SkMatrix root_surface_transformation;
  root_surface_transformation.reset();

  // TODO(amirh): figure out how to take a screenshot with embedded UIView.
  // https://github.com/flutter/flutter/issues/23435
  auto frame = compositor_context.AcquireFrame(
      nullptr, recorder.getRecordingCanvas(), nullptr,
      clay::ConvertSkMatrixToSkityMatrix(root_surface_transformation), false,
      true);
  frame->Raster(*tree, true, nullptr, background_color);

#if defined(OS_FUCHSIA)
  SkSerialProcs procs = {0};
  procs.fImageProc = SerializeImageWithoutData;
  procs.fTypefaceProc = SerializeTypefaceWithoutData;
#else
  SkSerialProcs procs = {nullptr};
  procs.fTypefaceProc = SerializeTypefaceWithData;
#endif  // OS_FUCHSIA

  return recorder.finishRecordingAsPicture()->serialize(&procs);
}

static sk_sp<SkData> ScreenshotLayerTreeAsImage(
    clay::LayerTree* tree, Surface* surface,
    clay::CompositorContext& compositor_context, bool compressed,
    uint32_t background_color) {
  auto sk_image = TakeScreenshotWithOpaque(tree, surface, compositor_context,
                                           true, background_color);
  if (!sk_image) {
    return nullptr;
  }
  // If the caller want the pixels to be compressed, there is a Skia utility to
  // compress to PNG. Use that.
  if (compressed) {
    return sk_image->encodeToData();
  }
  // Copy it into a bitmap and return the same.
  SkPixmap pixmap;
  if (!sk_image->peekPixels(&pixmap)) {
    FML_DLOG(ERROR) << "Screenshot: unable to obtain bitmap pixels";
    return nullptr;
  }
  return SkData::MakeWithCopy(pixmap.addr32(), pixmap.computeByteSize());
}

// static
sk_sp<SkImage> TakeScreenshotWithOpaque(
    clay::LayerTree* tree, Surface* surface,
    clay::CompositorContext& compositor_context, bool opaque,
    uint32_t background_color) {
  if (!surface) {
    FML_LOG(ERROR) << "Failed to screenshot because surface is null";
    return nullptr;
  }
  auto surface_context = surface->GetContext();

  // Attempt to create a snapshot surface depending on whether we have access
  // to a valid GPU rendering context.
  std::unique_ptr<OffscreenSurface> snapshot_surface =
      std::make_unique<OffscreenSurface>(surface_context, tree->frame_size(),
                                         opaque);

  if (!snapshot_surface->IsValid()) {
    FML_DLOG(ERROR) << "Screenshot: unable to create snapshot surface";
    return nullptr;
  }

  // Draw the current layer tree into the snapshot surface.
  auto* canvas = snapshot_surface->GetCanvas();

  // There is no root surface transformation for the screenshot layer. Reset
  // the matrix to identity.
  skity::Matrix root_surface_transformation;
  root_surface_transformation.Reset();

  // snapshot_surface->makeImageSnapshot needs the GL context to be set if the
  // render context is GL. frame->Raster() pops the gl context in platforms
  // that gl context switching are used. (For example, older iOS that uses GL)
  // We reset the GL context using the context switch.
  auto context_switch = surface->MakeRenderContextCurrent();
  if (!context_switch->GetResult()) {
    FML_DLOG(ERROR) << "Screenshot: unable to make image screenshot";
    return nullptr;
  }

  auto frame = compositor_context.AcquireFrame(
      surface_context,              // skia context
      canvas,                       // canvas
      nullptr,                      // compositor state
      root_surface_transformation,  // root surface transformation
      false,                        // instrumentation enabled
      true                          // render buffer readback supported
  );                                // NOLINT
  canvas->clear(SK_ColorTRANSPARENT);
  frame->Raster(*tree, true, nullptr, background_color);
  canvas->flush();

  return snapshot_surface->GetRasterImage();
}
#else
static std::shared_ptr<skity::Data> ScreenshotLayerTreeAsImage(
    clay::LayerTree* tree, Surface* surface,
    clay::CompositorContext& compositor_context, bool compressed,
    uint32_t background_color) {
  if (compressed) {
    FML_LOG(ERROR) << "Screenshot: 'compressed' not supported on skity yet. ";
    return nullptr;
  }

  auto image = TakeScreenshotWithOpaque(tree, surface, compositor_context,
                                        false, background_color);
  if (!image) {
    FML_DLOG(ERROR) << "Screenshot: failed to snapshot a CPU-backed image";
    return nullptr;
  }
  auto pixmap = *image->GetPixmap();
  if (!pixmap) {
    FML_DLOG(ERROR) << "Screenshot: failed to get pixmap from Image";
    return nullptr;
  }

  return skity::Data::MakeWithCopy(pixmap->Addr(),
                                   pixmap->RowBytes() * pixmap->Height());
}

std::shared_ptr<skity::Image> TakeScreenshotWithOpaque(
    clay::LayerTree* tree, Surface* surface,
    clay::CompositorContext& compositor_context, bool opaque,
    uint32_t background_color) {
  if (!surface) {
    FML_DLOG(ERROR) << "Screenshot: unable to get surface";
    return nullptr;
  }

  auto* gpu_context = surface->GetContext();
  if (!gpu_context) {
    FML_DLOG(ERROR) << "Screenshot: unable to get gpu context";
    return nullptr;
  }

  auto context_switch = surface->MakeRenderContextCurrent();
  if (!context_switch->GetResult()) {
    FML_DLOG(ERROR) << "Screenshot: unable to make image screenshot";
    return nullptr;
  }

  uint32_t frame_width = tree->frame_size().x;
  uint32_t frame_height = tree->frame_size().y;

  skity::GPURenderTargetDescriptor desc;
  desc.width = frame_width;
  desc.height = frame_height;
  desc.sample_count = 1;
  auto render_target = gpu_context->CreateRenderTarget(desc);

  if (!render_target) {
    FML_DLOG(ERROR) << "Screenshot: unable to create offscreen RenderTarget";
    return nullptr;
  }

  auto skity_canvas = render_target->GetCanvas();

  // There is no root surface transformation for the screenshot layer. Reset
  // the matrix to identity.
  skity::Matrix root_surface_transformation;
  root_surface_transformation.Reset();

  auto frame =
      compositor_context.AcquireFrame(gpu_context, skity_canvas, nullptr,
                                      root_surface_transformation, false, true);
  skity_canvas->Clear(clay::Color::kTransparent());
  frame->Raster(*tree, true, nullptr, background_color);

  // Flush the canvas and snapshot a GPU-backed image.
  auto image = gpu_context->MakeSnapshot(std::move(render_target));
  if (!image) {
    FML_DLOG(ERROR) << "Screenshot: unable to snapshot offscreen RenderTarget";
    return nullptr;
  }

  // Make CPU-backed pixmap from the texture image.
  auto pixmap = image->ReadPixels(gpu_context);
  if (!pixmap) {
    FML_DLOG(ERROR) << "Screenshot: failed to read pixels from Image";
    return nullptr;
  }

  return skity::Image::MakeImage(pixmap);
}
#endif  // ENABLE_SKITY

// static
ScreenshotData TakeScreenshotWithBase64(LayerTree* layer_tree,
                                        ScreenshotData::ScreenshotType type,
                                        Surface* surface,
                                        CompositorContext* compositor_context,
                                        bool base64_encode,
                                        uint32_t background_color) {
  if (layer_tree == nullptr) {
    FML_DLOG(ERROR) << "Last layer tree was null when screenshot.";
    return {};
  }

  GrDataPtr data = nullptr;
#ifndef ENABLE_SKITY
  switch (type) {
    case ScreenshotData::ScreenshotType::SkiaPicture:
      data = ScreenshotLayerTreeAsPicture(layer_tree, *compositor_context,
                                          background_color);
      break;
    case ScreenshotData::ScreenshotType::UncompressedImage:
      data = ScreenshotLayerTreeAsImage(
          layer_tree, surface, *compositor_context, false, background_color);
      break;
    case ScreenshotData::ScreenshotType::CompressedImage:
      data = ScreenshotLayerTreeAsImage(
          layer_tree, surface, *compositor_context, true, background_color);
      break;
  }
#else
  switch (type) {
    case ScreenshotData::ScreenshotType::SkiaPicture:
      // Not supported.
      break;
    case ScreenshotData::ScreenshotType::UncompressedImage:
      data = ScreenshotLayerTreeAsImage(
          layer_tree, surface, *compositor_context, false, background_color);
      break;
    case ScreenshotData::ScreenshotType::CompressedImage:
      FML_UNIMPLEMENTED();
      break;
  }
#endif  // ENABLE_SKITY

  if (data == nullptr) {
    FML_DLOG(ERROR) << "Screenshot data was null.";
    return {};
  }

  if (base64_encode) {
    size_t b64_size =
        fml::Base64::Encode(DATA_GET_DATA(data), DATA_GET_SIZE(data), nullptr);
    GrDataPtr b64_data;
#ifndef ENABLE_SKITY
    b64_data = SkData::MakeUninitialized(b64_size);
#else
    if (b64_size == 0) {
      b64_data = skity::Data::MakeEmpty();
    } else {
      void* pixels = malloc(b64_size);
      b64_data = skity::Data::MakeFromMalloc(pixels, b64_size);
    }
#endif  // ENABLE_SKITY
    fml::Base64::Encode(DATA_GET_DATA(data), DATA_GET_SIZE(data),
                        DATA_GET_WRITABLE_DATA(b64_data));
    return ScreenshotData{b64_data, layer_tree->frame_size()};
  }

  return ScreenshotData{data, layer_tree->frame_size()};
}

// static
fml::RefPtr<PaintImage> TakeScreenshot(
    std::unique_ptr<clay::LayerTree> layer_tree, Surface* surface,
    CompositorContext* compositor_context) {
  auto image = TakeScreenshotWithOpaque(layer_tree.get(), surface,
                                        *compositor_context, false);
  if (image) {
#ifndef ENABLE_SKITY
    return PaintImage::Make(image);
#else
    return PaintImageSkity::Make(image);
#endif  // ENABLE_SKITY
  }
  return nullptr;
}

// static
fml::RefPtr<PaintImage> TakeScreenshot(GrPicturePtr picture, skity::Vec2 size) {
  FML_DCHECK(picture);
  // Use 16384 as a proxy for the maximum texture size for a GPU image.
  // This is meant to be large enough to avoid false positives in test contexts,
  // but not so artificially large to be completely unrealistic on any platform.
  // This limit is taken from the Metal specification. D3D, Vulkan, and GL
  // generally have lower limits.
  if (size.x > 16384 || size.y > 16384) {
    return nullptr;
  }
#ifndef ENABLE_SKITY
  sk_sp<SkSurface> surface =
      SkSurface::MakeRaster(SkImageInfo::MakeN32Premul(size.x, size.y));
  auto canvas = surface->getCanvas();
  canvas->clear(SK_ColorTRANSPARENT);
  picture->playback(canvas);

  sk_sp<SkImage> image = surface->makeImageSnapshot();
  return PaintImage::Make(image);
#else
  auto bitmap = std::make_unique<skity::Bitmap>(
      size.x, size.y, skity::AlphaType::kPremul_AlphaType);
  auto canvas = skity::Canvas::MakeSoftwareCanvas(bitmap.get());
  canvas->Clear(skity::Color_TRANSPARENT);
  picture->Draw(canvas.get());
  auto skity_image = skity::Image::MakeImage(bitmap->GetPixmap());
  return PaintImageSkity::Make(skity_image);
#endif  // ENABLE_SKITY
}

ScreenshotData::ScreenshotData() = default;

ScreenshotData::ScreenshotData(GrDataPtr p_data, const skity::Vec2& p_size)
    : data(std::move(p_data)), frame_size(p_size) {}

ScreenshotData::ScreenshotData(const ScreenshotData& other) = default;

ScreenshotData::~ScreenshotData() = default;

}  // namespace clay
