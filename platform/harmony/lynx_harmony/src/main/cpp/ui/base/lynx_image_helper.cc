// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"

#include <arkui/drawable_descriptor.h>
#include <resourcemanager/ohresmgr.h>

#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/resource/lynx_resource_loader_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_pixel_map.h"

namespace lynx {
namespace tasm {
namespace harmony {
void LynxImageHelper::DecodeImageAsync(
    napi_env env, const std::string& url, bool is_base64,
    base::MoveOnlyClosure<void, ImageResponse&> callback,
    LynxImageEffectProcessor params) {
  base::NapiHandleScope scope(env);
  auto context = new CallbackContext;
  context->env = env;
  context->url = url;
  context->callback = std::move(callback);
  context->is_base64 = is_base64;
  context->params = std::move(params);
  napi_value work_name;
  napi_create_string_utf8(env, "LynxImageHelper::DecodeImageAsync",
                          NAPI_AUTO_LENGTH, &work_name);
  napi_create_async_work(
      env, nullptr, work_name,
      [](napi_env env, void* data) {
        CallbackContext* context = reinterpret_cast<CallbackContext*>(data);
        context->response = LynxImageHelper::DecodeImageSync(
            context->url, context->is_base64, context->params);
      },
      [](napi_env env, napi_status status, void* data) {
        CallbackContext* context = reinterpret_cast<CallbackContext*>(data);
        context->callback(context->response);
        napi_delete_async_work(env, context->work);
        delete context;
      },
      reinterpret_cast<void*>(context), &context->work);
  napi_queue_async_work(env, context->work);
}

LynxImageHelper::ImageResponse LynxImageHelper::DecodeImageSync(
    const std::string& url, bool is_base64,
    const LynxImageEffectProcessor& params) {
  OH_ImageSourceNative* image_source_native = nullptr;
  Image_ErrorCode code = IMAGE_SUCCESS;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, IMAGE_HELPER_DECODE_IMAGE_SYNC, "url", url,
              "is_base64", is_base64);
  if (is_base64) {
    uint8_t* data = reinterpret_cast<uint8_t*>(const_cast<char*>(url.data()));
    code = OH_ImageSourceNative_CreateFromData(data, url.size(),
                                               &image_source_native);
  } else if (base::BeginsWith(url, image::kResourceRawFile)) {
    // for rawfile
    code = IMAGE_BAD_PARAMETER;
    RawFile* raw_file = OH_ResourceManager_OpenRawFile(
        lynx::harmony::LynxResourceLoaderHarmony::resource_manager,
        url.substr(std::strlen(image::kResourceRawFile)).c_str());
    if (raw_file) {
      RawFileDescriptor descriptor;
      if (OH_ResourceManager_GetRawFileDescriptor(raw_file, descriptor)) {
        code = OH_ImageSourceNative_CreateFromRawFile(&descriptor,
                                                      &image_source_native);
        OH_ResourceManager_ReleaseRawFileDescriptor(descriptor);
      }
      OH_ResourceManager_CloseRawFile(raw_file);
    }
  } else if (base::BeginsWith(url, image::kResourceBaseMedia)) {
    // for base media
    std::string file_name = url.substr(std::strlen(image::kResourceBaseMedia));
    size_t file_format = file_name.find_last_of('.');
    if (file_format != std::string::npos) {
      file_name = file_name.substr(0, file_format);
    }
    LOGD("LynxImage decode image file name:" << file_name);

    uint64_t resource_len = 0;
    uint8_t* resource_value = nullptr;
    ResourceManager_ErrorCode error_code = OH_ResourceManager_GetMediaByName(
        lynx::harmony::LynxResourceLoaderHarmony::resource_manager,
        file_name.c_str(), &resource_value, &resource_len);
    if (error_code != ResourceManager_ErrorCode::SUCCESS) {
      LOGE("LynxImage resource manager get media failed:" << error_code);
      ImageResponse response = {.err_code = IMAGE_BAD_SOURCE};
      return response;
    }
    if (resource_value != nullptr) {
      code = OH_ImageSourceNative_CreateFromData(resource_value, resource_len,
                                                 &image_source_native);
      free(resource_value);
    } else {
      LOGE("LynxImage Resource Manager retrieves a null media resource.");
      code = IMAGE_ALLOC_FAILED;
    }
  } else {
    code = OH_ImageSourceNative_CreateFromUri(const_cast<char*>(url.data()),
                                              url.size(), &image_source_native);
  }

  ImageResponse response;
  if (code != IMAGE_SUCCESS) {
    response.err_code = code;
    return response;
  }
  DecodeImageFromImageSource(image_source_native, response, params);
  OH_ImageSourceNative_Release(image_source_native);
  return response;
}

void LynxImageHelper::DecodeImageFromImageSource(
    OH_ImageSourceNative* image_source, ImageResponse& response,
    const LynxImageEffectProcessor& params) {
  OH_DecodingOptions* options;
  OH_DecodingOptions_Create(&options);
  OH_DecodingOptions_SetPixelFormat(options, PIXEL_FORMAT_RGBA_8888);
  OH_PixelmapNative* pixel_map;
  uint32_t frameCount = 0;
  std::unique_ptr<OH_PixelmapNative*[]> pixelmap_list{};

  auto code = OH_ImageSourceNative_GetFrameCount(image_source, &frameCount);
  if (code != IMAGE_SUCCESS) {
    response.err_code = code;
    OH_DecodingOptions_Release(options);
    return;
  }
  if (frameCount > 1) {
    pixelmap_list = std::make_unique<OH_PixelmapNative*[]>(frameCount);
    code = OH_ImageSourceNative_CreatePixelmapList(
        image_source, options, pixelmap_list.get(), frameCount);
  } else {
    OH_DecodingOptions_SetIndex(options, 0);
    code =
        OH_ImageSourceNative_CreatePixelmap(image_source, options, &pixel_map);
  }
  OH_DecodingOptions_Release(options);
  if (code != IMAGE_SUCCESS) {
    response.err_code = code;
    return;
  }
  response.frame_count = frameCount;
  if (params.GetEffectType() != LynxImageEffectProcessor::ImageEffect::kNone &&
      frameCount == 1) {
    OH_PixelmapNative* new_pixel_map = params.Process(pixel_map);
    if (new_pixel_map) {
      OH_PixelmapNative_Release(pixel_map);
      pixel_map = new_pixel_map;
    }
  }
  std::vector<std::unique_ptr<LynxPixelMap>> data;
  if (frameCount > 1) {
    int32_t* delayTimeList = new int32_t[frameCount];
    OH_ImageSourceNative_GetDelayTimeList(image_source, delayTimeList,
                                          frameCount);
    for (uint32_t i = 0; i < frameCount; i++) {
      data.emplace_back(std::make_unique<LynxPixelMap>(pixelmap_list.get()[i],
                                                       delayTimeList[i]));
    }
    delete[] delayTimeList;
  } else {
    data.emplace_back(std::make_unique<LynxPixelMap>(pixel_map, 0));
  }
  auto ptr = std::make_unique<LynxBaseImage>(std::move(data), frameCount);
  response.data = std::move(ptr);
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
