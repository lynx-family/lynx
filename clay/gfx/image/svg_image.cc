// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/gfx/image/svg_image.h"

#include "clay/fml/logging.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/base_image.h"
#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {
std::shared_ptr<SVGImage> SVGImage::Make(const std::string& content) {
  auto image = std::shared_ptr<SVGImage>(new SVGImage(content));
  image->type_ = ImageType::kSVG;
  return image;
}

SVGImage::SVGImage(const std::string& content) : content_(content) {
  svg_dom_ = SVGDom::Create(
      GrData::MakeWithProc(content_.data(), content_.size(), nullptr, nullptr));
}

void SVGImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  if (!gpu_image_.object() || gpu_image_.object()->width() < size.width() ||
      gpu_image_.object()->height() < size.height()) {
    auto data = svg_dom_->Render(size.width(), size.height());
    if (!data) {
      FML_LOG(ERROR) << "SVGImage::Paint: " << size.width() << " "
                     << size.height();
      return;
    }
    auto image_info = ImageInfo::makeWH(size.width(), size.height());
    auto alpha_type = ConvertToSkityAlphaType(image_info.alphaType());
    auto color_type = ConvertToSkityColorType(image_info.colorType());
    std::shared_ptr<skity::Pixmap> pixmap = std::make_shared<skity::Pixmap>(
        std::move(data), image_info.width() * image_info.bytesPerPixel(),
        image_info.width(), image_info.height(), alpha_type, color_type);
    auto image = skity::Image::MakeDeferredTextureImage(
        skity::Texture::FormatFromColorType(color_type), image_info.width(),
        image_info.height(), alpha_type);
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
    unref_queue->GetTaskRunner()->PostTask([context = unref_queue->GetContext(),
                                            image, pixmap,
                                            weak = weak_from_this()]() {
      if (auto self = weak.lock()) {
        auto texture = context->CreateTexture(
            skity::Texture::FormatFromColorType(pixmap->GetColorType()),
            pixmap->Width(), pixmap->Height(), pixmap->GetAlphaType());
        if (texture) {
          texture->DeferredUploadImage(std::move(pixmap));
          image->SetTexture(texture);
        }
      }
    });
  }
}
}  // namespace clay
