// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/gfx/image/static_image.h"

#include <algorithm>
#include <memory>

#include "clay/fml/logging.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/skity_to_skia_utils.h"

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
  return std::max(scaling_width, scaling_height);
}

std::shared_ptr<skity::Pixmap> ScaleImage(std::shared_ptr<skity::Pixmap> pixmap,
                                          float scale) {
  size_t image_width = pixmap->Width();
  size_t image_height = pixmap->Height();

  if (scale == 1.f) {
    return pixmap;
  }

  auto image = skity::Image::MakeImage(pixmap);
  if (!image) {
    return pixmap;
  }

  // Scale image.
  int scaled_width = scale * image_width;
  int scaled_height = scale * image_height;
  std::shared_ptr<skity::Pixmap> scaled_pixmap =
      std::make_shared<skity::Pixmap>(scaled_width, scaled_height,
                                      pixmap->GetAlphaType(),
                                      pixmap->GetColorType());

  if (!image->ScalePixels(scaled_pixmap, nullptr,
                          skity::SamplingOptions(skity::FilterMode::kLinear,
                                                 skity::MipmapMode::kNone))) {
    return pixmap;
  }

  return scaled_pixmap;
}
}  // namespace

namespace clay {
std::shared_ptr<StaticImage> StaticImage::Make(
    fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
    std::shared_ptr<PlatformImage> platform_image) {
  auto image = std::shared_ptr<StaticImage>(new StaticImage);
  image->type_ = ImageType::kStatic;
  image->image_fetcher_ = image_fetcher;
  image->url_ = std::move(url);
  image->image_ = platform_image;
  return image;
}

void StaticImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  if (!unref_queue || !unref_queue->GetContext()) {
    FML_LOG(ERROR) << "StaticImage::Upload: unref_queue or context is null";
    return;
  }
  if (!gpu_image_.object()) {
    auto pixmap = image_->ToBitmap();
    if (!pixmap) {
      FML_LOG(ERROR) << "StaticImage::Upload: Bitmap is null";
      return;
    }

    float scale = GetScale(pixmap->Width(), pixmap->Height(), size.width(),
                           size.height());
    int scaled_width = scale * pixmap->Width();
    int scaled_height = scale * pixmap->Height();

    auto image = skity::Image::MakeDeferredTextureImage(
        skity::Texture::FormatFromColorType(pixmap->GetColorType()),
        scaled_width, scaled_height, pixmap->GetAlphaType());
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
    unref_queue->GetTaskRunner()->PostTask([context = unref_queue->GetContext(),
                                            image, pixmap, scale,
                                            weak = weak_from_this()]() {
      if (auto self = weak.lock()) {
        auto final_pixmap = ScaleImage(pixmap, scale);
        auto texture = context->CreateTexture(
            skity::Texture::FormatFromColorType(final_pixmap->GetColorType()),
            final_pixmap->Width(), final_pixmap->Height(),
            final_pixmap->GetAlphaType());
        if (texture) {
          texture->DeferredUploadImage(std::move(final_pixmap));
          image->SetTexture(texture);
        }
      }
    });
  }
}

}  // namespace clay
