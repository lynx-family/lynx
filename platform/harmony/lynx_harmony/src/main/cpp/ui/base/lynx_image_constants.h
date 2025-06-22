// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_CONSTANTS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_CONSTANTS_H_

#include <cstdint>

namespace lynx {
namespace tasm {
namespace harmony {
namespace image {

// image mode
inline constexpr const char* const kModeAspectFit = "aspectFit";
inline constexpr const char* const kModeAspectFill = "aspectFill";
inline constexpr const char* const kModeScaleToFill = "scaleToFill";

// image path protocol
inline constexpr const char* const kBase64Scheme = "data:image";
inline constexpr const char* const kLocalScheme = "file://";

// event name
inline constexpr const char* const kLoadEventName = "load";
inline constexpr const char* const kLoadEventImageWidth = "width";
inline constexpr const char* const kLoadEventImageHeight = "height";
inline constexpr const char* const kErrorEventName = "error";
inline constexpr const char* const kErrorEventCode = "error_code";
inline constexpr const char* const kErrorEventMsg = "err_msg";
inline constexpr const char* const kStartPlayEventName = "startplay";
inline constexpr const char* const kCurrentLoopEventName =
    "currentloopcomplete";
inline constexpr const char* const kFinalLoopEventName = "finalloopcomplete";

// error message
inline constexpr const char* const kPathErrorMsg =
    "The image could not be obtained because the image path is invalid.";
inline constexpr const char* const kFormatErrorMsg =
    "The image format is not supported.";
inline constexpr int32_t kPathErrorCode = 401;

// dirty flag
inline constexpr uint64_t kFlagSrcChanged = 1;
inline constexpr uint64_t kFlagPlaceholderChanged = 1 << 1;
inline constexpr uint64_t kFlagCapInsetsChanged = 1 << 2;
inline constexpr uint64_t kFlagImageRenderingChanged = 1 << 3;
inline constexpr uint64_t kFlagTintColorChanged = 1 << 4;
inline constexpr uint64_t kFlagDropShadowChanged = 1 << 5;
inline constexpr uint64_t kFlagPaddingChanged = 1 << 6;
inline constexpr uint64_t kFlagFrameSizeChanged = 1 << 7;
inline constexpr uint64_t kFlagModeChanged = 1 << 8;

// event flag
inline constexpr uint64_t kFlagImageLoadEvent = 1;
inline constexpr uint64_t kFlagImageErrorEvent = 1 << 1;
inline constexpr uint64_t kFlagImageStartPlayEvent = 1 << 2;
inline constexpr uint64_t kFlagImageCurrentLoopEvent = 1 << 3;
inline constexpr uint64_t kFlagImageFinalLoopEvent = 1 << 4;
}  // namespace image
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_CONSTANTS_H_
