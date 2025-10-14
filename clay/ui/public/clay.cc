// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/public/clay.h"

#include <future>
#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/macros.h"
#include "build/build_config.h"
#include "clay/fml/logging.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/image/image_descriptor.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/shared_image/shared_image_sink_accessor.h"
#include "clay/net/loader/data_image_loader.h"
#include "clay/ui/common/isolate.h"

namespace clay {

namespace {

SharedImageBacking::BackingType ClaySharedImageBackingTypeToBackingType(
    ClaySharedImageBackingType type) {
  switch (type) {
    case kClaySharedImageBackingTypeIOSurface:
      return SharedImageBacking::BackingType::kIOSurface;
    case kClaySharedImageBackingTypeCVPixelBuffer:
      return SharedImageBacking::BackingType::kCVPixelBuffer;
    case kClaySharedImageBackingTypeD3DTexture:
      return SharedImageBacking::BackingType::kD3DTexture;
    case kClaySharedImageBackingTypeNativeImage:
      return SharedImageBacking::BackingType::kNativeImage;
    case kClaySharedImageBackingTypeShmImage:
      return SharedImageBacking::BackingType::kShmImage;
  }
  FML_UNREACHABLE();
}

SharedImageBacking::PixelFormat ClaySharedImageBackingPixelFormatToPixelFormat(
    ClaySharedImageBackingPixelFormat type) {
  switch (type) {
    case kClaySharedImageBackingPixelFormatNative8888:
      return SharedImageBacking::PixelFormat::kNative8888;
    case kClaySharedImageBackingPixelFormatRGBA8:
      return SharedImageBacking::PixelFormat::kRGBA8888;
  }
  FML_UNREACHABLE();
}

class ExternalFenceSync final : public FenceSync {
 public:
  ExternalFenceSync(ClayVoidCallback wait_callback,
                    ClayVoidCallback delete_callback, void* user_data)
      : wait_callback_(wait_callback),
        delete_callback_(delete_callback),
        user_data_(user_data) {}

  ~ExternalFenceSync() override {
    if (delete_callback_) {
      delete_callback_(user_data_);
    }
  }

  bool ClientWait() override {
    if (wait_callback_) {
      wait_callback_(user_data_);
      return true;
    }
    return false;
  }

  Type GetType() const override {
    return clay::FenceSync::Type::kClientWaitOnly;
  }

 private:
  ClayVoidCallback wait_callback_;
  ClayVoidCallback delete_callback_;
  void* user_data_;
};

GrDataPtr GetImagePixels(fml::RefPtr<clay::GraphicsImage> image) {
#ifndef ENABLE_SKITY
  SkPixmap pixmap;
  if (!image || !image->peekPixels(&pixmap)) {
    FML_LOG(ERROR) << "Could not peekPixels from image.";
    return nullptr;
  }

  if (pixmap.colorType() == kRGBA_8888_SkColorType &&
      pixmap.alphaType() == kPremul_SkAlphaType) {
    return SkData::MakeWithCopy(pixmap.addr(), pixmap.computeByteSize());
  }

  // Perform swizzle if the type doesnt match the specification.
  auto surface = SkSurface::MakeRaster(
      SkImageInfo::Make(image->width(), image->height(), kRGBA_8888_SkColorType,
                        kPremul_SkAlphaType, nullptr));

  if (!surface) {
    FML_LOG(ERROR) << "Could not set up the surface for swizzle.";
    return nullptr;
  }

  surface->writePixels(pixmap, 0, 0);

  if (!surface->peekPixels(&pixmap)) {
    FML_LOG(ERROR) << "Pixel address is not available.";
    return nullptr;
  }

  return SkData::MakeWithCopy(pixmap.addr(), pixmap.computeByteSize());
#else
  if (!image) {
    FML_LOG(ERROR) << "Could not peekPixels from null image.";
    return nullptr;
  }
  std::shared_ptr<skity::Pixmap> pixmap = image->peekPixels();
  if (pixmap->GetColorType() == skity::ColorType::kRGBA &&
      pixmap->GetAlphaType() == skity::AlphaType::kPremul_AlphaType) {
    return skity::Data::MakeWithCopy(pixmap->Addr(),
                                     pixmap->Height() * pixmap->RowBytes());
  }

  // TODO: Need raster surface to write pixels.
  return skity::Data::MakeEmpty();
#endif  // ENABLE_SKITY
}

void DecodeImage(
    GrDataPtr data,
    std::function<void(const char* error_message, const ClayBitmap* bitmap)>
        callback) {
  fml::RefPtr<clay::ImageDescriptor> image_descriptor =
      clay::ImageDescriptor::Create(std::move(data));

  // It seems that the GetImageDecoder is not thread safe
  // so we post the task to UI thread first
  fml::TaskRunner::RunNowOrPostTask(
      clay::Isolate::Instance().GetIOTaskRunner(),
      [image_descriptor = std::move(image_descriptor),
       callback = std::move(callback)] {
        // TODO(yudingqian): Implement with PlatformCodec later.
        clay::GraphicsIsolate::Instance().GetImageDecoder()->Decode(
            image_descriptor, image_descriptor->width(),
            image_descriptor->height(),
            [callback =
                 std::move(callback)](fml::RefPtr<clay::GraphicsImage> image) {
              auto image_pixels = GetImagePixels(image);
              if (!image_pixels) {
                callback("Failed to decode image", nullptr);
              } else {
                auto* shared_ptr_holder =
                    new GrDataPtr(std::move(image_pixels));
                ClayBitmap result{};
                result.struct_size = sizeof(result);
                result.width = image->width();
                result.height = image->height();
                result.pixels.size = DATA_GET_SIZE((*shared_ptr_holder));
                result.pixels.ptr = DATA_GET_DATA((*shared_ptr_holder));
                result.pixels.user_data = shared_ptr_holder;
                result.pixels.destruction_callback = [](const void*,
                                                        void* user_data) {
                  delete static_cast<GrDataPtr*>(user_data);
                };
                callback(nullptr, &result);
              }
            });
      });
}

class ClayBitmapCallbackWrapper {
 public:
  ClayBitmapCallbackWrapper(ClayBitmapDecodeCallback callback, void* user_data)
      : callback_(callback), user_data_(user_data) {}

