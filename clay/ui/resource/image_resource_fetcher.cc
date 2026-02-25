// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/resource/image_resource_fetcher.h"

#include "base/include/fml/time/time_point.h"
#include "base/include/md5.h"
#include "clay/gfx/image/image_descriptor.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_factory.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/net/url/url_helper.h"
#include "clay/ui/resource/image_manager.h"

namespace clay {

namespace {

// To improve performance, we can reuse the `ResourceLoader` for all images.
std::shared_ptr<ResourceLoader> GetOrCreateResourceLoader(
    std::shared_ptr<ResourceLoaderIntercept> intercept, const std::string& url,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ServiceManager> service_manager) {
#if OS_ANDROID
  // Assuming that the `task_runner` will never be changed.
  if (url.compare(0, 5, "data:") == 0) {
    static auto data_loader =
        ResourceLoaderFactory::Create("data:", task_runner);
    return data_loader;
  }

  static auto url_loader =
      ResourceLoaderFactory::Create("https://", std::move(task_runner));
  return url_loader;
#else
  std::shared_ptr<ResourceLoader> loader = ResourceLoaderFactory::Create(
      url, task_runner, intercept, service_manager);
  return loader;
#endif
}

}  // namespace

ImageResourceFetcher::ImageResourceFetcher(
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    clay::TaskRunners task_runners, fml::RefPtr<GPUUnrefQueue> unref_queue,
    std::shared_ptr<ServiceManager> service_manager)
    : resource_loader_intercept_(intercept),
      service_manager_(service_manager),
      task_runners_(std::move(task_runners)),
      weak_factory_(this) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  image_manager_ =
      ImageManager::GetOrCreateImageManager(task_runners_, unref_queue);
  image_manager_->AddAccessor(this);
}

ImageResourceFetcher::~ImageResourceFetcher() {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  image_manager_->RemoveAccessor(this);
}

ImageFetchID ImageResourceFetcher::FetchImageAsync(
    const std::string& original_url, const ImageResourceCallback& callback,
    bool use_texture_backend, bool is_deferred, bool decode_with_priority,
    bool need_redirect, bool enable_low_quality_image, bool is_promise,
    bool is_svg) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  std::string trimmed_url = url::TrimUrl(original_url);
  if (trimmed_url.empty()) {
    callback(nullptr, false);
    return kDefaultImageFetchID;
  }

  auto begin = fml::TimePoint::Now();
  std::string cache_identifier = trimmed_url.compare(0, 5, "data:") == 0
                                     ? lynx::base::md5(trimmed_url)
                                     : trimmed_url;
  // check if image is already cached.
  if (image_manager_->GetImageResource(
          trimmed_url, cache_identifier,
          [callback, begin, trimmed_url,
           ui_task_runner = task_runners_.GetUITaskRunner()](
              std::unique_ptr<ImageResource> image_resource, bool hit_cache) {
            FML_DCHECK(ui_task_runner->RunsTasksOnCurrentThread());

            auto end = fml::TimePoint::Now();
            FML_DLOG(INFO)
                << "ImageResourceFetcher: fetch image and hit cache; url = "
                << trimmed_url
                << " , cost_time = " << (end - begin).ToMicroseconds();
            callback(std::move(image_resource), hit_cache);
          },
          use_texture_backend, is_deferred, decode_with_priority,
          enable_low_quality_image, is_promise, is_svg)) {
    return kDefaultImageFetchID;
  }

  return FetchImageAsyncInternal(trimmed_url, cache_identifier, callback,
                                 use_texture_backend, is_deferred,
                                 decode_with_priority, need_redirect,
                                 enable_low_quality_image, is_promise, is_svg);
}

