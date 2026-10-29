// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UI_SCREENSHOT_HELPER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UI_SCREENSHOT_HELPER_H_

#include <arkui/native_type.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <node_api.h>

#include <cstdint>
#include <string>

#include "base/include/closure.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxUIScreenshotHelper {
 public:
  static void TakeScreenshotForNode(
      napi_env env, ArkUI_NodeHandle node, const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

  static void TakeContentScreenshotForNode(
      napi_env env, ArkUI_NodeHandle node, const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

  struct SnapshotResult {
    bool success{false};
    uint32_t width{0};
    uint32_t height{0};
    // Raw Base64 on success, or an error message on failure.
    std::string data;
  };

  // Must be called on the UI thread. The caller owns the returned pixel map.
  static OH_PixelmapNative* TakeSnapshotForNode(ArkUI_NodeHandle node,
                                                float scale,
                                                std::string& error_message);

  // Takes ownership of pixel_map and returns raw Base64 data on success.
  static SnapshotResult EncodeSnapshot(OH_PixelmapNative* pixel_map,
                                       const std::string& format,
                                       int quality = 100);

 private:
  static OH_PixelmapNative* TakeSnapshotForNode(ArkUI_NodeHandle node,
                                                const lepus::Value& args,
                                                std::string& format,
                                                std::string& error_message);

  static void Base64EncodeTask(
      napi_env env, OH_PixelmapNative* pixel_map, const std::string& format,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UI_SCREENSHOT_HELPER_H_