  void OnDecode(const ClayBitmap* result) {
    callback_(nullptr, result, user_data_);
    callback_ = nullptr;
  }

  void OnError(const char* error_message) {
    callback_(error_message, nullptr, user_data_);
    callback_ = nullptr;
  }

  ~ClayBitmapCallbackWrapper() {
    if (callback_) {
      callback_("Unable to call Callback due to shutdown", nullptr, user_data_);
    }
  }

  BASE_DISALLOW_COPY_AND_ASSIGN(ClayBitmapCallbackWrapper);

 private:
  ClayBitmapDecodeCallback callback_;
  void* user_data_;
};

}  // namespace

CLAY_EXTERN_C ClaySharedImageRef ClayCreateSharedImage(
    ClaySharedImageBackingType type,
    ClaySharedImageBackingPixelFormat pixel_format, const ClaySize* size) {
  skity::Vec2 sk_size{static_cast<int32_t>(size->width),
                      static_cast<int32_t>(size->height)};
  fml::RefPtr<SharedImageBacking> backing = SharedImageBacking::Create(
      ClaySharedImageBackingTypeToBackingType(type),
      ClaySharedImageBackingPixelFormatToPixelFormat(pixel_format), sk_size,
      {});
  backing->AddRef();
  return reinterpret_cast<ClaySharedImageRef>(backing.get());
}

CLAY_EXTERN_C ClaySharedImageRef ClayCreateSharedImageFromHandle(
    ClaySharedImageBackingType type,
    ClaySharedImageBackingPixelFormat pixel_format, const ClaySize* size,
    ClaySharedImageNativeHandle handle) {
  skity::Vec2 sk_size{static_cast<int32_t>(size->width),
                      static_cast<int32_t>(size->height)};

  fml::RefPtr<SharedImageBacking> backing = SharedImageBacking::Create(
      ClaySharedImageBackingTypeToBackingType(type),
      ClaySharedImageBackingPixelFormatToPixelFormat(pixel_format), sk_size,
      reinterpret_cast<GraphicsMemoryHandle>(handle));
  backing->AddRef();
  return reinterpret_cast<ClaySharedImageRef>(backing.get());
}

CLAY_EXTERN_C ClaySharedImageRef
ClayRetainSharedImage(ClaySharedImageRef shared_image_ref) {
  reinterpret_cast<SharedImageBacking*>(shared_image_ref)->AddRef();
  return shared_image_ref;
}

CLAY_EXTERN_C void ClayReleaseSharedImage(ClaySharedImageRef shared_image_ref) {
  if (shared_image_ref) {
    reinterpret_cast<SharedImageBacking*>(shared_image_ref)->Release();
  }
}

CLAY_EXTERN_C void ClaySharedImageGetBacking(
    ClaySharedImageRef shared_image_ref, ClaySharedImageBackingType* out_type,
    ClaySharedImageBackingPixelFormat* out_format,
    ClaySharedImageNativeHandle* out_handle) {
  SharedImageBacking* shared_image =
      reinterpret_cast<SharedImageBacking*>(shared_image_ref);
  if (out_type) {
    switch (shared_image->GetType()) {
      case SharedImageBacking::BackingType::kIOSurface:
        *out_type = kClaySharedImageBackingTypeIOSurface;
        break;
      case SharedImageBacking::BackingType::kD3DTexture:
        *out_type = kClaySharedImageBackingTypeD3DTexture;
        break;
      case SharedImageBacking::BackingType::kNativeImage:
        *out_type = kClaySharedImageBackingTypeNativeImage;
        break;
      case SharedImageBacking::BackingType::kShmImage:
        *out_type = kClaySharedImageBackingTypeShmImage;
        break;
      default:
        break;
    }
  }
  if (out_format) {
    switch (shared_image->GetPixelFormat()) {
      case SharedImageBacking::PixelFormat::kNative8888:
        *out_format = kClaySharedImageBackingPixelFormatNative8888;
        break;
      case SharedImageBacking::PixelFormat::kRGBA8888:
        *out_format = kClaySharedImageBackingPixelFormatRGBA8;
        break;
      default:
        break;
    }
  }
  if (out_handle) {
    *out_handle = reinterpret_cast<ClaySharedImageNativeHandle>(
        shared_image->GetGFXHandle());
  }
}

CLAY_EXTERN_C void ClaySharedImageGetSize(ClaySharedImageRef shared_image_ref,
                                          ClaySize* out) {
  auto& size =
      reinterpret_cast<SharedImageBacking*>(shared_image_ref)->GetSize();
  out->width = size.x;
  out->height = size.y;
}

CLAY_EXTERN_C void ClaySharedImageGetTransformation(
    ClaySharedImageRef shared_image_ref, ClayTransformation* out) {
  auto& mat = reinterpret_cast<SharedImageBacking*>(shared_image_ref)
                  ->GetTransformation();

  out->scaleX = mat.GetScaleX();
  out->skewX = mat.GetSkewX();
  out->transX = mat.GetTranslateX();
  out->skewY = mat.GetSkewY();
  out->scaleY = mat.GetScaleY();
  out->transY = mat.GetTranslateY();
  out->pers0 = mat.GetPersp0();
  out->pers1 = mat.GetPersp1();
  out->pers2 = mat.GetPersp2();
}

CLAY_EXTERN_C void ClaySharedImageSetTransformation(
    ClaySharedImageRef shared_image_ref,
    const ClayTransformation* transformation) {
  skity::Matrix mat = skity::Matrix(
      transformation->scaleX, transformation->skewX, transformation->transX,
      transformation->skewY, transformation->scaleY, transformation->transY,
      transformation->pers0, transformation->pers1, transformation->pers2);

  reinterpret_cast<SharedImageBacking*>(shared_image_ref)
      ->SetTransformation(mat);
}

CLAY_EXTERN_C ClayFenceSyncRef
ClayCreateExternalFenceSync(ClayVoidCallback wait_callback,
                            ClayVoidCallback delete_callback, void* user_data) {
  return reinterpret_cast<ClayFenceSyncRef>(
      new ExternalFenceSync(wait_callback, delete_callback, user_data));
}

CLAY_EXTERN_C bool ClayFenceSyncClientWait(ClayFenceSyncRef fence_sync) {
  if (fence_sync) {
    return reinterpret_cast<FenceSync*>(fence_sync)->ClientWait();
  }
  return true;
}

CLAY_EXTERN_C void ClayDestroyFenceSync(ClayFenceSyncRef fence_sync) {
  delete reinterpret_cast<FenceSync*>(fence_sync);
}

CLAY_EXTERN_C void ClaySharedImageSetFenceSync(
    ClaySharedImageRef shared_image_ref, ClayFenceSyncRef fence_sync_ref) {
  reinterpret_cast<SharedImageBacking*>(shared_image_ref)
      ->SetFenceSync(std::unique_ptr<FenceSync>(
          reinterpret_cast<FenceSync*>(fence_sync_ref)));
}

CLAY_EXTERN_C ClayFenceSyncRef
ClaySharedImageGetFenceSync(ClaySharedImageRef shared_image_ref) {
  std::unique_ptr<FenceSync> fence_sync =
      reinterpret_cast<SharedImageBacking*>(shared_image_ref)->GetFenceSync();
  return reinterpret_cast<ClayFenceSyncRef>(fence_sync.release());
}

CLAY_EXTERN_C ClaySharedImageSinkRef
ClayCreateSharedImageSink(ClaySharedImageSinkBufferMode clay_buffer_mode,
                          ClaySharedImageBackingType type,
                          ClaySharedImageBackingPixelFormat pixel_format) {
  SharedImageSink::BufferMode buffer_mode;
  switch (clay_buffer_mode) {
    case kClaySharedImageSinkBufferModeSingleBuffer:
      buffer_mode = SharedImageSink::kSingleBuffer;
      break;
    case kClaySharedImageSinkBufferModeDoubleBuffer:
      buffer_mode = SharedImageSink::kDoubleBuffer;
      break;
    case kClaySharedImageSinkBufferModeTripleBuffer:
      buffer_mode = SharedImageSink::kTripleBuffer;
      break;
    default:
      FML_LOG(ERROR) << "unknown ClaySharedImageSinkBufferMode value: "
                     << clay_buffer_mode;
      return nullptr;
  }
  fml::RefPtr<SharedImageSink> sink =
      fml::MakeRefCounted<SharedImageSinkManaged>(
          buffer_mode,
          [type, pixel_format](skity::Vec2 size,
                               std::optional<GraphicsMemoryHandle> gfx_handle)
              -> fml::RefPtr<SharedImageBacking> {
            ClaySize clay_size;
            clay_size.width = size.x;
            clay_size.height = size.y;
            SharedImageBacking* backing;
            if (gfx_handle.has_value() && gfx_handle.value()) {
              backing = reinterpret_cast<SharedImageBacking*>(
                  ClayCreateSharedImageFromHandle(
                      type, pixel_format, &clay_size, gfx_handle.value()));
            } else {
              backing = reinterpret_cast<SharedImageBacking*>(
                  ClayCreateSharedImage(type, pixel_format, &clay_size));
            }
            fml::RefPtr<SharedImageBacking> result(backing);
            backing->Release();
            return result;
          });
  sink->AddRef();

  return reinterpret_cast<ClaySharedImageSinkRef>(sink.get());
}

CLAY_EXTERN_C ClaySharedImageSinkRef
ClayRetainSharedImageSink(ClaySharedImageSinkRef sink_ref) {
  reinterpret_cast<SharedImageSink*>(sink_ref)->AddRef();
  return sink_ref;
}

CLAY_EXTERN_C void ClayReleaseSharedImageSink(ClaySharedImageSinkRef sink_ref) {
  if (sink_ref) {
    reinterpret_cast<SharedImageSink*>(sink_ref)->Release();
  }
}

CLAY_EXTERN_C ClaySharedImageSinkBufferMode
ClaySharedImageSinkGetBufferMode(ClaySharedImageSinkRef sink_ref) {
  uint32_t capacity = reinterpret_cast<SharedImageSink*>(sink_ref)->Capacity();
  switch (capacity) {
    case 1:
      return kClaySharedImageSinkBufferModeSingleBuffer;
    case 2:
      return kClaySharedImageSinkBufferModeDoubleBuffer;
    case 3:
      return kClaySharedImageSinkBufferModeTripleBuffer;
  }
  FML_LOG(ERROR) << "Unknown buffer mode, capacity: " << capacity;
  // We don't known the result, but single buffer is safest
  return kClaySharedImageSinkBufferModeSingleBuffer;
}

CLAY_EXTERN_C void ClaySharedImageSinkSetFrameAvailableCallback(
    ClaySharedImageSinkRef sink_ref, ClayVoidCallback callback,
    void* user_data) {
  if (callback) {
    reinterpret_cast<SharedImageSink*>(sink_ref)->SetFrameAvailableCallback(
        [callback, user_data] { callback(user_data); });
  } else {
    reinterpret_cast<SharedImageSink*>(sink_ref)->SetFrameAvailableCallback(
        nullptr);
  }
}

CLAY_EXTERN_C bool ClaySharedImageSinkUpdateFront(
    ClaySharedImageSinkRef sink_ref, ClayFenceSyncRef produced_fence_sync_ref,
    ClaySharedImageRef* out) {
  SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref);
  std::unique_ptr<FenceSync> produced_fence_sync(
      reinterpret_cast<FenceSync*>(produced_fence_sync_ref));
  fml::RefPtr<SharedImageBacking> backing =
      sink->UpdateFront(std::move(produced_fence_sync));
  if (!backing) {
    return false;
  }
  if (out) {
    *out = reinterpret_cast<ClaySharedImageRef>(backing.get());
  }
  return true;
}

