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

}  // namespace clay
