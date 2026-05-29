// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/emoji_resource_manager.h"

#include <multimedia/image_framework/image/image_source_native.h>
#include <native_drawing/drawing_pixel_map.h>
#include <resourcemanager/ohresmgr.h>

#include <cstdlib>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/resource/lynx_resource_loader_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

static constexpr const char* kGetEmojiResourcesMethod = "getEmojiResources";
// Emoji names that should be decoded after the resource list is loaded.
// Keep this list small because each entry may trigger an async image decode.
static constexpr std::string_view kCommonEmojiNames[] = {
    "\xE7\xAC\x91\xE8\x84\xB8",  // smile face
    "\xE6\xB5\x81\xE6\xB3\xAA",  // crying
    "\xE5\xBE\xAE\xE7\xAC\x91",  // smile
    "\xE6\x8D\x82\xE8\x84\xB8",  // facepalm
    "\xE5\xAE\xB3\xE7\xBE\x9E",  // shy
};

bool StartsWith(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() &&
         value.compare(0, prefix.size(), prefix) == 0;
}

bool IsSupportedImageUri(std::string_view image_uri) {
  return StartsWith(image_uri, image::kLocalScheme) ||
         StartsWith(image_uri, image::kResourceScheme);
}

}  // namespace

EmojiImage::~EmojiImage() {
  if (draw_pixel_map) {
    OH_Drawing_PixelMapDissolve(draw_pixel_map);
    draw_pixel_map = nullptr;
  }
  if (pixel_map) {
    OH_PixelmapNative_Release(pixel_map);
    pixel_map = nullptr;
  }
}

OH_Drawing_PixelMap* EmojiImage::DrawPixelMap() const {
  if (base_image && base_image->FirstFrame()) {
    return base_image->FirstFrame()->DrawBitmap();
  }
  return draw_pixel_map;
}

uint32_t EmojiImage::Width() const {
  return base_image ? base_image->Width() : width;
}

uint32_t EmojiImage::Height() const {
  return base_image ? base_image->Height() : height;
}

EmojiResourceManager& EmojiResourceManager::GetInstance() {
  static EmojiResourceManager instance;
  return instance;
}

void EmojiResourceManager::SetEmojiResourceFetcher(napi_env env,
                                                   napi_value fetcher) {
  if (!env || !fetcher) {
    return;
  }

  napi_valuetype value_type = napi_undefined;
  if (napi_typeof(env, fetcher, &value_type) != napi_ok ||
      value_type != napi_object) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  if (fetcher_ref_) {
    return;
  }
  fetcher_env_ = env;
  if (napi_create_reference(env, fetcher, 1, &fetcher_ref_) != napi_ok ||
      !fetcher_ref_) {
    fetcher_env_ = nullptr;
    fetcher_ref_ = nullptr;
    return;
  }
}

bool EmojiResourceManager::EnsureEmojiResourcesLoaded() {
  if (!LynxEnv::GetInstance().EnableHarmonyTextCustomEmoji()) {
    return false;
  }

  napi_env fetcher_env = nullptr;
  napi_ref fetcher_ref = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!image_cache_.empty()) {
      return true;
    }
    if (!fetcher_env_ || !fetcher_ref_) {
      return false;
    }
    fetcher_env = fetcher_env_;
    fetcher_ref = fetcher_ref_;
  }

  bool loaded = false;
  std::unordered_map<std::string, std::unique_ptr<EmojiImage>> images;
  base::closure fetch_resources = [this, fetcher_env, fetcher_ref, &loaded,
                                   &images] {
    napi_value fetcher = nullptr;
    if (napi_get_reference_value(fetcher_env, fetcher_ref, &fetcher) !=
            napi_ok ||
        !fetcher) {
      return;
    }
    loaded = FetchEmojiResourcesFromFetcher(fetcher_env, fetcher, images);
  };

  auto ui_runner = base::UIThread::GetRunner();
  if (ui_runner) {
    ui_runner->PostSyncTask(std::move(fetch_resources));
  } else {
    fetch_resources();
  }
  if (!loaded) {
    return false;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    image_cache_ = std::move(images);
  }

  DecodeCommonEmojiImages();
  return true;
}

void EmojiResourceManager::DecodeCommonEmojiImages() {
  for (auto name : kCommonEmojiNames) {
    GetEmojiImage(name);
  }
}

EmojiImage* EmojiResourceManager::GetEmojiImage(std::string_view name) {
  if (name.empty()) {
    return nullptr;
  }
  std::string name_key(name);

  EmojiImage* cached_image = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto cache_it = image_cache_.find(name_key);
    if (cache_it != image_cache_.end()) {
      cached_image = cache_it->second.get();
    }
  }
  if (cached_image) {
    DecodeEmojiImageIfNeeded(name_key, cached_image);
    return cached_image;
  }

  if (!EnsureEmojiResourcesLoaded()) {
    return nullptr;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto cache_it = image_cache_.find(name_key);
    if (cache_it != image_cache_.end()) {
      cached_image = cache_it->second.get();
    }
  }
  if (!cached_image) {
    return nullptr;
  }
  DecodeEmojiImageIfNeeded(name_key, cached_image);
  return cached_image;
}

