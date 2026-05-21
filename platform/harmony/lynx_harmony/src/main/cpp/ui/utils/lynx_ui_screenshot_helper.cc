// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_screenshot_helper.h"

#include <multimedia/image_framework/image/image_packer_native.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "core/base/harmony/harmony_function_loader.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/utils/value_utils.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

static constexpr size_t kImageBufferMargin = 2048;
static constexpr uint64_t kMaxScreenshotMemoryBytes =
    256ULL * 1024ULL * 1024ULL;

bool CalculateImageBufferSize(uint32_t width, uint32_t height,
                              uint32_t bytes_per_pixel, size_t& buffer_size,
                              std::string& error_message) {
  if (bytes_per_pixel == 0) {
    error_message = "image buffer pixel size is invalid";
    return false;
  }
  const uint64_t pixels = static_cast<uint64_t>(width) * height;
  if (pixels > (std::numeric_limits<uint64_t>::max() - kImageBufferMargin) /
                   bytes_per_pixel) {
    error_message = "image buffer size overflow";
    return false;
  }
  const uint64_t required_bytes = pixels * bytes_per_pixel + kImageBufferMargin;
  if (required_bytes > kMaxScreenshotMemoryBytes) {
    error_message = "screenshot memory exceeds limit";
    return false;
  }
  if (required_bytes > std::numeric_limits<size_t>::max()) {
    error_message = "image buffer size overflow";
    return false;
  }
  buffer_size = static_cast<size_t>(required_bytes);
  return true;
}

struct EncodeAsyncContexts {
  napi_async_work async_work = nullptr;
  OH_PixelmapNative* pixel_map = nullptr;
  std::string format;
  uint32_t width;
  uint32_t height;
  bool res{false};
  std::string data;
  base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback;

  EncodeAsyncContexts(
      OH_PixelmapNative* pixel_map, const std::string& format,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback)
      : pixel_map(pixel_map), format(format), callback(std::move(callback)) {}
};

}  // namespace

OH_PixelmapNative* LynxUIScreenshotHelper::TakeSnapshotForNode(
    ArkUI_NodeHandle node, const lepus::Value& args, std::string& format,
    std::string& error_message) {
  if (node == nullptr) {
    error_message = "node is invalid";
    return nullptr;
  }
  base::harmony::HarmonyCompatFunctionsHandler* handle =
      base::harmony::GetHarmonyCompatFunctionsHandler();
  if (handle == nullptr) {
    error_message = "load symbol failed";
    return nullptr;
  }
  format = "jpeg";
  float scale = 1.0f;
  if (args.IsTable() || args.IsJSTable()) {
    tasm::ForEachLepusValue(
        args, [&format, &scale](const auto& key, const auto& val) {
          if (key.StdString() == "format") {
            format = val.StdString();
          }
          if (key.StdString() == "scale") {
            scale = val.Number();
          }
        });
  }
  scale = std::clamp(scale, 0.0f, 1.0f);
  ArkUI_SnapshotOptions* snapshot_option =
      handle->oh_create_snapshot_option_func();
  if (snapshot_option == nullptr) {
    error_message = "The current version does not support screenshot";
    return nullptr;
  }
  int32_t res =
      handle->oh_snapshot_option_set_scale_func(snapshot_option, scale);
  if (res != 0) {
    handle->oh_destroy_snapshot_option_func(snapshot_option);
    error_message = "screenshot args init failed";
    return nullptr;
  }
  OH_PixelmapNative* pixel_map = nullptr;
  handle->oh_get_node_snapshot_func(node, snapshot_option, &pixel_map);
  handle->oh_destroy_snapshot_option_func(snapshot_option);
  if (pixel_map == nullptr) {
    error_message = "task snapshot failed";
    return nullptr;
  }
  return pixel_map;
}

