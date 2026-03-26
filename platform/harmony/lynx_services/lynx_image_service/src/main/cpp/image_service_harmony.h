// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_SERVICE_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_SERVICE_HARMONY_H_

#include <node_api.h>
#include <rawfile/raw_file_manager.h>

#include <functional>
#include <memory>
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"

namespace lynx {
namespace service {

class ImageServiceHarmony : public tasm::harmony::ImageService {
 public:
  ImageServiceHarmony() = default;
  ~ImageServiceHarmony() override = default;

  std::unique_ptr<tasm::harmony::ImageNode> CreateImageNode() override;
  void DecodeImage(const tasm::harmony::ImageRequestInfo& info,
                   ImageDataCallback callback,
                   ImageSuccessCallback on_load_success,
                   ImageFailedCallback on_load_failed) override;
  NativeResourceManager* CreateNativeResourceManager() const;
  napi_env env_ = nullptr;
  napi_ref resource_manager_ref_ = nullptr;
  static napi_value Init(napi_env env, napi_value exports);
  static napi_value Constructor(napi_env env, napi_callback_info info);
};

}  // namespace service
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_SERVICE_HARMONY_H_
