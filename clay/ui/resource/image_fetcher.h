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

class ImageFetcher : public fml::RefCountedThreadSafe<ImageFetcher> {
 public:
  using ImageCallback = std::function<void(std::shared_ptr<BaseImage>, bool)>;
  static fml::RefPtr<ImageFetcher> Create(
      clay::TaskRunners task_runners, fml::RefPtr<GPUUnrefQueue> unref_queue);

  virtual ~ImageFetcher() = default;
  ImageFetcher(clay::TaskRunners task_runners,
               fml::RefPtr<GPUUnrefQueue> unref_queue);
  uint64_t FetchImage(const std::string& url, bool is_svg,
                      const ImageCallback& callback);
  uint64_t FetchSVGImageWithContent(const std::string& content,
                                    const ImageCallback& callback);
  void TryCancelAsyncFetch(const std::string& url, uint64_t fetch_id);

 protected:
  fml::WeakPtr<ImageFetcher> GetWeakPtr() const {
    return weak_factory_.GetWeakPtr();
  }
  virtual void FetchImage(
      const std::string& url,
      const std::function<void(std::shared_ptr<PlatformImage>)>& callback) = 0;

  void OnFetchFinish(const std::string& url, std::shared_ptr<BaseImage> image);

 protected:
  fml::WeakPtrFactory<ImageFetcher> weak_factory_;
  clay::TaskRunners task_runners_;
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  std::shared_ptr<ImageCache<BaseImage>> image_cache_;
  std::unordered_map<std::string, std::shared_ptr<ResourceLoader>>
      url_loader_map_;
  std::multimap<std::string, ImageCallback> image_callback_map_;
};

}  // namespace clay
#endif  // CLAY_UI_RESOURCE_IMAGE_FETCHER_H_
