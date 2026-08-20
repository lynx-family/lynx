// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_knife_option_compat.h"

#include <dlfcn.h>

#include "base/include/log/logging.h"

namespace lynx {
namespace service {
namespace {
constexpr char kImageKnifeSoName[] = "libimageknifepro.so";
constexpr char kSetUseHighestPrioritySymbol[] =
    "ImageKnifeOption_SetUseHighestPriority";
constexpr char kSetMappedMemoryCacheKeySymbol[] =
    "ImageKnifeOption_SetMappedMemoryCacheKey";
constexpr char kSetMappedFileCacheKeySymbol[] =
    "ImageKnifeOption_SetMappedFileCacheKey";

using SetUseHighestPriority = void (*)(ImageKnifePro::ImageKnifeOption*, bool);
using SetMappedCacheKey = void (*)(ImageKnifePro::ImageKnifeOption*,
                                   const char*);

template <typename Function>
Function Resolve(void* handle, const char* symbol) {
  void* address = dlsym(handle, symbol);
  if (address == nullptr) {
    LOGE("ImageKnife optional API is unavailable: " << symbol);
  }
  return reinterpret_cast<Function>(address);
}

struct ImageKnifeOptionApi {
  ImageKnifeOptionApi()
      : handle(dlopen(kImageKnifeSoName, RTLD_NOW | RTLD_NOLOAD)) {
    if (handle == nullptr) {
      LOGE("ImageKnife library is not loaded: " << kImageKnifeSoName);
      return;
    }
    set_use_highest_priority =
        Resolve<SetUseHighestPriority>(handle, kSetUseHighestPrioritySymbol);
    set_mapped_memory_cache_key =
        Resolve<SetMappedCacheKey>(handle, kSetMappedMemoryCacheKeySymbol);
    set_mapped_file_cache_key =
        Resolve<SetMappedCacheKey>(handle, kSetMappedFileCacheKeySymbol);
  }

  void* handle = nullptr;
  SetUseHighestPriority set_use_highest_priority = nullptr;
  SetMappedCacheKey set_mapped_memory_cache_key = nullptr;
  SetMappedCacheKey set_mapped_file_cache_key = nullptr;
};

const ImageKnifeOptionApi& GetImageKnifeOptionApi() {
  static const ImageKnifeOptionApi api;
  return api;
}

}  // namespace

void ImageKnifeOptionCompat::Apply(
    ImageKnifePro::ImageKnifeOption* option,
    const tasm::harmony::ImageRequestInfo& info) {
  const auto& mapped_memory_key = info.mapped_memory_cache_key;
  const auto& mapped_file_key = info.mapped_file_cache_key;
  if (!info.use_highest_priority && mapped_memory_key.empty() &&
      mapped_file_key.empty()) {
    return;
  }

  const auto& api = GetImageKnifeOptionApi();
  if (info.use_highest_priority) {
    if (api.set_use_highest_priority != nullptr) {
      api.set_use_highest_priority(option, true);
    }
  }

  if (!mapped_memory_key.empty()) {
    if (api.set_mapped_memory_cache_key != nullptr) {
      api.set_mapped_memory_cache_key(option, mapped_memory_key.c_str());
    }
  }

  if (!mapped_file_key.empty()) {
    if (api.set_mapped_file_cache_key != nullptr) {
      api.set_mapped_file_cache_key(option, mapped_file_key.c_str());
    }
  }
}

}  // namespace service
}  // namespace lynx
