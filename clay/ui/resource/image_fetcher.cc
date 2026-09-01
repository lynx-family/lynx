// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/ui/resource/image_fetcher.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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
#include "skity/codec/codec.hpp"
#include "skity/graphic/image.hpp"

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

fml::RefPtr<fml::TaskRunner> GetImageCacheCleanupTaskRunner(
    const TaskRunners& task_runners) {
#if defined(OS_IOS)
  // The iOS IO runner may be backed by LynxNormalTask. Keep the concurrent
  // worker fallback for embedders whose IO and UI work run on the same thread.
  auto io_task_runner = task_runners.GetIOTaskRunner();
  if (io_task_runner && !io_task_runner->RunsTasksOnCurrentThread()) {
    return io_task_runner;
  }
#endif
  return nullptr;
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
          task_runners_.GetUITaskRunner(),
          GetImageCacheCleanupTaskRunner(task_runners_))) {}

ImageFetcher::~ImageFetcher() = default;

uint64_t ImageFetcher::FetchImage(const std::string& original_url, bool is_svg,
                                  const ImageCallback& callback,
                                  bool need_redirect,
                                  std::optional<Size> decode_size) {
  auto fetch_id = NextUniqueID();

  std::string trimmed_url = url::TrimUrl(original_url);
  if (trimmed_url.empty()) {
    callback(nullptr, false);
    return 0;
  }

  const std::string base_identifier = trimmed_url.compare(0, 5, "data:") == 0
                                          ? lynx::base::md5(trimmed_url)
                                          : trimmed_url;
  auto image = FindImageFromCache(std::hash<std::string>{}(base_identifier),
                                  base_identifier, decode_size);
  if (image) {
    callback(image->NewInstance(), true);
    return 0;
  }
  std::string request_key = trimmed_url;
  if (decode_size && !decode_size->IsZero()) {
    request_key += "\x1f" + std::to_string(decode_size->width()) + "x" +
                   std::to_string(decode_size->height());
  }
  image_callback_map_.insert({request_key, {fetch_id, callback}});
  fetch_request_map_.insert({fetch_id, request_key});
  auto [it, inserted] = image_request_map_.try_emplace(
      request_key, ImageRequest{base_identifier, decode_size, {}});
  if (inserted) {
    std::weak_ptr<bool> lifetime = it->second.lifetime;
    if (is_svg) {
      auto loader = GetOrCreateResourceLoader(
          resource_loader_intercept_, trimmed_url,
          task_runners_.GetUITaskRunner(), service_manager_);
      if (!loader) {
        OnFetchFinish(request_key, nullptr);
        return 0;
      }
      url_loader_map_.insert({request_key, loader});
      loader->Load(
          trimmed_url,
          [self = GetWeakPtr(), trimmed_url, request_key, base_identifier,
           lifetime](const uint8_t* data, size_t size) {
            if (!self || lifetime.expired()) {
              return;
            }
            if (data == nullptr || size == 0) {
              self->OnFetchFinish(request_key, nullptr);
              return;
            }

            auto image = SVGImage::Make(
                self->weak_factory_.GetWeakPtr(), trimmed_url,
                std::string(reinterpret_cast<const char*>(data), size));
            image->SetCacheIdentifier(base_identifier);
            self->active_image_map_.insert({base_identifier, image});
            self->OnFetchFinish(request_key, image);
          },
          ResourceType::kImage, need_redirect);
    } else {
      FetchImage(
          trimmed_url, request_key,
          [self = GetWeakPtr(), trimmed_url, request_key, base_identifier,
           lifetime](std::shared_ptr<PlatformImage> platform_image,
                     Size applied_decode_size) {
            if (!self || lifetime.expired()) {
              return;
            }
            if (!platform_image) {
              self->OnFetchFinish(request_key, nullptr);
              return;
            }

            std::shared_ptr<BaseImage> image;
            if (platform_image->IsAnimated()) {
              applied_decode_size = {};
              image = AnimatedImage::Make(
                  self->weak_factory_.GetWeakPtr(), trimmed_url,
                  self->task_runners_.GetUITaskRunner(), platform_image);
            } else {
              image = StaticImage::Make(self->weak_factory_.GetWeakPtr(),
                                        trimmed_url, platform_image);
            }
            image->SetCacheIdentifier(base_identifier);
            image->SetDecodeSize(applied_decode_size);
            self->active_image_map_.insert_or_assign(base_identifier, image);
            self->OnFetchFinish(request_key, image);
          },
          need_redirect);
    }
  }
  return fetch_request_map_.count(fetch_id) ? fetch_id : 0;
}

void ImageFetcher::ResumeDeferredDecode(uint64_t fetch_id, Size decode_size) {
  auto it = fetch_request_map_.find(fetch_id);
  if (it == fetch_request_map_.end()) {
    return;
  }
  const std::string request_key = it->second;
  auto request_it = image_request_map_.find(request_key);
  if (request_it == image_request_map_.end() ||
      request_it->second.decode_size) {
    return;
  }
  const auto& base_identifier = request_it->second.base_identifier;
  auto image = FindImageFromCache(std::hash<std::string>{}(base_identifier),
                                  base_identifier, decode_size);
  if (image) {
    CancelRequestLoad(request_key);
    OnFetchFinish(request_key, image, true);
    return;
  }
  request_it->second.ResolveDecodeSize(decode_size);
}

