// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_descriptor_skia.h"

#include "base/include/fml/synchronization/shared_mutex.h"
#include "base/trace/native/trace_event.h"
#include "build/build_config.h"
#include "clay/fml/logging.h"
#include "clay/fml/mapping.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/image/multi_frame_codec.h"
#include "clay/gfx/image/single_frame_codec.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColorPriv.h"
#include "third_party/skia/include/core/SkImageInfo.h"

#ifdef ENABLE_TT_HEIF_DECODER
#include "clay/gfx/image/heif_decoder.h"
#endif

#ifdef OS_MACOSX
#include "third_party/skia/include/ports/SkImageGeneratorCG.h"
#define PLATFORM_IMAGE_GENERATOR(data) \
  SkImageGeneratorCG::MakeFromEncodedCG(data)
#elif OS_WIN
#include "third_party/skia/include/ports/SkImageGeneratorWIC.h"
#define PLATFORM_IMAGE_GENERATOR(data) \
  SkImageGeneratorWIC::MakeFromEncodedWIC(data)
#else
#define PLATFORM_IMAGE_GENERATOR(data) \
  std::unique_ptr<SkImageGenerator>(nullptr)
#endif

namespace clay {

// static
fml::RefPtr<ImageDescriptor> ImageDescriptor::Create(
    sk_sp<SkData> data, bool enable_low_quality_image) {
  if (!data) {
    FML_DLOG(ERROR) << "Buffer parameter must not be null";
    return nullptr;
  }

  // This call will succeed if Skia has a built-in codec for this.
  // If it fails, we will check if the platform knows how to decode this image.
  std::unique_ptr<SkCodec> codec = SkCodec::MakeFromData(data);
  fml::RefPtr<ImageDescriptor> descriptor;
  if (!codec) {
    std::unique_ptr<SkImageGenerator> generator =
        PLATFORM_IMAGE_GENERATOR(data);
    if (!generator) {
      // We don't have a Skia codec for this image, and the platform doesn't
      // know how to decode it.
#ifdef ENABLE_TT_HEIF_DECODER
      // Try using fresco decoder
      const uint8_t* heif_data = data->bytes();
      uint32_t data_size = data->size();
      if (HeifDecoder::IsHeifFile(heif_data, data_size)) {
        uint32_t width, height, rotation = 0;
        uint64_t duration = 0;
        bool is_sequence = false;
        HeifDecoder::GetHeifImageInfo(heif_data, data_size, &width, &height,
                                      &rotation, &is_sequence, &duration);
        if (!is_sequence) {
          HeifOutputStream output =
              enable_low_quality_image
                  ? HeifDecoder::DecodeStaticHeifImgToRgb(heif_data, data_size,
                                                          width, height)
                  : HeifDecoder::DecodeStaticHeifImgToRgba(heif_data, data_size,
                                                           width, height);
          if (output.size == 0 || output.data == nullptr) {
            FML_DLOG(ERROR) << "Fresco decode failed";
            return nullptr;
          }
          sk_sp<SkData> data = SkData::MakeWithCopy(output.data, output.size);
          return ImageDescriptorSkia::Create(
              data, width, height, -1,
              enable_low_quality_image
                  ? ImageDescriptorSkia::PixelFormat::kRGB565
                  : ImageDescriptorSkia::PixelFormat::kRGBA8888);
        } else {
          // TODO(fangzheyuan): support anim heic.
          FML_DLOG(ERROR) << "UnSupport decoding anim heic format current";
          return nullptr;
        }
      }
#endif
      FML_DLOG(ERROR) << "Invalid image data";
      return nullptr;
    }
    descriptor =
        fml::MakeRefCounted<ImageDescriptorSkia>(data, std::move(generator));
  } else {
    if (enable_low_quality_image) {
      auto width = codec->dimensions().width();
      auto height = codec->dimensions().height();
      auto image_info = SkImageInfo::Make(width, height, kRGB_565_SkColorType,
                                          kOpaque_SkAlphaType);
      descriptor = fml::MakeRefCounted<ImageDescriptorSkia>(data, image_info,
                                                            std::move(codec));
    } else {
      descriptor =
          fml::MakeRefCounted<ImageDescriptorSkia>(data, std::move(codec));
    }
  }
  FML_DCHECK(descriptor);
  return descriptor;
}

fml::RefPtr<ImageDescriptorSkia> ImageDescriptorSkia::Create(
    sk_sp<SkData> data, int width, int height, int row_bytes,
    PixelFormat pixel_format) {
  if (!data) {
    FML_DLOG(ERROR) << "Buffer parameter must not be null";
    return nullptr;
  }

  SkColorType color_type = kUnknown_SkColorType;
  SkAlphaType alpha_type = kPremul_SkAlphaType;
  switch (pixel_format) {
    case PixelFormat::kRGBA8888:
      color_type = kRGBA_8888_SkColorType;
      break;
    case PixelFormat::kBGRA8888:
      color_type = kBGRA_8888_SkColorType;
      break;
    case PixelFormat::kRGB565:
      color_type = kRGB_565_SkColorType;
      alpha_type = kOpaque_SkAlphaType;
      break;
  }
  FML_DCHECK(color_type != kUnknown_SkColorType);
  auto image_info = SkImageInfo::Make(width, height, color_type, alpha_type);
  return fml::MakeRefCounted<ImageDescriptorSkia>(
      data, std::move(image_info),
      row_bytes == -1 ? std::nullopt : std::optional<size_t>(row_bytes));
}

ImageDescriptorSkia::ImageDescriptorSkia(sk_sp<SkData> buffer,
                                         const SkImageInfo& image_info,
                                         std::optional<size_t> row_bytes)
    : ImageDescriptor(buffer, row_bytes) {
  image_info_ = std::move(image_info);
}

ImageDescriptorSkia::ImageDescriptorSkia(sk_sp<SkData> buffer,
                                         std::unique_ptr<SkCodec> codec)
    : ImageDescriptor(buffer),
      generator_(std::shared_ptr<SkCodecImageGenerator>(
          static_cast<SkCodecImageGenerator*>(
              SkCodecImageGenerator::MakeFromCodec(std::move(codec))
                  .release()))),
      platform_image_generator_(nullptr) {
  image_info_ = CreateImageInfo();
  single_frame_ = !generator_ || generator_->getFrameCount() == 1;
}

ImageDescriptorSkia::ImageDescriptorSkia(sk_sp<SkData> buffer,
                                         const SkImageInfo& image_info,
                                         std::unique_ptr<SkCodec> codec)
    : ImageDescriptor(buffer),
      generator_(std::shared_ptr<SkCodecImageGenerator>(
          static_cast<SkCodecImageGenerator*>(
              SkCodecImageGenerator::MakeFromCodec(std::move(codec))
                  .release()))),
      platform_image_generator_(nullptr) {
  image_info_ = image_info;
  single_frame_ = !generator_ || generator_->getFrameCount() == 1;
}

ImageDescriptorSkia::ImageDescriptorSkia(
    sk_sp<SkData> buffer, std::unique_ptr<SkImageGenerator> generator)
    : ImageDescriptor(buffer),
      generator_(nullptr),
      platform_image_generator_(std::move(generator)) {
  image_info_ = CreateImageInfo();
}

const SkImageInfo ImageDescriptorSkia::CreateImageInfo() const {
  if (generator_) {
    return generator_->getInfo();
  }
  if (platform_image_generator_) {
    return platform_image_generator_->getInfo();
  }
  return SkImageInfo::MakeUnknown();
}

fml::RefPtr<Codec> ImageDescriptorSkia::InstantiateCodec(int target_width,
                                                         int target_height) {
  fml::RefPtr<Codec> ui_codec;
  if (!generator_ || generator_->getFrameCount() == 1) {
    ui_codec = fml::MakeRefCounted<SingleFrameCodec>(
        fml::RefPtr<ImageDescriptor>(this), target_width, target_height);
  } else {
    ui_codec = fml::MakeRefCounted<MultiFrameCodec>(generator_);
  }
  FML_DCHECK(ui_codec);
  return ui_codec;
}

fml::RefPtr<GraphicsImage> ImageDescriptorSkia::image() const {
  SkAlphaType codec_alpha_type = GetCodecAlphaType();
  // When using skia generator, if the origin image has alpha channel, we need
  // to convert it to 565 manually. Otherwise decode will fail.
  if (generator_ != nullptr &&
      image_info_.colorType() == kRGB_565_SkColorType &&
      codec_alpha_type != kOpaque_SkAlphaType) {
    return Get565Image();
  }
  SkBitmap bitmap;
  if (!bitmap.tryAllocPixels(image_info_)) {
    FML_DLOG(ERROR) << "Failed to allocate memory for bitmap of size "
                    << image_info_.computeMinByteSize() << "B";
    return nullptr;
  }

  const auto& pixmap = bitmap.pixmap();
  if (!GetPixels(pixmap)) {
    FML_DLOG(ERROR) << "Failed to get pixels for image.";
    return nullptr;
  }
  bitmap.setImmutable();
  return GraphicsImage::MakeFromBitmap(bitmap);
}

bool ImageDescriptorSkia::GetPixels(const SkPixmap& pixmap) const {
  fml::UniqueLock lock(*mutex_);
  if (generator_) {
    return generator_->getPixels(pixmap.info(), pixmap.writable_addr(),
                                 pixmap.rowBytes());
  }
  FML_DCHECK(platform_image_generator_);
  return platform_image_generator_->getPixels(pixmap);
}

SkAlphaType ImageDescriptorSkia::GetCodecAlphaType() const {
  if (generator_) {
    return generator_->getInfo().alphaType();
  }
  FML_DCHECK(platform_image_generator_);
  return platform_image_generator_->getInfo().alphaType();
}

fml::RefPtr<GraphicsImage> ImageDescriptorSkia::Get565Image() const {
  FML_DCHECK(generator_);
  SkImageInfo info = generator_->getInfo();
  int width = info.width();
  int height = info.height();

  SkBitmap temp_bitmap;
  SkImageInfo temp_info =
      info.makeColorType(kN32_SkColorType).makeAlphaType(kPremul_SkAlphaType);
  if (!temp_bitmap.tryAllocPixels(temp_info)) {
    FML_DLOG(ERROR) << "Failed to allocate memory for bitmap of size "
                    << temp_info.computeMinByteSize() << "B";
    return nullptr;
  }

  if (!generator_->getPixels(temp_info, temp_bitmap.getPixels(),
                             temp_bitmap.rowBytes())) {
    FML_DLOG(ERROR) << "Failed to get pixels for image.";
    return nullptr;
  }

  SkImageInfo dst_info = info.makeColorType(kRGB_565_SkColorType)
                             .makeAlphaType(kOpaque_SkAlphaType);
  SkBitmap dst_bitmap;
  if (!dst_bitmap.tryAllocPixels(dst_info)) {
    FML_DLOG(ERROR) << "Failed to allocate memory for bitmap of size "
                    << dst_info.computeMinByteSize() << "B";
    return nullptr;
  }

  for (int y = 0; y < height; ++y) {
    uint32_t* src_row = temp_bitmap.getAddr32(0, y);
    uint16_t* dst_row = dst_bitmap.getAddr16(0, y);
    for (int x = 0; x < width; ++x) {
      uint32_t c = src_row[x];
      // Convert 8888->565
      unsigned r = SkGetPackedR32(c);
      unsigned g = SkGetPackedG32(c);
      unsigned b = SkGetPackedB32(c);
      dst_row[x] = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    }
  }

  dst_bitmap.setImmutable();
  return GraphicsImage::MakeFromBitmap(dst_bitmap);
}

}  // namespace clay
