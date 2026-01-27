// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/base_image.h"

#include "clay/ui/resource/image_fetcher.h"

namespace clay {

std::unique_ptr<BaseImageInstance> BaseImage::NewInstance() {
  return std::make_unique<BaseImageInstance>(shared_from_this());
}

void BaseImage::OnInstanceCreated(BaseImageInstance* instance) {
  instances_.insert(instance);
}

void BaseImage::OnInstanceDestroyed(BaseImageInstance* instance) {
  instances_.erase(instance);
  if (instances_.empty() && image_fetcher_) {
    image_fetcher_->OnImageHasNoAccessor(this);
  }
}

void BaseImage::SetExpectSizeCalculator(
    std::function<skity::Vec2(skity::Vec2)> calculator,
    bool force_use_original_size) {
  skity::Vec2 expect_size =
      calculator(skity::Vec2(orig_info_.width(), orig_info_.height()));
  // if origin_size can contain expect_size, use expect_size, else origin_size.
  auto chooser = [this, &expect_size, force_use_original_size]() {
    if (expect_size.x > orig_info_.width() ||
        expect_size.y > orig_info_.height() || force_use_original_size) {
      render_info_ = orig_info_;
    } else {
      render_info_ = ImageInfo::makeWH(expect_size.x, expect_size.y);
    }
  };

  // If hasn't set, just choose a proper size.
  if (render_info_.isEmpty()) {
    chooser();
    return;
  }

  // If expect_size enlarged, choose a new proper size.
  if (expect_size.x > render_info_.width() ||
      expect_size.y > render_info_.height() || force_use_original_size) {
    int old_width = render_info_.width();
    int old_height = render_info_.height();
    chooser();
    // If size changed, release graphics image.
    if (old_width != render_info_.width() ||
        old_height != render_info_.height()) {
      gpu_image_.reset();
    }
  }
}

}  // namespace clay
