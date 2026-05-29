// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_EMOJI_RESOURCE_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_EMOJI_RESOURCE_MANAGER_H_

#include <multimedia/image_framework/image/pixelmap_native.h>
#include <native_drawing/drawing_types.h>
#include <node_api.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "base/include/closure.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_base_image.h"

namespace lynx {
namespace tasm {
namespace harmony {

struct EmojiResourceInfo {
  std::string image_uri;
  uint32_t resource_id{0};
};

struct EmojiImage {
  EmojiResourceInfo resource_info;
  std::unique_ptr<LynxBaseImage> base_image;
  OH_PixelmapNative* pixel_map{nullptr};
  OH_Drawing_PixelMap* draw_pixel_map{nullptr};
  uint32_t width{0};
  uint32_t height{0};
  bool decode_requested{false};
  bool decode_failed{false};
  std::vector<base::closure> ready_callbacks;

  ~EmojiImage();
  OH_Drawing_PixelMap* DrawPixelMap() const;
  uint32_t Width() const;
  uint32_t Height() const;
};

class EmojiResourceManager {
 public:
  static EmojiResourceManager& GetInstance();
  ~EmojiResourceManager() = default;

  void SetEmojiResourceFetcher(napi_env env, napi_value fetcher);
  bool EnsureEmojiResourcesLoaded();
  EmojiImage* GetEmojiImage(std::string_view name);
  bool DecodeEmojiImageIfNeeded(std::string_view name, EmojiImage* image,
                                base::closure ready_callback = nullptr);

 private:
  EmojiResourceManager() = default;
  EmojiResourceManager(const EmojiResourceManager&) = delete;
  EmojiResourceManager& operator=(const EmojiResourceManager&) = delete;

  void DecodeCommonEmojiImages();
  bool FetchEmojiResourcesFromFetcher(
      napi_env env, napi_value fetcher,
      std::unordered_map<std::string, std::unique_ptr<EmojiImage>>& images)
      const;
  bool ReadEmojiResources(
      napi_env env, napi_value resources,
      std::unordered_map<std::string, std::unique_ptr<EmojiImage>>&
          parsed_images) const;
  std::unique_ptr<EmojiImage> DecodeImageByPath(const std::string& image_uri);
  std::unique_ptr<EmojiImage> DecodeImageByResourceId(uint32_t resource_id);
  void CompleteDecodeEmojiImage(std::string name,
                                std::unique_ptr<EmojiImage> decoded_image);

  mutable std::mutex mutex_;
  napi_env fetcher_env_{nullptr};
  napi_ref fetcher_ref_{nullptr};
  std::unordered_map<std::string, std::unique_ptr<EmojiImage>> image_cache_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_EMOJI_RESOURCE_MANAGER_H_
