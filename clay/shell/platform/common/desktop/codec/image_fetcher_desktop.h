// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_FETCHER_DESKTOP_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_FETCHER_DESKTOP_H_
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/ui/resource/image_fetcher.h"

namespace skity {
class Data;
}

namespace clay {
class ImageFetcherDesktop : public ImageFetcher {
 public:
  ImageFetcherDesktop(std::shared_ptr<ResourceLoaderIntercept> intercept,
                      clay::TaskRunners task_runners,
                      fml::RefPtr<GPUUnrefQueue> unref_queue,
                      std::shared_ptr<ServiceManager> service_manager);
  ~ImageFetcherDesktop() override;
  void FetchImage(const std::string& url, const std::string& request_key,
                  uint64_t request_generation, ImageRequestOptions options,
                  const PlatformImageCallback& callback,
                  bool need_redirect) override;

 private:
  struct PendingDecodeRequest {
    uint64_t request_generation = 0;
    ImageRequestOptions options;
    PlatformImageCallback callback;
    std::shared_ptr<skity::Data> encoded_data;
  };

  void ResumeDeferredDecodeInternal(const std::string& request_key,
                                    Size decode_size) override;
  void DisableDeferredDecodeInternal() override;
  void CancelFetchInternal(const std::string& request_key) override;
  void OnResourceLoaded(const std::string& request_key,
                        uint64_t request_generation, const uint8_t* data,
                        size_t size);
  void TryStartDecode(const std::string& request_key);

  std::unordered_map<std::string, PendingDecodeRequest>
      pending_decode_requests_;
  fml::WeakPtrFactory<ImageFetcherDesktop> desktop_weak_factory_{this};
};
}  // namespace clay
#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_IMAGE_FETCHER_DESKTOP_H_
