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
#include "skity/codec/codec.hpp"
#include "skity/graphic/image.hpp"

namespace clay {

namespace {

class SkityStaticPlatformImage : public PlatformImage {
 public:
  explicit SkityStaticPlatformImage(std::shared_ptr<skity::Pixmap> pixmap)
      : pixmap_(std::move(pixmap)) {}

  int GetWidth() override { return pixmap_ ? pixmap_->Width() : 0; }
  int GetHeight() override { return pixmap_ ? pixmap_->Height() : 0; }
  int64_t GetDuration() override { return 0; }
  std::shared_ptr<skity::Pixmap> ToBitmap() override { return pixmap_; }
  void DrawFrame(std::function<void()> on_frame_changed) override {}
  bool IsAnimated() override { return false; }
  void SetAutoPlay(bool auto_play) override {}
  void SetLoopCount(int loop_count) override {}
  void StartAnimation() override {}
  void StopAnimation() override {}
  void PauseAnimation() override {}
  void ResumeAnimation() override {}

 private:
  std::shared_ptr<skity::Pixmap> pixmap_;
};

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

std::shared_ptr<PlatformImage> DecodeStaticImage(const uint8_t* data,
                                                 size_t size) {
  if (!data || size == 0) {
    return nullptr;
  }

  auto image_data = skity::Data::MakeWithCopy(data, size);
  auto codec = skity::Codec::MakeFromData(image_data);
  if (!codec) {
    return nullptr;
  }

  codec->SetData(image_data);
  auto pixmap = codec->Decode();
  if (!pixmap) {
    return nullptr;
  }

  return std::make_shared<SkityStaticPlatformImage>(std::move(pixmap));
}

class DefaultImageFetcher : public ImageFetcher {
 public:
  using ImageFetcher::ImageFetcher;

 private:
  void FetchImage(
      const std::string& trimmed_url,
      const std::function<void(std::shared_ptr<PlatformImage>)>& callback,
      bool need_redirect) override {
    auto loader =
        GetOrCreateResourceLoader(resource_loader_intercept_, trimmed_url,
                                  task_runners_.GetUITaskRunner(),
                                  service_manager_);
    if (!loader) {
      callback(nullptr);
      return;
    }

    loader->Load(
        trimmed_url,
        [callback](const uint8_t* data, size_t size) {
          callback(DecodeStaticImage(data, size));
        },
        ResourceType::kImage, need_redirect);
  }
};

}  // namespace

fml::RefPtr<ImageFetcher> ImageFetcher::Create(
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    clay::TaskRunners task_runners, fml::RefPtr<GPUUnrefQueue> unref_queue,
    std::shared_ptr<ServiceManager> service_manager) {
  return fml::MakeRefCounted<DefaultImageFetcher>(
      std::move(intercept), std::move(task_runners), std::move(unref_queue),
      std::move(service_manager));
}

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
                                  const ImageCallback& callback,
                                  bool need_redirect) {
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
  image_callback_map_.insert({trimmed_url, {fetchID, callback}});
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
      loader->Load(
          trimmed_url,
          [self = GetWeakPtr(), trimmed_url, identifier](const uint8_t* data,
                                                         size_t size) {
            if (!self) {
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
          },
          ResourceType::kImage, need_redirect);
    } else {
      url_loader_map_.insert({trimmed_url, nullptr});
      FetchImage(
          trimmed_url,
          [self = GetWeakPtr(), trimmed_url,
           identifier](std::shared_ptr<PlatformImage> platform_image) {
            if (!self) {
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
          },
          need_redirect);
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

void ImageFetcher::OnFetchFinish(const std::string& trimmed_url,
                                 std::shared_ptr<BaseImage> image) {
  auto loader_it = url_loader_map_.find(trimmed_url);
  if (loader_it != url_loader_map_.end()) {
    url_loader_map_.erase(loader_it);
  }

  auto range = image_callback_map_.equal_range(trimmed_url);
  for (auto it = range.first; it != range.second; ++it) {
    if (image) {
      it->second.second(image->NewInstance(), false);
    } else {
      it->second.second(nullptr, false);
    }
  }
  image_callback_map_.erase(trimmed_url);
}

void ImageFetcher::TryCancelAsyncFetch(const std::string& original_url,
                                       uint64_t fetch_id) {
  if (fetch_id == 0) {
    return;
  }

  std::string trimmed_url = url::TrimUrl(original_url);

  // Remove the ImageCallback from the map.
  auto range = image_callback_map_.equal_range(trimmed_url);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second.first == fetch_id) {
      image_callback_map_.erase(it);
      break;
    }
  }

  // Cancel the loading only when the number of ImageCallbacks reaches
  // zero, because multiple ImageCallbacks may exist for the same url.
  if (!image_callback_map_.count(trimmed_url)) {
    auto iter = url_loader_map_.find(trimmed_url);
    if (iter != url_loader_map_.end()) {
      if (iter->second) {
        iter->second->CancelAll();
      }
      url_loader_map_.erase(iter);
    }
  }
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

void ImageFetcher::ClearCache() { inactive_image_cache_->ClearCache(); }

}  // namespace clay
