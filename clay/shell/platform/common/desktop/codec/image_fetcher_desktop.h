// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_FETCHER_DESKTOP_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_FETCHER_DESKTOP_H_
#include <memory>
#include <string>
#include <utility>

#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/ui/resource/image_fetcher.h"

namespace clay {
class ImageFetcherDesktop : public ImageFetcher {
 public:
  ImageFetcherDesktop(std::shared_ptr<ResourceLoaderIntercept> intercept,
                      clay::TaskRunners task_runners,
                      fml::RefPtr<GPUUnrefQueue> unref_queue,
                      std::shared_ptr<ServiceManager> service_manager);
  ~ImageFetcherDesktop() override;
  void FetchImage(
      const std::string& url,
      const std::function<void(std::shared_ptr<PlatformImage>)>& callback,
      bool need_redirect) override;
};
}  // namespace clay
#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_FETCHER_DESKTOP_H_
