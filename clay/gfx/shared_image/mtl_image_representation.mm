// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/mtl_image_representation.h"

#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/cv_pixelbuffer_image_backing.h"
#include "clay/gfx/shared_image/iosurface_image_backing.h"
#include "clay/gfx/shared_image/mtl_fence_sync.h"

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace clay {

MTLStorageManager::MTLStorageManager(id<MTLDevice> device) {
  CVMetalTextureCacheRef out_texture_cache = nil;
  if (CVMetalTextureCacheCreate(kCFAllocatorDefault, (__bridge CFDictionaryRef)(@{
                                  (id)kCVMetalTextureCacheMaximumTextureAgeKey : @0,
                                }),
                                device, nil, &out_texture_cache) != kCVReturnSuccess) {
    FML_LOG(ERROR) << "Failed to create CVMetalTextureCache.";
    return;
  }
  cv_mtl_texture_cache_ = out_texture_cache;
}

MTLStorageManager::~MTLStorageManager() { FlushStorageRecycle(); }

CVMetalTextureRef MTLStorageManager::CreateTextureFromStorage(CVPixelBufferRef pixel_buffer,
                                                              MTLPixelFormat pixel_format,
                                                              size_t width, size_t height) {
  if (!cv_mtl_texture_cache_) {
    FML_LOG(ERROR) << "No available CVMetalTextureCacheRef to create texture";
    return nil;
  }
  CVMetalTextureRef out_texture = nil;
  if (CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, cv_mtl_texture_cache_,
                                                pixel_buffer, nil, pixel_format, width, height, 0,
                                                &out_texture) != kCVReturnSuccess) {
    FML_LOG(ERROR) << "Failed to create CVMetalTexture from CVPixelBuffer.";
    return nil;
  }
  return out_texture;
}

void MTLStorageManager::FlushStorageRecycle() {
  if (cv_mtl_texture_cache_) {
    CVMetalTextureCacheFlush(cv_mtl_texture_cache_, 0);
  }
}

MTLImageRepresentation::MTLImageRepresentation(id<MTLDevice> device, id<MTLCommandQueue> queue,
                                               fml::RefPtr<SharedImageBacking> backing)
    : SharedImageRepresentation(std::move(backing)), device_(device), queue_(queue) {}

MTLImageRepresentation::~MTLImageRepresentation() {
  id<MTLTexture> tex = GetMTLTexture();
  if (tex == nil) {
    return;
  }
  auto command_buffer = fml::scoped_nsprotocol<id<MTLCommandBuffer>>([queue_ commandBuffer]);
  fml::RefPtr<RepresentationStorageManager> temp_mlt_storage_manager = storage_manager_;
  [command_buffer.get() addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
    // GPU work is complete，safely release texture.
    if (temp_mlt_storage_manager) {
      static_cast<MTLStorageManager*>(temp_mlt_storage_manager.get())->FlushStorageRecycle();
    }
  }];
  [command_buffer.get() commit];
}

static MTLPixelFormat GetMetalPixelFormat(SharedImageBacking::PixelFormat pixel_format) {
  switch (pixel_format) {
    case SharedImageBacking::PixelFormat::kNative8888:
      return MTLPixelFormatBGRA8Unorm;
    default:
      break;
  }
  FML_UNREACHABLE();
}

ImageRepresentationType MTLImageRepresentation::GetType() const {
  return ImageRepresentationType::kMetal;
}

id<MTLTexture> MTLImageRepresentation::GetMTLTexture() {
  if (texture_) {
    return texture_;
  }
  switch (GetBacking()->GetType()) {
    case SharedImageBacking::BackingType::kIOSurface:
      if (!GetMTLTextureFromIOSurface()) {
        return nil;
      }
      break;
    case SharedImageBacking::BackingType::kCVPixelBuffer:
      if (!GetMTLTextureFromCVPixelBuffer()) {
        return nil;
      }
      break;
    default:
      FML_LOG(ERROR) << "Invalid shared image backing type for MTLImageRepr.";
      return nil;
  }
  return texture_;
}

bool MTLImageRepresentation::GetMTLTextureFromIOSurface() {
  if (@available(macos 10.6, ios 11.0, tvos 11.0, *)) {
    auto* backing = GetBacking();
    IOSurfaceRef surface = static_cast<IOSurfaceRef>(backing->GetGFXHandle());
    MTLPixelFormat format = GetMetalPixelFormat(backing->GetPixelFormat());
    MTLTextureDescriptor* textureDescriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
                                                           width:IOSurfaceGetWidth(surface)
                                                          height:IOSurfaceGetHeight(surface)
                                                       mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    texture_.reset([device_ newTextureWithDescriptor:textureDescriptor iosurface:surface plane:0]);
    return true;
  } else {
    return false;
  }
}

