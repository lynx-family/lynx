// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_IMAGE_RESOURCE_FETCHER_H_
#define CLAY_UI_RESOURCE_IMAGE_RESOURCE_FETCHER_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/common/task_runners.h"
#include "clay/gfx/gpu_object.h"
#include "clay/ui/resource/image_manager.h"

namespace clay {

class Image;
class ImageResource;
class ResourceLoader;
class ResourceLoaderIntercept;
class ServiceManager;

typedef uint64_t ImageFetchID;
static constexpr ImageFetchID kDefaultImageFetchID = 0;

class ImageResourceFetcher
    : public fml::RefCountedThreadSafe<ImageResourceFetcher> {
 public:
  ImageResourceFetcher(std::shared_ptr<ResourceLoaderIntercept> intercept,
                       clay::TaskRunners task_runners,
                       fml::RefPtr<GPUUnrefQueue> unref_queue,
                       std::shared_ptr<ServiceManager> service_manager);
  ~ImageResourceFetcher();

  ImageFetchID FetchImageAsync(const std::string& url,
                               const ImageResourceCallback& callback,
                               bool use_texture_backend,
                               bool is_deferred = false,
                               bool decode_with_priority = false,
                               bool need_redirect = true,
                               bool enable_low_quality_image = false,
                               bool is_promise = false, bool is_svg = false);
  void TryCancelAsyncFetch(const std::string& url, ImageFetchID fetch_id);

  // if fetching an image who has not been downloaded, an ImageResource without
  // image data will be return immediately, and a asynchronous loading task will
  // be post, then the image data will be put into the ImageResource in UI
  // thread when finishing the download, else if an image has been downloaded
  // before, the an ImageResource with image data will be return directly. The
  // design idea is similar to std::promise.
  std::unique_ptr<ImageResource> FetchPromiseImage(
      const std::string& url, const ImageResourceCallback& callback,
      bool use_texture_backend, bool need_redirect = false);

  void GetImageResource(const std::string& url,
                        const ImageResourceCallback& callback,
                        const uint8_t* source, const int len,
                        bool use_texture_backend, bool is_deferred,
                        bool decode_with_priority,
                        bool enable_low_quality_image);
  fml::WeakPtr<ImageResourceFetcher> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

#if defined(ENABLE_SVG)
  std::unique_ptr<ImageResource> GetImageResourceFromSVGContent(
      const std::string& source, bool use_texture_backend, bool is_deferred,
      bool decode_with_priority);
#endif

  void ClearCache();

  // Compare whether two urls point to a same image content. Url may be trimmed
  // to compare.
  static bool SameImage(const std::string& lhs, const std::string& rhs);

 private:
  ImageFetchID GenerateFetchID();
  ImageFetchID FetchImageAsyncInternal(
      const std::string& final_url, const ImageResourceCallback& callback,
      bool use_texture_backend, bool is_deferred, bool decode_with_priority,
      bool need_redirect, bool enable_low_quality_image, bool is_promise,
      bool is_svg);
  void OnDownloadEnd(bool success, const std::string& url, GrDataPtr data,
                     bool is_svg, bool use_texture_backend,
                     bool enable_low_quality_image = false,
                     bool is_deferred = false,
                     bool decode_with_priority = false,
                     bool is_promise = false);
  void RunImageResourceCallback(const std::string& url);

  std::shared_ptr<ResourceLoaderIntercept> resource_loader_intercept_;
  std::shared_ptr<ServiceManager> service_manager_;
  clay::TaskRunners task_runners_;

  fml::WeakPtrFactory<ImageResourceFetcher> weak_factory_;

  std::shared_ptr<ImageManager> image_manager_;

  std::unordered_map<std::string, std::shared_ptr<ResourceLoader>>
      url_loader_map_;
  std::multimap<std::string, std::pair<ImageFetchID, ImageResourceCallback>>
      image_resource_callback_map_;
};

}  // namespace clay

#endif  // CLAY_UI_RESOURCE_IMAGE_RESOURCE_FETCHER_H_
