// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_HELPER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_HELPER_H_

#include <multimedia/image_framework/image/image_source_native.h>
#include <node_api.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_effect_processor.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxImageHelper {
 public:
  using PixelmapDeleter = void (*)(OH_PixelmapNative*);
  struct ImageResponse {
    std::unique_ptr<OH_PixelmapNative, PixelmapDeleter> data{nullptr,
                                                             &ReleasePixelmap};
    Image_ErrorCode err_code{IMAGE_SUCCESS};
    uint32_t image_width{0};
    uint32_t image_height{0};

    bool Success() { return err_code == IMAGE_SUCCESS; };
  };

  struct CallbackContext {
    napi_env env = nullptr;
    std::string url;
    bool is_base64{false};
    ImageResponse response{};
    napi_async_work work;
    base::MoveOnlyClosure<void, ImageResponse&> callback;
    LynxImageEffectProcessor params;
  };

  static void DecodeImageAsync(
      napi_env env, const std::string& url, bool is_base64,
      base::MoveOnlyClosure<void, ImageResponse&> callback,
      LynxImageEffectProcessor params = {});

  static ImageResponse DecodeImageSync(const std::string& url, bool is_base64,
                                       const LynxImageEffectProcessor& params);

 private:
  static void DecodeImageFromImageSource(
      OH_ImageSourceNative* image_source, ImageResponse& response,
      const LynxImageEffectProcessor& params);
  static void ReleasePixelmap(OH_PixelmapNative* pixel_map) {
    OH_PixelmapNative_Release(pixel_map);
  }
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_HELPER_H_
