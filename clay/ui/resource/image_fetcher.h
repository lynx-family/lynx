// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_IMAGE_FETCHER_H_
#define CLAY_UI_RESOURCE_IMAGE_FETCHER_H_

#include <map>
#include <memory>
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

struct ImageRequestOptions {
  // Static images are decoded to the view size when this is true. Animated
  // images always keep their intrinsic size.
  bool use_view_size = false;
  // Download may start before layout. Decoding waits until this becomes true.
  bool decode_size_ready = true;
  Size decode_size;
};

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
                      const ImageCallback& callback, bool need_redirect = true,
                      ImageRequestOptions options = {});
  uint64_t FetchSVGImageWithContent(const std::string& content,
                                    const ImageCallback& callback);

  // Supplies the decode size for a request whose download started before
  // layout. A zero size marks the size as resolved and falls back to decoding
  // at the image's intrinsic size.
  void ResumeDeferredDecode(uint64_t fetch_id, Size decode_size);
  // Disabling also resumes requests currently waiting for a decode size at
  // their intrinsic image size.
  void SetDeferredDecodeEnabled(bool enabled);
  bool IsDeferredDecodeEnabled() const { return deferred_decode_enabled_; }

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
                          uint64_t request_generation,
                          ImageRequestOptions options,
                          const PlatformImageCallback& callback,
                          bool need_redirect) = 0;
  virtual void ResumeDeferredDecodeInternal(const std::string& request_key,
                                            Size decode_size) = 0;
  virtual void DisableDeferredDecodeInternal() = 0;
  virtual void CancelFetchInternal(const std::string& request_key) = 0;

  void OnFetchFinish(const std::string& request_key,
                     std::shared_ptr<BaseImage> image, bool hit_cache = false);

  std::shared_ptr<BaseImage> FindImageFromCache(size_t cache_key_hash,
                                                const std::string& identifier);
  std::shared_ptr<BaseImage> FindReusableImage(
      const std::string& base_identifier, const Size& decode_size);
  bool HasImageVariant(const std::string& base_identifier);
  void RegisterImageVariant(const std::string& base_identifier,
                            const Size& decode_size,
                            const std::string& identifier,
                            const std::shared_ptr<BaseImage>& image);
  void StartImageFetch(const std::string& trimmed_url,
                       const std::string& request_key,
                       uint64_t request_generation,
                       const std::string& base_identifier,
                       ImageRequestOptions options, bool need_redirect);
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
  std::unordered_map<std::string, uint64_t> request_generation_map_;
  std::unordered_map<std::string, std::string> request_base_identifier_map_;
  struct ImageVariant {
    Size decode_size;
    std::string identifier;
    // The index must not keep images alive after the decoded-memory cache
    // evicts them.
    std::weak_ptr<BaseImage> image;
  };
  std::unordered_map<std::string, ImageVariant> largest_image_variant_map_;
  struct DeferredImageRequest {
    std::string trimmed_url;
    uint64_t request_generation;
    std::string base_identifier;
    ImageRequestOptions options;
    bool need_redirect;
  };
  std::unordered_map<std::string, DeferredImageRequest>
      deferred_image_request_map_;
  bool deferred_decode_enabled_ = true;
};

}  // namespace clay
#endif  // CLAY_UI_RESOURCE_IMAGE_FETCHER_H_
