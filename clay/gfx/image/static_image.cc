// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/gfx/image/static_image.h"

#include <memory>

#include "clay/fml/logging.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {
std::shared_ptr<StaticImage> StaticImage::Make(
    std::shared_ptr<PlatformImage> platform_image) {
  auto image = std::shared_ptr<StaticImage>(new StaticImage);
  image->type_ = ImageType::kStatic;
  image->image_ = platform_image;
  return image;
}

void StaticImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  if (!gpu_image_.object()) {
    auto bitmap = image_->ToBitmap();
    auto data = std::get<0>(bitmap);
    auto image_info = std::get<1>(bitmap);
    if (!data) {
      FML_LOG(ERROR) << "StaticImage::Upload: Bitmap is null";
      return;
    }
    auto alpha_type = ConvertToSkityAlphaType(image_info.alphaType());
    auto color_type = ConvertToSkityColorType(image_info.colorType());
    std::shared_ptr<skity::Pixmap> skity_pixmap =
        std::make_shared<skity::Pixmap>(
            std::move(data), image_info.width() * image_info.bytesPerPixel(),
            image_info.width(), image_info.height(), alpha_type, color_type);
    auto image = skity::Image::MakeDeferredTextureImage(
        skity::Texture::FormatFromColorType(color_type), image_info.width(),
        image_info.height(), alpha_type);
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
    unref_queue->GetTaskRunner()->PostTask(
        [context = unref_queue->GetContext(), image, skity_pixmap]() {
          auto texture = context->CreateTexture(
              skity::Texture::FormatFromColorType(skity_pixmap->GetColorType()),
              skity_pixmap->Width(), skity_pixmap->Height(),
              skity_pixmap->GetAlphaType());
          texture->DeferredUploadImage(std::move(skity_pixmap));
          image->SetTexture(texture);
        });
  }
}

}  // namespace clay
