// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_IMAGE_FETCHER_H_
#define CLAY_UI_RESOURCE_IMAGE_FETCHER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "clay/common/task_runners.h"
#include "clay/gfx/geometry/size.h"
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
  // nullopt defers decoding; zero, SVG and animations use intrinsic size.
  // Returns zero if the request completes synchronously.
  uint64_t FetchImage(const std::string& original_url, bool is_svg,
                      const ImageCallback& callback, bool need_redirect = true,
                      std::optional<Size> decode_size = Size{});
  uint64_t FetchSVGImageWithContent(const std::string& content,
                                    const ImageCallback& callback);

  // Resumes only requests whose decode size has not been resolved.
  void ResumeDeferredDecode(uint64_t fetch_id, Size decode_size);

  std::shared_ptr<skity::Image> LoadImage(const std::string& url);
  fml::RefPtr<fml::TaskRunner> GetUITaskRunner() const {
    return task_runners_.GetUITaskRunner();
  }

  void TryCancelAsyncFetch(const std::string& original_url, uint64_t fetch_id);

  void OnImageHasNoAccessor(BaseImage* image);

  void ClearCache();

 protected:
  fml::WeakPtr<ImageFetcher> GetWeakPtr() const {
    return weak_factory_.GetWeakPtr();
  }
  using PlatformImageCallback =
      std::function<void(std::shared_ptr<PlatformImage>, Size)>;
  virtual void FetchImage(const std::string& trimmed_url,
                          const std::string& request_key,
                          const PlatformImageCallback& callback,
                          bool need_redirect) = 0;
  void DecodeWhenReady(const std::string& request_key,
                       std::function<void(Size)> decode);

  void OnFetchFinish(const std::string& request_key,
                     std::shared_ptr<BaseImage> image, bool hit_cache = false);

  // nullopt accepts any cached size until layout determines the bounds.
  std::shared_ptr<BaseImage> FindImageFromCache(
      size_t cache_key_hash, const std::string& identifier,
      std::optional<Size> decode_size = Size{});
  void CancelRequestLoad(const std::string& request_key);
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
  std::multimap<std::string, std::pair<uint64_t, ImageCallback>>
      image_callback_map_;
  std::unordered_map<uint64_t, std::string> fetch_request_map_;
  struct ImageRequest {
    void ResolveDecodeSize(Size decode_size);
    std::string base_identifier;
    std::optional<Size> decode_size;
    std::function<void(Size)> decode;
    // Expiring this token invalidates queued callbacks, including reused keys.
    std::shared_ptr<bool> lifetime = std::make_shared<bool>();
    std::function<void()> cancel = {};
  };
  std::unordered_map<std::string, ImageRequest> image_request_map_;
};

}  // namespace clay
#endif  // CLAY_UI_RESOURCE_IMAGE_FETCHER_H_
