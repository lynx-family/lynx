// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/shell/platform/common/desktop/codec/image_fetcher_desktop.h"

#include <utility>
#include <vector>

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
void ImageFetcherDesktop::FetchImage(const std::string& url,
                                     const std::string& request_key,
                                     uint64_t request_generation,
                                     ImageRequestOptions options,
                                     const PlatformImageCallback& callback,
                                     bool need_redirect) {
  std::shared_ptr<ResourceLoader> loader = GetOrCreateResourceLoader(
      resource_loader_intercept_, url, task_runners_.GetUITaskRunner(),
      service_manager_);
  if (!loader) {
    callback(nullptr, {});
    return;
  }
  pending_decode_requests_.insert(
      {request_key, {request_generation, options, callback, nullptr}});
  url_loader_map_[request_key] = loader;
  loader->Load(
      url,
      [self = desktop_weak_factory_.GetWeakPtr(), request_key,
       request_generation](const uint8_t* data, size_t size) {
        if (self) {
          self->OnResourceLoaded(request_key, request_generation, data, size);
        }
      },
      ResourceType::kImage, need_redirect);
}

void ImageFetcherDesktop::ResumeDeferredDecodeInternal(
    const std::string& request_key, Size decode_size) {
  auto it = pending_decode_requests_.find(request_key);
  if (it == pending_decode_requests_.end()) {
    return;
  }
  it->second.options.decode_size = decode_size;
  it->second.options.decode_size_ready = true;
  TryStartDecode(request_key);
}

void ImageFetcherDesktop::DisableDeferredDecodeInternal() {
  std::vector<std::string> requests_to_resume;
  for (auto& [request_key, request] : pending_decode_requests_) {
    if (request.options.use_view_size) {
      request.options = {};
      requests_to_resume.emplace_back(request_key);
    }
  }
  for (const auto& request_key : requests_to_resume) {
    TryStartDecode(request_key);
  }
}

void ImageFetcherDesktop::CancelFetchInternal(const std::string& request_key) {
  pending_decode_requests_.erase(request_key);
}

void ImageFetcherDesktop::OnResourceLoaded(const std::string& request_key,
                                           uint64_t request_generation,
                                           const uint8_t* data, size_t size) {
  auto it = pending_decode_requests_.find(request_key);
  if (it == pending_decode_requests_.end() ||
      it->second.request_generation != request_generation) {
    return;
  }
  if (!data || size == 0) {
    auto callback = std::move(it->second.callback);
    pending_decode_requests_.erase(it);
    callback(nullptr, {});
    return;
  }
  auto encoded_data = skity::Data::MakeWithCopy(data, size);
  if (!encoded_data || encoded_data->IsEmpty()) {
    auto callback = std::move(it->second.callback);
    pending_decode_requests_.erase(it);
    callback(nullptr, {});
    return;
  }
  it->second.encoded_data = std::move(encoded_data);
  TryStartDecode(request_key);
}

void ImageFetcherDesktop::TryStartDecode(const std::string& request_key) {
  auto it = pending_decode_requests_.find(request_key);
  if (it == pending_decode_requests_.end() || !it->second.encoded_data ||
      !it->second.options.decode_size_ready) {
    return;
  }

  auto request = std::move(it->second);
  pending_decode_requests_.erase(it);
  auto ui_task_runner = task_runners_.GetUITaskRunner();
  GraphicsIsolate::Instance().GetConcurrentWorkerTaskRunner()->PostTask(
      [request = std::move(request), ui_task_runner]() mutable {
        auto codec = skity::Codec::MakeFromData(request.encoded_data);
        if (!codec) {
          ui_task_runner->PostTask([callback = std::move(request.callback)]() {
            callback(nullptr, {});
          });
          return;
        }
        codec->SetData(request.encoded_data);
        const Size decode_size = request.options.use_view_size
                                     ? request.options.decode_size
                                     : Size{};
        auto image =
            std::make_shared<DesktopImage>(std::move(codec), decode_size);
        const Size applied_decode_size =
            request.options.use_view_size && !image->IsAnimated() ? decode_size
                                                                  : Size{};
        ui_task_runner->PostTask([image = std::move(image),
                                  callback = std::move(request.callback),
                                  applied_decode_size]() {
          callback(std::move(image), applied_decode_size);
        });
      });
}
}  // namespace clay
