// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_SKIMAGE_HOLDER_H_
#define CLAY_GFX_IMAGE_SKIMAGE_HOLDER_H_

#include <utility>

#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/graphics_image.h"

namespace clay {

class Codec;

namespace testing {
class ImagePainterTest;
}  // namespace testing

// SkImageHolder represents a SkImage holding GPU resources.
class SkImageHolder : public fml::RefCountedThreadSafe<SkImageHolder> {
 public:
  explicit SkImageHolder(bool decode_with_hierarchy = false);
  ~SkImageHolder();

  enum class State { kNotDecode = 0, kDecoding, kDecoded, kUploaded };

  fml::RefPtr<GraphicsImage> GetGraphicsImage();
  void SetPromiseImage(GPUObject<GraphicsImage> image) {
    image_ = std::move(image);
  }
  void ReleaseResource();

  int FrameDuration() const;
  void SetFrameDuration(int);

  bool FrameIsReady() const {
    return decode_with_hierarchy_ ? state_ == State::kUploaded
                                  : state_ == State::kDecoded;
  }
  bool FrameIsNotLoad() const { return state_ == State::kNotDecode; }
  void MarkLoadStart() {
    FML_DCHECK(FrameIsNotLoad());
    state_ = State::kDecoding;
  }
  void MarkLoadResult(bool result) {
    state_ = result ? State::kDecoded : State::kNotDecode;
  }
  void MarkUploadResult(bool result) {
    state_ = result ? State::kUploaded : State::kNotDecode;
  }

  void CacheFrame(int duration, GPUObject<GraphicsImage> image,
                  bool promise = false);

  size_t GetAllocationSize() const { return allocation_size_; }

  bool ShouldUseCache() const { return should_use_cache_; }
  void SetUseCache(bool use) { should_use_cache_ = use; }

  void set_timestamp(fml::TimePoint time) { timestamp_ = time; }
  fml::TimePoint timestamp() { return timestamp_; }

 private:
  friend class testing::ImagePainterTest;

  void UpdateAllocationSize();

  State state_ = State::kNotDecode;
  bool should_use_cache_ = true;
  size_t allocation_size_ = sizeof(SkImageHolder);
  GPUObject<GraphicsImage> image_;
  int duration_ = 0;
  fml::TimePoint timestamp_;

  bool decode_with_hierarchy_ = false;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_SKIMAGE_HOLDER_H_
