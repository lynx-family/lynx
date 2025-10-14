// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/resource/image_manager.h"

#include <string>
#include <utility>

#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/image/image_data_cache.h"
#include "clay/gfx/image/image_producer.h"
#include "clay/ui/resource/image_cache.h"

namespace clay {

namespace {
// TODO: Determine delay time based on specific conditions.
constexpr int64_t kDeferredRemovalInterval = 30;  // 30 seconds
// [key]: raster task queue id, [value]: ImageManager.
static std::unordered_map<size_t, std::shared_ptr<ImageManager>>
    image_manager_map;
static uint64_t clean_up_data_task_id = 0;
constexpr int64_t kDeferredCleanupImageDataInterval = 30;  // 30 seconds

uint64_t GenerateTaskID() {
  static uint64_t task_id = 0;
  ++task_id;
  if (task_id == 0) {
    ++task_id;
  }
  return task_id;
}

}  // namespace

ImageManager::ImageManager(clay::TaskRunners task_runners,
                           fml::RefPtr<GPUUnrefQueue> unref_queue)
    : task_runners_(task_runners),
      unref_queue_(unref_queue),
      inactive_image_cache_(std::make_shared<ImageCache<Image>>(
          task_runners_.GetUITaskRunner())) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
}

ImageManager::~ImageManager() {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
}

void ImageManager::AddAccessor(ImageResourceFetcher* accessor) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  accessors_.insert(accessor);
}

void ImageManager::RemoveAccessor(ImageResourceFetcher* accessor) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  accessors_.erase(accessor);
  // If there is no accessor, then perform removal.
  if (accessors_.empty()) {
#if defined(ENABLE_PLATFORM_DECODE)
    PerformDeferredRemoval();
#else
    PerformRemoval();
#endif
  }
}

void ImageManager::PerformDeferredRemoval() {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  std::weak_ptr<ImageManager> weak_this = shared_from_this();
  removal_task_id_ = GenerateTaskID();
  task_runners_.GetUITaskRunner()->PostDelayedTask(
      [weak_this, id = removal_task_id_]() {
        auto self = weak_this.lock();
        if (self && self->accessors_.empty() && self->removal_task_id_ == id) {
          self->PerformRemoval();
        }
      },
      fml::TimeDelta::FromSeconds(kDeferredRemovalInterval));
}

void ImageManager::PerformRemoval() {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  auto task_runner = task_runners_.GetUITaskRunner();
  size_t id = task_runners_.GetRasterTaskRunner()->GetTaskQueueId();
  image_manager_map.erase(id);

  // Perform the deferred cleanup of ImageDataCache when the last ImageManager
  // is destructed. This is to ensure that the resources managed by DataCache
  // are not prematurely cleaned up if there is a possibility of creating new
  // ImageManager instances shortly after.
  if (image_manager_map.empty()) {
    clean_up_data_task_id = GenerateTaskID();
    task_runners_.GetUITaskRunner()->PostDelayedTask(
        [id = clean_up_data_task_id]() {
          if (id == clean_up_data_task_id && image_manager_map.empty()) {
            ImageDataCache::GetInstance().ClearCache();
          }
        },
        fml::TimeDelta::FromSeconds(kDeferredCleanupImageDataInterval));
  }
}

void ImageManager::ClearCache() { inactive_image_cache_->ClearCache(); }

void ImageManager::ImageHasNoAccessor(const Image* image) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
  FML_DCHECK(image != nullptr);
#if defined(ENABLE_SVG)
  if (image->IsSVG() && !image->GetContentHash().empty()) {
    MoveToInactiveCacheIfNeeded(image->GetContentHash(), image);
    return;
  }
#endif
  MoveToInactiveCacheIfNeeded(image->GetUrl(), image);
}

void ImageManager::MoveToInactiveCacheIfNeeded(const std::string& url,
                                               const Image* image) {
  if (url.empty()) {
    return;
  }
  const_cast<Image*>(image)->SetIsActive(false);
  // Remove the image from the active_url_image_map_.
  auto range = active_url_image_map_.equal_range(url);
  size_t count = std::distance(range.first, range.second);
  if (count == 1) {
    auto id = std::hash<std::string>()(url);
    // If there is only one image, then move it to inactive image cache.
    inactive_image_cache_->StoreImage(id, range.first->second);
    active_url_image_map_.erase(range.first);
  } else {
    for (auto image_iter = range.first; image_iter != range.second;
         ++image_iter) {
      if (image_iter->second.get() == image) {
        active_url_image_map_.erase(image_iter);
        break;
      }
    }
  }
}

