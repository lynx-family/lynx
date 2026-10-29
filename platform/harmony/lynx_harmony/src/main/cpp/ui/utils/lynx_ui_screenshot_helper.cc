// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_screenshot_helper.h"

#include <multimedia/image_framework/image/image_packer_native.h>

#include <algorithm>
#include <cstdint>
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
static constexpr size_t kMaxImageBufferSize = 256 * 1024 * 1024;

bool CalculateImageBufferSize(uint32_t width, uint32_t height,
                              size_t bytes_per_pixel, size_t& buffer_size) {
  uint64_t pixel_count = static_cast<uint64_t>(width) * height;
  if (pixel_count >
      (kMaxImageBufferSize - kImageBufferMargin) / bytes_per_pixel) {
    return false;
  }
  buffer_size =
      static_cast<size_t>(pixel_count) * bytes_per_pixel + kImageBufferMargin;
  return true;
}

struct EncodeAsyncContexts {
  napi_async_work async_work = nullptr;
  OH_PixelmapNative* pixel_map = nullptr;
  std::string format;
  LynxUIScreenshotHelper::SnapshotResult result;
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
  format = "jpeg";
  float scale = 1.0f;
  if (args.IsTable() || args.IsJSTable()) {
    tasm::ForEachLepusValue(
        args, [&format, &scale](const auto& key, const auto& val) {
          if (key.StdString() == "format" && val.StdString() == "png") {
            format = "png";
          }
          if (key.StdString() == "scale") {
            scale = val.Number();
          }
        });
  }
  return TakeSnapshotForNode(node, scale, error_message);
}

OH_PixelmapNative* LynxUIScreenshotHelper::TakeSnapshotForNode(
    ArkUI_NodeHandle node, float scale, std::string& error_message) {
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

LynxUIScreenshotHelper::SnapshotResult LynxUIScreenshotHelper::EncodeSnapshot(
    OH_PixelmapNative* pixel_map, const std::string& format, int quality) {
  SnapshotResult result;
  OH_Pixelmap_ImageInfo* image_info = nullptr;
  OH_PixelmapImageInfo_Create(&image_info);
  OH_PixelmapNative_GetImageInfo(pixel_map, image_info);
  OH_PixelmapImageInfo_GetWidth(image_info, &result.width);
  OH_PixelmapImageInfo_GetHeight(image_info, &result.height);
  OH_PixelmapImageInfo_Release(image_info);

  // To optimize memory usage, we start with 0.5 * raw size (2 bytes per
  // pixel). This should be enough for most cases.
  // We also add a constant margin (2KB) to cover image headers/metadata,
  // which is significant for very small images.
  size_t buffer_size = 0;
  if (!CalculateImageBufferSize(result.width, result.height, 2, buffer_size)) {
    OH_PixelmapNative_Release(pixel_map);
    result.data = "image encoding buffer exceeds 256 MiB";
    return result;
  }

  OH_ImagePackerNative* image_packer = nullptr;
  OH_PackingOptions* pack_option = nullptr;
  OH_ImagePackerNative_Create(&image_packer);
  OH_PackingOptions_Create(&pack_option);
  std::string image_format = "image/" + format;
  Image_MimeType mime_type = {image_format.data(), image_format.length()};
  OH_PackingOptions_SetMimeType(pack_option, &mime_type);
  OH_PackingOptions_SetQuality(pack_option, std::clamp(quality, 0, 100));

  std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(buffer_size);
  int32_t res = OH_ImagePackerNative_PackToDataFromPixelmap(
      image_packer, pack_option, pixel_map, buffer.get(), &buffer_size);

  if (res == IMAGE_ENCODE_FAILED) {
    // Retry with max safe size: raw RGBA size + margin
    if (!CalculateImageBufferSize(result.width, result.height, 4,
                                  buffer_size)) {
      OH_PixelmapNative_Release(pixel_map);
      OH_PackingOptions_Release(pack_option);
      OH_ImagePackerNative_Release(image_packer);
      result.data = "image encoding buffer exceeds 256 MiB";
      return result;
    }
    buffer = std::make_unique<uint8_t[]>(buffer_size);
    res = OH_ImagePackerNative_PackToDataFromPixelmap(
        image_packer, pack_option, pixel_map, buffer.get(), &buffer_size);
  }
  OH_PixelmapNative_Release(pixel_map);
  OH_PackingOptions_Release(pack_option);
  OH_ImagePackerNative_Release(image_packer);
  if (res != IMAGE_SUCCESS) {
    result.data = "pack image failed: " + std::to_string(res);
    result.success = false;
    return result;
  }
  std::unique_ptr<char[]> base64_buf =
      std::make_unique<char[]>(lynx_modp_b64_encode_len(buffer_size));
  size_t base64_len = lynx_modp_b64_encode(
      base64_buf.get(), reinterpret_cast<const char*>(buffer.get()),
      buffer_size);
  if (base64_len == static_cast<size_t>(-1)) {
    result.data = "image base64 encode failed";
    result.success = false;
    return result;
  }
  result.data.assign(base64_buf.get(), base64_len);
  result.success = true;
  return result;
}

void LynxUIScreenshotHelper::Base64EncodeTask(
    napi_env env, OH_PixelmapNative* pixel_map, const std::string& format,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  auto async_context =
      new EncodeAsyncContexts(pixel_map, format, std::move(callback));
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
        encode_context->result =
            EncodeSnapshot(encode_context->pixel_map, encode_context->format);
      },
      [](napi_env env, napi_status status, void* data) {
        EncodeAsyncContexts* encode_context =
            reinterpret_cast<EncodeAsyncContexts*>(data);
        if (encode_context == nullptr) {
          LOGE("Failed to get encode task context");
          return;
        }
        if (encode_context->result.success) {
          auto ret = lepus::Dictionary::Create();
          ret->SetValue("width", encode_context->result.width);
          ret->SetValue("height", encode_context->result.height);
          std::string header = "data:image/jpeg;base64,";
          if (encode_context->format == "png") {
            header = "data:image/png;base64,";
          }
          ret->SetValue("data", header + encode_context->result.data);
          encode_context->callback(LynxGetUIResult::SUCCESS, lepus::Value(ret));
        } else {
          encode_context->callback(LynxGetUIResult::OPERATION_ERROR,
                                   lepus::Value(encode_context->result.data));
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
