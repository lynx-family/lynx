// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_IMAGE_SERVICE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_IMAGE_SERVICE_H_

#include <arkui/native_node.h>
#include <multimedia/image_framework/image/pixelmap_native.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lynx {
namespace tasm {
namespace harmony {

class ImageData {
 public:
  virtual ~ImageData() = default;
  virtual OH_PixelmapNative* Pixelmap() const = 0;
  virtual uint32_t FrameCount() const = 0;
  virtual OH_PixelmapNative** PixelmapList() const = 0;
  virtual int* DelayTimeList() const = 0;
};

class ImageProcessor {
 public:
  virtual ~ImageProcessor() = default;
  virtual OH_PixelmapNative* Process(OH_PixelmapNative* pixel_map) const = 0;
  virtual std::string Info() const = 0;
};

struct ImageRequestInfo {
  std::string url;
  std::string placeholder;
  ArkUI_ObjectFit mode;
  bool downsampling = true;
  std::vector<std::unique_ptr<ImageProcessor>> processors;
  std::unordered_map<std::string, std::string> custom_param;
  std::vector<std::string> fallback_urls;
};

enum class LynxImageOrigin : int32_t {
  kUnknown = 1,
  kNetwork = 2,
  kDisk = 3,
  kMemoryEncoded = 4,
  kMemoryDecoded = 5,
  kLocal = 6,  // local file or base64
  kMax = 7,    // reserved field
};

struct ImageMonitorInfo {
  uint64_t load_start = 0;
  uint64_t load_finish = 0;
  LynxImageOrigin origin = LynxImageOrigin::kUnknown;
};

class ImageLoadListener {
 public:
  virtual ~ImageLoadListener() = default;
  virtual void OnImageLoadSuccess(float image_width, float image_height) = 0;
  virtual void OnImageMonitorInfo(const ImageMonitorInfo& info) = 0;
  virtual bool NeedMonitorInfo() = 0;
  virtual void OnImageLoadFailure(int error_code,
                                  const std::string& error_msg) = 0;
};

class AnimationListener {
 public:
  virtual ~AnimationListener() = default;
  virtual void OnAnimationStart() = 0;
  virtual void OnAnimationRepeat() = 0;
  virtual void OnAnimationFinish() = 0;
};

struct ImageNode {
  virtual ~ImageNode() = default;
  virtual ArkUI_NodeHandle GetNodeHandle() = 0;
  virtual void FetchImage(ImageRequestInfo info) = 0;
  virtual void InitImageLoadListener(
      const std::weak_ptr<ImageLoadListener>& listener) = 0;
  virtual void InitAnimationListener(
      const std::weak_ptr<AnimationListener>& listener) = 0;
  virtual void StartAnimation() = 0;
  virtual void StopAnimation() = 0;
  virtual void PauseAnimation() = 0;
  virtual void ResumeAnimation() = 0;
  virtual void UpdateAutoPlay(bool autoplay) = 0;
  virtual void UpdateLoopCount(int count) = 0;
  virtual void Clear() = 0;
};

class ImageService {
 public:
  virtual ~ImageService() = default;
  virtual std::unique_ptr<ImageNode> CreateImageNode() = 0;
  using ImageDataCallback =
      std::function<void(const std::shared_ptr<ImageData>&)>;
  using ImageSuccessCallback = std::function<void(float, float)>;
  using ImageFailedCallback = std::function<void(int, const std::string&)>;
  virtual void DecodeImage(const ImageRequestInfo& info,
                           ImageDataCallback callback,
                           ImageSuccessCallback on_load_success,
                           ImageFailedCallback on_load_failed) = 0;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_IMAGE_SERVICE_H_
