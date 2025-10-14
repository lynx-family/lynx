// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_IMAGE_MANAGER_H_
#define CLAY_UI_RESOURCE_IMAGE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/common/task_runners.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/image.h"
#include "clay/ui/resource/image_cache.h"

namespace clay {

class ImageResourceFetcher;
class ServiceManager;

typedef std::function<void(std::unique_ptr<ImageResource>, bool hit_cache)>
    ImageResourceCallback;
typedef std::function<void(std::shared_ptr<Image> image)> ImageCallback;

// On some platforms, multiple engine instances share a single Raster thread,
// while on others, there are multiple Raster threads. The number of
// GrDirectContexts and GPUUnrefQueues corresponds one-to-one with the number
// of Raster threads. When creating an Image, a GrDirectContext and
// GPUUnrefQueues are required. Therefore, to enhance the reusability of
// Images, the same Raster thread shares a single ImageManager, allowing for the
// sharing of cached Images within it.
class ImageManager : public Image::Notifier,
                     public std::enable_shared_from_this<ImageManager> {
 public:
  ImageManager(clay::TaskRunners task_runners,
               fml::RefPtr<GPUUnrefQueue> unref_queue);
  ~ImageManager() override;

  void AddAccessor(ImageResourceFetcher* accessor);
  void RemoveAccessor(ImageResourceFetcher* accessor);

  void ClearCache();

  bool GetImageResource(const std::string& final_url,
                        const ImageResourceCallback& callback,
                        bool use_texture_backend, bool is_deferred,
                        bool decode_with_priority,
                        bool enable_low_quality_image, bool is_promise,
                        bool is_svg);

  void GetImageResource(const std::string& url,
                        const ImageResourceCallback& callback,
                        const uint8_t* source, const int len,
                        bool use_texture_backend, bool is_deferred,
                        bool decode_with_priority,
                        bool enable_low_quality_image);

  std::unique_ptr<ImageResource> GetImageResourceFromCache(
      const std::string& url);

  bool UpdateCachedImageData(const std::string& url, GrDataPtr data);

#if defined(ENABLE_SVG)
  std::unique_ptr<ImageResource> GetImageResourceFromSVGContent(
      const std::string& source, bool use_texture_backend, bool is_deferred,
      bool decode_with_priority);
#endif

  std::shared_ptr<Image> CreateAndCacheImage(
      const std::string& url, GrDataPtr data, bool is_svg,
      bool use_texture_backend, bool is_promise, bool enable_low_quality_image,
      bool is_deferred, bool decode_with_priority);

  std::unique_ptr<ImageResource> CreateImageResourceFromCachedData(
      const std::string& url, bool is_svg, bool use_texture_backend,
      bool is_promise, bool enable_low_quality_image, bool is_deferred,
      bool decode_with_priority);

  void CacheImage(const std::string& url, std::shared_ptr<Image> image);

#if defined(ENABLE_PLATFORM_DECODE)
  void CreateAndCacheImageAsync(const std::string& url, GrDataPtr data,
                                bool is_svg, bool use_texture_backend,
                                bool is_promise, bool enable_low_quality_image,
                                bool is_deferred, const fml::closure& callback);
#endif

  // impelementation of Image::Notifier
  void ImageHasNoAccessor(const Image* image) override;
  fml::RefPtr<GPUUnrefQueue> GetUnrefQueue() override { return unref_queue_; }

  static std::shared_ptr<ImageManager> GetOrCreateImageManager(
      clay::TaskRunners task_runners, fml::RefPtr<GPUUnrefQueue> unref_queue);

 private:
  void MoveToInactiveCacheIfNeeded(const std::string& url, const Image* image);

  void PerformDeferredRemoval();
  void PerformRemoval();

  clay::TaskRunners task_runners_;
  fml::RefPtr<GPUUnrefQueue> unref_queue_;

  // Contains ImageResourceFetcher which are currently referencing ImageManager.
  // After all ImageResourceFetcher are removed, ImageManager will be destroyed.
  std::unordered_set<ImageResourceFetcher*> accessors_;

  // Contains images which are being used by rendering.
  std::multimap<std::string, std::shared_ptr<Image>> active_url_image_map_;
  // Contains images which are not being used by rendering.
  std::shared_ptr<ImageCache<Image>> inactive_image_cache_;

  uint64_t removal_task_id_ = 0;
};

}  // namespace clay

#endif  // CLAY_UI_RESOURCE_IMAGE_MANAGER_H_
