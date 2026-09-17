// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/shell/platform/common/desktop/codec/image_fetcher_desktop.h"

#include <utility>

#include "clay/gfx/graphics_isolate.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_factory.h"
#include "clay/shell/platform/common/desktop/codec/desktop_image.h"
#include "skity/codec/codec.hpp"

namespace clay {
namespace {
std::shared_ptr<ResourceLoader> GetOrCreateResourceLoader(
    std::shared_ptr<ResourceLoaderIntercept> intercept, const std::string& url,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ServiceManager> service_manager) {
  std::shared_ptr<ResourceLoader> loader = ResourceLoaderFactory::Create(
      url, task_runner, intercept, service_manager);
  return loader;
}
}  // namespace
fml::RefPtr<ImageFetcher> ImageFetcher::Create(
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    clay::TaskRunners task_runners, fml::RefPtr<GPUUnrefQueue> unref_queue,
    std::shared_ptr<ServiceManager> service_manager) {
  return fml::MakeRefCounted<ImageFetcherDesktop>(intercept, task_runners,
                                                  unref_queue, service_manager);
}
ImageFetcherDesktop::~ImageFetcherDesktop() = default;
ImageFetcherDesktop::ImageFetcherDesktop(
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    clay::TaskRunners task_runners, fml::RefPtr<GPUUnrefQueue> unref_queue,
    std::shared_ptr<ServiceManager> service_manager)
    : ImageFetcher(intercept, task_runners, unref_queue, service_manager) {}
void ImageFetcherDesktop::FetchImage(
    const std::string& url,
    const std::function<void(std::shared_ptr<PlatformImage>)>& callback,
    bool need_redirect) {
  std::shared_ptr<ResourceLoader> loader = GetOrCreateResourceLoader(
      resource_loader_intercept_, url, task_runners_.GetUITaskRunner(),
      service_manager_);
  if (!loader) {
    callback(nullptr);
    return;
  }
  url_loader_map_[url] = loader;
  loader->Load(
      url,
      [callback, ui_task_runner = task_runners_.GetUITaskRunner()](
          const uint8_t* data, size_t size) {
        if (!data || size == 0) {
          callback(nullptr);
          return;
        }
        auto raw_data = skity::Data::MakeWithCopy(data, size);
        if (raw_data->IsEmpty()) {
          callback(nullptr);
          return;
        }
        GraphicsIsolate::Instance().GetConcurrentWorkerTaskRunner()->PostTask(
            [callback, raw_data, ui_task_runner]() {
              auto codec = skity::Codec::MakeFromData(raw_data);
              if (!codec) {
                ui_task_runner->PostTask([callback]() { callback(nullptr); });
                return;
              }
              codec->SetData(raw_data);
              auto image = std::make_shared<DesktopImage>(std::move(codec));
              ui_task_runner->PostTask(
                  [image, callback]() { callback(image); });
            });
      },
      ResourceType::kImage, need_redirect);
}
}  // namespace clay
