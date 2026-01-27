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

std::shared_ptr<skity::Pixmap> ScaleImage(std::shared_ptr<skity::Pixmap> pixmap,
                                          clay::ImageInfo render_info) {
  int image_width = pixmap->Width();
  int image_height = pixmap->Height();

  if (image_width == render_info.width() &&
      image_height == render_info.height()) {
    return pixmap;
  }

  auto image = skity::Image::MakeImage(pixmap);
  if (!image) {
    return pixmap;
  }

  std::shared_ptr<skity::Pixmap> scaled_pixmap =
      std::make_shared<skity::Pixmap>(render_info.width(), render_info.height(),
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
  image->orig_info_ = ImageInfo::makeWH(platform_image->GetWidth(),
                                        platform_image->GetHeight());
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

    if (render_info_.isEmpty()) {
      render_info_ = orig_info_;
    }

    auto image = skity::Image::MakeDeferredTextureImage(
        skity::Texture::FormatFromColorType(pixmap->GetColorType()),
        render_info_.width(), render_info_.height(), pixmap->GetAlphaType());
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
    unref_queue->GetTaskRunner()->PostTask([context = unref_queue->GetContext(),
                                            image, pixmap,
                                            render_info = render_info_,
                                            weak = weak_from_this()]() {
      if (auto self = weak.lock()) {
        auto final_pixmap = ScaleImage(pixmap, render_info);
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
