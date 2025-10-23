// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/iosurface_image_backing.h"

#import <Foundation/Foundation.h>

#include "build/build_config.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/mtl_image_representation.h"
#include "clay/gfx/shared_image/utils/image_utils.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

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

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace clay {

namespace {

struct API_AVAILABLE(macos(10.6), ios(11.0), tvos(11.0)) ScopedIOSurfaceLock {
  ScopedIOSurfaceLock(IOSurfaceRef iosurface, IOSurfaceLockOptions options)
      : io_surface_(iosurface), options_(options) {
    kern_return_t r = IOSurfaceLock(io_surface_, options_, nullptr);
    FML_CHECK(KERN_SUCCESS == r);
  }

  ~ScopedIOSurfaceLock() {
    kern_return_t r = IOSurfaceUnlock(io_surface_, options_, nullptr);
    FML_CHECK(KERN_SUCCESS == r);
  }

  ScopedIOSurfaceLock(const ScopedIOSurfaceLock&) = delete;
  ScopedIOSurfaceLock& operator=(const ScopedIOSurfaceLock&) = delete;

 private:
  IOSurfaceRef io_surface_;
  IOSurfaceLockOptions options_;
};
}  // namespace

fml::CFRef<IOSurfaceRef> IOSurfaceImageBacking::CreateIOSurface(
    SharedImageBacking::PixelFormat pixel_format, skity::Vec2 size) {
  // TODO support other formats
  unsigned pixelFormat = 'BGRA';
  unsigned bytesPerElement = 4;

  size_t bytesPerRow = IOSurfaceAlignProperty(kIOSurfaceBytesPerRow, size.x * bytesPerElement);
  size_t totalBytes = IOSurfaceAlignProperty(kIOSurfaceAllocSize, size.y * bytesPerRow);
  NSDictionary* options = @{
    (id)kIOSurfaceWidth : @(size.x),
    (id)kIOSurfaceHeight : @(size.y),
    (id)kIOSurfacePixelFormat : @(pixelFormat),
    (id)kIOSurfaceBytesPerElement : @(bytesPerElement),
    (id)kIOSurfaceBytesPerRow : @(bytesPerRow),
    (id)kIOSurfaceAllocSize : @(totalBytes),
    (id)kIOSurfaceIsGlobal : @YES,
  };

  return IOSurfaceCreate((CFDictionaryRef)options);
}

IOSurfaceImageBacking::IOSurfaceImageBacking(PixelFormat pixel_format, skity::Vec2 size,
                                             std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBacking(pixel_format, size) {
  if (gfx_handle) {
    io_surface_.Reset((IOSurfaceRef)CFRetain(*gfx_handle));
  } else {
    io_surface_ = CreateIOSurface(pixel_format, size);
  }
}

SharedImageBacking::BackingType IOSurfaceImageBacking::GetType() const {
  return BackingType::kIOSurface;
}

GraphicsMemoryHandle IOSurfaceImageBacking::GetGFXHandle() const { return io_surface_; }

fml::RefPtr<SharedImageRepresentation> IOSurfaceImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  FML_CHECK(config->struct_size == sizeof(ClaySharedImageRepresentationConfig));
  switch (config->type) {
#if OS_MAC
    case kClaySharedImageRepresentationTypeGL:
      return fml::MakeRefCounted<CGLImageRepresentation>(fml::Ref(this));
#endif
    case kClaySharedImageRepresentationTypeMetal:
      FML_CHECK(config->metal_config.struct_size ==
                sizeof(ClaySharedImageMetalRepresentationConfig));
      return fml::MakeRefCounted<MTLImageRepresentation>(
          (__bridge id<MTLDevice>)config->metal_config.device,
          (__bridge id<MTLCommandQueue>)config->metal_config.command_queue, fml::Ref(this));
    default:
      break;
  }

  FML_LOG(ERROR) << "Unable to call IOSurfaceImageBacking::CreateRepresentation with type: "
                 << static_cast<uint32_t>(config->type);
  return nullptr;
}

#ifndef ENABLE_SKITY
fml::RefPtr<SkiaImageRepresentation> IOSurfaceImageBacking::CreateSkiaRepresentation(
    GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
#if SKIA_ENABLE_GL
    case GrBackendApi::kOpenGL:
#if OS_MAC
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<CGLImageRepresentation>(fml::Ref(this)));
#endif
      break;
#endif
#if SKIA_ENABLE_METAL
    case GrBackendApi::kMetal:
      return fml::MakeRefCounted<SkiaMTLImageRepresentation>(gr_context, fml::Ref(this));
#endif
    default:
      break;
  }

  FML_LOG(ERROR) << "Unable to call IOSurfaceImageBacking::CreateSkiaRepresentation with backend: "
                 << static_cast<uint32_t>(gr_context->backend());
  return nullptr;
}

bool IOSurfaceImageBacking::ReadbackToMemory(const SkPixmap* pixmaps, uint32_t planes) {
  ScopedIOSurfaceLock io_surface_lock(io_surface_, kIOSurfaceLockReadOnly);

  for (uint32_t plane_index = 0; plane_index < planes; plane_index++) {
    const void* io_surface_base_address = IOSurfaceGetBaseAddressOfPlane(io_surface_, plane_index);
    FML_DCHECK(size_.x == static_cast<int32_t>(IOSurfaceGetWidthOfPlane(io_surface_, plane_index)));
    FML_DCHECK(size_.y ==
               static_cast<int32_t>(IOSurfaceGetHeightOfPlane(io_surface_, plane_index)));

    size_t io_surface_row_bytes = IOSurfaceGetBytesPerRowOfPlane(io_surface_, plane_index);
    size_t dst_bytes_per_row = pixmaps[plane_index].rowBytes();

    const uint8_t* src_ptr = static_cast<const uint8_t*>(io_surface_base_address);
    uint8_t* dst_ptr = static_cast<uint8_t*>(pixmaps[plane_index].writable_addr());

    size_t copy_bytes = pixmaps[plane_index].info().minRowBytes();
    FML_DCHECK(copy_bytes <= io_surface_row_bytes);
    FML_DCHECK(copy_bytes <= dst_bytes_per_row);

    CopyPlane(src_ptr, io_surface_row_bytes, dst_ptr, dst_bytes_per_row, copy_bytes, size_.y);
  }

  return true;
}
#else
fml::RefPtr<SkityImageRepresentation> IOSurfaceImageBacking::CreateSkityRepresentation(
    skity::GPUContext* skity_context) {
  switch (skity_context->GetBackendType()) {
    case skity::GPUBackendType::kMetal: {
      return fml::MakeRefCounted<SkityMTLImageRepresentation>(skity_context, fml::Ref(this));
    }
    default: {
      FML_LOG(ERROR) << "Unable to call "
                        "IOSurfaceImageBacking::CreateSkityRepresentation with "
                        "backend: "
                     << static_cast<uint32_t>(skity_context->GetBackendType());
      return nullptr;
    }
  }
}
#endif  // ENABLE_SKITY

}  // namespace clay