ImageFetchID ImageResourceFetcher::FetchImageAsyncInternal(
    const std::string& trimmed_url, const std::string& cache_identifier,
    const ImageResourceCallback& callback, bool use_texture_backend,
    bool is_deferred, bool decode_with_priority, bool need_redirect,
    bool enable_low_quality_image, bool is_promise, bool is_svg) {
  auto begin = fml::TimePoint::Now();
  ImageFetchID current_fetch_id = GenerateFetchID();
  auto it = url_loader_map_.find(trimmed_url);
  if (it == url_loader_map_.end()) {
    std::shared_ptr<ResourceLoader> loader = GetOrCreateResourceLoader(
        resource_loader_intercept_, trimmed_url,
        task_runners_.GetUITaskRunner(), service_manager_);
    if (!loader) {
      callback(nullptr, false);
      return kDefaultImageFetchID;
    }
    // task will be executed in ui_task_runner.
    auto task = [self = weak_factory_.GetWeakPtr(), trimmed_url,
                 cache_identifier, begin, is_svg, use_texture_backend,
                 enable_low_quality_image, is_deferred, decode_with_priority,
                 is_promise](const uint8_t* image, size_t len) {
      auto end = fml::TimePoint::Now();
      FML_DLOG(INFO) << "ImageResourceFetcher: fetch image finish; url = "
                     << trimmed_url
                     << " , cost_time = " << (end - begin).ToMicroseconds();
      if (!self) {
        return;
      }
      bool success = (image != nullptr && len > 0);
      GrDataPtr data;
      if (success) {
        data = GrData::MakeWithCopy(image, len);
      } else {
        data = GrData::MakeEmpty();
      }
      self->OnDownloadEnd(success, trimmed_url, cache_identifier, data, is_svg,
                          use_texture_backend, enable_low_quality_image,
                          is_deferred, decode_with_priority, is_promise);
    };
    url_loader_map_.insert({trimmed_url, loader});
    image_resource_callback_map_.insert(
        {trimmed_url, {current_fetch_id, callback}});
    loader->Load(trimmed_url, task, ResourceType::kImage, need_redirect);
  } else {
    image_resource_callback_map_.insert(
        {trimmed_url, {current_fetch_id, callback}});
  }
  return current_fetch_id;
}

std::unique_ptr<ImageResource> ImageResourceFetcher::FetchPromiseImage(
    const std::string& original_url, const ImageResourceCallback& callback,
    bool use_texture_backend, bool need_redirect) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  std::string trimmed_url = url::TrimUrl(original_url);
  if (trimmed_url.empty()) {
    callback(nullptr, false);
    return nullptr;
  }

  bool use_promise = true;
  if (!use_texture_backend) {
    use_promise = false;
  }

  std::string cache_identifier = trimmed_url.compare(0, 5, "data:") == 0
                                     ? lynx::base::md5(trimmed_url)
                                     : trimmed_url;
  auto image_resource =
      image_manager_->GetImageResourceFromCache(cache_identifier);
  if (image_resource) {
    // if image has been cached, there is no need to invoke callback
    return image_resource;
  }

  image_resource = image_manager_->CreateImageResourceFromCachedData(
      trimmed_url, cache_identifier, false, use_texture_backend, use_promise,
      false, false, false);
  if (image_resource) {
    // if image data has been downloaded, there is no need to invoke callback
    return image_resource;
  }

  // image data is not prepared and initiate an asynchronous load request
  FetchImageAsyncInternal(trimmed_url, cache_identifier, callback,
                          use_texture_backend, false, false, need_redirect,
                          false, use_promise, false);

  // create a clay::Image without actual data in it, when download finished,
  // put the download data to the image.
  auto image = image_manager_->CreateAndCacheImage(
      trimmed_url, cache_identifier, nullptr, false, use_texture_backend,
      use_promise, false, false, false);
  return image->GetAccessor();
}

void ImageResourceFetcher::GetImageResource(
    const std::string& original_url, const ImageResourceCallback& callback,
    const uint8_t* source, const int len, bool use_texture_backend,
    bool is_deferred, bool decode_with_priority,
    bool enable_low_quality_image) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  std::string trimmed_url = url::TrimUrl(original_url);
  std::string cache_identifier = trimmed_url.compare(0, 5, "data:") == 0
                                     ? lynx::base::md5(trimmed_url)
                                     : trimmed_url;
  image_manager_->GetImageResource(
      trimmed_url, cache_identifier, callback, source, len, use_texture_backend,
      is_deferred, decode_with_priority, enable_low_quality_image);
}