bool EmojiResourceManager::DecodeEmojiImageIfNeeded(
    std::string_view name, EmojiImage* image, base::closure ready_callback) {
  if (image == nullptr) {
    return false;
  }

  EmojiResourceInfo info;
  std::string name_key(name);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (image->DrawPixelMap()) {
      return true;
    }
    if (image->decode_failed) {
      return false;
    }
    if (ready_callback) {
      image->ready_callbacks.emplace_back(std::move(ready_callback));
    }
    if (image->decode_requested) {
      return false;
    }
    image->decode_requested = true;
    info = image->resource_info;
  }

  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [this, name_key = std::move(name_key), info] {
        std::unique_ptr<EmojiImage> decoded_image;
        if (info.resource_id != 0) {
          decoded_image = DecodeImageByResourceId(info.resource_id);
        } else {
          decoded_image = DecodeImageByPath(info.image_uri);
        }

        auto complete_decode = [this, name_key,
                                decoded_image =
                                    std::move(decoded_image)]() mutable {
          CompleteDecodeEmojiImage(name_key, std::move(decoded_image));
        };
        auto ui_runner = base::UIThread::GetRunner();
        if (ui_runner) {
          ui_runner->PostTask(std::move(complete_decode));
        } else {
          complete_decode();
        }
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
  return false;
}

void EmojiResourceManager::CompleteDecodeEmojiImage(
    std::string name, std::unique_ptr<EmojiImage> decoded_image) {
  std::vector<base::closure> ready_callbacks;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto cache_it = image_cache_.find(name);
    if (cache_it == image_cache_.end()) {
      return;
    }
    EmojiImage* image = cache_it->second.get();
    if (!decoded_image || !decoded_image->DrawPixelMap()) {
      image->decode_failed = true;
    } else {
      image->base_image = std::move(decoded_image->base_image);
      image->pixel_map = decoded_image->pixel_map;
      decoded_image->pixel_map = nullptr;
      image->draw_pixel_map = decoded_image->draw_pixel_map;
      decoded_image->draw_pixel_map = nullptr;
      image->width = decoded_image->width;
      image->height = decoded_image->height;
      image->decode_failed = false;
    }
    ready_callbacks = std::move(image->ready_callbacks);
  }
  for (auto& callback : ready_callbacks) {
    if (callback) {
      callback();
    }
  }
}

bool EmojiResourceManager::FetchEmojiResourcesFromFetcher(
    napi_env env, napi_value fetcher,
    std::unordered_map<std::string, std::unique_ptr<EmojiImage>>& images)
    const {
  if (!env || !fetcher) {
    return false;
  }

  base::NapiHandleScope scope(env);

  napi_value method = nullptr;
  if (napi_get_named_property(env, fetcher, kGetEmojiResourcesMethod,
                              &method) != napi_ok) {
    return false;
  }

  napi_value resources_value = nullptr;
  napi_status status =
      napi_call_function(env, fetcher, method, 0, nullptr, &resources_value);
  if (status != napi_ok) {
    LOGE("EmojiResourceManager: failed to get emoji resources, status: "
         << base::NapiUtil::StatusToString(status));
    return false;
  }

  return ReadEmojiResources(env, resources_value, images);
}

bool EmojiResourceManager::ReadEmojiResources(
    napi_env env, napi_value resources,
    std::unordered_map<std::string, std::unique_ptr<EmojiImage>>& parsed_images)
    const {
  bool is_array = false;
  if (napi_is_array(env, resources, &is_array) != napi_ok || !is_array) {
    return false;
  }

  uint32_t length = 0;
  if (napi_get_array_length(env, resources, &length) != napi_ok) {
    return false;
  }

  for (uint32_t index = 0; index < length; ++index) {
    napi_value resource = nullptr;
    if (napi_get_element(env, resources, index, &resource) != napi_ok ||
        !resource) {
      continue;
    }
    napi_valuetype resource_type = napi_undefined;
    if (napi_typeof(env, resource, &resource_type) != napi_ok ||
        resource_type != napi_object) {
      continue;
    }

    napi_value name_value = nullptr;
    if (napi_get_named_property(env, resource, "name", &name_value) !=
            napi_ok ||
        !base::NapiUtil::NapiIsType(env, name_value, napi_string)) {
      continue;
    }

    napi_value uri_value = nullptr;
    if (napi_get_named_property(env, resource, "uri", &uri_value) != napi_ok) {
      continue;
    }
    napi_valuetype uri_type = napi_undefined;
    if (napi_typeof(env, uri_value, &uri_type) != napi_ok) {
      continue;
    }

    EmojiResourceInfo resource_info;
    if (uri_type == napi_string) {
      resource_info.image_uri = base::NapiUtil::ConvertToString(env, uri_value);
    } else if (uri_type == napi_number) {
      resource_info.resource_id =
          base::NapiUtil::ConvertToUInt32(env, uri_value);
    } else {
      continue;
    }

    auto image = std::make_unique<EmojiImage>();
    image->resource_info = std::move(resource_info);
    parsed_images.emplace(base::NapiUtil::ConvertToString(env, name_value),
                          std::move(image));
  }
  return !parsed_images.empty();
}

