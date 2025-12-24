// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_TRANSFORM_H_
#define PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_TRANSFORM_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "imageknife.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_effect_processor.h"

namespace lynx {
namespace service {

class ImageTransform : public ImageKnifePro::Transformation {
 public:
  explicit ImageTransform(
      std::vector<std::unique_ptr<tasm::harmony::ImageProcessor>> processors)
      : processors_(std::move(processors)) {}
  bool Transform(std::shared_ptr<ImageKnifePro::ImageKnifeTask> task) override;

  std::string GetTransformInfo() override;

 private:
  std::vector<std::unique_ptr<tasm::harmony::ImageProcessor>> processors_;
};

}  // namespace service
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_TRANSFORM_H_