#if defined(ENABLE_SVG)
std::unique_ptr<ImageResource>
ImageResourceFetcher::GetImageResourceFromSVGContent(
    const std::string& source, bool use_texture_backend, bool is_deferred,
    bool decode_with_priority) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  return image_manager_->GetImageResourceFromSVGContent(
      source, use_texture_backend, is_deferred, decode_with_priority);
}
#endif

void ImageResourceFetcher::TryCancelAsyncFetch(const std::string& original_url,
                                               ImageFetchID fetch_id) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (fetch_id == kDefaultImageFetchID) {
    return;
  }

  std::string trimmed_url = url::TrimUrl(original_url);
  if (trimmed_url.empty()) {
    return;
  }

  // Remove the ImageResourceCallback from the map.
  auto range = image_resource_callback_map_.equal_range(trimmed_url);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second.first == fetch_id) {
      image_resource_callback_map_.erase(it);
      break;
    }
  }

  // Cancel the loading only when the number of ImageResourceCallbacks reaches
  // zero, because multiple ImageResourceCallbacks may exist for the same url.
  if (!image_resource_callback_map_.count(trimmed_url)) {
    auto it = url_loader_map_.find(trimmed_url);
    if (it != url_loader_map_.end()) {
      FML_LOG(INFO) << "Cancel image fetch: " << fetch_id
                    << ", url: " << trimmed_url;
      it->second->CancelAll();
      url_loader_map_.erase(it);
    }
  }
}

void ImageResourceFetcher::ClearCache() {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
  image_manager_->ClearCache();
}

void ImageResourceFetcher::OnDownloadEnd(
    bool success, const std::string& trimmed_url,
    const std::string& cache_identifier, GrDataPtr data, bool is_svg,
    bool use_texture_backend, bool enable_low_quality_image, bool is_deferred,
    bool decode_with_priority, bool is_promise) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
  // remove loader first.
  auto loader_it = url_loader_map_.find(trimmed_url);
  if (loader_it != url_loader_map_.end()) {
    url_loader_map_.erase(loader_it);
  }

  if (!success) {
    RunImageResourceCallback(trimmed_url, cache_identifier);
    return;
  }

  if (!image_manager_->UpdateCachedImageData(cache_identifier, data)) {
    // If the image is not cached, create a new image and cache it.
    image_manager_->CreateAndCacheImage(trimmed_url, cache_identifier, data,
                                        is_svg, use_texture_backend, is_promise,
                                        enable_low_quality_image, is_deferred,
                                        decode_with_priority);
    RunImageResourceCallback(trimmed_url, cache_identifier);
  } else {
    RunImageResourceCallback(trimmed_url, cache_identifier);
  }
}

void ImageResourceFetcher::RunImageResourceCallback(
    const std::string& trimmed_url, const std::string& cache_identifier) {
  auto range = image_resource_callback_map_.equal_range(trimmed_url);
  for (auto it = range.first; it != range.second; ++it) {
    it->second.second(
        image_manager_->GetImageResourceFromCache(cache_identifier), false);
  }
  image_resource_callback_map_.erase(trimmed_url);
}

bool ImageResourceFetcher::SameImage(const std::string& lhs,
                                     const std::string& rhs) {
  return url::TrimUrl(lhs) == url::TrimUrl(rhs);
}

ImageFetchID ImageResourceFetcher::GenerateFetchID() {
  static ImageFetchID fetch_id = kDefaultImageFetchID;
  ++fetch_id;
  if (fetch_id == kDefaultImageFetchID) {
    ++fetch_id;
  }
  return fetch_id;
}

}  // namespace clay
