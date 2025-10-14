// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/skimage_holder.h"

#include <utility>

#include "clay/gfx/graphics_isolate.h"

namespace clay {

SkImageHolder::SkImageHolder(bool decode_with_hierarchy)
    : decode_with_hierarchy_(decode_with_hierarchy) {}

SkImageHolder::~SkImageHolder() { ReleaseResource(); }

fml::RefPtr<GraphicsImage> SkImageHolder::GetGraphicsImage() {
  return image_.object();
}

void SkImageHolder::CacheFrame(int duration, GPUObject<GraphicsImage> image,
                               bool promise) {
  duration_ = duration;
  if (!promise) {
    // if promise, frame_ already has a promise image with texture backend.
    image_ = std::move(image);
  }
  if (decode_with_hierarchy_) {
    state_ = State::kUploaded;
  } else {
    state_ = State::kDecoded;
  }
  UpdateAllocationSize();
}

void SkImageHolder::ReleaseResource() {
  state_ = State::kNotDecode;
  duration_ = 0;
  image_.reset();
}

int SkImageHolder::FrameDuration() const { return duration_; }

void SkImageHolder::SetFrameDuration(int duration) { duration_ = duration; }

void SkImageHolder::UpdateAllocationSize() {
  if (image_.object()) {
    const auto& info = image_.object()->imageInfo();
#ifndef ENABLE_SKITY
    const size_t image_byte_size = info.computeMinByteSize();
#else
    const size_t image_byte_size =
        info.width() * info.height() * info.bytesPerPixel();
#endif  // ENABLE_SKITY
    allocation_size_ = image_byte_size + sizeof(*this);
  } else {
    allocation_size_ = sizeof(*this);
  }
}

}  // namespace clay
