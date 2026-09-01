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

std::string MakeSizedKey(const std::string& key, const Size& decode_size) {
  if (decode_size.IsZero()) {
    return key;
  }
  // Static images are decoded during fetch, so differently sized requests
  // must not share an in-flight decode or a decoded-memory-cache entry.
  return key + "\x1f" + std::to_string(decode_size.width()) + "x" +
         std::to_string(decode_size.height());
}

std::string MakePendingKey(const std::string& key, uint64_t fetch_id) {
  // Requests awaiting layout cannot be coalesced: their final decode sizes may
  // differ even when they load the same URL.
  return key + "\x1epending:" + std::to_string(fetch_id);
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
                                  ImageRequestOptions options) {
  auto fetchID = NextUniqueID();

  std::string trimmed_url = url::TrimUrl(original_url);
  if (trimmed_url.empty()) {
    callback(nullptr, false);
    return fetchID;
  }

  if (is_svg) {
    options = {};
  }
  if (!deferred_decode_enabled_) {
    options = {};
  }
  const std::string base_identifier = trimmed_url.compare(0, 5, "data:") == 0
                                          ? lynx::base::md5(trimmed_url)
                                          : trimmed_url;
  const bool waiting_for_decode_size =
      options.use_view_size && !options.decode_size_ready;
  const Size cache_decode_size =
      options.use_view_size ? options.decode_size : Size{};
  const std::string identifier =
      MakeSizedKey(base_identifier, cache_decode_size);
  const std::string request_key =
      waiting_for_decode_size ? MakePendingKey(trimmed_url, fetchID)
                              : MakeSizedKey(trimmed_url, cache_decode_size);

  size_t cache_key_hash = std::hash<std::string>{}(identifier);
  auto image = FindImageFromCache(cache_key_hash, identifier);
  if (image) {
    callback(image->NewInstance(), true);
    return fetchID;
  }
  if (options.use_view_size && options.decode_size_ready &&
      !cache_decode_size.IsZero()) {
    image = FindReusableImage(base_identifier, cache_decode_size);
    if (image) {
      callback(image->NewInstance(), true);
      return fetchID;
    }
  }
  image_callback_map_.insert({request_key, {fetchID, callback}});
  fetch_request_map_.insert({fetchID, request_key});
  if (!is_svg) {
    request_base_identifier_map_[request_key] = base_identifier;
  }
  auto it = url_loader_map_.find(request_key);
  if (it == url_loader_map_.end()) {
    request_generation_map_[request_key] = fetchID;
    if (is_svg) {
      auto loader = GetOrCreateResourceLoader(
          resource_loader_intercept_, trimmed_url,
          task_runners_.GetUITaskRunner(), service_manager_);
      if (!loader) {
        OnFetchFinish(request_key, nullptr);
        return fetchID;
      }
      url_loader_map_.insert({request_key, loader});
      loader->Load(
          trimmed_url,
          [self = GetWeakPtr(), trimmed_url, request_key, identifier,
           request_generation = fetchID](const uint8_t* data, size_t size) {
            if (!self) {
              return;
            }
            auto generation_it =
                self->request_generation_map_.find(request_key);
            if (generation_it == self->request_generation_map_.end() ||
                generation_it->second != request_generation) {
              return;
            }
            if (data == nullptr || size == 0) {
              self->OnFetchFinish(request_key, nullptr);
              return;
            }

            auto image = SVGImage::Make(
                self->weak_factory_.GetWeakPtr(), trimmed_url,
                std::string(reinterpret_cast<const char*>(data), size));
            image->SetCacheIdentifier(identifier);
            self->active_image_map_.insert({identifier, image});
            self->OnFetchFinish(request_key, image);
          },
          ResourceType::kImage, need_redirect);
    } else {
      // If a decoded variant already exists, wait for layout before starting
      // another platform request. The resolved size may be covered by it.
      if (waiting_for_decode_size && HasImageVariant(base_identifier)) {
        deferred_image_request_map_.insert(
            {request_key,
             {trimmed_url, fetchID, base_identifier, options, need_redirect}});
      } else {
        StartImageFetch(trimmed_url, request_key, fetchID, base_identifier,
                        options, need_redirect);
      }
    }
  }
  return fetchID;
}

void ImageFetcher::ResumeDeferredDecode(uint64_t fetch_id, Size decode_size) {
  auto it = fetch_request_map_.find(fetch_id);
  if (it == fetch_request_map_.end()) {
    return;
  }
  const std::string request_key = it->second;
  auto identifier_it = request_base_identifier_map_.find(request_key);
  if (identifier_it != request_base_identifier_map_.end()) {
    const std::string identifier =
        MakeSizedKey(identifier_it->second, decode_size);
    auto image =
        FindImageFromCache(std::hash<std::string>{}(identifier), identifier);
    if (!image && !decode_size.IsZero()) {
      image = FindReusableImage(identifier_it->second, decode_size);
    }
    if (image) {
      CancelRequestLoad(request_key);
      OnFetchFinish(request_key, image, true);
      return;
    }
  }
  auto deferred_it = deferred_image_request_map_.find(request_key);
  if (deferred_it != deferred_image_request_map_.end()) {
    auto request = std::move(deferred_it->second);
    deferred_image_request_map_.erase(deferred_it);
    request.options.decode_size = decode_size;
    request.options.decode_size_ready = true;
    StartImageFetch(request.trimmed_url, request_key,
                    request.request_generation, request.base_identifier,
                    request.options, request.need_redirect);
    return;
  }
  ResumeDeferredDecodeInternal(request_key, decode_size);
}

