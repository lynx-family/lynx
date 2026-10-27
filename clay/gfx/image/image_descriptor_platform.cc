// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_descriptor_platform.h"

#include "base/include/fml/synchronization/shared_mutex.h"
#include "base/trace/native/trace_event.h"
#include "build/build_config.h"
#include "clay/fml/logging.h"
#include "clay/fml/mapping.h"
#include "clay/gfx/image/image_decoder.h"
#include "clay/ui/common/isolate.h"

namespace clay {

class FrameCodec : public Codec {
 public:
  explicit FrameCodec(fml::RefPtr<ImageDescriptorPlatform> descriptor,
                      int target_width, int target_height)
      : descriptor_(descriptor),
        target_width_(target_width),
        target_height_(target_height) {}

  int FrameCount() const override { return 0; }

  void NextFrame(const CodecCallback& callback) override {}

  int FrameDuration(int index) const override { return 0; }

 private:
  fml::RefPtr<ImageDescriptorPlatform> descriptor_;
  [[maybe_unused]] int target_width_;
  [[maybe_unused]] int target_height_;
};

// static
fml::RefPtr<ImageDescriptor> ImageDescriptor::Create(
    GrDataPtr data, bool enable_low_quality_image) {
  return nullptr;
}

fml::RefPtr<ImageDescriptor> ImageDescriptorPlatform::Create(
    std::shared_ptr<PlatformImage> codec) {
  return fml::MakeRefCounted<ImageDescriptorPlatform>(nullptr, codec);
}

ImageDescriptorPlatform::ImageDescriptorPlatform(
    GrDataPtr buffer, std::shared_ptr<PlatformImage> codec)
    : ImageDescriptorPlatform(
          buffer, IMAGE_INFO_MAKE_WH(codec->GetWidth(), codec->GetHeight()),
          codec) {}

ImageDescriptorPlatform::ImageDescriptorPlatform(
    GrDataPtr buffer, const GrImageInfo& image_info,
    std::shared_ptr<PlatformImage> image)
    : ImageDescriptor(buffer), image_(image) {
  image_info_ = image_info;
  // single_frame_ = codec_->GetFrameCount() == 1;
}

fml::RefPtr<Codec> ImageDescriptorPlatform::InstantiateCodec(
    int target_width, int target_height) {
  return fml::MakeRefCounted<FrameCodec>(
      fml::RefPtr<ImageDescriptorPlatform>(this), target_width, target_height);
}

}  // namespace clay
