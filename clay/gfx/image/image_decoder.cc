// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_decoder.h"

#include <algorithm>
#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/trace/native/trace_event.h"
#include "clay/gfx/image/image_descriptor.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

fml::RefPtr<GraphicsImage> ResizeRasterImage(
    fml::RefPtr<GraphicsImage> image, const skity::Vec2& resized_dimensions) {
  FML_DCHECK(!image->isTextureBacked());

  if (resized_dimensions.x <= 0 || resized_dimensions.y <= 0) {
    FML_DLOG(ERROR) << "Could not resize to empty dimensions.";
    return nullptr;
  }

#ifndef ENABLE_SKITY
  if (image->dimensions() == resized_dimensions) {
    return image->makeRasterImage();
  }
#else
  FML_UNIMPLEMENTED();
#endif  // ENABLE_SKITY

  const auto scaled_image_info =
      IMAGE_INFO_MAKE_DIMENSIONS(image->imageInfo(), resized_dimensions);
#ifndef ENABLE_SKITY
  SkBitmap scaled_bitmap;
  if (!scaled_bitmap.tryAllocPixels(scaled_image_info)) {
    FML_LOG(ERROR) << "Failed to allocate memory for bitmap of size "
                   << scaled_image_info.computeMinByteSize() << "B";
    return nullptr;
  }

  if (!image->scalePixels(
          scaled_bitmap.pixmap(),
          SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone),
          SkImage::kDisallow_CachingHint)) {
    FML_LOG(ERROR) << "Could not scale pixels";
    return nullptr;
  }

  // Marking this as immutable makes the MakeFromBitmap call share the pixels
  // instead of copying.
  scaled_bitmap.setImmutable();
#else
  skity::Bitmap scaled_bitmap(scaled_image_info.width(),
                              scaled_image_info.height());
  if (!image->scalePixels(scaled_bitmap.GetPixmap(), nullptr,
                          skity::SamplingOptions(skity::FilterMode::kLinear,
                                                 skity::MipmapMode::kNone))) {
    FML_LOG(ERROR) << "Could not scale pixels";
    return nullptr;
  }
#endif  // ENABLE_SKITY

  auto scaled_image = GraphicsImage::MakeFromBitmap(scaled_bitmap);
  if (!scaled_image) {
    FML_LOG(ERROR) << "Could not create a scaled image from a scaled bitmap.";
    return nullptr;
  }

  return scaled_image;
}

static fml::RefPtr<GraphicsImage> ImageFromDecompressedData(
    ImageDescriptor* descriptor, uint32_t target_width,
    uint32_t target_height) {
  auto image = GraphicsImage::MakeRasterData(
      descriptor->image_info(), descriptor->data(), descriptor->RowBytes());

  if (!image) {
    FML_LOG(ERROR) << "Could not create image from decompressed bytes.";
    return nullptr;
  }

  if (!target_width && !target_height) {
    // No resizing requested. Just rasterize the image.
    return image->makeRasterImage();
  }

  return ResizeRasterImage(std::move(image),
                           skity::Vec2{target_width, target_height});
}