CLAY_EXTERN_C void ClaySharedImageSinkReleaseFront(
    ClaySharedImageRef sink_ref, ClayFenceSyncRef produced_fence_sync_ref) {
  SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref);
  std::unique_ptr<FenceSync> produced_fence_sync(
      reinterpret_cast<FenceSync*>(produced_fence_sync_ref));
  sink->ReleaseFront(std::move(produced_fence_sync));
}

CLAY_EXTERN_C bool ClaySharedImageSinkAcquireBack(
    ClaySharedImageSinkRef sink_ref, const ClaySize* size,
    ClaySharedImageRef* out, uint32_t* out_buffer_age) {
  SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref);
  auto [backing, buffer_age] =
      sink->AcquireBack(skity::Vec2(size->width, size->height));
  if (!backing) {
    return false;
  }
  if (out) {
    *out = reinterpret_cast<ClaySharedImageRef>(backing.get());
  }
  if (out_buffer_age) {
    *out_buffer_age = buffer_age;
  }
  return true;
}

CLAY_EXTERN_C bool ClaySharedImageSinkTryAcquireBack(
    ClaySharedImageSinkRef sink_ref, const ClaySize* size,
    ClaySharedImageRef* out, uint32_t* out_buffer_age,
    ClaySharedImageNativeHandle handle) {
  SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref);
  std::optional<GraphicsMemoryHandle> gfx_handle;
  if (handle) {
    gfx_handle = handle;
  }
  auto [backing, buffer_age, status] =
      sink->TryAcquireBack(skity::Vec2(size->width, size->height), gfx_handle);
  if (!backing) {
    return false;
  }
  auto fenceSync = backing->GetFenceSync();
  if (fenceSync) {
    fenceSync->ClientWait();
  }
  if (out) {
    *out = reinterpret_cast<ClaySharedImageRef>(backing.get());
  }
  if (out_buffer_age) {
    *out_buffer_age = buffer_age;
  }
  return true;
}

