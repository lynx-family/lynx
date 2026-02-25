// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/gfx/image/svg_image.h"

#include "base/include/md5.h"
#include "clay/fml/logging.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/base_image.h"
#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {
std::shared_ptr<SVGImage> SVGImage::Make(
    fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
    const std::string& content) {
  auto image = std::shared_ptr<SVGImage>(new SVGImage(content));
  image->type_ = ImageType::kSVG;
  image->image_fetcher_ = image_fetcher;
  image->url_ = std::move(url);
  return image;
}

SVGImage::SVGImage(const std::string& content) : content_(content) {
  svg_dom_ = SVGDom::Create(
      GrData::MakeWithProc(content_.data(), content_.size(), nullptr, nullptr),
      [](std::string url) { return nullptr; });
}

void SVGImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  if (!unref_queue || !unref_queue->GetContext()) {
    FML_LOG(ERROR) << "SVGImage::Upload: unref_queue or context is null";
    return;
  }
  if (!gpu_image_.object() || gpu_image_.object()->width() < size.width() ||
      gpu_image_.object()->height() < size.height()) {
    auto image = svg_dom_->Render(size.width(), size.height(), unref_queue);
    if (!image) {
      FML_LOG(ERROR) << "SVGImage::Paint: " << size.width() << " "
                     << size.height();
      return;
    }
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
  }
}
}  // namespace clay