void LynxUIScreenshotHelper::Base64EncodeTask(
    napi_env env, OH_PixelmapNative* pixel_map, const std::string& format,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  auto async_context =
      new EncodeAsyncContexts(pixel_map, format, std::move(callback));
  if (async_context == nullptr) {
    callback(LynxGetUIResult::OPERATION_ERROR,
             lepus::Value("Create Base64 encode task failed"));
    return;
  }
  napi_value work_name;
  napi_create_string_utf8(env, "TakeScreenshotBase64EncodeTask",
                          NAPI_AUTO_LENGTH, &work_name);
  napi_create_async_work(
      env, nullptr, work_name,
      [](napi_env env, void* data) {
        EncodeAsyncContexts* encode_context =
            reinterpret_cast<EncodeAsyncContexts*>(data);
        if (encode_context == nullptr) {
          LOGE("Failed to get encode task context");
          return;
        }
        OH_Pixelmap_ImageInfo* image_info = nullptr;
        OH_PixelmapImageInfo_Create(&image_info);
        OH_PixelmapNative_GetImageInfo(encode_context->pixel_map, image_info);
        OH_PixelmapImageInfo_GetWidth(image_info, &encode_context->width);
        OH_PixelmapImageInfo_GetHeight(image_info, &encode_context->height);
        OH_PixelmapImageInfo_Release(image_info);

        size_t buffer_size = 0;
        if (!CalculateImageBufferSize(encode_context->width,
                                      encode_context->height, 2, buffer_size,
                                      encode_context->data)) {
          OH_PixelmapNative_Release(encode_context->pixel_map);
          encode_context->res = false;
          return;
        }

        OH_ImagePackerNative* image_packer = nullptr;
        OH_PackingOptions* pack_option = nullptr;
        OH_ImagePackerNative_Create(&image_packer);
        OH_PackingOptions_Create(&pack_option);
        std::string image_format = "image/" + encode_context->format;
        Image_MimeType mime_type = {image_format.data(), image_format.length()};
        OH_PackingOptions_SetMimeType(pack_option, &mime_type);
        OH_PackingOptions_SetQuality(pack_option, 100);

        std::unique_ptr<uint8_t[]> buffer =
            std::make_unique<uint8_t[]>(buffer_size);
        int32_t res = OH_ImagePackerNative_PackToDataFromPixelmap(
            image_packer, pack_option, encode_context->pixel_map, buffer.get(),
            &buffer_size);

        if (res == IMAGE_ENCODE_FAILED) {
          if (!CalculateImageBufferSize(encode_context->width,
                                        encode_context->height, 4, buffer_size,
                                        encode_context->data)) {
            OH_PixelmapNative_Release(encode_context->pixel_map);
            OH_PackingOptions_Release(pack_option);
            OH_ImagePackerNative_Release(image_packer);
            encode_context->res = false;
            return;
          }
          buffer = std::make_unique<uint8_t[]>(buffer_size);
          res = OH_ImagePackerNative_PackToDataFromPixelmap(
              image_packer, pack_option, encode_context->pixel_map,
              buffer.get(), &buffer_size);
        }
        OH_PixelmapNative_Release(encode_context->pixel_map);
        OH_PackingOptions_Release(pack_option);
        OH_ImagePackerNative_Release(image_packer);
        if (res != IMAGE_SUCCESS) {
          encode_context->data = "pack image failed: " + std::to_string(res);
          encode_context->res = false;
          return;
        }
        std::unique_ptr<char[]> base64_buf =
            std::make_unique<char[]>(lynx_modp_b64_encode_len(buffer_size));
        size_t base64_len = lynx_modp_b64_encode(
            base64_buf.get(), reinterpret_cast<const char*>(buffer.get()),
            buffer_size);
        if (base64_len == static_cast<size_t>(-1)) {
          encode_context->data = "image base64 encode failed";
          encode_context->res = false;
          return;
        }
        std::string base64_str(reinterpret_cast<const char*>(base64_buf.get()),
                               base64_len);
        encode_context->data = base64_str;
        encode_context->res = true;
      },
      [](napi_env env, napi_status status, void* data) {
        EncodeAsyncContexts* encode_context =
            reinterpret_cast<EncodeAsyncContexts*>(data);
        if (encode_context == nullptr) {
          LOGE("Failed to get encode task context");
          return;
        }
        if (encode_context->res) {
          auto ret = lepus::Dictionary::Create();
          ret->SetValue("width", encode_context->width);
          ret->SetValue("height", encode_context->height);
          std::string header = "data:image/jpeg;base64,";
          if (encode_context->format == "png") {
            header = "data:image/png;base64,";
          }
          ret->SetValue("data", header + encode_context->data);
          encode_context->callback(LynxGetUIResult::SUCCESS, lepus::Value(ret));
        } else {
          encode_context->callback(LynxGetUIResult::OPERATION_ERROR,
                                   lepus::Value(encode_context->data));
        }
        napi_delete_async_work(env, encode_context->async_work);
        delete encode_context;
      },
      reinterpret_cast<void*>(async_context), &async_context->async_work);
  napi_queue_async_work(env, async_context->async_work);
}

void LynxUIScreenshotHelper::TakeScreenshotForNode(
    napi_env env, ArkUI_NodeHandle node, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  std::string format = "jpeg";
  std::string error_message;
  OH_PixelmapNative* pixel_map =
      TakeSnapshotForNode(node, args, format, error_message);
  if (pixel_map == nullptr) {
    callback(LynxGetUIResult::OPERATION_ERROR, lepus::Value(error_message));
    return;
  }
  Base64EncodeTask(env, pixel_map, format, std::move(callback));
}

void LynxUIScreenshotHelper::TakeContentScreenshotForNode(
    napi_env env, ArkUI_NodeHandle node, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  TakeScreenshotForNode(env, node, args, std::move(callback));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