CLAY_EXTERN_C bool ClaySharedImageSinkSwapBack(
    ClaySharedImageSinkRef sink_ref, ClayFenceSyncRef fence_sync_ref) {
  SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref);
  std::unique_ptr<FenceSync> fence_sync(
      reinterpret_cast<FenceSync*>(fence_sync_ref));
  return sink->SwapBack(std::move(fence_sync));
}

CLAY_EXTERN_C ClaySharedImageSinkAccessorRef ClayCreateSharedImageSinkAccessor(
    ClaySharedImageSinkRef sink_ref,
    const ClaySharedImageRepresentationConfig* config) {
  SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref);
  return reinterpret_cast<ClaySharedImageSinkAccessorRef>(
      new SharedImageSinkAccessor(
          fml::Ref(sink),
          [config = *config](fml::RefPtr<SharedImageBacking> backing)
              -> fml::RefPtr<SharedImageRepresentation> {
            return backing->CreateRepresentation(&config);
          }));
}

CLAY_EXTERN_C void ClayDestroySharedImageSinkAccessor(
    ClaySharedImageSinkAccessorRef sink_accessor) {
  delete reinterpret_cast<SharedImageSinkAccessor*>(sink_accessor);
}

CLAY_EXTERN_C bool ClaySharedImageSinkRead(
    ClaySharedImageSinkAccessorRef sink_accessor_ref,
    ClaySharedImageRef* out_image, ClaySharedImageReadResult* out) {
  SharedImageSinkAccessor* sink_accessor =
      reinterpret_cast<SharedImageSinkAccessor*>(sink_accessor_ref);
  fml::RefPtr<SharedImageRepresentation> repr = sink_accessor->UpdateFront();
  if (!repr) {
    return false;
  }
  if (out_image) {
    *out_image = reinterpret_cast<ClaySharedImageRef>(repr->GetBacking());
  }
  if (out) {
    if (!repr->BeginRead(out)) {
      return false;
    }
  }
  return true;
}

