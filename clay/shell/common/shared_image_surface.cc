// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/shared_image_surface.h"

#include <cmath>
#include <utility>

#include "build/build_config.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#if OS_ANDROID
#include <GLES3/gl3.h>

#ifdef ENABLE_SKITY
#include "skity/gpu/gpu_context_gl.hpp"
#else
#include "third_party/skia/include/gpu/gl/GrGLTypes.h"
#endif
#endif

namespace clay {
#if (OS_MAC || OS_IOS) && (defined(ENABLE_SKITY) || defined(SK_METAL))
ClaySharedImageMetalRepresentationConfig GetMetalRepresentationConfig(
    GrContext* context);
GrSurfacePtr WrapMetalWriteTexture(GrContext* context, const skity::Vec2& size,
                                   uint32_t sample_count,
                                   const ClayMetalTexture& texture);
#endif

namespace {

#ifdef ENABLE_SKITY
// SurfaceFrame needs a GPUSurface interface for both GPU and software targets.
class SnapshotSurface final : public skity::GPUSurface {
 public:
  SnapshotSurface(GrContext* context, skity::Vec2 size, uint32_t sample_count)
      : context_(context), size_(size) {
    if (context_) {
      skity::GPURenderTargetDescriptor descriptor;
      descriptor.width = size.x;
      descriptor.height = size.y;
      descriptor.sample_count = sample_count;
      target_ = context_->CreateRenderTarget(descriptor);
    } else {
      bitmap_ = std::make_unique<skity::Bitmap>(
          size.x, size.y, skity::AlphaType::kPremul_AlphaType);
      if (bitmap_->GetPixelAddr()) {
        canvas_ = skity::Canvas::MakeSoftwareCanvas(bitmap_.get());
      }
    }
  }

  bool IsValid() const { return target_ || canvas_; }
  uint32_t GetWidth() const override { return size_.x; }
  uint32_t GetHeight() const override { return size_.y; }
  float ContentScale() const override { return 1.f; }
  GrCanvas* LockCanvas(bool) override {
    return target_ ? target_->GetCanvas() : canvas_.get();
  }
  void Flush() override {
    if (canvas_) canvas_->Flush();
  }
  std::shared_ptr<skity::Pixmap> ReadPixels(const skity::Rect&) override {
    return nullptr;
  }
  GrImagePtr Snapshot() {
    if (target_) return context_->MakeSnapshot(std::move(target_));
    Flush();
    return skity::Image::MakeImage(bitmap_->GetPixmap());
  }

