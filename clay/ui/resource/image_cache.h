// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_IMAGE_CACHE_H_
#define CLAY_UI_RESOURCE_IMAGE_CACHE_H_

#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/common/sys_info.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/image/image.h"

namespace clay {

constexpr size_t kMBytes = 1024 * 1024;  // 1MB
#if OS_ANDROID
constexpr size_t kImageCacheMaxBytes = 32 * kMBytes;  // 32MB
#else
constexpr size_t kImageCacheMaxBytes = 16 * kMBytes;  // 16MB
#endif
constexpr size_t kImageCacheMaxBytesLowMem = 16 * kMBytes;  // 16MB
constexpr float kDesiredOccupancyRatio = 0.8f;

constexpr size_t kMaxItemBytes = 4 * kMBytes;
constexpr size_t kMaxCleanItemsPerCell = 10;

constexpr int64_t kMaxTimeToLive = 60;  // 60 seconds
constexpr int64_t kPruneInterval = 1;   // 1 second.

template <class T>
class ImageCache : public std::enable_shared_from_this<ImageCache<T>> {
 public:
  explicit ImageCache(fml::RefPtr<fml::TaskRunner> ui_task_runner)
      : ui_task_runner_(ui_task_runner),
        current_cache_bytes_(0),
        max_cached_bytes_(SysInfo::IsLowEndDevice() ? kImageCacheMaxBytesLowMem
                                                    : kImageCacheMaxBytes) {
    desired_cache_bytes_ = max_cached_bytes_ * kDesiredOccupancyRatio;
  }
  ~ImageCache() { ClearCache(); }
  void StoreImage(int64_t id, std::shared_ptr<T> image,
                  bool is_prune_old_images = true) {
#if defined(ENABLE_PLATFORM_DECODE)
    FML_DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

    size_t bytes = image->GetGraphicsImageAllocSize();
    if (bytes > kMaxItemBytes || bytes == 0) {
      FML_LOG(ERROR) << "image bytes is " + std::to_string(bytes);
      return;
    }

    FML_DCHECK(cache_map_.find(id) == cache_map_.end());
    cache_list_.emplace_back(id, fml::TimePoint::Now(), bytes, image);
    cache_map_[id] = --cache_list_.end();
    current_cache_bytes_ += bytes;

    // Clean to max bytes if cache bytes is too large.
    if (current_cache_bytes_ > max_cached_bytes_) {
      CleanTo(desired_cache_bytes_);
    }

    if (!has_flying_prune_task_ && is_prune_old_images) {
      PruneOldImages();
    }
#endif
  }

  std::shared_ptr<T> TakeImage(int64_t id) {
#if defined(ENABLE_PLATFORM_DECODE)
    FML_DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

    auto found = cache_map_.find(id);
    if (found == cache_map_.end()) {
      return nullptr;
    }

    auto image = found->second->image_;
    current_cache_bytes_ -= found->second->bytes_;
    // Delete from inactive cache.
    cache_list_.erase(found->second);
    cache_map_.erase(found);

    return image;
#else
    return nullptr;
#endif
  }

  std::shared_ptr<T> FindImage(int64_t id) {
#if defined(ENABLE_PLATFORM_DECODE)
    FML_DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

    auto found = cache_map_.find(id);
    if (found == cache_map_.end()) {
      return nullptr;
    }

    auto image = found->second->image_;
    return image;
#else
    return nullptr;
#endif
  }

  void ClearCache() {
#if defined(ENABLE_PLATFORM_DECODE)
    FML_DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

    ClearCacheInternal();
#endif
  }

 private:
  void CleanTo(size_t bytes) {
    std::vector<std::shared_ptr<T>> images;
    size_t cleaned_items = 0;
    auto iter = cache_list_.begin();
    while (iter != cache_list_.end() && (current_cache_bytes_ > bytes) &&
           (cleaned_items++ < kMaxCleanItemsPerCell)) {
      images.push_back(iter->image_);
      current_cache_bytes_ -= iter->bytes_;
      cache_map_.erase(iter->id_);
      iter = cache_list_.erase(iter);
    }

    if (!images.empty()) {
      ReleaseImagesOnWorkerThread(images);
    }
  }
  void ClearCacheInternal() {
    if (current_cache_bytes_ > 0) {
      current_cache_bytes_ = 0;
      GraphicsIsolate::Instance().GetConcurrentWorkerTaskRunner()->PostTask(
          [cache_list = std::move(cache_list_),
           cache_map = std::move(cache_map_)]() mutable {
            cache_map.clear();
            cache_list.clear();
          });
    }
  }
  void PruneOldImages() {
    std::vector<std::shared_ptr<T>> images;
    size_t cleaned_items = 0;

    fml::TimePoint now = fml::TimePoint::Now();
    auto iter = cache_list_.begin();
    while (iter != cache_list_.end()) {
      fml::TimePoint expiry_time =
          iter->time_stamp_ + fml::TimeDelta::FromSeconds(kMaxTimeToLive);
      bool expired = now > expiry_time;
      if (!expired || cleaned_items >= kMaxCleanItemsPerCell) {
        FML_DLOG(INFO) << "post delayed task to prune old images, expired: "
                       << expired << ", cleaned_items: " << cleaned_items;
        has_flying_prune_task_ = true;
        std::weak_ptr<ImageCache<T>> weak_this = this->shared_from_this();
        ui_task_runner_->PostDelayedTask(
            [weak_this]() {
              auto self = weak_this.lock();
              if (self) {
                self->has_flying_prune_task_ = false;
                self->PruneOldImages();
              }
            },
            !expired ? expiry_time - now
                     : fml::TimeDelta::FromSeconds(kPruneInterval));
        break;
      } else {
        images.push_back(iter->image_);
        current_cache_bytes_ -= iter->bytes_;
        cache_map_.erase(iter->id_);
        iter = cache_list_.erase(iter);
        cleaned_items++;
      }
    }
    if (!images.empty()) {
      ReleaseImagesOnWorkerThread(images);
    }
  }

  void ReleaseImagesOnWorkerThread(
      const std::vector<std::shared_ptr<T>>& images) {
    // Destroy images maybe time consuming, we post to worker thread to avoid
    // blocking ui.
    GraphicsIsolate::Instance().GetConcurrentWorkerTaskRunner()->PostTask(
        [released_images = std::move(images)]() mutable {
          released_images.clear();
        });
  }

  struct ImageItem {
    ImageItem(int64_t id, const fml::TimePoint& time_stamp, size_t bytes,
              std::shared_ptr<T> image)
        : id_(id), time_stamp_(time_stamp), bytes_(bytes), image_(image) {}
    int64_t id_;
    fml::TimePoint time_stamp_;
    size_t bytes_;
    std::shared_ptr<T> image_;
  };

  std::list<ImageItem> cache_list_;
  std::unordered_map<int64_t, decltype(cache_list_.begin())> cache_map_;

  fml::RefPtr<fml::TaskRunner> ui_task_runner_;

  size_t current_cache_bytes_;
  size_t max_cached_bytes_;
  size_t desired_cache_bytes_;
  bool has_flying_prune_task_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_RESOURCE_IMAGE_CACHE_H_