CLAY_EXTERN_C void ClaySharedImageSinkEndRead(
    ClaySharedImageSinkAccessorRef sink_accessor_ref) {
  SharedImageSinkAccessor* sink_accessor =
      reinterpret_cast<SharedImageSinkAccessor*>(sink_accessor_ref);
  sink_accessor->ReleaseFront();
}

CLAY_EXTERN_C bool ClaySharedImageSinkBeginWrite(
    ClaySharedImageSinkAccessorRef sink_accessor_ref, const ClaySize* size,
    ClaySharedImageRef* out_image, ClaySharedImageWriteResult* out,
    uint32_t* out_buffer_age) {
  SharedImageSinkAccessor* sink_accessor =
      reinterpret_cast<SharedImageSinkAccessor*>(sink_accessor_ref);
  auto [repr, buffer_age] =
      sink_accessor->AcquireBack(skity::Vec2(size->width, size->height));
  if (!repr) {
    return false;
  }
  if (out_image) {
    *out_image = reinterpret_cast<ClaySharedImageRef>(repr->GetBacking());
  }
  if (out) {
    if (!repr->BeginWrite(out)) {
      return false;
    }
  }
  if (out_buffer_age) {
    *out_buffer_age = buffer_age;
  }
  return true;
}

CLAY_EXTERN_C bool ClaySharedImageSinkEndWrite(
    ClaySharedImageSinkAccessorRef sink_accessor_ref) {
  SharedImageSinkAccessor* sink_accessor =
      reinterpret_cast<SharedImageSinkAccessor*>(sink_accessor_ref);
  return sink_accessor->SwapBack();
}

