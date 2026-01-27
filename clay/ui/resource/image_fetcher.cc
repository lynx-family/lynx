// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/ui/resource/image_fetcher.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "clay/common/service/service_manager.h"
#include "clay/gfx/image/animated_image.h"
#include "clay/gfx/image/base_image.h"
#include "clay/gfx/image/static_image.h"
#include "clay/gfx/image/svg_image.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_factory.h"
#include "clay/net/loader/resource_loader_intercept.h"

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

uint64_t ImageFetcher::FetchImage(const std::string& url, bool is_svg,
                                  const ImageCallback& callback) {
  auto fetchID = NextUniqueID();
  auto image = FindImageFromCache(url);
  if (image) {
    callback(image->NewInstance(), true);
    return fetchID;
  }
  image_callback_map_.insert({url, callback});
  auto it = url_loader_map_.find(url);
  if (it == url_loader_map_.end()) {
    if (is_svg) {
      auto loader = GetOrCreateResourceLoader(resource_loader_intercept_, url,
                                              task_runners_.GetUITaskRunner(),
                                              service_manager_);
      if (!loader) {
        OnFetchFinish(url, nullptr);
        return fetchID;
      }
      url_loader_map_.insert({url, loader});
      loader->Load(url, [self = GetWeakPtr(), url, callback](
                            const uint8_t* data, size_t size) {
        if (!self) {
          callback(nullptr, false);
          return;
        }
        if (data == nullptr || size == 0) {
          self->OnFetchFinish(url, nullptr);
          return;
        }
        auto image = SVGImage::Make(
            self->weak_factory_.GetWeakPtr(), url,
            std::string(reinterpret_cast<const char*>(data), size));
        self->active_url_image_map_.insert({url, image});
        self->OnFetchFinish(url, image);
      });
    } else {
      url_loader_map_.insert({url, nullptr});
      FetchImage(url, [self = GetWeakPtr(), url, callback](
                          std::shared_ptr<PlatformImage> platform_image) {
        if (!self) {
          callback(nullptr, false);
          return;
        }
        if (!platform_image) {
          self->OnFetchFinish(url, nullptr);
          return;
        }
        std::shared_ptr<BaseImage> image;
        if (platform_image->IsAnimated()) {
          image = AnimatedImage::Make(self->weak_factory_.GetWeakPtr(), url,
                                      self->task_runners_.GetUITaskRunner(),
                                      platform_image);
        } else {
          image = StaticImage::Make(self->weak_factory_.GetWeakPtr(), url,
                                    platform_image);
        }
        self->active_url_image_map_.insert({url, image});
        self->OnFetchFinish(url, image);
      });
    }
  }
  return fetchID;
}

uint64_t ImageFetcher::FetchSVGImageWithContent(const std::string& content,
                                                const ImageCallback& callback) {
  auto fetchID = NextUniqueID();
  std::string hash_string = std::to_string(std::hash<std::string>{}(content));
  auto image = FindImageFromCache(hash_string);
  if (image) {
    callback(image->NewInstance(), true);
    return fetchID;
  }
  image = SVGImage::Make(weak_factory_.GetWeakPtr(), hash_string, content);
  active_url_image_map_.insert({hash_string, image});
  callback(image->NewInstance(), false);
  return fetchID;
}

void ImageFetcher::OnFetchFinish(const std::string& url,
                                 std::shared_ptr<BaseImage> image) {
  auto loader_it = url_loader_map_.find(url);
  if (loader_it != url_loader_map_.end()) {
    url_loader_map_.erase(loader_it);
  }

  auto range = image_callback_map_.equal_range(url);
  for (auto it = range.first; it != range.second; ++it) {
    if (image) {
      it->second(image->NewInstance(), false);
    } else {
      it->second(nullptr, false);
    }
  }
  image_callback_map_.erase(url);
}

void ImageFetcher::TryCancelAsyncFetch(const std::string& url,
                                       uint64_t fetch_id) {
  auto iter = url_loader_map_.find(url);
  if (iter != url_loader_map_.end()) {
    if (iter->second) {
      iter->second->CancelAll();
    }
    url_loader_map_.erase(iter);
  }
  image_callback_map_.erase(url);
}

std::shared_ptr<BaseImage> ImageFetcher::FindImageFromCache(
    const std::string& url) {
  auto it = active_url_image_map_.find(url);
  if (it != active_url_image_map_.end()) {
    return it->second;
  }

  auto image = inactive_image_cache_->TakeImage(url);
  if (image) {
    active_url_image_map_.insert({url, image});
    return image;
  }
  return nullptr;
}

void ImageFetcher::OnImageHasNoAccessor(BaseImage* image) {
  auto& url = image->GetUrl();
  if (url.empty()) {
    return;
  }
  auto it = active_url_image_map_.find(url);
  if (it != active_url_image_map_.end()) {
    inactive_image_cache_->StoreImage(url, it->second);
    active_url_image_map_.erase(it);
  }
}

}  // namespace clay
