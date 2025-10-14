// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skity/skity_lazy_image_traveller.h"

#include "clay/gfx/skity/skity_decoding_image.h"
#include "skity/graphic/image.hpp"

namespace clay {

bool SkityLazyImageTraveller::Traversal(
    skity::DisplayList* disp_list, const LazyImageDecodeCallback& callback) {
  TraversalHelper helper = TraversalHelper(callback);
  disp_list->Draw(&helper);
  return helper.hasLazyImage();
}

void SkityLazyImageTraveller::TraversalHelper::DecodeAndUploadImageIfNeeded(
    const std::shared_ptr<skity::Image> image) {
  if (image && image->GetImageType() != skity::ImageType::kCustom) {
    return;
  }
  SkityDecodingImage* decoding_image =
      static_cast<SkityDecodingImage*>(image.get());
  if (decoding_image) {
    // For multi-frame images, it is necessary to keep triggering redrawing to
    // drive the image animation.
    if (decoding_image->MaybeAnimated() || !decoding_image->gr_image()) {
      decoding_image->ScheduleDecodeAndUpload(callback_);
      has_lazy_image_ = true;
    }
  }
}

}  // namespace clay
