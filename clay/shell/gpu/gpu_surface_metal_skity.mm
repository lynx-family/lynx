// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/gpu/gpu_surface_metal_skity.h"

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/fml/platform/darwin/cf_utils.h"
#include "clay/fml/platform/darwin/scoped_nsobject.h"
#include "clay/shell/gpu/gpu_surface_metal_delegate.h"
#include "skity/gpu/gpu_context_mtl.h"

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace clay {

namespace {
std::shared_ptr<skity::GPUSurface> CreateSurfaceFromMetalTexture(
    std::shared_ptr<skity::GPUContext> context, const skity::Vec2& size, id<MTLTexture> texture,
    uint32_t sample_cnt) {
  skity::GPUSurfaceDescriptorMTL descriptor = {
      {skity::GPUBackendType::kMetal, static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y),
       sample_cnt},
      skity::MTLSurfaceType::kTexture,
      nil,
      texture};
  return context->CreateSurface(&descriptor);
}
}  // namespace

GPUSurfaceMetalSkity::GPUSurfaceMetalSkity(GPUSurfaceMetalDelegate* delegate,
                                           std::shared_ptr<skity::GPUContext> context,
                                           MsaaSampleCount msaa_samples, bool render_to_surface)
    : delegate_(delegate),
      render_target_type_(delegate->GetRenderTargetType()),
      context_(std::move(context)),
      msaa_samples_(msaa_samples),
      render_to_surface_(render_to_surface) {
  context_->SetEnableSimpleShapePipeline(true);
}

GPUSurfaceMetalSkity::~GPUSurfaceMetalSkity() = default;

// |Surface|
bool GPUSurfaceMetalSkity::IsValid() { return context_ != nullptr; }

// |Surface|
std::unique_ptr<SurfaceFrame> GPUSurfaceMetalSkity::AcquireFrame(const skity::Vec2& frame_size) {
  if (!IsValid()) {
    FML_LOG(ERROR) << "Metal surface was invalid.";
    return nullptr;
  }

  if (frame_size.x == 0 && frame_size.y == 0) {
    FML_LOG(ERROR) << "Metal surface was asked for an empty frame.";
    return nullptr;
  }

  if (!render_to_surface_) {
    return std::make_unique<SurfaceFrame>(
        nullptr, SurfaceFrame::FramebufferInfo(),
        [](const SurfaceFrame& surface_frame, skity::Canvas* canvas) { return true; },
        [](const SurfaceFrame::SubmitInfo& surface_frame) { return true; }, frame_size);
  }

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

std::unique_ptr<SurfaceFrame> GPUSurfaceMetalSkity::AcquireFrameFromCAMetalLayer(
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

  auto surface = CreateSurfaceFromMetalTexture(context_, frame_info, drawable.get().texture,
                                               static_cast<uint32_t>(msaa_samples_));
  if (!surface) {
    FML_LOG(ERROR) << "Could not create gpuSurface from the CAMetalLayer.";
    return nullptr;
  }

  auto encode_callback = [this, drawable, surface](SurfaceFrame& surface_frame,
                                                   skity::Canvas* canvas) -> bool {
    if (!canvas) {
      FML_LOG(ERROR) << "skity Canvas is null.";
      return false;
    }
    TRACE_EVENT("clay", "skity::Canvas::Flush");
    canvas->Flush();
    surface->Flush();

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

    return true;
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

  return std::make_unique<SurfaceFrame>(surface, framebuffer_info, encode_callback, submit_callback,
                                        frame_info);
}

std::unique_ptr<SurfaceFrame> GPUSurfaceMetalSkity::AcquireFrameFromMTLTexture(
    const skity::Vec2& frame_info) {
  GPUMTLTextureInfo texture = delegate_->GetMTLTexture(frame_info);
  id<MTLTexture> mtl_texture = (__bridge id<MTLTexture>)(texture.texture);

  if (!mtl_texture) {
    FML_LOG(ERROR) << "Invalid MTLTexture given by the embedder.";
    return nullptr;
  }

  auto surface = CreateSurfaceFromMetalTexture(context_, frame_info, mtl_texture,
                                               static_cast<uint32_t>(msaa_samples_));
  if (!surface) {
    FML_LOG(ERROR) << "Could not create gpuSurface from the CAMetalLayer.";
    return nullptr;
  }

  auto encode_callback = [this, texture, surface](SurfaceFrame& surface_frame,
                                                  skity::Canvas* canvas) -> bool {
    if (!canvas) {
      FML_LOG(ERROR) << "skity Canvas is null.";
      return false;
    }
    TRACE_EVENT("clay", "skity::Canvas::Flush");
    canvas->Flush();
    surface->Flush();

    if (delegate_->EnablePartialRepaint()) {
      for (auto& entry : damage_) {
        if (entry.first != texture.texture_id) {
          // Accumulate damage for other framebuffers
          if (surface_frame.submit_info().frame_damage) {
            entry.second.Join(*surface_frame.submit_info().frame_damage);
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
    auto i = damage_.find(texture.texture_id);
    if (i != damage_.end()) {
      framebuffer_info.existing_damage = i->second;
    }
    framebuffer_info.supports_partial_repaint = true;
  }

  return std::make_unique<SurfaceFrame>(surface, framebuffer_info, encode_callback, submit_callback,
                                        frame_info);
}

// |Surface|
skity::Matrix GPUSurfaceMetalSkity::GetRootTransformation() const {
  // This backend does not currently support root surface transformations. Just
  // return identity.
  return {};
}

// |Surface|
skity::GPUContext* GPUSurfaceMetalSkity::GetContext() { return context_.get(); }

// |Surface|
std::unique_ptr<GLContextResult> GPUSurfaceMetalSkity::MakeRenderContextCurrent() {
  // This backend has no such concept.
  return std::make_unique<GLContextDefaultResult>(true);
}

bool GPUSurfaceMetalSkity::AllowsDrawingWhenGpuDisabled() const {
  return delegate_->AllowsDrawingWhenGpuDisabled();
}

}  // namespace clay
