// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/gpu/gpu_surface_metal_skia.h"

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/flow/surface_frame.h"
#include "clay/fml/platform/darwin/cf_utils.h"
#include "clay/fml/platform/darwin/scoped_nsobject.h"
#include "clay/shell/gpu/gpu_surface_metal_delegate.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpace.h"
// #include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkSize.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSurfaceProps.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/ports/SkCFObject.h"

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace clay {

namespace {
sk_sp<SkSurface> CreateSurfaceFromMetalTexture(GrDirectContext* context, id<MTLTexture> texture,
                                               GrSurfaceOrigin origin, MsaaSampleCount sample_cnt,
                                               SkColorType color_type,
                                               sk_sp<SkColorSpace> color_space,
                                               const SkSurfaceProps* props,
                                               SkSurface::TextureReleaseProc release_proc,
                                               SkSurface::ReleaseContext release_context) {
  GrMtlTextureInfo info;
  info.fTexture.reset((__bridge_retained GrMTLHandle)texture);
  GrBackendTexture backend_texture(texture.width, texture.height, GrMipmapped::kNo, info);
  return SkSurface::MakeFromBackendTexture(
      context, backend_texture, origin, static_cast<int>(sample_cnt), color_type,
      std::move(color_space), props, release_proc, release_context);
}
}  // namespace

GPUSurfaceMetalSkia::GPUSurfaceMetalSkia(GPUSurfaceMetalDelegate* delegate,
                                         sk_sp<GrDirectContext> context,
                                         MsaaSampleCount msaa_samples, bool render_to_surface)
    : delegate_(delegate),
      render_target_type_(delegate->GetRenderTargetType()),
      context_(std::move(context)),
      msaa_samples_(msaa_samples),
      render_to_surface_(render_to_surface) {}

GPUSurfaceMetalSkia::~GPUSurfaceMetalSkia() = default;

// |Surface|
bool GPUSurfaceMetalSkia::IsValid() { return context_ != nullptr; }

void GPUSurfaceMetalSkia::PrecompileKnownSkSLsIfNecessary() {
  auto* current_context = GetContext();
  if (current_context == precompiled_sksl_context_) {
    // Known SkSLs have already been prepared in this context.
    return;
  }
  precompiled_sksl_context_ = current_context;
  clay::PersistentCache::GetCacheForProcess()->PrecompileKnownSkSLs(precompiled_sksl_context_);
}

// |Surface|
std::unique_ptr<SurfaceFrame> GPUSurfaceMetalSkia::AcquireFrame(const skity::Vec2& frame_size) {
  if (!IsValid()) {
    FML_LOG(ERROR) << "Metal surface was invalid.";
    return nullptr;
  }

  if (frame_size.x == 0 & frame_size.y == 0) {
    FML_LOG(ERROR) << "Metal surface was asked for an empty frame.";
    return nullptr;
  }

  if (!render_to_surface_) {
    return std::make_unique<SurfaceFrame>(
        nullptr, SurfaceFrame::FramebufferInfo(),
        [](const SurfaceFrame& surface_frame, SkCanvas* canvas) { return true; },
        [](const SurfaceFrame::SubmitInfo&) { return true; },  //
        frame_size);
  }

  PrecompileKnownSkSLsIfNecessary();

  switch (render_target_type_) {
    case MTLRenderTargetType::kCAMetalLayer:
      return AcquireFrameFromCAMetalLayer(frame_size);
    case MTLRenderTargetType::kMTLTexture:
      return AcquireFrameFromMTLTexture(frame_size);
    default:
      FML_CHECK(false) << "Unknown MTLRenderTargetType type.";
  }

  return nullptr;
}

std::unique_ptr<SurfaceFrame> GPUSurfaceMetalSkia::AcquireFrameFromCAMetalLayer(
    const skity::Vec2& frame_info) {
  auto layer = delegate_->GetCAMetalLayer(frame_info);
  if (!layer) {
    FML_LOG(ERROR) << "Invalid CAMetalLayer given by the embedder.";
    return nullptr;
  }

  auto* mtl_layer = (__bridge CAMetalLayer*)layer;
  // Get the drawable eagerly, we will need texture object to identify target framebuffer
  fml::scoped_nsprotocol<id<CAMetalDrawable>> drawable([mtl_layer nextDrawable]);

  if (!drawable.get()) {
    FML_LOG(ERROR) << "Could not obtain drawable from the metal layer.";
    return nullptr;
  }

  auto surface = CreateSurfaceFromMetalTexture(context_.get(), drawable.get().texture,
                                               kTopLeft_GrSurfaceOrigin,  // origin
                                               msaa_samples_,             // sample count
                                               kBGRA_8888_SkColorType,    // color type
                                               nullptr,                   // colorspace
                                               nullptr,                   // surface properties
                                               nullptr,                   // release proc
                                               nullptr                    // release context
  );

  if (!surface) {
    FML_LOG(ERROR) << "Could not create the SkSurface from the CAMetalLayer.";
    return nullptr;
  }

  auto encode_callback = [this, drawable, mtl_layer](const SurfaceFrame& surface_frame,
                                                     SkCanvas* canvas) -> bool {
    if (canvas == nullptr) {
      FML_DLOG(ERROR) << "Canvas not available.";
      return false;
    }

    {
      TRACE_EVENT("clay", "SkCanvas::Flush");
      canvas->flush();
    }

    if (delegate_->EnablePartialRepaint()) {
      intptr_t texture = reinterpret_cast<intptr_t>(drawable.get().texture);
      for (auto& entry : damage_) {
        if (entry.first != texture) {
          // Accumulate damage for other framebuffers
          if (surface_frame.submit_info().frame_damage) {
            entry.second.Join(*surface_frame.submit_info().frame_damage);
          }
        }
      }
      // Reset accumulated damage for current framebuffer
      damage_[texture] = skity::Rect::MakeEmpty();
    }

    return delegate_->PreparePresent((__bridge GrMTLHandle)drawable.get());
  };

  // submit_callback must capture objects which is safe to use in platform thread
  auto submit_callback = [drawable,
                          mtl_layer](const SurfaceFrame::SubmitInfo& submit_info) -> bool {
    TRACE_EVENT("clay", "GPUSurfaceMetal::Submit");
    mtl_layer.presentsWithTransaction = submit_info.present_with_transaction;
    [drawable present];
    return true;
  };

  SurfaceFrame::FramebufferInfo framebuffer_info;
  framebuffer_info.supports_readback = true;

  if (delegate_->EnablePartialRepaint()) {
    // Provide accumulated damage to rasterizer (area in current framebuffer that lags behind
    // front buffer)
    intptr_t texture = reinterpret_cast<intptr_t>(drawable.get().texture);
    auto i = damage_.find(texture);
    if (i != damage_.end()) {
      framebuffer_info.existing_damage = i->second;
    }
    framebuffer_info.supports_partial_repaint = true;
  }

  return std::make_unique<SurfaceFrame>(std::move(surface), framebuffer_info, encode_callback,
                                        submit_callback, frame_info);
}

std::unique_ptr<SurfaceFrame> GPUSurfaceMetalSkia::AcquireFrameFromMTLTexture(
    const skity::Vec2& frame_info) {
  GPUMTLTextureInfo texture = delegate_->GetMTLTexture(frame_info);
  id<MTLTexture> mtl_texture = (__bridge id<MTLTexture>)(texture.texture);

  if (!mtl_texture) {
    FML_LOG(ERROR) << "Invalid MTLTexture given by the embedder.";
    return nullptr;
  }

  sk_sp<SkSurface> surface = CreateSurfaceFromMetalTexture(
      context_.get(), mtl_texture, kTopLeft_GrSurfaceOrigin, msaa_samples_, kBGRA_8888_SkColorType,
      nullptr, nullptr, static_cast<SkSurface::TextureReleaseProc>(texture.destruction_callback),
      texture.destruction_context);

  if (!surface) {
    FML_LOG(ERROR) << "Could not create the SkSurface from the metal texture.";
    return nullptr;
  }

  SurfaceFrame::EncodeCallback encode_callback = [this, texture](const SurfaceFrame& surface_frame,
                                                                 SkCanvas* canvas) -> bool {
    if (canvas == nullptr) {
      FML_DLOG(ERROR) << "Canvas not available.";
      return false;
    }

    {
      TRACE_EVENT("clay", "SkCanvas::Flush");
      canvas->flush();
    }

    if (delegate_->EnablePartialRepaint()) {
      for (auto& [texture_id, damage] : damage_) {
        if (texture_id != texture.texture_id) {
          // Accumulate damage for other framebuffers
          if (surface_frame.submit_info().frame_damage) {
            damage.Join(*surface_frame.submit_info().frame_damage);
          }
        }
      }
      // Reset accumulated damage for current framebuffer
      damage_[texture.texture_id] = skity::Rect::MakeEmpty();
    }

    return true;
  };

  // This code path is only used on Mac platform, which ensures rasterizer teardown before shell,
  // thus safe to cauptre this
  auto submit_callback = [this, texture](const SurfaceFrame::SubmitInfo&) -> bool {
    TRACE_EVENT("clay", "GPUSurfaceMetal::PresentTexture");
    return delegate_->PresentTexture(texture);
  };

  SurfaceFrame::FramebufferInfo framebuffer_info;
  framebuffer_info.supports_readback = true;

  if (size_ != frame_info) {
    damage_.clear();
    size_ = frame_info;
  }

  if (delegate_->EnablePartialRepaint()) {
    // Provide accumulated damage to rasterizer (area in current framebuffer that lags behind
    // front buffer)
    auto i = damage_.find(texture.texture_id);
    if (i != damage_.end()) {
      framebuffer_info.existing_damage = i->second;
    }
    framebuffer_info.supports_partial_repaint = true;
  }

  return std::make_unique<SurfaceFrame>(std::move(surface), framebuffer_info, encode_callback,
                                        submit_callback, frame_info);
}

// |Surface|
skity::Matrix GPUSurfaceMetalSkia::GetRootTransformation() const {
  // This backend does not currently support root surface transformations. Just
  // return identity.
  return {};
}

// |Surface|
GrDirectContext* GPUSurfaceMetalSkia::GetContext() { return context_.get(); }

// |Surface|
std::unique_ptr<GLContextResult> GPUSurfaceMetalSkia::MakeRenderContextCurrent() {
  // A context may either be necessary to render to the surface or to snapshot an offscreen
  // surface. Either way, SkSL precompilation must be attempted.
  PrecompileKnownSkSLsIfNecessary();

  // This backend has no such concept.
  return std::make_unique<GLContextDefaultResult>(true);
}

bool GPUSurfaceMetalSkia::AllowsDrawingWhenGpuDisabled() const {
  return delegate_->AllowsDrawingWhenGpuDisabled();
}

}  // namespace clay
