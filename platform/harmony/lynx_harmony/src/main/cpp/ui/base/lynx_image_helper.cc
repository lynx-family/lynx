// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"

#include <uv.h>

#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"

namespace lynx {
namespace tasm {
namespace harmony {
void LynxImageHelper::DecodeImageAsync(
    napi_env env, const std::string& url, bool is_base64,
    base::MoveOnlyClosure<void, ImageResponse&> callback,
    LynxImageEffectProcessor params) {
  auto context = new CallbackContext;
  context->env = env;
  context->url = url;
  context->callback = std::move(callback);
  context->is_base64 = is_base64;
  context->params = std::move(params);
  napi_value work_name;
  napi_async_work async_work;
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
      reinterpret_cast<void*>(context), &async_work);
  context->work = async_work;
  napi_queue_async_work(env, async_work);
}

LynxImageHelper::ImageResponse LynxImageHelper::DecodeImageSync(
    const std::string& url, bool is_base64,
    const LynxImageEffectProcessor& params) {
  OH_ImageSourceNative* image_source_native;
  Image_ErrorCode code = IMAGE_SUCCESS;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, IMAGE_HELPER_DECODE_IMAGE_SYNC, "url", url,
              "is_base64", is_base64);
  if (is_base64) {
    uint8_t* data = reinterpret_cast<uint8_t*>(const_cast<char*>(url.data()));
    code = OH_ImageSourceNative_CreateFromData(data, url.size(),
                                               &image_source_native);
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
  OH_DecodingOptions_SetIndex(options, 0);
  OH_PixelmapNative* pixel_map;
  auto code =
      OH_ImageSourceNative_CreatePixelmap(image_source, options, &pixel_map);

  OH_DecodingOptions_Release(options);
  if (code != IMAGE_SUCCESS) {
    response.err_code = code;
    return;
  }
  OH_Pixelmap_ImageInfo* pixel_map_info;
  OH_PixelmapImageInfo_Create(&pixel_map_info);
  OH_PixelmapNative_GetImageInfo(pixel_map, pixel_map_info);
  OH_PixelmapImageInfo_GetWidth(pixel_map_info, &response.image_width);
  OH_PixelmapImageInfo_GetHeight(pixel_map_info, &response.image_height);
  OH_PixelmapImageInfo_Release(pixel_map_info);

  if (params.GetEffectType() != LynxImageEffectProcessor::ImageEffect::kNone) {
    OH_PixelmapNative* new_pixel_map = params.Process(pixel_map);
    if (new_pixel_map) {
      OH_PixelmapNative_Release(pixel_map);
      pixel_map = new_pixel_map;
    }
  }
  std::unique_ptr<OH_PixelmapNative, PixelmapDeleter> ptr(pixel_map,
                                                          &ReleasePixelmap);
  response.data = std::move(ptr);
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
