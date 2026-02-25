// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/ui/resource/image_fetcher.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "base/include/md5.h"
#include "clay/common/service/service_manager.h"
#include "clay/gfx/image/animated_image.h"
#include "clay/gfx/image/base_image.h"
#include "clay/gfx/image/static_image.h"
#include "clay/gfx/image/svg_image.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_factory.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/net/url/url_helper.h"

namespace clay {

namespace {

uint64_t NextUniqueID() {
  static std::atomic<uint64_t> next_id(1);
  uint64_t id;
  do {
    id = next_id.fetch_add(1);
  } while (id == 0);  // 0 is reserved for an invalid id.
  return id;
}

std::shared_ptr<ResourceLoader> GetOrCreateResourceLoader(
    std::shared_ptr<ResourceLoaderIntercept> intercept, const std::string& url,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ServiceManager> service_manager) {
#if OS_ANDROID
  // Assuming that the `task_runner` will never be changed.
  if (url.compare(0, 5, "data:") == 0) {
    static auto data_loader =
        ResourceLoaderFactory::Create("data:", task_runner);
    return data_loader;
  }

  static auto url_loader =
      ResourceLoaderFactory::Create("https://", std::move(task_runner));
  return url_loader;
#else
  std::shared_ptr<ResourceLoader> loader = ResourceLoaderFactory::Create(
      url, task_runner, intercept, service_manager);
  return loader;
#endif
}

}  // namespace

ImageFetcher::ImageFetcher(std::shared_ptr<ResourceLoaderIntercept> intercept,
                           clay::TaskRunners task_runners,
                           fml::RefPtr<GPUUnrefQueue> unref_queue,
                           std::shared_ptr<ServiceManager> service_manager)
    : weak_factory_(this),
      resource_loader_intercept_(std::move(intercept)),
      service_manager_(std::move(service_manager)),
      task_runners_(std::move(task_runners)),
      unref_queue_(unref_queue),
      inactive_image_cache_(std::make_shared<ImageCache<BaseImage>>(
          task_runners_.GetUITaskRunner())) {}

ImageFetcher::~ImageFetcher() = default;

uint64_t ImageFetcher::FetchImage(const std::string& original_url, bool is_svg,
                                  const ImageCallback& callback) {
  auto fetchID = NextUniqueID();

  std::string trimmed_url = url::TrimUrl(original_url);
  if (trimmed_url.empty()) {
    callback(nullptr, false);
    return fetchID;
  }

  std::string identifier = trimmed_url.compare(0, 5, "data:") == 0
                               ? lynx::base::md5(trimmed_url)
                               : trimmed_url;
  size_t cache_key_hash = std::hash<std::string>{}(identifier);
  auto image = FindImageFromCache(cache_key_hash, identifier);
  if (image) {
    callback(image->NewInstance(), true);
    return fetchID;
  }
  image_callback_map_.insert({trimmed_url, callback});
  auto it = url_loader_map_.find(trimmed_url);
  if (it == url_loader_map_.end()) {
    if (is_svg) {
      auto loader = GetOrCreateResourceLoader(
          resource_loader_intercept_, trimmed_url,
          task_runners_.GetUITaskRunner(), service_manager_);
      if (!loader) {
        OnFetchFinish(trimmed_url, nullptr);
        return fetchID;
      }
      url_loader_map_.insert({trimmed_url, loader});
      loader->Load(trimmed_url, [self = GetWeakPtr(), trimmed_url, identifier,
                                 callback](const uint8_t* data, size_t size) {
        if (!self) {
          callback(nullptr, false);
          return;
        }
        if (data == nullptr || size == 0) {
          self->OnFetchFinish(trimmed_url, nullptr);
          return;
        }

        auto image = SVGImage::Make(
            self->weak_factory_.GetWeakPtr(), trimmed_url,
            std::string(reinterpret_cast<const char*>(data), size));
        image->SetCacheIdentifier(identifier);
        self->active_image_map_.insert({identifier, image});
        self->OnFetchFinish(trimmed_url, image);
      });
    } else {
      url_loader_map_.insert({trimmed_url, nullptr});
      FetchImage(trimmed_url,
                 [self = GetWeakPtr(), trimmed_url, identifier,
                  callback](std::shared_ptr<PlatformImage> platform_image) {
                   if (!self) {
                     callback(nullptr, false);
                     return;
                   }
                   if (!platform_image) {
                     self->OnFetchFinish(trimmed_url, nullptr);
                     return;
                   }
                   std::shared_ptr<BaseImage> image;
                   if (platform_image->IsAnimated()) {
                     image = AnimatedImage::Make(
                         self->weak_factory_.GetWeakPtr(), trimmed_url,
                         self->task_runners_.GetUITaskRunner(), platform_image);
                   } else {
                     image = StaticImage::Make(self->weak_factory_.GetWeakPtr(),
                                               trimmed_url, platform_image);
                   }
                   image->SetCacheIdentifier(identifier);
                   self->active_image_map_.insert({identifier, image});
                   self->OnFetchFinish(trimmed_url, image);
                 });
    }
  }
  return fetchID;
}

uint64_t ImageFetcher::FetchSVGImageWithContent(const std::string& content,
                                                const ImageCallback& callback) {
  auto fetchID = NextUniqueID();

  auto content_md5 = lynx::base::md5(content);
  size_t cache_key_hash = std::hash<std::string>{}(content_md5);
  auto image = FindImageFromCache(cache_key_hash, content_md5);
  if (image) {
    callback(image->NewInstance(), true);
    return fetchID;
  }
  auto svg_image = SVGImage::Make(weak_factory_.GetWeakPtr(), "", content);
  svg_image->SetCacheKeyHash(cache_key_hash);
  svg_image->SetContentMD5(content_md5);
  active_image_map_.insert({content_md5, svg_image});
  callback(svg_image->NewInstance(), false);
  return fetchID;
}

void ImageFetcher::OnFetchFinish(const std::string& trimmed_url,
                                 std::shared_ptr<BaseImage> image) {
  auto loader_it = url_loader_map_.find(trimmed_url);
  if (loader_it != url_loader_map_.end()) {
    url_loader_map_.erase(loader_it);
  }

  auto range = image_callback_map_.equal_range(trimmed_url);
  for (auto it = range.first; it != range.second; ++it) {
    if (image) {
      it->second(image->NewInstance(), false);
    } else {
      it->second(nullptr, false);
    }
  }
  image_callback_map_.erase(trimmed_url);
}

void ImageFetcher::TryCancelAsyncFetch(const std::string& original_url,
                                       uint64_t fetch_id) {
  std::string trimmed_url = url::TrimUrl(original_url);
  auto iter = url_loader_map_.find(trimmed_url);
  if (iter != url_loader_map_.end()) {
    if (iter->second) {
      iter->second->CancelAll();
    }
    url_loader_map_.erase(iter);
  }
  image_callback_map_.erase(trimmed_url);
}

std::shared_ptr<BaseImage> ImageFetcher::FindImageFromCache(
    size_t cache_key_hash, const std::string& identifier) {
  auto it = active_image_map_.find(identifier);
  if (it != active_image_map_.end()) {
    return it->second;
  }

  auto image = inactive_image_cache_->TakeImage(cache_key_hash, identifier);
  if (image) {
    active_image_map_.insert({identifier, image});
    return image;
  }
  return nullptr;
}

void ImageFetcher::OnImageHasNoAccessor(BaseImage* image) {
  if (!image->GetCacheIdentifier().empty()) {
    size_t cache_key_hash =
        std::hash<std::string>{}(image->GetCacheIdentifier());
    MoveToInactiveCacheIfNeeded(cache_key_hash, image->GetCacheIdentifier(),
                                image);
    return;
  }

  if (image->IsSVG()) {
    SVGImage* svg_image = static_cast<SVGImage*>(image);
    if (svg_image->GetContentMD5().empty()) {
      return;
    }
    MoveToInactiveCacheIfNeeded(svg_image->GetCacheKeyHash(),
                                svg_image->GetContentMD5(), svg_image);
  }
}

void ImageFetcher::MoveToInactiveCacheIfNeeded(size_t cache_key_hash,
                                               const std::string& identifier,
                                               const BaseImage* image) {
  // Remove the image from the active_image_map_.
  auto range = active_image_map_.equal_range(identifier);
  size_t count = std::distance(range.first, range.second);
  if (count == 1) {
    // If there is only one image, then move it to inactive image cache.
    inactive_image_cache_->StoreImage(cache_key_hash, identifier,
                                      range.first->second);
    active_image_map_.erase(range.first);
  } else {
    for (auto image_iter = range.first; image_iter != range.second;
         ++image_iter) {
      if (image_iter->second.get() == image) {
        active_image_map_.erase(image_iter);
        break;
      }
    }
  }
}

}  // namespace clay