void ImageFetcher::DecodeWhenReady(const std::string& request_key,
                                   std::function<void(Size)> decode) {
  auto it = image_request_map_.find(request_key);
  if (it == image_request_map_.end()) {
    return;
  }
  it->second.decode = std::move(decode);
  if (it->second.decode_size) {
    it->second.ResolveDecodeSize(*it->second.decode_size);
  }
}

void ImageFetcher::ImageRequest::ResolveDecodeSize(Size decode_size) {
  this->decode_size = decode_size;
  if (auto pending = decode) {
    pending(decode_size);
  }
}

uint64_t ImageFetcher::FetchSVGImageWithContent(const std::string& content,
                                                const ImageCallback& callback) {
  auto fetch_id = NextUniqueID();

  auto content_md5 = lynx::base::md5(content);
  size_t cache_key_hash = std::hash<std::string>{}(content_md5);
  auto image = FindImageFromCache(cache_key_hash, content_md5);
  if (image) {
    callback(image->NewInstance(), true);
    return fetch_id;
  }
  auto svg_image = SVGImage::Make(weak_factory_.GetWeakPtr(), "", content);
  svg_image->SetCacheKeyHash(cache_key_hash);
  svg_image->SetContentMD5(content_md5);
  active_image_map_.insert({content_md5, svg_image});
  callback(svg_image->NewInstance(), false);
  return fetch_id;
}

std::shared_ptr<skity::Image> ImageFetcher::LoadImage(const std::string& url) {
  std::string trimmed_url = url::TrimUrl(url);
  auto loader = GetOrCreateResourceLoader(
      resource_loader_intercept_, trimmed_url, task_runners_.GetUITaskRunner(),
      service_manager_);
  if (!loader) {
    return nullptr;
  }
  auto raw_resource = loader->LoadSync(trimmed_url, ResourceType::kImage, true);
  if (!raw_resource.data) {
    return nullptr;
  }
  auto data = skity::Data::MakeWithProc(raw_resource.data.get(),
                                        raw_resource.length, nullptr, nullptr);
  auto codec = skity::Codec::MakeFromData(data);
  if (!codec) {
    return nullptr;
  }
  codec->SetData(data);
  auto pixel = codec->Decode();
  if (!pixel) {
    return nullptr;
  }

  return skity::Image::MakeImage(pixel);
}

void ImageFetcher::OnFetchFinish(const std::string& request_key,
                                 std::shared_ptr<BaseImage> image,
                                 bool hit_cache) {
  url_loader_map_.erase(request_key);
  image_request_map_.erase(request_key);

  std::vector<ImageCallback> callbacks;
  auto range = image_callback_map_.equal_range(request_key);
  for (auto it = range.first; it != range.second; ++it) {
    fetch_request_map_.erase(it->second.first);
    callbacks.emplace_back(std::move(it->second.second));
  }
  image_callback_map_.erase(request_key);
  for (const auto& callback : callbacks) {
    callback(image ? image->NewInstance() : nullptr, hit_cache);
  }
}

void ImageFetcher::TryCancelAsyncFetch(const std::string& /*original_url*/,
                                       uint64_t fetch_id) {
  if (fetch_id == 0) {
    return;
  }

  auto request_it = fetch_request_map_.find(fetch_id);
  if (request_it == fetch_request_map_.end()) {
    return;
  }
  const std::string request_key = request_it->second;
  fetch_request_map_.erase(request_it);

  auto range = image_callback_map_.equal_range(request_key);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second.first == fetch_id) {
      image_callback_map_.erase(it);
      break;
    }
  }

  // Cancel the loading only when the number of ImageCallbacks reaches
  // zero, because multiple ImageCallbacks may exist for the same request.
  if (!image_callback_map_.count(request_key)) {
    CancelRequestLoad(request_key);
  }
}

void ImageFetcher::CancelRequestLoad(const std::string& request_key) {
  auto request = image_request_map_.find(request_key);
  if (request != image_request_map_.end()) {
    auto cancel = std::move(request->second.cancel);
    image_request_map_.erase(request);
    if (cancel) {
      cancel();
    }
  }
  auto loader_it = url_loader_map_.find(request_key);
  if (loader_it != url_loader_map_.end()) {
    if (loader_it->second) {
      loader_it->second->CancelAll();
    }
    url_loader_map_.erase(loader_it);
  }
}

std::shared_ptr<BaseImage> ImageFetcher::FindImageFromCache(
    size_t cache_key_hash, const std::string& identifier,
    std::optional<Size> decode_size) {
  auto matches_decode_size = [&decode_size](const BaseImage& image) {
    const Size cached_size = image.GetDecodeSize();
    return !decode_size || cached_size.IsZero() ||
           (!decode_size->IsZero() &&
            cached_size.width() >= decode_size->width() &&
            cached_size.height() >= decode_size->height());
  };
  auto it = active_image_map_.find(identifier);
  if (it != active_image_map_.end()) {
    std::shared_ptr<BaseImage> image = it->second;
    if (!matches_decode_size(*image)) {
      active_image_map_.erase(it);
      return nullptr;
    }
    return image;
  }

  auto image = inactive_image_cache_->TakeImage(cache_key_hash, identifier);
  if (image && matches_decode_size(*image)) {
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
  if (count == 1 && range.first->second.get() == image) {
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

void ImageFetcher::ClearCache() { inactive_image_cache_->ClearCache(); }

}  // namespace clay
