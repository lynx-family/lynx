// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_BACKING_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/include/closure.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/shared_image/shared_image_representation.h"
#include "clay/public/clay.h"
#include "skity/geometry/matrix.hpp"

namespace clay {
/// IOSurfaceRef for kIOSurface
/// CVPixelBufferRef for kCVPixelBuffer
/// SharedHandle for kD3DTexture
/// EGLImage for kEGLImage
/// AHardwareBuffer* for kAHardwareBuffer
/// jobject of SurfaceTexture for kSurfaceTexture
/// ShmFd for kShmImage
using GraphicsMemoryHandle = void*;

class FenceSync;

#ifndef ENABLE_SKITY
using SharedImageReadbackPixmap = SkPixmap;
#else
using SharedImageReadbackPixmap = skity::Pixmap;
#endif  // ENABLE_SKITY

inline uint8_t* SharedImagePixmapWritableAddr(
    SharedImageReadbackPixmap& pixmap) {
#ifndef ENABLE_SKITY
  return static_cast<uint8_t*>(pixmap.writable_addr());
#else
  return static_cast<uint8_t*>(pixmap.WritableAddr());
#endif  // ENABLE_SKITY
}

inline size_t SharedImagePixmapRowBytes(
    const SharedImageReadbackPixmap& pixmap) {
#ifndef ENABLE_SKITY
  return pixmap.rowBytes();
#else
  return pixmap.RowBytes();
#endif  // ENABLE_SKITY
}

inline size_t SharedImagePixmapMinRowBytes(
    const SharedImageReadbackPixmap& pixmap) {
#ifndef ENABLE_SKITY
  return pixmap.info().minRowBytes();
#else
  // SharedImage readback currently supports 32-bit RGBA/BGRA formats only.
  return pixmap.Width() * 4;
#endif  // ENABLE_SKITY
}

class SharedImageBacking
    : public fml::RefCountedThreadSafe<SharedImageBacking> {
 public:
  enum class BackingType {
    kIOSurface,
    kCVPixelBuffer,
    kD3DTexture,
    kEGLImage,
    kAHardwareBuffer,
    kSurfaceTexture,
    kNativeImage,
    kShmImage,
    kAngleShmImage,
    kMocking,
  };

  enum class PixelFormat {
    kNative8888,  // BGRA8888
    kRGBA8888,
  };

  SharedImageBacking(PixelFormat pixel_format, skity::Vec2 size);
  virtual ~SharedImageBacking();

  virtual BackingType GetType() const = 0;
  virtual GraphicsMemoryHandle GetGFXHandle() const = 0;

  virtual fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) = 0;
  // User MUST client wait fence before readback.
  virtual bool ReadbackToMemory(SharedImageReadbackPixmap* pixmaps,
                                uint32_t planes);
#ifndef ENABLE_SKITY
  virtual fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrContext* gr_context) = 0;
#else
  virtual fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      GrContext* skity_context) = 0;
#endif  // ENABLE_SKITY

  virtual PixelFormat GetPixelFormat() const { return pixel_format_; }
  virtual const skity::Vec2 GetSize() const { return size_; }
  virtual bool SetSize(skity::Vec2 size) { return false; }
  // UV transform
  virtual const skity::Matrix GetTransformation() const {
    return transformation_;
  }
  virtual void SetTransformation(const skity::Matrix& mat) {
    transformation_ = mat;
  }

  void SetFenceSync(std::unique_ptr<FenceSync> fence_sync);
  std::unique_ptr<FenceSync> GetFenceSync();

  static fml::RefPtr<SharedImageBacking> Create(
      BackingType backing_type, PixelFormat pixel_format, skity::Vec2 size,
      std::optional<GraphicsMemoryHandle> gfx_handle);

  BASE_DISALLOW_COPY_AND_ASSIGN(SharedImageBacking);

#ifndef NDEBUG
  void DumpToPng(const std::string& file_name);
#endif

 protected:
  PixelFormat pixel_format_;
  skity::Vec2 size_;
  skity::Matrix transformation_;
  std::unique_ptr<FenceSync> fence_sync_;
};

class SharedImageBackingUnmanaged : public SharedImageBacking {
 public:
  SharedImageBackingUnmanaged(PixelFormat pixel_format, skity::Vec2 size);

  virtual void SetFrameAvailableCallback(const fml::closure& callback) = 0;

  /// Move the current front buffer
  virtual bool UpdateFront() = 0;

  // Releases the front buffer. This is needed in single buffered mode to
  // allow the producer to take ownership of the buffer.
  virtual void ReleaseFront() = 0;

  /// Acquire the latest back buffer.
  /// The return value is the buffer age, kinda like
  /// `EGL_BUFFER_AGE_EXT`, 0 means a new buffer without swap history
  ///
  /// This is a blocking call, if no backing buffer available and the buffer
  /// queue is full, it will wait until front buffer swap to back
  virtual uint32_t AcquireBack() = 0;

  /// Swap the current back buffer to pending front
  virtual bool SwapBack() = 0;

  virtual uint32_t Capacity() const = 0;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_BACKING_H_
