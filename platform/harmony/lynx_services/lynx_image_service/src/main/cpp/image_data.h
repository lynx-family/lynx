// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_DATA_H_
#define PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_DATA_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <utility>

#include "imageknife.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"

namespace lynx {
namespace service {

class ImageKnifeImageData final : public tasm::harmony::ImageData {
 public:
  explicit ImageKnifeImageData(std::shared_ptr<ImageKnifePro::ImageData> inner)
      : impl_(std::move(inner)) {}

  OH_PixelmapNative* Pixelmap() const override {
    return impl_ ? impl_->GetPixelmap() : nullptr;
  }

  uint32_t FrameCount() const override {
    return impl_ ? impl_->GetFrameCount() : 0;
  }

  OH_PixelmapNative** PixelmapList() const override {
    return impl_ ? impl_->GetPixelmapList() : nullptr;
  }

  int* DelayTimeList() const override {
    return impl_ ? impl_->GetDelayTimeList() : nullptr;
  }

  const std::shared_ptr<ImageKnifePro::ImageData>& Inner() const {
    return impl_;
  }

 private:
  std::shared_ptr<ImageKnifePro::ImageData> impl_;
};

}  // namespace service
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_DATA_H_