void ImageFetcher::SetDeferredDecodeEnabled(bool enabled) {
  if (deferred_decode_enabled_ == enabled) {
    return;
  }
  deferred_decode_enabled_ = enabled;
  if (!enabled) {
    DisableDeferredDecodeInternal();
    auto deferred_requests = std::move(deferred_image_request_map_);
    deferred_image_request_map_.clear();
    for (auto& [request_key, request] : deferred_requests) {
      request.options = {};
      StartImageFetch(request.trimmed_url, request_key,
                      request.request_generation, request.base_identifier,
                      request.options, request.need_redirect);
    }
  }
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
  auto loader_it = url_loader_map_.find(request_key);
  if (loader_it != url_loader_map_.end()) {
    url_loader_map_.erase(loader_it);
  }
  request_generation_map_.erase(request_key);
  request_base_identifier_map_.erase(request_key);
  deferred_image_request_map_.erase(request_key);

  std::vector<ImageCallback> callbacks;
  auto range = image_callback_map_.equal_range(request_key);
  for (auto it = range.first; it != range.second; ++it) {
    fetch_request_map_.erase(it->second.first);
    callbacks.emplace_back(it->second.second);
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
  auto loader_it = url_loader_map_.find(request_key);
  if (loader_it != url_loader_map_.end()) {
    if (loader_it->second) {
      loader_it->second->CancelAll();
    }
    url_loader_map_.erase(loader_it);
  }
  request_generation_map_.erase(request_key);
  request_base_identifier_map_.erase(request_key);
  deferred_image_request_map_.erase(request_key);
  CancelFetchInternal(request_key);
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

bool ImageFetcher::HasImageVariant(const std::string& base_identifier) {
  auto variant_it = largest_image_variant_map_.find(base_identifier);
  if (variant_it == largest_image_variant_map_.end()) {
    return false;
  }
  if (variant_it->second.image.expired()) {
    largest_image_variant_map_.erase(variant_it);
    return false;
  }
  return true;
}

std::shared_ptr<BaseImage> ImageFetcher::FindReusableImage(
    const std::string& base_identifier, const Size& decode_size) {
  auto variant_it = largest_image_variant_map_.find(base_identifier);
  if (variant_it == largest_image_variant_map_.end()) {
    return nullptr;
  }

  auto& variant = variant_it->second;
  if (variant.image.expired()) {
    largest_image_variant_map_.erase(variant_it);
    return nullptr;
  }

  const bool intrinsic_size = variant.decode_size.IsZero();
  // A decoded image can be scaled down safely. Decode again when either
  // requested dimension exceeds the dimensions used by the old request.
  if (!intrinsic_size &&
      (variant.decode_size.width() < decode_size.width() ||
       variant.decode_size.height() < decode_size.height())) {
    return nullptr;
  }

  auto image = FindImageFromCache(std::hash<std::string>{}(variant.identifier),
                                  variant.identifier);
  if (!image) {
    largest_image_variant_map_.erase(variant_it);
  }
  return image;
}

void ImageFetcher::RegisterImageVariant(
    const std::string& base_identifier, const Size& decode_size,
    const std::string& identifier, const std::shared_ptr<BaseImage>& image) {
  auto variant_it = largest_image_variant_map_.find(base_identifier);
  if (variant_it == largest_image_variant_map_.end() ||
      variant_it->second.image.expired()) {
    largest_image_variant_map_[base_identifier] = {decode_size, identifier,
                                                   image};
    return;
  }

  auto& largest = variant_it->second;
  if (largest.identifier == identifier) {
    largest.image = image;
    return;
  }
  if (largest.decode_size.IsZero()) {
    return;
  }
  const int64_t largest_area =
      static_cast<int64_t>(largest.decode_size.width()) *
      largest.decode_size.height();
  const int64_t new_area =
      static_cast<int64_t>(decode_size.width()) * decode_size.height();
  if (decode_size.IsZero() || new_area > largest_area) {
    largest = {decode_size, identifier, image};
  }
}

void ImageFetcher::StartImageFetch(const std::string& trimmed_url,
                                   const std::string& request_key,
                                   uint64_t request_generation,
                                   const std::string& base_identifier,
                                   ImageRequestOptions options,
                                   bool need_redirect) {
  url_loader_map_.insert({request_key, nullptr});
  FetchImage(
      trimmed_url, request_key, request_generation, options,
      [self = GetWeakPtr(), trimmed_url, request_key, base_identifier,
       request_generation](std::shared_ptr<PlatformImage> platform_image,
                           Size applied_decode_size) {
        if (!self) {
          return;
        }
        auto generation_it = self->request_generation_map_.find(request_key);
        if (generation_it == self->request_generation_map_.end() ||
            generation_it->second != request_generation) {
          return;
        }
        if (!platform_image) {
          self->OnFetchFinish(request_key, nullptr);
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
        const std::string identifier =
            MakeSizedKey(base_identifier, applied_decode_size);
        image->SetCacheIdentifier(identifier);
        auto [image_it, inserted] =
            self->active_image_map_.insert({identifier, image});
        if (!inserted) {
          image = image_it->second;
        }
        self->RegisterImageVariant(base_identifier, applied_decode_size,
                                   identifier, image);
        self->OnFetchFinish(request_key, image);
      },
      need_redirect);
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

void ImageFetcher::ClearCache() { inactive_image_cache_->ClearCache(); }

}  // namespace clay