CLAY_EXTERN_C void ClayDecodeImage(const ClayDataHolder* data_holder,
                                   ClayBitmapDecodeCallback decode_callback,
                                   void* user_data) {
  GrDataPtr data = GrData::MakeWithProc(data_holder->ptr, data_holder->size,
                                        data_holder->destruction_callback,
                                        data_holder->user_data);

  std::shared_ptr<ClayBitmapCallbackWrapper> callback =
      std::make_shared<ClayBitmapCallbackWrapper>(decode_callback, user_data);

  DecodeImage(std::move(data),
              [callback = std::move(callback)](const char* error_message,
                                               const ClayBitmap* result) {
                if (error_message) {
                  callback->OnError(error_message);
                } else {
                  callback->OnDecode(result);
                }
              });
}

CLAY_EXTERN_C bool ClayDecodeDataUrlImage(const char* data_url, size_t length,
                                          ClayBitmap* out) {
  clay::DataImageLoader data_image_loader(nullptr);
  clay::RawResource raw_resource =
      data_image_loader.LoadSync(std::string(data_url, length));
  if (!raw_resource.data) {
    return false;
  }

  std::promise<std::optional<ClayBitmap>> promise;
  // It's safe to MakeWithoutCopy since the decoding will block the thread
  GrDataPtr data = GrData::MakeWithProc(raw_resource.data.get(),
                                        raw_resource.length, nullptr, nullptr);

  DecodeImage(std::move(data),
              [&promise](const char* error_message, const ClayBitmap* bitmap) {
                if (error_message) {
                  promise.set_value({});
                } else {
                  promise.set_value(*bitmap);
                }
              });

  std::optional<ClayBitmap> result = promise.get_future().get();
  if (result) {
    *out = *result;
    return true;
  } else {
    return false;
  }
}

CLAY_EXTERN_C bool ClayEncodeBitmap(const ClayBitmap* bitmap,
                                    ClayImageFormat encoding,
                                    float compress_ratio, ClayDataHolder* out) {
#ifndef ENABLE_SKITY
  SkEncodedImageFormat format;

  switch (encoding) {
    case kClayImageFormatJPEG:
      format = SkEncodedImageFormat::kJPEG;
      break;
    case kClayImageFormatPNG:
      format = SkEncodedImageFormat::kPNG;
      break;
    default:
      return false;
  }

  SkImageInfo image_info =
      SkImageInfo::Make(bitmap->width, bitmap->height, kRGBA_8888_SkColorType,
                        kPremul_SkAlphaType);
  SkPixmap pixmap(image_info, bitmap->pixels.ptr, bitmap->width * 4);
  sk_sp<SkImage> image = SkImages::RasterFromPixmap(pixmap, nullptr, nullptr);
  sk_sp<SkData> data = image->encodeToData(format, compress_ratio * 100);
  if (!data) {
    return false;
  }
  out->size = data->size();
  out->ptr = data->data();
  out->destruction_callback = [](const void*, void* user_data) {
    static_cast<SkData*>(user_data)->unref();
  };
  out->user_data = data.release();
#else
  FML_UNIMPLEMENTED();
  return false;
#endif  // ENABLE_SKITY
  return true;
}

}  // namespace clay
