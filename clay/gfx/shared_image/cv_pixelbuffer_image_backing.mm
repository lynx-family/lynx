// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/cv_pixelbuffer_image_backing.h"

#include <CoreVideo/CVPixelBuffer.h>
#import <CoreVideo/CoreVideo.h>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/mtl_image_representation.h"
#include "clay/gfx/shared_image/utils/image_utils.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

#if OS_IOS
#include "clay/gfx/shared_image/eagl_image_representation.h"
#endif

#if ENABLE_SKITY
#include "clay/gfx/shared_image/skity_mtl_image_representation.h"
#include "skity/gpu/gpu_context.hpp"
#else
#if OS_MAC
#include "clay/gfx/shared_image/cgl_image_representation.h"
#endif

#if SKIA_ENABLE_GL
#include "clay/gfx/shared_image/skia_gl_image_representation.h"
#endif
#if SKIA_ENABLE_METAL
#include "clay/gfx/shared_image/skia_mtl_image_representation.h"
#endif
#endif

namespace clay {

fml::CFRef<CVPixelBufferRef> CVPixelBufferImageBacking::CreateCVPixelBuffer(
    SharedImageBacking::PixelFormat pixel_format, skity::Vec2 size) {
  // TODO support other formats
  OSType format = kCVPixelFormatType_32BGRA;
  NSDictionary* options = @{
    (id)kCVPixelBufferOpenGLCompatibilityKey : @(true),
    (id)kCVPixelBufferMetalCompatibilityKey : @(true),
  };

  CVPixelBufferRef pixel_buffer = nullptr;
  CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, size.x, size.y, format,
                                        (__bridge CFDictionaryRef)options, &pixel_buffer);
  if (status != kCVReturnSuccess || pixel_buffer == nullptr) {
    FML_LOG(ERROR) << "Failed to create CVPixelBuffer";
  }
  return pixel_buffer;
}

CVPixelBufferImageBacking::CVPixelBufferImageBacking(PixelFormat pixel_format, skity::Vec2 size,
                                                     std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBacking(pixel_format, size) {
  if (gfx_handle) {
    pixel_buffer_.Reset((CVPixelBufferRef)(*gfx_handle));
  } else {
    pixel_buffer_ = CreateCVPixelBuffer(pixel_format, size);
  }
}

SharedImageBacking::BackingType CVPixelBufferImageBacking::GetType() const {
  return BackingType::kCVPixelBuffer;
}

GraphicsMemoryHandle CVPixelBufferImageBacking::GetGFXHandle() const { return pixel_buffer_; }

fml::RefPtr<SharedImageRepresentation> CVPixelBufferImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  FML_CHECK(config->struct_size == sizeof(ClaySharedImageRepresentationConfig));
  switch (config->type) {
    case kClaySharedImageRepresentationTypeGL:
#if OS_MAC
      return fml::MakeRefCounted<CGLImageRepresentation>(fml::Ref(this));
#elif OS_IOS
      return fml::MakeRefCounted<EAGLImageRepresentation>(fml::Ref(this));
#else
      break;
#endif
    case kClaySharedImageRepresentationTypeMetal:
      FML_CHECK(config->metal_config.struct_size ==
                sizeof(ClaySharedImageMetalRepresentationConfig));
      if (@available(macos 10.6, ios 11.0, tvos 11.0, *)) {
        return fml::MakeRefCounted<MTLImageRepresentation>(
            (__bridge id<MTLDevice>)config->metal_config.device,
            (__bridge id<MTLCommandQueue>)config->metal_config.command_queue, fml::Ref(this));
      }
      break;
    default:
      break;
  }

  FML_LOG(ERROR) << "Unable to call CVPixelBufferImageBacking::CreateRepresentation with type: "
                 << static_cast<uint32_t>(config->type);
  return nullptr;
}

#ifndef ENABLE_SKITY
fml::RefPtr<SkiaImageRepresentation> CVPixelBufferImageBacking::CreateSkiaRepresentation(
    GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
#if SKIA_ENABLE_GL
    case GrBackendApi::kOpenGL:
#if OS_MAC
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<CGLImageRepresentation>(fml::Ref(this)));
#elif OS_IOS
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<EAGLImageRepresentation>(fml::Ref(this)));
#endif
      break;
#endif
#if SKIA_ENABLE_METAL
    case GrBackendApi::kMetal:
      if (@available(macos 10.6, ios 11.0, tvos 11.0, *)) {
        return fml::MakeRefCounted<SkiaMTLImageRepresentation>(gr_context, fml::Ref(this));
      }
      break;
#endif
    default:
      break;
  }

  FML_LOG(ERROR)
      << "Unable to call CVPixelBufferImageBacking::CreateSkiaRepresentation with backend: "
      << static_cast<uint32_t>(gr_context->backend());
  return nullptr;
}
#else
fml::RefPtr<SkityImageRepresentation> CVPixelBufferImageBacking::CreateSkityRepresentation(
    skity::GPUContext* skity_context) {
  switch (skity_context->GetBackendType()) {
    case skity::GPUBackendType::kMetal:
      if (@available(macos 10.6, ios 11.0, tvos 11.0, *)) {
        return fml::MakeRefCounted<SkityMTLImageRepresentation>(skity_context, fml::Ref(this));
      }
      break;
    default:
      break;
  }

  FML_LOG(ERROR)
      << "Unable to call CVPixelBufferImageBacking::CreateSkityRepresentation with backend: "
      << static_cast<uint32_t>(skity_context->GetBackendType());
  return nullptr;
}
#endif  // ENABLE_SKITY

#ifndef ENABLE_SKITY
bool CVPixelBufferImageBacking::ReadbackToMemory(const SkPixmap* pixmaps, uint32_t planes) {
  CVReturn lockStatus = CVPixelBufferLockBaseAddress(pixel_buffer_, 0);
  if (lockStatus != kCVReturnSuccess) {
    FML_LOG(ERROR) << "Failed to lock CVPixelBuffer";
    return false;
  }

  void* baseAddress = CVPixelBufferGetBaseAddress(pixel_buffer_);
  size_t stride = CVPixelBufferGetBytesPerRow(pixel_buffer_);
  size_t width = CVPixelBufferGetWidth(pixel_buffer_);
  size_t height = CVPixelBufferGetHeight(pixel_buffer_);
  OSType pixel_format = CVPixelBufferGetPixelFormatType(pixel_buffer_);
  uint8_t* typeAsChars = (uint8_t*)&pixel_format;
  FML_LOG(INFO) << "CVPixelBufferImageBacking::ReadbackToMemory: "
                << "pixel_format: " << typeAsChars[3] << typeAsChars[2] << typeAsChars[1]
                << typeAsChars[0] << " width: " << width << " height: " << height
                << " stride: " << stride;
  size_t source_offset = 0;
  for (uint32_t plane = 0; plane < planes; ++plane) {
    auto& pixmap = pixmaps[plane];
    uint8_t* dest_memory = static_cast<uint8_t*>(pixmap.writable_addr());
    const size_t dest_stride = pixmap.rowBytes();
    const uint8_t* source_memory = static_cast<uint8_t*>(baseAddress) + source_offset;
    const size_t source_stride = stride;

    CopyPlane(source_memory, source_stride, dest_memory, dest_stride, pixmap.info().minRowBytes(),
              size_.y);

    source_offset += stride * size_.y;
  }
  CVPixelBufferUnlockBaseAddress(pixel_buffer_, kCVPixelBufferLock_ReadOnly);
  return true;
}
#endif  // ENABLE_SKITY

}  // namespace clay