std::unique_ptr<EmojiImage> EmojiResourceManager::DecodeImageByPath(
    const std::string& image_uri) {
  if (!IsSupportedImageUri(image_uri)) {
    return nullptr;
  }
  auto response = LynxImageHelper::DecodeImageSync(image_uri, false, {});
  if (!response.Success() || !response.data) {
    return nullptr;
  }

  auto image = std::make_unique<EmojiImage>();
  image->base_image = std::move(response.data);
  return image;
}

std::unique_ptr<EmojiImage> EmojiResourceManager::DecodeImageByResourceId(
    uint32_t resource_id) {
  NativeResourceManager* res_mgr =
      lynx::harmony::LynxResourceLoaderHarmony::resource_manager;
  if (!res_mgr) {
    LOGE("EmojiResourceManager: NativeResourceManager is null");
    return nullptr;
  }

  uint64_t resource_len = 0;
  uint8_t* resource_value = nullptr;
  ResourceManager_ErrorCode error_code = OH_ResourceManager_GetMedia(
      res_mgr, resource_id, &resource_value, &resource_len, 0);
  if (error_code != SUCCESS || !resource_value || resource_len == 0) {
    if (resource_value) {
      free(resource_value);
    }
    LOGE("EmojiResourceManager: failed to get media resource id: "
         << resource_id << ", error: " << error_code);
    return nullptr;
  }

  OH_ImageSourceNative* image_source = nullptr;
  Image_ErrorCode image_code = OH_ImageSourceNative_CreateFromData(
      resource_value, resource_len, &image_source);
  free(resource_value);
  if (image_code != IMAGE_SUCCESS || !image_source) {
    LOGE("EmojiResourceManager: failed to create image source, error: "
         << image_code);
    return nullptr;
  }

  OH_DecodingOptions* options = nullptr;
  if (OH_DecodingOptions_Create(&options) != IMAGE_SUCCESS || !options) {
    LOGE("EmojiResourceManager: failed to create decoding options");
    OH_ImageSourceNative_Release(image_source);
    return nullptr;
  }
  OH_DecodingOptions_SetPixelFormat(options, PIXEL_FORMAT_RGBA_8888);

  OH_PixelmapNative* pixel_map = nullptr;
  image_code =
      OH_ImageSourceNative_CreatePixelmap(image_source, options, &pixel_map);
  OH_DecodingOptions_Release(options);
  OH_ImageSourceNative_Release(image_source);

  if (image_code != IMAGE_SUCCESS || !pixel_map) {
    LOGE("EmojiResourceManager: failed to decode image, error: " << image_code);
    return nullptr;
  }

  OH_Pixelmap_ImageInfo* pixel_map_info = nullptr;
  if (OH_PixelmapImageInfo_Create(&pixel_map_info) != IMAGE_SUCCESS ||
      !pixel_map_info) {
    LOGE("EmojiResourceManager: failed to create pixelmap image info");
    OH_PixelmapNative_Release(pixel_map);
    return nullptr;
  }
  if (OH_PixelmapNative_GetImageInfo(pixel_map, pixel_map_info) !=
      IMAGE_SUCCESS) {
    LOGE("EmojiResourceManager: failed to get pixelmap image info");
    OH_PixelmapImageInfo_Release(pixel_map_info);
    OH_PixelmapNative_Release(pixel_map);
    return nullptr;
  }
  uint32_t image_width = 0;
  uint32_t image_height = 0;
  OH_PixelmapImageInfo_GetWidth(pixel_map_info, &image_width);
  OH_PixelmapImageInfo_GetHeight(pixel_map_info, &image_height);
  OH_PixelmapImageInfo_Release(pixel_map_info);

  OH_Drawing_PixelMap* draw_pixel_map =
      OH_Drawing_PixelMapGetFromOhPixelMapNative(pixel_map);
  if (!draw_pixel_map) {
    LOGE("EmojiResourceManager: failed to create drawing pixel map");
    OH_PixelmapNative_Release(pixel_map);
    return nullptr;
  }

  auto image = std::make_unique<EmojiImage>();
  image->pixel_map = pixel_map;
  image->draw_pixel_map = draw_pixel_map;
  image->width = image_width;
  image->height = image_height;
  return image;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
