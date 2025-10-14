// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_data_cache.h"

#include "base/include/no_destructor.h"
#include "clay/common/sys_info.h"
#include "clay/fml/logging.h"

namespace clay {

namespace {
constexpr size_t kImageCacheMaxBytes = 8 * 1024 * 1024;        // 8MB
constexpr size_t kImageCacheMaxBytesLowMem = 4 * 1024 * 1024;  // 4MB
constexpr float kDesiredOccupancyRatio = 0.8f;
}  // namespace

ImageDataCache& ImageDataCache::GetInstance() {
  static fml::NoDestructor<ImageDataCache> instance;
  return *(instance.get());
}

ImageDataCache::ImageDataCache()
    : max_cached_bytes_(SysInfo::IsLowEndDevice() ? kImageCacheMaxBytesLowMem
                                                  : kImageCacheMaxBytes) {
  desired_cache_bytes_ = max_cached_bytes_ * kDesiredOccupancyRatio;
}

void ImageDataCache::SetMaxCachedBytes(size_t max_bytes) {
  std::lock_guard<std::mutex> lock(mutex_);
  max_cached_bytes_ = max_bytes;
  desired_cache_bytes_ = max_cached_bytes_ * kDesiredOccupancyRatio;
}

void ImageDataCache::ClearCache() {
  std::lock_guard<std::mutex> lock(mutex_);
  ClearCacheInternal();
}

void ImageDataCache::ClearCacheInternal() {
  active_cache_.clear();
  inactive_cache_list_.clear();
  inactive_cache_map_.clear();
  inactive_cache_bytes_ = 0;
}

void ImageDataCache::CacheImageData(const std::string& url,
                                    GrDataPtr image_data) {
  // url is empty or data url, do not cache.
  if (url.empty() || (url.compare(0, 5, "data:") == 0)) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  auto found = active_cache_.find(url);
  if (found != active_cache_.end()) {
    // Already in active cache, add ref count.
    found->second.second++;
  } else {
    // Not in active cache, add to active cache.
    active_cache_[url] = {image_data, 1};
  }
}

void ImageDataCache::ReleaseOrArchiveImageData(const std::string& url) {
  if (url.empty() || (url.compare(0, 5, "data:") == 0)) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  auto found = active_cache_.find(url);
  if (found != active_cache_.end()) {
    // Already in active cache, remove ref count.
    found->second.second--;
    // Remove from active cache if ref count is zero.
    if (found->second.second == 0) {
      MoveToInactiveCacheIfNeeded(url, found->second.first);
      active_cache_.erase(found);
    }
  }
}

void ImageDataCache::RemoveImageDataIfExist(const std::string& url) {
  if (url.empty() || (url.compare(0, 5, "data:") == 0)) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  auto found = active_cache_.find(url);
  if (found != active_cache_.end()) {
    // Already in active cache, remove from active cache.
    active_cache_.erase(found);
  }
}

GrDataPtr ImageDataCache::GetImageData(const std::string& url) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto found = active_cache_.find(url);
  if (found != active_cache_.end()) {
    // Already in active cache, return data.
    return found->second.first;
  }
  // Not in active cache, try to get from inactive cache.
  return TakeFromInactiveCache(url);
}

GrDataPtr ImageDataCache::TakeFromInactiveCache(const std::string& url) {
  auto found = inactive_cache_map_.find(url);
  // Not in inactive cache.
  if (found == inactive_cache_map_.end()) {
    return nullptr;
  }

  // Found in inactive cache.
  auto data = found->second->second;
  // Delete from inactive cache.
  inactive_cache_list_.erase(found->second);
  inactive_cache_map_.erase(found);

  if (data) {
    // update inactive cache bytes.
    inactive_cache_bytes_ -= DATA_GET_SIZE(data);
  }

  return data;
}

void ImageDataCache::MoveToInactiveCacheIfNeeded(const std::string& url,
                                                 GrDataPtr data) {
  // Data is too large, don't add to inactive cache.
  if (DATA_GET_SIZE(data) > max_cached_bytes_) {
    return;
  }

  FML_DCHECK(inactive_cache_map_.find(url) == inactive_cache_map_.end());

  // add to inactive cache and update inactive cache bytes.
  inactive_cache_list_.emplace_front(url, data);
  inactive_cache_map_[url] = inactive_cache_list_.begin();
  inactive_cache_bytes_ += DATA_GET_SIZE(data);

  // Clean to max bytes if inactive cache bytes is too large.
  if (inactive_cache_bytes_ > max_cached_bytes_) {
    CleanTo(desired_cache_bytes_);
  }
}

// Clean to max bytes.
void ImageDataCache::CleanTo(size_t bytes) {
  // Note: the inactive cache is a list, so the least recently used data is at
  // the front.
  auto iter = inactive_cache_list_.begin();
  while (iter != inactive_cache_list_.end()) {
    auto data_size = DATA_GET_SIZE(iter->second);
    if (data_size > bytes) {
      // Exceed available bytes, remove from inactive cache.
      inactive_cache_bytes_ -= data_size;
      inactive_cache_map_.erase(iter->first);
      iter = inactive_cache_list_.erase(iter);
    } else {
      // Keep this data, decrease available bytes.
      bytes -= data_size;
      iter++;
    }
  }
}

}  // namespace clay
