// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_service_harmony.h"

#include <js_native_api.h>
#include <js_native_api_types.h>

#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_data.h"
#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_service_node.h"

namespace lynx {
namespace service {

napi_value InitLynxImageService(napi_env env, napi_callback_info info) {
  static ImageServiceHarmony instance;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  napi_create_reference(env, args[0], 0, &instance.resource_manager_ref_);
  instance.env_ = env;
  return base::NapiUtil::CreatePtrArray(env,
                                        reinterpret_cast<uint64_t>(&instance));
}

std::unique_ptr<tasm::harmony::ImageNode>
ImageServiceHarmony::CreateImageNode() {
  return std::unique_ptr<tasm::harmony::ImageNode>(new ImageServiceNode(this));
}

void ImageServiceHarmony::DecodeImage(
    const tasm::harmony::ImageRequestInfo& info, ImageDataCallback callback,
    ImageSuccessCallback on_load_success, ImageFailedCallback on_load_failed) {
  auto options = std::make_shared<ImageKnifePro::ImageKnifeOption>();
  options->onLoadListener = std::make_shared<ImageKnifePro::OnLoadCallBack>();
  options->onLoadListener->onLoadSuccess =
      [on_load_success = std::move(on_load_success)](
          ImageKnifePro::ImageInfo image_info) mutable {
        on_load_success(image_info.imageWidth, image_info.imageHeight);
      };
  options->onLoadListener->onLoadFailed =
      [on_load_failed = std::move(on_load_failed)](
          std::string error, ImageKnifePro::ImageInfo image_info) mutable {
        on_load_failed(static_cast<int>(image_info.errorInfo.code), error);
      };
  ImageServiceNode::UpdateImageSource(this, options, info.url,
                                      options->loadSrc);
  ImageKnifePro::ImageKnife::GetInstance().GetCacheImage(
      options, [callback = std::move(callback)](
                   std::shared_ptr<ImageKnifePro::ImageData> data) mutable {
        callback(std::make_shared<ImageKnifeImageData>(data));
      });
}

NativeResourceManager* ImageServiceHarmony::CreateNativeResourceManager()
    const {
  base::NapiHandleScope scope(env_);
  auto value =
      base::NapiUtil::GetReferenceNapiValue(env_, resource_manager_ref_);
  return OH_ResourceManager_InitNativeResourceManager(env_, value);
}

napi_value ImageServiceHarmony::Init(napi_env env, napi_value exports) {
  NAPI_CREATE_FUNCTION(env, exports, "nativeInitLynxImageService",
                       InitLynxImageService);
  return exports;
}

napi_value ImageServiceHarmony::Constructor(napi_env env,
                                            napi_callback_info info) {
  napi_value js_this = nullptr;
  return js_this;
}

}  // namespace service
}  // namespace lynx
