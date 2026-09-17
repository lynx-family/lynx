// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/shell/platform/common/desktop/codec/image_fetcher_desktop.h"

#include <utility>

#include "clay/common/service/service_manager.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_factory.h"
#include "clay/shell/platform/common/desktop/codec/desktop_image.h"
#include "clay/shell/platform/common/desktop/codec/desktop_image_codec_service.h"

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

std::shared_ptr<DesktopImageCodecService> GetImageCodecService(
    const std::shared_ptr<ServiceManager>& service_manager) {
  if (!service_manager) {
    return nullptr;
  }
  return service_manager->GetMultiThreadService<DesktopImageCodecService>();
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
    : ImageFetcher(intercept, task_runners, unref_queue, service_manager),
      codec_service_(GetImageCodecService(service_manager)) {}
void ImageFetcherDesktop::FetchImage(const std::string& url,
                                     const std::string& request_key,
                                     const PlatformImageCallback& callback,
                                     bool need_redirect) {
  auto codec_service = codec_service_;
  std::shared_ptr<ResourceLoader> loader = GetOrCreateResourceLoader(
      resource_loader_intercept_, url, task_runners_.GetUITaskRunner(),
      service_manager_);
  if (!loader) {
    callback(nullptr, {});
    return;
  }
  url_loader_map_[request_key] = loader;
  loader->Load(
      url,
      [self = GetWeakPtr(), request_key, callback, codec_service,
       ui_task_runner = task_runners_.GetUITaskRunner()](const uint8_t* data,
                                                         size_t size) {
        if (!self) {
          return;
        }
        if (!data || size == 0) {
          callback(nullptr, {});
          return;
        }
        auto raw_data = skity::Data::MakeWithCopy(data, size);
        if (!raw_data || raw_data->IsEmpty()) {
          callback(nullptr, {});
          return;
        }
        static_cast<ImageFetcherDesktop*>(self.get())
            ->DecodeWhenReady(request_key, [callback, codec_service, raw_data,
                                            ui_task_runner](Size decode_size) {
              GraphicsIsolate::Instance()
                  .GetConcurrentWorkerTaskRunner()
                  ->PostTask([callback, codec_service, raw_data, ui_task_runner,
                              decode_size]() {
                    auto generator = codec_service
                                         ? codec_service->CreateCodecGenerator(
                                               raw_data, decode_size)
                                         : nullptr;
                    if (!generator) {
                      ui_task_runner->PostTask(
                          [callback]() { callback(nullptr, {}); });
                      return;
                    }
                    auto image =
                        std::make_shared<DesktopImage>(std::move(generator));
                    ui_task_runner->PostTask([image, callback, decode_size]() {
                      callback(image, decode_size);
                    });
                  });
            });
      },
      ResourceType::kImage, need_redirect);
}
}  // namespace clay
