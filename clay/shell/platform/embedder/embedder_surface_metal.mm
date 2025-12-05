// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "clay/shell/platform/embedder/embedder_surface_metal.h"

#import "FlutterMacros.h"
#include "clay/fml/logging.h"
#include "clay/shell/gpu/gpu_surface_metal_delegate.h"

#ifndef ENABLE_SKITY
#include "clay/shell/gpu/gpu_surface_metal_skia.h"
#import "clay/shell/platform/darwin/graphics/FlutterDarwinContextMetalSkia.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#else
#include "clay/shell/gpu/gpu_surface_metal_skity.h"
#include "clay/shell/platform/darwin/graphics/FlutterDarwinContextMetalSkity.h"
#endif  // ENABLE_SKITY

FLUTTER_ASSERT_ARC
namespace clay {

EmbedderSurfaceMetal::EmbedderSurfaceMetal(EmbedderSurfaceMetalDelegate* delegate)
    : GPUSurfaceMetalDelegate(MTLRenderTargetType::kMTLTexture), delegate_(delegate) {
#ifndef ENABLE_SKITY
  main_context_ = [FlutterDarwinContextMetalSkia
      createGrContext:(__bridge id<MTLDevice>)delegate_->GetMTLDevice()
         commandQueue:(__bridge id<MTLCommandQueue>)delegate_->GetMTLCommandQueue()];
#else
  main_context_ = [FlutterDarwinContextMetalSkity
      createGPUContext:(__bridge id<MTLDevice>)delegate_->GetMTLDevice()
          commandQueue:(__bridge id<MTLCommandQueue>)delegate_->GetMTLCommandQueue()];
#endif
  valid_ = main_context_ != nullptr;
}

GPUCAMetalLayerHandle EmbedderSurfaceMetal::GetCAMetalLayer(const skity::Vec2& frame_size) const {
  return nullptr;
}

clay::GrContextPtr EmbedderSurfaceMetal::GetMainGrContext() { return main_context_; }

EmbedderSurfaceMetal::~EmbedderSurfaceMetal() = default;

bool EmbedderSurfaceMetal::IsValid() const { return valid_; }

std::unique_ptr<Surface> EmbedderSurfaceMetal::CreateGPUSurface(clay::GrContext* context)
    API_AVAILABLE(ios(13.0)) {
#ifndef ENABLE_SKITY
  if (@available(iOS 13.0, *)) {
  } else {
    return nullptr;
  }
  if (!IsValid()) {
    return nullptr;
  }

  auto surface = std::make_unique<GPUSurfaceMetalSkia>(
      this, context ? sk_ref_sp(context) : main_context_, MsaaSampleCount::kNone, true);

  if (!surface->IsValid()) {
    return nullptr;
  }

  return surface;
#else
  if (!IsValid()) {
    return nullptr;
  }
  auto surface = std::make_unique<GPUSurfaceMetalSkity>(
      this, context ? std::shared_ptr<clay::GrContext>(context) : main_context_,
      MsaaSampleCount::kNone, true);

  if (!surface->IsValid()) {
    return nullptr;
  }

  return surface;
#endif  // ENABLE_SKITY
}

GPUMTLTextureInfo EmbedderSurfaceMetal::GetMTLTexture(const skity::Vec2& frame_info) const {
  return delegate_->GetMTLTexture(frame_info);
}

bool EmbedderSurfaceMetal::PresentTexture(GPUMTLTextureInfo texture) const {
  return delegate_->PresentTexture(texture);
}

bool EmbedderSurfaceMetal::EnablePartialRepaint() const {
  return delegate_->EnablePartialRepaint();
}

}  // namespace clay