bool MTLImageRepresentation::GetMTLTextureFromCVPixelBuffer() {
  if (!storage_manager_) {
    FML_LOG(ERROR) << "No available MTLStorageManager to create texture";
    return false;
  }
  MTLPixelFormat format = GetMetalPixelFormat(GetBacking()->GetPixelFormat());
  CVMetalTextureRef out_texture = storage_manager_->CreateTextureFromStorage(
      static_cast<CVPixelBufferRef>(GetBacking()->GetGFXHandle()), format,
      GetBacking()->GetSize().x, GetBacking()->GetSize().y);
  if (!out_texture) {
    return false;
  }
  texture_.reset(CVMetalTextureGetTexture(out_texture));
  CFRelease(out_texture);
  return true;
}

bool MTLImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  id<MTLTexture> texture = GetMTLTexture();
  if (texture == nil) {
    return false;
  }
  memset(out, 0, sizeof(ClaySharedImageReadResult));
  out->struct_size = sizeof(ClaySharedImageReadResult);
  out->type = kClaySharedImageRepresentationTypeMetal;
  out->metal_texture.struct_size = sizeof(ClayMetalTexture);
  out->metal_texture.texture = (__bridge ClayMetalTextureHandle)texture;
  out->metal_texture.user_data = this;
  this->AddRef();
  out->metal_texture.destruction_callback = [](void* user_data) {
    static_cast<MTLImageRepresentation*>(user_data)->Release();
  };
  return true;
}

bool MTLImageRepresentation::EndRead() { return true; }
bool MTLImageRepresentation::BeginWrite(ClaySharedImageWriteResult* out) {
  id<MTLTexture> texture = GetMTLTexture();
  if (texture == nil) {
    return false;
  }
  memset(out, 0, sizeof(ClaySharedImageWriteResult));
  out->struct_size = sizeof(ClaySharedImageWriteResult);
  out->type = kClaySharedImageRepresentationTypeMetal;
  out->metal_texture.struct_size = sizeof(ClayMetalTexture);
  out->metal_texture.texture = (__bridge ClayMetalTextureHandle)texture;
  out->metal_texture.user_data = this;
  this->AddRef();
  out->metal_texture.destruction_callback = [](void* user_data) {
    static_cast<MTLImageRepresentation*>(user_data)->Release();
  };
  return true;
}
bool MTLImageRepresentation::EndWrite() { return true; }

void MTLImageRepresentation::ConsumeFence(std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  if (@available(macos 10.14, ios 12.0, tvos 12.0, *)) {
    if (fence_sync->GetType() == FenceSync::Type::kMetalEvent) {
      MTLSharedEventFenceSync* mtl_fence_sync =
          static_cast<MTLSharedEventFenceSync*>(fence_sync.get());
      FML_DCHECK(mtl_fence_sync->Event() != nil);
      id<MTLCommandBuffer> buffer = [queue_ commandBuffer];
      [buffer encodeWaitForEvent:mtl_fence_sync->Event() value:mtl_fence_sync->Value()];
      [buffer commit];
      return;
    }
  }
  fence_sync->ClientWait();
}

std::unique_ptr<FenceSync> MTLImageRepresentation::ProduceFence() {
  std::unique_ptr<FenceSync> fence_sync;
  if (@available(macos 10.14, ios 12.0, tvos 12.0, *)) {
    if (event_ == nil) {
      // TODO(youfeng) Use MTLEvent instead of MTLSharedEvent for shared device
      event_.reset([device_ newSharedEvent]);
    }
    event_value_++;
    id<MTLCommandBuffer> buffer = [queue_ commandBuffer];
    fence_sync = std::make_unique<MTLSharedEventFenceSync>(buffer, event_, event_value_);
    [buffer encodeSignalEvent:event_ value:event_value_];
    [buffer commit];
  } else {
    id<MTLCommandBuffer> buffer = [queue_ commandBuffer];
    fence_sync = std::make_unique<MTLCompleteFenceSync>(buffer);
    [buffer commit];
  }

  return fence_sync;
}

fml::RefPtr<RepresentationStorageManager> MTLImageRepresentation::GetTextureManager() const {
  return fml::MakeRefCounted<MTLStorageManager>(device_);
}

void MTLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager> storage_manager) {
  storage_manager_ = fml::Ref(static_cast<MTLStorageManager*>(storage_manager.get()));
}

}  // namespace clay
