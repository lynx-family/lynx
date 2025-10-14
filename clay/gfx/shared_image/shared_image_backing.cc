// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/shared_image_backing.h"

#include <utility>

#include "build/build_config.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/fence_sync.h"

#if OS_MACOSX || OS_IOS
#include "clay/gfx/shared_image/cv_pixelbuffer_image_backing.h"
#include "clay/gfx/shared_image/iosurface_image_backing.h"
#elif OS_WIN
#include "clay/gfx/shared_image/d3d9_texture_image_backing.h"
#include "clay/gfx/shared_image/d3d_texture_image_backing.h"
#elif OS_ANDROID
#include "clay/gfx/shared_image/android/android_hardwarebuffer_utils.h"
#include "clay/gfx/shared_image/android_hardwarebuffer_image_backing.h"
#include "clay/gfx/shared_image/egl_image_backing.h"
#elif OS_HARMONY
#include "clay/gfx/shared_image/native_image_image_backing.h"
#elif OS_LINUX
#if defined(ENABLE_SOFTWARE_RENDERING)
#include "clay/gfx/shared_image/angle_software_shm_image_backing.h"
#endif
#include "clay/gfx/shared_image/epoxy_shm_image_backing.h"
#endif

namespace clay {

SharedImageBacking::SharedImageBacking(PixelFormat pixel_format,
                                       skity::Vec2 size)
    : pixel_format_(pixel_format), size_(size) {
  FML_DLOG(INFO) << "Create SharedImageBacking with size: " << size.x << "x"
                 << size.y;
}

SharedImageBacking::~SharedImageBacking() {
  FML_DLOG(INFO) << "Destory SharedImageBacking with size: " << size_.x << "x"
                 << size_.y;
}

void SharedImageBacking::SetFenceSync(std::unique_ptr<FenceSync> fence_sync) {
  fence_sync_ = std::move(fence_sync);
}

std::unique_ptr<FenceSync> SharedImageBacking::GetFenceSync() {
  return std::move(fence_sync_);
}

fml::RefPtr<SharedImageBacking> SharedImageBacking::Create(
    BackingType type, PixelFormat pixel_format, skity::Vec2 size,
    std::optional<GraphicsMemoryHandle> gfx_handle) {
#if OS_MACOSX || OS_IOS
  if (type == BackingType::kIOSurface) {
    if (__builtin_available(macos 10.6, ios 11.0, tvos 11.0, *)) {
      return fml::MakeRefCounted<IOSurfaceImageBacking>(pixel_format, size,
                                                        gfx_handle);
    }
  } else if (type == BackingType::kCVPixelBuffer) {
    return fml::MakeRefCounted<CVPixelBufferImageBacking>(pixel_format, size,
                                                          gfx_handle);
  }
#elif OS_WIN
  if (type == BackingType::kD3DTexture) {
    if (D3DTextureImageBacking::IsSupported()) {
      return fml::MakeRefCounted<D3DTextureImageBacking>(pixel_format, size,
                                                         gfx_handle);
    } else {
      FML_LOG(ERROR) << "Fallback to D3D9TextureImageBacking because D3D11 is "
                        "not supported.";
      return fml::MakeRefCounted<D3D9TextureImageBacking>(pixel_format, size,
                                                          gfx_handle);
    }
  }
#elif OS_ANDROID
  if (type == BackingType::kAHardwareBuffer) {
    if (AHardwareBufferUtils::GetInstance().IsSupportAvailable()) {
      return fml::MakeRefCounted<AHardwareBufferImageBacking>(
          pixel_format, size, std::nullopt);
    }
    FML_LOG(ERROR) << "Fallback to EGLImageBacking because AHardwareBuffer is "
                      "not supported.";
    return fml::MakeRefCounted<EGLImageBacking>(pixel_format, size,
                                                std::nullopt);
  } else if (type == BackingType::kEGLImage) {
    return fml::MakeRefCounted<EGLImageBacking>(pixel_format, size,
                                                std::nullopt);
  }
#elif OS_HARMONY
  if (type == BackingType::kNativeImage) {
    return fml::MakeRefCounted<NativeImageImageBacking>(pixel_format, size,
                                                        gfx_handle);
  }
#elif OS_LINUX
  if (type == BackingType::kShmImage) {
    return fml::MakeRefCounted<EpoxyShmImageBacking>(pixel_format, size,
                                                     std::nullopt);
  } else if (type == BackingType::kAngleShmImage) {
#if defined(ENABLE_SOFTWARE_RENDERING)
    return fml::MakeRefCounted<AngleSoftwareShmImageBacking>(pixel_format,
                                                             size);
#endif
  }
#endif

  FML_LOG(ERROR) << "Unable to Create SharedImageBacking with type: "
                 << static_cast<uint32_t>(type);

  return nullptr;
}

#ifndef ENABLE_SKITY
bool SharedImageBacking::ReadbackToMemory(const SkPixmap* pixmaps,
                                          uint32_t planes) {
  FML_UNIMPLEMENTED()
  return false;
}

#ifndef NDEBUG
void SharedImageBacking::DumpToPng(const std::string& file_name) {
  SkBitmap bitmap;
  auto image_info =
      SkImageInfo::Make(SkISize::Make(size_.x, size_.y), kBGRA_8888_SkColorType,
                        kPremul_SkAlphaType);
  bitmap.allocPixels(image_info, 0);
  bool success = ReadbackToMemory(&bitmap.pixmap(), 1);
  FML_DCHECK(success);
  SkDynamicMemoryWStream buf;
  success = SkPngEncoder::Encode(&buf, bitmap.pixmap(), {});
  FML_DCHECK(success);
  sk_sp<SkData> data = buf.detachAsData();
  // cspell:disable-next-line
  success = SkFILEWStream(file_name.c_str()).write(data->data(), data->size());
  FML_DCHECK(success);
}
#endif  // NDEBUG
#endif  // ENABLE_SKITY

SharedImageBackingUnmanaged::SharedImageBackingUnmanaged(
    PixelFormat pixel_format, skity::Vec2 size)
    : SharedImageBacking(pixel_format, size) {}

}  // namespace clay
