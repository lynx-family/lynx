// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_IMAGE_FETCHER_H_
#define CLAY_UI_RESOURCE_IMAGE_FETCHER_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "clay/common/task_runners.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/base_image.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/ui/resource/image_cache.h"

namespace clay {

class ResourceLoaderIntercept;
class ServiceManager;

class ImageFetcher : public fml::RefCountedThreadSafe<ImageFetcher> {
 public:
  using ImageCallback =
      std::function<void(std::unique_ptr<BaseImageInstance>, bool)>;
  static fml::RefPtr<ImageFetcher> Create(
      std::shared_ptr<ResourceLoaderIntercept> intercept,
      clay::TaskRunners task_runners, fml::RefPtr<GPUUnrefQueue> unref_queue,
      std::shared_ptr<ServiceManager> service_manager);

  virtual ~ImageFetcher();
  ImageFetcher(std::shared_ptr<ResourceLoaderIntercept> intercept,
               clay::TaskRunners task_runners,
               fml::RefPtr<GPUUnrefQueue> unref_queue,
               std::shared_ptr<ServiceManager> service_manager);
  uint64_t FetchImage(const std::string& original_url, bool is_svg,
                      const ImageCallback& callback);
  uint64_t FetchSVGImageWithContent(const std::string& content,
                                    const ImageCallback& callback);
  void TryCancelAsyncFetch(const std::string& original_url, uint64_t fetch_id);

  void OnImageHasNoAccessor(BaseImage* image);

 protected:
  fml::WeakPtr<ImageFetcher> GetWeakPtr() const {
    return weak_factory_.GetWeakPtr();
  }
  virtual void FetchImage(
      const std::string& trimmed_url,
      const std::function<void(std::shared_ptr<PlatformImage>)>& callback) = 0;

  void OnFetchFinish(const std::string& trimmed_url,
                     std::shared_ptr<BaseImage> image);

  std::shared_ptr<BaseImage> FindImageFromCache(size_t cache_key_hash,
                                                const std::string& identifier);
  void MoveToInactiveCacheIfNeeded(size_t cache_key_hash,
                                   const std::string& identifier,
                                   const BaseImage* image);

 protected:
  fml::WeakPtrFactory<ImageFetcher> weak_factory_;
  std::shared_ptr<ResourceLoaderIntercept> resource_loader_intercept_;
  std::shared_ptr<ServiceManager> service_manager_;
  clay::TaskRunners task_runners_;
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  std::unordered_map<std::string, std::shared_ptr<BaseImage>> active_image_map_;
  std::shared_ptr<ImageCache<BaseImage>> inactive_image_cache_;
  std::unordered_map<std::string, std::shared_ptr<ResourceLoader>>
      url_loader_map_;
  std::multimap<std::string, ImageCallback> image_callback_map_;
};

}  // namespace clay
#endif  // CLAY_UI_RESOURCE_IMAGE_FETCHER_H_
