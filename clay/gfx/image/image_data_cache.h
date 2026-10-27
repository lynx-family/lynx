// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_DATA_CACHE_H_
#define CLAY_GFX_IMAGE_IMAGE_DATA_CACHE_H_

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class ImageDataCache {
 public:
  static ImageDataCache& GetInstance();

  ImageDataCache();

  void SetMaxCachedBytes(size_t bytes);
  void ClearCache();

  GrDataPtr GetImageData(const std::string& url);
  void CacheImageData(const std::string& url, GrDataPtr image_data);
  void ReleaseOrArchiveImageData(const std::string& url);
  void RemoveImageDataIfExist(const std::string& url);

 private:
  friend class ImageDataCacheTest_SetMaxCachedBytesTest_Test;
  friend class ImageDataCacheTest_CacheImageDataTest_Test;
  friend class ImageDataCacheTest_RemoveImageDataIfExistTest_Test;
  friend class ImageDataCacheTest_ClearCacheTest_Test;
  friend class ImageDataCacheTest_CleanToTest_Test;

  GrDataPtr TakeFromInactiveCache(const std::string& url);
  void MoveToInactiveCacheIfNeeded(const std::string& url, GrDataPtr data);

  void CleanTo(size_t bytes);
  void ClearCacheInternal();

  size_t max_cached_bytes_;
  size_t desired_cache_bytes_;
  size_t inactive_cache_bytes_ = 0;

  std::mutex mutex_;

  // key: url, value: {image_data, ref_count}
  std::unordered_map<std::string, std::pair<GrDataPtr, size_t>> active_cache_;
  std::list<std::pair<std::string, GrDataPtr>> inactive_cache_list_;
  std::unordered_map<std::string, decltype(inactive_cache_list_.begin())>
      inactive_cache_map_;
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_DATA_CACHE_H_
