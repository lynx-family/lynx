// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_transform.h"

#include <node_api.h>

#include <memory>
#include <string>

#include "base/include/log/logging.h"
#include "imageknife.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"

namespace lynx {
namespace service {
bool ImageTransform::Transform(
    std::shared_ptr<ImageKnifePro::ImageKnifeTask> task) {
  if (!task) {
    return false;
  }

  for (const auto& processor : processors_) {
    std::shared_ptr<ImageKnifePro::ImageData> image_data =
        task->product.imageData;

    if (!image_data) {
      return false;
    }
    OH_PixelmapNative* result = processor->Process(image_data->GetPixelmap());
    if (result != nullptr) {
      // The `result` is managed by ImageKnifePro::ImageData
      task->product.imageData =
          std::make_shared<ImageKnifePro::ImageData>(result);
    } else {
      LOGE("image transform return null pixelmap, params: "
           << processor->Info());
    }
  }
  return true;
}

std::string ImageTransform::GetTransformInfo() {
  std::string info = "LynxTransform:[";
  for (const auto& processor : processors_) {
    info += processor->Info();
  }
  info += "]";
  return info;
}

}  // namespace service
}  // namespace lynx
