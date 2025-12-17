// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/screenshot_encoder.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

#include "clay/fml/base64.h"
#include "clay/fml/logging.h"

namespace clay {

namespace {
float GetScale(int original_width, int original_height, int max_width,
               int max_height) {
  float scaling_width = 1.0;
  float scaling_height = 1.0;

  if (max_width != 0 && max_height != 0 &&
      ((original_width > max_width) || (original_height > max_height))) {
    scaling_width = max_width / (float)(original_width);
    scaling_height = max_height / (float)(original_height);
  }
  return std::min(scaling_width, scaling_height);
}
}  // namespace

// static
ScreenshotEncodeResult ScreenshotEncoder::ScaleAndEncode(
    ScreenshotData screenshot, const clay::ScreenshotRequest& request) {
  auto data = screenshot.data;
  if (!data) {
    FML_DLOG(ERROR) << "Failed to take screenshot with empty data!";
    return {};
  }
  if (request.page_width_ == 0 || request.page_height_ == 0) {
    FML_DLOG(ERROR) << "Take screenshot but image width or size is zero!";
    return {};
  }

#ifndef ENABLE_SKITY
  // Declare scaled_bitmap outside the ScaleImage function to prevent the local
  // variable from being released when the function stack unwinds which may
  // cause the dangling pointer exception during Encode caused by the
  // invalidation of the pixelRef.
  SkBitmap scaled_bitmap;
  SkPixmap scaled_pixmap = ScaleImage(screenshot, scaled_bitmap, request);
  return Encode(scaled_pixmap, request);
#else
  std::shared_ptr<skity::Pixmap> scaled_pixmap =
      ScaleImage(screenshot, request);
  return Encode(scaled_pixmap, request);
#endif  // ENABLE_SKITY
}

#ifndef ENABLE_SKITY
SkPixmap ScreenshotEncoder::ScaleImage(ScreenshotData screenshot,
                                       SkBitmap& scaled_bitmap,
                                       const clay::ScreenshotRequest& request) {
  auto data = screenshot.data;
  size_t image_width = screenshot.frame_size.x;
  size_t image_height = screenshot.frame_size.y;

  auto image_info = SkImageInfo::Make(
      image_width, image_height, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
  auto image = SkImages::RasterFromData(image_info, data, image_width * 4);

  if (!image) {
    FML_DLOG(ERROR) << "Take screenshot but image is null!";
    return SkPixmap();
  }

  float scale = GetScale(image_width, image_height, request.max_width_,
                         request.max_height_);
  if (scale == 1.f) {
    SkPixmap pixmap;
    if (!image->peekPixels(&pixmap)) {
      FML_DLOG(ERROR) << "TakeSnapshot, peekPixels failed!";
      return SkPixmap();
    }
    return pixmap;
  }
  int scaled_width = scale * image_width;
  int scaled_height = scale * image_height;

  SkImageInfo scaled_image_info = image->imageInfo().makeDimensions(
      SkISize::Make(scaled_width, scaled_height));

  if (!scaled_bitmap.tryAllocPixels(scaled_image_info)) {
    FML_DLOG(ERROR) << "TakeSnapshot, tryAllocPixels failed!";
    return SkPixmap();
  }

  SkPixmap scaled_pixmap;
  if (!scaled_bitmap.peekPixels(&scaled_pixmap)) {
    FML_DLOG(ERROR) << "TakeSnapshot, peekPixels failed!";
    return SkPixmap();
  }

  if (!image->scalePixels(
          scaled_pixmap,
          SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone),
          SkImage::kDisallow_CachingHint)) {
    FML_DLOG(ERROR) << "TakeSnapshot, scalePixels failed!";
    return SkPixmap();
  }

  return scaled_pixmap;
}

// static
ScreenshotEncodeResult ScreenshotEncoder::Encode(
    const SkPixmap& pixmap, const clay::ScreenshotRequest& request) {
  if (pixmap.addr() == nullptr) {
    FML_DLOG(ERROR) << "Encode screenshot but pixmap is null!";
    return {};
  }

  clay::ScreenMetadata metadata;
  metadata.device_width_ = request.page_width_;
  metadata.device_height_ = request.page_height_;
  metadata.page_scale_factor_ = request.screen_scale_factor_;

  SkDynamicMemoryWStream sk_stream;

  switch (request.type_) {
    case clay::ScreenshotType::JPEG: {
      SkJpegEncoder::Options options;
      if (request.quality_ >= 0 && request.quality_ <= 100) {
        options.fQuality = request.quality_;
      }
      if (!SkJpegEncoder::Encode(&sk_stream, pixmap, options)) {
        FML_DLOG(ERROR) << "TakeSnapshot, SkJpegEncoder::Encode failed!";
        return {};
      }
      break;
    }
    case clay::ScreenshotType::WEBP: {
      FML_UNIMPLEMENTED();
      FML_DCHECK(false);
      return {};
      break;
    }
    case clay::ScreenshotType::PNG: {
      SkPngEncoder::Options options;
      if (!SkPngEncoder::Encode(&sk_stream, pixmap, options)) {
        FML_DLOG(ERROR) << "TakeSnapshot, SkPngEncoder::Encode failed!";
        return {};
      }
      break;
    }
    case clay::ScreenshotType::BITMAP: {
      sk_sp<SkData> data =
          SkData::MakeWithCopy(pixmap.addr32(), pixmap.computeByteSize());
      metadata.timestamp_ =
          std::chrono::steady_clock::now().time_since_epoch().count();
      return {data, metadata};
    }
    default:
      FML_DCHECK(false);
      return {};
  }

  auto image_data = sk_stream.detachAsData();
  if (!image_data) {
    return {};
  }
  size_t length =
      fml::Base64::Encode(image_data->data(), image_data->size(), nullptr);
  auto base64_data = SkData::MakeUninitialized(length);

  fml::Base64::Encode(image_data->data(), image_data->size(),
                      base64_data->writable_data());

  metadata.timestamp_ =
      std::chrono::steady_clock::now().time_since_epoch().count();
  return {base64_data, metadata};
}
#else
std::shared_ptr<skity::Pixmap> ScreenshotEncoder::ScaleImage(
    ScreenshotData screenshot, const clay::ScreenshotRequest& request) {
  auto data = screenshot.data;
  size_t image_width = screenshot.frame_size.x;
  size_t image_height = screenshot.frame_size.y;

  std::shared_ptr<skity::Data> skity_data =
      skity::Data::MakeWithCopy(data->RawData(), data->Size());
  std::shared_ptr<skity::Pixmap> pixmap = std::make_shared<skity::Pixmap>(
      std::move(skity_data), image_width, image_height);

  float scale = GetScale(image_width, image_height, request.max_width_,
                         request.max_height_);
  if (scale == 1.f) {
    return pixmap;
  }

  auto image = skity::Image::MakeImage(std::move(pixmap));
  if (!image) {
    FML_DLOG(ERROR) << "Take screenshot but image is null!";
    return nullptr;
  }

  // Scale image.
  int scaled_width = scale * image_width;
  int scaled_height = scale * image_height;
  std::shared_ptr<skity::Pixmap> scaled_pixmap =
      std::make_shared<skity::Pixmap>(scaled_width, scaled_height);

  if (!image->ScalePixels(scaled_pixmap, nullptr,
                          skity::SamplingOptions(skity::FilterMode::kLinear,
                                                 skity::MipmapMode::kNone))) {
    FML_DLOG(ERROR) << "TakeSnapshot, scalePixels failed!";
    return nullptr;
  }

  return scaled_pixmap;
}

ScreenshotEncodeResult ScreenshotEncoder::Encode(
    std::shared_ptr<skity::Pixmap> pixmap,
    const clay::ScreenshotRequest& request) {
  if (!pixmap) {
    return {};
  }

  clay::ScreenMetadata metadata;
  metadata.device_width_ = request.page_width_;
  metadata.device_height_ = request.page_height_;
  metadata.page_scale_factor_ = request.screen_scale_factor_;

  if (request.type_ == clay::ScreenshotType::BITMAP) {
    std::shared_ptr<skity::Data> data = skity::Data::MakeWithCopy(
        pixmap->Addr(), pixmap->RowBytes() * pixmap->Height());
    metadata.timestamp_ =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return {data, metadata};
  }
  if (request.type_ != clay::ScreenshotType::JPEG) {
    FML_DLOG(ERROR) << "Skity can only encode JPEG image for now!";
    return {};
  }

  // Encode image.
  auto jpeg_codec = clay::JPEGCodecSkity();
  auto jpeg_data = jpeg_codec.Encode(pixmap.get());
  if (!jpeg_data) {
    FML_DLOG(ERROR) << "TakeSnapshot, Skity JpegCodec::Encode failed!";
    return {};
  }
  std::shared_ptr<skity::Data> image_data =
      skity::Data::MakeWithCopy(jpeg_data->RawData(), jpeg_data->Size());
  if (!image_data) {
    return {};
  }
  size_t length =
      fml::Base64::Encode(image_data->RawData(), image_data->Size(), nullptr);
  void* pixels = malloc(length);
  std::shared_ptr<skity::Data> base64_data =
      skity::Data::MakeFromMalloc(pixels, length);

  fml::Base64::Encode(image_data->RawData(), image_data->Size(),
                      const_cast<void*>(base64_data->RawData()));

  metadata.timestamp_ =
      std::chrono::steady_clock::now().time_since_epoch().count();
  return {base64_data, metadata};
}
#endif  // ENABLE_SKITY

}  // namespace clay