bool ImageManager::GetImageResource(const std::string& url,
                                    const ImageResourceCallback& callback,
                                    bool use_texture_backend, bool is_deferred,
                                    bool decode_with_priority,
                                    bool enable_low_quality_image,
                                    bool is_promise, bool is_svg) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  // First check if the image is already cached.
  auto image_resource = GetImageResourceFromCache(url);
  if (image_resource) {
    callback(std::move(image_resource), true);
    return true;
  }

  // If the image is not cached, then check if data is cached.
  auto sk_data = ImageDataCache::GetInstance().GetImageData(url);
  if (sk_data) {
#if defined(ENABLE_PLATFORM_DECODE)
    std::weak_ptr<ImageManager> weak_this = shared_from_this();
    CreateAndCacheImageAsync(
        url, sk_data, is_svg, use_texture_backend, is_promise,
        enable_low_quality_image, is_deferred,
        [weak_this, image_callback = std::move(callback), url]() {
          auto self = weak_this.lock();
          if (!self) {
            return;
          }
          FML_DCHECK(self->task_runners_.GetUITaskRunner()
                         ->RunsTasksOnCurrentThread());
          image_callback(self->GetImageResourceFromCache(url), true);
        });
#else
    std::shared_ptr<Image> image = CreateAndCacheImage(
        url, sk_data, is_svg, use_texture_backend, is_promise,
        enable_low_quality_image, is_deferred, decode_with_priority);
    callback(image->GetAccessor(), true);
#endif  // ENABLE_PLATFORM_DECODE
    return true;
  }

  return false;
}

void ImageManager::GetImageResource(const std::string& url,
                                    const ImageResourceCallback& callback,
                                    const uint8_t* source, const int len,
                                    bool use_texture_backend, bool is_deferred,
                                    bool decode_with_priority,
                                    bool enable_low_quality_image) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  // First check if the image is already cached.
  auto image_resource = GetImageResourceFromCache(url);
  if (image_resource) {
    callback(std::move(image_resource), true);
    return;
  }

  if (source == nullptr || len <= 0) {
    callback(nullptr, false);
    return;
  }

  // If the image is not cached, create new image by the source.
  auto sk_data = GrData::MakeWithCopy(source, len);
#if defined(ENABLE_PLATFORM_DECODE)
  std::weak_ptr<ImageManager> weak_this = shared_from_this();
  CreateAndCacheImageAsync(
      url, sk_data, false, use_texture_backend, false, enable_low_quality_image,
      is_deferred, [weak_this, image_callback = std::move(callback), url]() {
        auto self = weak_this.lock();
        if (!self) {
          return;
        }
        FML_DCHECK(
            self->task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
        image_callback(self->GetImageResourceFromCache(url), false);
      });
#else
  auto image = CreateAndCacheImage(url, sk_data, false, use_texture_backend,
                                   false, enable_low_quality_image, is_deferred,
                                   decode_with_priority);
  callback(image->GetAccessor(), false);
#endif
}

bool ImageManager::UpdateCachedImageData(const std::string& url,
                                         GrDataPtr data) {
  auto it = active_url_image_map_.find(url);
  // if the url is not in the map, it means that the image is not cached.
  if (it == active_url_image_map_.end()) {
    return false;
  } else {
    // if the url is in the map, it means that the image is cached, and we need
    // to update the image data if needed (e.g. promise image).
    auto range = active_url_image_map_.equal_range(url);
    for (auto iter = range.first; iter != range.second; ++iter) {
      iter->second->SetRawData(data);
    }
    return true;
  }
}

std::unique_ptr<ImageResource> ImageManager::GetImageResourceFromCache(
    const std::string& url) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  // First check if the image is already cached in active image map.
  auto it = active_url_image_map_.find(url);
  if (it != active_url_image_map_.end()) {
    std::shared_ptr<Image> image = it->second;
    // If the image is single frame image, then return a new ui accessor.
    if (image->IsSingleFrameImage()) {
      return image->GetAccessor();
    } else {
      // The first found image is multi frame image, then check all the images
      // whth the url key in the map to see if there is any image with no ui
      // accessor.
      auto range = active_url_image_map_.equal_range(url);
      for (auto iter = range.first; iter != range.second; ++iter) {
        if (image->GetUIAccessorCount() == 0) {
          return image->GetAccessor();
        }
      }

      // If all the multi frame images with the url key in the map have ui
      // accessor, then create a new image, and return it's ui accessor.
      auto copied_image = Image::CloneImage(image);
      copied_image->SetIsActive(true);
      active_url_image_map_.insert({url, copied_image});
      return copied_image->GetAccessor();
    }
  }

  // If the image is not cached, then check if the image is cached in inactive
  // image map.
  auto id = std::hash<std::string>()(url);
  auto image = inactive_image_cache_->TakeImage(id);
  if (image) {
    image->SetIsActive(true);
    active_url_image_map_.insert({url, image});
    return image->GetAccessor();
  }
  return nullptr;
}

