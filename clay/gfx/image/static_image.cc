// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/gfx/image/static_image.h"

#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/image/graphics_image_skity.h"

namespace clay {
std::shared_ptr<StaticImage> StaticImage::Make(
    fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
    std::shared_ptr<PlatformImage> platform_image,
    std::shared_ptr<ClayMemoryTracker> memory_tracker) {
  auto image = std::shared_ptr<StaticImage>(new StaticImage);
  image->type_ = ImageType::kStatic;
  image->image_fetcher_ = image_fetcher;
  image->url_ = std::move(url);
  image->image_ = std::move(platform_image);
  image->memory_tracker_ = std::move(memory_tracker);
  image->orig_info_ =
      ImageInfo::makeWH(image->image_->GetWidth(), image->image_->GetHeight());
  return image;
}

void StaticImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  if (!unref_queue || !unref_queue->GetContext()) {
    FML_LOG(ERROR) << "StaticImage::Upload: unref_queue or context is null";
    return;
  }

  if (!gpu_image_.object()) {
    if (render_info_.isEmpty()) {
      render_info_ = orig_info_;
    }

    bool need_mipmapped = IsMipmapped() && HasResized();
    auto image = skity::Image::MakeDeferredTextureImage(
        skity::Texture::FormatFromColorType(image_->GetColorType()),
        render_info_.width(), render_info_.height(), image_->GetAlphaType());
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
    unref_queue->GetTaskRunner()->PostTask(
        [context = unref_queue->GetContext(), image, render_info = render_info_,
         mipmapped = need_mipmapped, weak = weak_from_this()]() {
          if (auto self = std::static_pointer_cast<StaticImage>(weak.lock())) {
            auto pixmap = self->image_->ToBitmap(render_info);
            if (!pixmap) {
              FML_LOG(ERROR) << "StaticImage::Upload: Bitmap is null";
              return;
            }
            SetDeferredRasterImageTexture(*context, *image, std::move(pixmap),
                                          mipmapped, self->memory_tracker_);
          }
        });
  }
}

}  // namespace clay
