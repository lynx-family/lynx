// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/shared_image_backing.h"

#if defined(ENABLE_SKITY) || defined(SK_METAL)
#ifdef ENABLE_SKITY
#include "skity/gpu/gpu_context_mtl.h"
#else
#include "third_party/skia/src/gpu/ganesh/GrDirectContextPriv.h"  // nogncheck
#include "third_party/skia/src/gpu/ganesh/mtl/GrMtlGpu.h"         // nogncheck
#endif

namespace clay {

ClaySharedImageMetalRepresentationConfig GetMetalRepresentationConfig(GrContext* context) {
#ifdef ENABLE_SKITY
  auto queue = skity::MTLContextGetCommandQueue(context);
#else
  auto* gpu = static_cast<GrMtlGpu*>(context->priv().getGpu());
  auto queue = gpu ? gpu->queue() : nil;
#endif
  return {sizeof(ClaySharedImageMetalRepresentationConfig), (__bridge void*)queue.device,
          (__bridge void*)queue};
}

GrSurfacePtr WrapMetalWriteTexture(GrContext* context, const skity::Vec2& size,
                                   uint32_t sample_count, const ClayMetalTexture& texture) {
  auto metal_texture = (__bridge id<MTLTexture>)texture.texture;
#ifdef ENABLE_SKITY
  skity::GPUSurfaceDescriptorMTL descriptor{};
  descriptor.backend = skity::GPUBackendType::kMetal;
  descriptor.width = size.x;
  descriptor.height = size.y;
  descriptor.sample_count = sample_count;
  descriptor.surface_type = skity::MTLSurfaceType::kTexture;
  descriptor.texture = metal_texture;
  return context->CreateSurface(&descriptor);
#else
  GrMtlTextureInfo info;
  info.fTexture.reset((__bridge_retained GrMTLHandle)metal_texture);
  GrBackendTexture target(size.x, size.y, GrMipmapped::kNo, info);
  // Match the origin of Clay's Skia Metal read representation.
  return SkSurface::MakeFromBackendTexture(context, target, kBottomLeft_GrSurfaceOrigin,
                                           sample_count, kBGRA_8888_SkColorType, nullptr, nullptr);
#endif
}

}  // namespace clay
#endif