fml::RefPtr<GraphicsImage> ImageFromCompressedData(ImageDescriptor* descriptor,
                                                   uint32_t target_width,
                                                   uint32_t target_height) {
  if (!descriptor->ShouldResize(target_width, target_height)) {
    // No resizing requested. Just decode & rasterize the image.
    fml::RefPtr<GraphicsImage> image = descriptor->image();
    return image ? image->makeRasterImage() : nullptr;
  }

  const skity::Vec2 source_dimensions = {descriptor->width(),
                                         descriptor->height()};
  const skity::Vec2 resized_dimensions = {target_width, target_height};

  float scale =
      std::max(static_cast<double>(resized_dimensions.x) / source_dimensions.x,
               static_cast<double>(resized_dimensions.y) / source_dimensions.y);

  // The SkJpegCodec only supports scaled decoding to integer multiples of 1/8,
  // and rather than rounding up, it rounds off at the boundary of 1/16.
  // When `decode_dimensions` are smaller than `resized_dimensions`,
  // the image quality is poor, so we attempt to round up.
  skity::Vec2 decode_dimensions = descriptor->GetScaledDimensions(scale);
  while (scale < 1.0 && (decode_dimensions.x < resized_dimensions.x ||
                         decode_dimensions.y < resized_dimensions.y)) {
    scale += 0.0625;
    decode_dimensions = descriptor->GetScaledDimensions(scale);
  }

  // If the codec supports efficient sub-pixel decoding, decoded at a resolution
  // close to the target resolution before resizing.
  if (decode_dimensions != source_dimensions) {
    auto scaled_image_info =
        IMAGE_INFO_MAKE_DIMENSIONS(descriptor->image_info(), decode_dimensions);

#ifndef ENABLE_SKITY
    SkBitmap scaled_bitmap;
    if (!scaled_bitmap.tryAllocPixels(scaled_image_info)) {
      FML_LOG(ERROR) << "Failed to allocate memory for bitmap of size "
                     << scaled_image_info.computeMinByteSize() << "B";
      return nullptr;
    }

    const auto& pixmap = scaled_bitmap.pixmap();
    if (descriptor->GetPixels(pixmap)) {
      // Marking this as immutable makes the MakeFromBitmap call share
      // the pixels instead of copying.
      scaled_bitmap.setImmutable();

      auto decoded_image = GraphicsImage::MakeFromBitmap(scaled_bitmap);
      FML_DCHECK(decoded_image);
      if (!decoded_image) {
        FML_LOG(ERROR)
            << "Could not create a scaled image from a scaled bitmap.";
        return nullptr;
      }
      return ResizeRasterImage(std::move(decoded_image), resized_dimensions);
    }
#else
    // TODO(zhangxiao.ninja) these all are mock logic for compile, implement
    // later
    FML_UNIMPLEMENTED();
    skity::Bitmap scaled_bitmap(scaled_image_info.width(),
                                scaled_image_info.height());
    auto decoded_image = GraphicsImage::MakeFromBitmap(scaled_bitmap);
    FML_DCHECK(decoded_image);
    if (!decoded_image) {
      FML_LOG(ERROR) << "Could not create a scaled image from a scaled bitmap.";
      return nullptr;
    }
    return ResizeRasterImage(std::move(decoded_image), resized_dimensions);
#endif  // ENABLE_SKITY
  }

  auto image = descriptor->image();
  if (!image) {
    return nullptr;
  }

  return ResizeRasterImage(std::move(image), resized_dimensions);
}

ImageDecoder::ImageDecoder(
    std::shared_ptr<fml::ConcurrentTaskRunner> concurrent_task_runner)
    : concurrent_task_runner_(std::move(concurrent_task_runner)),
      weak_factory_(this) {}

ImageDecoder::~ImageDecoder() = default;

void ImageDecoder::Decode(fml::RefPtr<ImageDescriptor> descriptor_ref_ptr,
                          uint32_t target_width, uint32_t target_height,
                          const ImageResult& callback) {
  // `ImageDecoder::Decode` itself is invoked on the UI thread, so the
  // collection of the smart pointer from which we obtained the raw descriptor
  // is fine in this scope.
  auto raw_descriptor = descriptor_ref_ptr.get();
  raw_descriptor->AddRef();

  FML_DCHECK(callback);

  // Always service the callback (and cleanup the descriptor) on the UI thread.
  auto result = [callback, raw_descriptor](fml::RefPtr<GraphicsImage> image) {
    callback(std::move(image));
    raw_descriptor->Release();
  };

  if (!raw_descriptor->data() || DATA_GET_SIZE(raw_descriptor->data()) == 0) {
    result({});
    return;
  }

  concurrent_task_runner_->PostTask(
      fml::MakeCopyable([raw_descriptor,                //
                         result,                        //
                         target_width = target_width,   //
                         target_height = target_height  //
  ]() mutable {
        // Decompress the image.
        // On Worker.
        TRACE_EVENT("clay", "RK::DecodeImage");
        auto decompressed = raw_descriptor->IsCompressed()
                                ? ImageFromCompressedData(raw_descriptor,  //
                                                          target_width,    //
                                                          target_height)
                                : ImageFromDecompressedData(raw_descriptor,  //
                                                            target_width,    //
                                                            target_height);

        if (!decompressed) {
          FML_LOG(ERROR) << "Could not decompress image.";
          result({});
          return;
        }
        // All done.
        result(std::move(decompressed));
      }));
}

fml::WeakPtr<ImageDecoder> ImageDecoder::GetWeakPtr() const {
  return weak_factory_.GetWeakPtr();
}

}  // namespace clay