#if defined(ENABLE_SVG)
std::unique_ptr<ImageResource> ImageManager::GetImageResourceFromSVGContent(
    const std::string& source, bool use_texture_backend, bool is_deferred,
    bool decode_with_priority) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (source.empty()) {
    return nullptr;
  }

  // First check if the image is already cached in Image Resource map.
  size_t hash = std::hash<std::string>()(source);
  auto hash_string = std::to_string(hash);
  auto image_resource = GetImageResourceFromCache(hash_string);
  if (image_resource) {
    return image_resource;
  }
  // If the image is not cached, then create a new image.
  auto sk_data = GrData::MakeWithCopy(source.data(), source.length());
  std::shared_ptr<Image> image = Image::CreateImage(
      "", sk_data, nullptr, shared_from_this(), task_runners_.GetUITaskRunner(),
      task_runners_.GetRasterTaskRunner(), true, use_texture_backend, false,
      false, is_deferred, decode_with_priority);
  image->SetContentHash(hash_string);
  image->SetIsActive(true);
  active_url_image_map_.insert({hash_string, image});
  return image->GetAccessor();
}
#endif

std::shared_ptr<Image> ImageManager::CreateAndCacheImage(
    const std::string& url, GrDataPtr data, bool is_svg,
    bool use_texture_backend, bool is_promise, bool enable_low_quality_image,
    bool is_deferred, bool decode_with_priority) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  std::shared_ptr<Image> image = Image::CreateImage(
      url, data, nullptr, shared_from_this(), task_runners_.GetUITaskRunner(),
      task_runners_.GetRasterTaskRunner(), is_svg, use_texture_backend,
      is_promise, enable_low_quality_image, is_deferred, decode_with_priority);
  image->SetIsActive(true);
  active_url_image_map_.insert({url, image});
  return image;
}

void ImageManager::CacheImage(const std::string& url,
                              std::shared_ptr<Image> image) {
  image->SetIsActive(true);
  active_url_image_map_.insert({url, std::move(image)});
}

std::unique_ptr<ImageResource> ImageManager::CreateImageResourceFromCachedData(
    const std::string& url, bool is_svg, bool use_texture_backend,
    bool is_promise, bool enable_low_quality_image, bool is_deferred,
    bool decode_with_priority) {
  auto sk_data = ImageDataCache::GetInstance().GetImageData(url);
  if (sk_data) {
    auto image = CreateAndCacheImage(url, sk_data, is_svg, use_texture_backend,
                                     is_promise, enable_low_quality_image,
                                     is_deferred, decode_with_priority);
    return image->GetAccessor();
  }
  return nullptr;
}

#if defined(ENABLE_PLATFORM_DECODE)
void ImageManager::CreateAndCacheImageAsync(
    const std::string& url, GrDataPtr data, bool is_svg,
    bool use_texture_backend, bool is_promise, bool enable_low_quality_image,
    bool is_deferred, const fml::closure& callback) {
  std::weak_ptr<ImageManager> weak_this = shared_from_this();
  // The image creation is moved to a worker thread
  // because when using the platform's decode function is time-consuming.
  GraphicsIsolate::Instance().GetConcurrentWorkerTaskRunner()->PostTask(
      [weak_this, task_runners = task_runners_, url, data, is_svg,
       use_texture_backend, is_promise, enable_low_quality_image, is_deferred,
       callback]() {
        auto image = Image::CreateImage(
            url, data, nullptr, weak_this, task_runners.GetUITaskRunner(),
            task_runners.GetRasterTaskRunner(), is_svg, use_texture_backend,
            is_promise, enable_low_quality_image, is_deferred);

        task_runners.GetUITaskRunner()->PostTask(
            [weak_this, callback, url, image]() {
              auto self = weak_this.lock();
              if (!self) {
                return;
              }

              // It the image is not cached, insert it into the map.
              auto it = self->active_url_image_map_.find(url);
              if (it == self->active_url_image_map_.end()) {
                image->SetIsActive(true);
                self->active_url_image_map_.insert({url, image});
              }
              callback();
            });
      });
}
#endif

std::shared_ptr<ImageManager> ImageManager::GetOrCreateImageManager(
    clay::TaskRunners task_runners, fml::RefPtr<GPUUnrefQueue> unref_queue) {
  FML_DCHECK(task_runners.GetUITaskRunner()->RunsTasksOnCurrentThread());

  // If the image manager is already created, return it.
  auto queue_id = task_runners.GetRasterTaskRunner()->GetTaskQueueId();
  auto iter = image_manager_map.find(queue_id);
  if (iter != image_manager_map.end()) {
    return iter->second;
  }

  // Otherwise, create a new image manager and return it.
  auto image_manager =
      std::make_shared<ImageManager>(task_runners, unref_queue);
  image_manager_map.emplace(queue_id, image_manager);

  return image_manager;
}

}  // namespace clay