 private:
  GrContext* context_;
  skity::Vec2 size_;
  std::unique_ptr<skity::GPURenderTarget> target_;
  std::unique_ptr<skity::Bitmap> bitmap_;
  std::unique_ptr<GrCanvas> canvas_;
};
#endif

#if OS_ANDROID
GrSurfacePtr WrapGLTarget(GrContext* context, const skity::Vec2& size,
                          uint32_t sample_count,
                          const ClayOpenGLFramebuffer& framebuffer,
                          [[maybe_unused]] SharedImageRepresentation& writer) {
#ifdef ENABLE_SKITY
  skity::GPUSurfaceDescriptorGL descriptor{};
  descriptor.backend = skity::GPUBackendType::kOpenGL;
  descriptor.width = size.x;
  descriptor.height = size.y;
  descriptor.sample_count = sample_count;
  descriptor.surface_type = skity::GLSurfaceType::kFramebuffer;
  descriptor.gl_id = framebuffer.name;
  descriptor.has_stencil_attachment = true;
  descriptor.surface_mode = sample_count > 1 ? skity::GLSurfaceMode::kBlit
                                             : skity::GLSurfaceMode::kDirect;
  return context->CreateSurface(&descriptor);
#else
  if (sample_count > 1) {
    auto texture = static_cast<GLImageRepresentation&>(writer).GetTexImage();
    if (!texture) return nullptr;
    GrGLTextureInfo info{texture->target, texture->name, texture->format};
    GrBackendTexture target(size.x, size.y, GrMipmapped::kNo, info);
    return SkSurface::MakeFromBackendTexture(
        context, target, kBottomLeft_GrSurfaceOrigin, sample_count,
        kRGBA_8888_SkColorType, nullptr, nullptr);
  }
  // The EGL backing provides a single-sample FBO with an 8-bit stencil.
  GrGLFramebufferInfo info{framebuffer.name, GL_RGBA8};
  GrBackendRenderTarget target(size.x, size.y, 1, 8, info);
  return SkSurface::MakeFromBackendRenderTarget(
      context, target, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType,
      nullptr, nullptr);
#endif
}
#endif

}  // namespace

bool SharedImageSurface::IsSupported(Surface* surface) {
  auto* context = surface ? surface->GetContext() : nullptr;
  if (!context) return false;
#if (OS_MAC || OS_IOS) && (defined(ENABLE_SKITY) || defined(SK_METAL))
#ifdef ENABLE_SKITY
  return context->GetBackendType() == skity::GPUBackendType::kMetal;
#else
  return context->backend() == GrBackendApi::kMetal;
#endif
#elif OS_ANDROID
#ifdef ENABLE_SKITY
  return context->GetBackendType() == skity::GPUBackendType::kOpenGL;
#else
  return context->backend() == GrBackendApi::kOpenGL;
#endif
#else
  return false;
#endif
}

SharedImageSurface::SharedImageSurface(Surface* display_surface,
                                       uint32_t sample_count,
                                       bool apply_root_transform,
                                       Storage storage)
    : display_surface_(display_surface),
      sample_count_(sample_count),
      apply_root_transform_(apply_root_transform),
      storage_(storage) {}
SharedImageSurface::~SharedImageSurface() = default;

bool SharedImageSurface::IsValid() { return display_surface_->IsValid(); }

std::unique_ptr<SurfaceFrame> SharedImageSurface::AcquireFrame(
    const skity::Vec2& frame_size) {
  backing_ = nullptr;
  image_ = nullptr;
  if (storage_ == Storage::kSharedImage && !IsSupported(display_surface_)) {
    return nullptr;
  }
  auto valid_size = [](const skity::Vec2& size) {
    return std::isfinite(size.x) && std::isfinite(size.y) && size.x > 0 &&
           size.y > 0 && size.x <= 16384 && size.y <= 16384 &&
           std::floor(size.x) == size.x && std::floor(size.y) == size.y;
  };
  if (!valid_size(frame_size)) return nullptr;
  const auto transform = GetRootTransformation();
  skity::Rect bounds;
  transform.MapRect(&bounds, skity::Rect::MakeWH(frame_size.x, frame_size.y));
  const skity::Vec2 size(bounds.Width(), bounds.Height());
  if (!valid_size(size)) {
    return nullptr;
  }
  auto current = MakeRenderContextCurrent();
  if (!current || !current->GetResult()) return nullptr;
  if (storage_ == Storage::kSnapshot) {
    return AcquireSnapshotFrame(frame_size, size, std::move(current));
  }

#if OS_ANDROID || \
    ((OS_MAC || OS_IOS) && (defined(ENABLE_SKITY) || defined(SK_METAL)))
  auto* context = GetContext();
  using BackingType = SharedImageBacking::BackingType;
  using PixelFormat = SharedImageBacking::PixelFormat;
  ClaySharedImageRepresentationConfig config{};
  config.struct_size = sizeof(config);
#if OS_ANDROID
  [[maybe_unused]] constexpr auto submit_gl_writes = &glFlush;
  constexpr auto type = BackingType::kAHardwareBuffer;
  constexpr auto format = PixelFormat::kRGBA8888;
  GLint limit = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
  if (size.x > limit || size.y > limit) return nullptr;
  config.type = kClaySharedImageRepresentationTypeGL;
  config.gl_config.struct_size = sizeof(config.gl_config);
#elif (OS_MAC || OS_IOS)
  [[maybe_unused]] constexpr void (*submit_gl_writes)() = nullptr;
  constexpr auto type = BackingType::kIOSurface;
  constexpr auto format = PixelFormat::kNative8888;
  config.type = kClaySharedImageRepresentationTypeMetal;
  config.metal_config = GetMetalRepresentationConfig(context);
  if (!config.metal_config.command_queue) return nullptr;
#endif
  auto backing = SharedImageBacking::Create(type, format, size, {});
  if (!backing || !backing->GetGFXHandle()) return nullptr;
  auto writer = backing->CreateRepresentation(&config);
  ClaySharedImageWriteResult result{};
  if (!writer || !writer->BeginWrite(&result)) return nullptr;
  GrSurfacePtr target;
  switch (result.type) {
#if OS_ANDROID
    case kClaySharedImageRepresentationTypeGL: {
      const auto& framebuffer = result.opengl_framebuffer;
      target = WrapGLTarget(context, size, sample_count_, framebuffer, *writer);
      framebuffer.destruction_callback(framebuffer.user_data);
      break;
    }
#elif (OS_MAC || OS_IOS)
    case kClaySharedImageRepresentationTypeMetal: {
      const auto& texture = result.metal_texture;
      target = WrapMetalWriteTexture(context, size, sample_count_, texture);
      texture.destruction_callback(texture.user_data);
      break;
    }
#endif
    default:
      break;
  }
  if (!target) {
    writer->EndWrite();
    return nullptr;
  }

  SurfaceFrame::FramebufferInfo framebuffer_info;
  framebuffer_info.supports_readback = true;
  auto frame = std::make_unique<SurfaceFrame>(
      std::move(target), framebuffer_info,
      [](SurfaceFrame& frame, GrCanvas* canvas) {
#ifdef ENABLE_SKITY
        CANVAS_FLUSH(canvas);
        frame.GetSurface()->Flush();
        if constexpr (submit_gl_writes != nullptr) {
          // Submit the GL writes before the compositor imports the buffer.
          submit_gl_writes();
        }
#else
        frame.GetSurface()->flushAndSubmit();
#endif
        return true;
      },
      [this, writer = std::move(writer)](const SurfaceFrame::SubmitInfo&) {
        if (!writer->EndWrite()) return false;
        backing_ = fml::Ref(writer->GetBacking());
        return true;
      },
      frame_size, std::move(current));
  if (!frame->GetCanvas()) return nullptr;
  // Preserve the platform surface's raster coordinate system, including the
  // transform used by preroll, raster caches and platform-view slices.
  CANVAS_CONCAT(frame->GetCanvas(), transform);
  return frame;
#else
  return nullptr;
#endif
}

std::unique_ptr<SurfaceFrame> SharedImageSurface::AcquireSnapshotFrame(
    const skity::Vec2& frame_size, const skity::Vec2& size,
    std::unique_ptr<GLContextResult> current) {
  auto* context = GetContext();
#ifndef ENABLE_SKITY
  auto info = SkImageInfo::Make(size.x, size.y, kRGBA_8888_SkColorType,
                                kPremul_SkAlphaType);
  auto target = context ? SkSurface::MakeRenderTarget(
                              context, skgpu::Budgeted::kYes, info,
                              sample_count_, kTopLeft_GrSurfaceOrigin, nullptr)
                        : SkSurface::MakeRaster(info);
  if (!target) return nullptr;
#else
  auto target = std::make_shared<SnapshotSurface>(context, size, sample_count_);
  if (!target->IsValid()) return nullptr;
#endif
  SurfaceFrame::FramebufferInfo framebuffer_info;
  framebuffer_info.supports_readback = true;
  auto frame = std::make_unique<SurfaceFrame>(
      target, framebuffer_info, [](SurfaceFrame&, GrCanvas*) { return true; },
      [this, target](const SurfaceFrame::SubmitInfo&) {
#ifndef ENABLE_SKITY
        image_ = target->makeImageSnapshot();
#else
        image_ = target->Snapshot();
#endif
        return static_cast<bool>(image_);
      },
      frame_size, std::move(current));
  if (!frame->GetCanvas()) return nullptr;
  CANVAS_CONCAT(frame->GetCanvas(), GetRootTransformation());
  return frame;
}

skity::Matrix SharedImageSurface::GetRootTransformation() const {
  return apply_root_transform_ ? display_surface_->GetRootTransformation()
                               : skity::Matrix();
}
GrContext* SharedImageSurface::GetContext() {
  return display_surface_->GetContext();
}
GrContextPtr SharedImageSurface::GetContextPtr() {
  return display_surface_->GetContextPtr();
}
std::unique_ptr<GLContextResult>
SharedImageSurface::MakeRenderContextCurrent() {
  return display_surface_->MakeRenderContextCurrent();
}
bool SharedImageSurface::ClearRenderContext() {
  return display_surface_->ClearRenderContext();
}
bool SharedImageSurface::AllowsDrawingWhenGpuDisabled() const {
  return display_surface_->AllowsDrawingWhenGpuDisabled();
}
bool SharedImageSurface::EnableRasterCache() const {
  return display_surface_->EnableRasterCache();
}

}  // namespace clay
