// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/resource/image_resource_fetcher.h"

#include "base/include/fml/time/time_point.h"
#include "clay/gfx/image/image_descriptor.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/loader/resource_loader_factory.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/ui/resource/image_manager.h"

namespace clay {

namespace {
const std::vector<std::string> TRIM_HOST_REGEX = {"p[0-9]*-sign.xsj.wasu.tv/"};

// Trim host and params after .jpeg .webp .jpg .png.
// Example: "http://SPECIFIC_HOST/xxx.jpg?param=a" to "xxx.jpg"
// Return: trimmed url if success, or empty string if failed.
std::string TrimUrl(const std::string& url) {
  // FIXME(liruokun): Due to performance issue in VideoCut, currently skip the
  // TrimUrl process. We should find a better way to reuse the same image on
  // different CDN hosts.
#if 0
  if (url.empty() || url.size() < 4) {
    // url is shorter than ".jpg" or ".png"
    return "";
  }
  size_t trim_start = 0;
  for (const auto& regex_str : TRIM_HOST_REGEX) {
    std::regex regex(regex_str);
    std::smatch match;
    if (std::regex_search(url, match, regex)) {
      trim_start = match.prefix().length() + match[0].length();
      break;
    }
  }
  if (trim_start == url.size() || trim_start == 0) {
    return "";
  }

  size_t trim_end = url.size();
  for (const char* p = url.c_str() + url.size() - 3; p != url.c_str();) {
    --p;
    if (static_cast<size_t>(p - url.c_str()) <= url.size() - 5) {
      if (strncmp(p, ".jpeg", 5) == 0 || strncmp(p, ".webp", 5) == 0) {
        trim_end = static_cast<size_t>(p - url.c_str()) + 5;
        break;
      }
    }
    if (strncmp(p, ".jpg", 4) == 0 || strncmp(p, ".png", 4) == 0) {
      trim_end = static_cast<size_t>(p - url.c_str()) + 4;
      break;
    }
  }
  if (trim_end <= trim_start) {
    return "";
  }
  return url.substr(trim_start, trim_end - trim_start);
#else
  return url;
#endif
}

#define USE_TRIM_URL_IF_COULD(trim_url, url) trim_url.empty() ? url : trim_url

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
    const std::string& url, const ImageResourceCallback& callback,
    bool use_texture_backend, bool is_deferred, bool decode_with_priority,
    bool need_redirect, bool enable_low_quality_image, bool is_promise,
    bool is_svg) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  std::string trim_url = TrimUrl(url);
  std::string final_url = USE_TRIM_URL_IF_COULD(trim_url, url);

  auto begin = fml::TimePoint::Now();

  // check if image is already cached.
  if (image_manager_->GetImageResource(
          final_url,
          [callback, begin, url,
           ui_task_runner = task_runners_.GetUITaskRunner()](
              std::unique_ptr<ImageResource> image_resource, bool hit_cache) {
            FML_DCHECK(ui_task_runner->RunsTasksOnCurrentThread());

            auto end = fml::TimePoint::Now();
            FML_DLOG(ERROR)
                << "ImageResourceFetcher: fetch image and hit cache; url = "
                << url << " , cost_time = " << (end - begin).ToMicroseconds();
            callback(std::move(image_resource), hit_cache);
          },
          use_texture_backend, is_deferred, decode_with_priority,
          enable_low_quality_image, is_promise, is_svg)) {
    return kDefaultImageFetchID;
  }

  return FetchImageAsyncInternal(final_url, callback, use_texture_backend,
                                 is_deferred, decode_with_priority,
                                 need_redirect, enable_low_quality_image,
                                 is_promise, is_svg);
}

ImageFetchID ImageResourceFetcher::FetchImageAsyncInternal(
    const std::string& final_url, const ImageResourceCallback& callback,
    bool use_texture_backend, bool is_deferred, bool decode_with_priority,
    bool need_redirect, bool enable_low_quality_image, bool is_promise,
    bool is_svg) {
  auto begin = fml::TimePoint::Now();
  ImageFetchID current_fetch_id = GenerateFetchID();
  auto it = url_loader_map_.find(final_url);
  if (it == url_loader_map_.end()) {
    std::shared_ptr<ResourceLoader> loader = GetOrCreateResourceLoader(
        resource_loader_intercept_, final_url, task_runners_.GetUITaskRunner(),
        service_manager_);
    if (!loader) {
      callback(nullptr, false);
      return kDefaultImageFetchID;
    }
    // task will be executed in ui_task_runner.
    auto task = [self = weak_factory_.GetWeakPtr(), final_url, begin, is_svg,
                 use_texture_backend, enable_low_quality_image, is_deferred,
                 decode_with_priority,
                 is_promise](const uint8_t* image, size_t len) {
      auto end = fml::TimePoint::Now();
      FML_DLOG(ERROR) << "ImageResourceFetcher: fetch image finish; url = "
                      << final_url
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
      self->OnDownloadEnd(success, final_url, data, is_svg, use_texture_backend,
                          enable_low_quality_image, is_deferred,
                          decode_with_priority, is_promise);
    };
    url_loader_map_.insert({final_url, loader});
    image_resource_callback_map_.insert(
        {final_url, {current_fetch_id, callback}});
    loader->Load(final_url, task, ResourceType::kImage, need_redirect);
  } else {
    image_resource_callback_map_.insert(
        {final_url, {current_fetch_id, callback}});
  }
  return current_fetch_id;
}

std::unique_ptr<ImageResource> ImageResourceFetcher::FetchPromiseImage(
    const std::string& url, const ImageResourceCallback& callback,
    bool use_texture_backend, bool need_redirect) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  std::string trim_url = TrimUrl(url);
  std::string final_url = USE_TRIM_URL_IF_COULD(trim_url, url);

  bool use_promise = true;
  if (!use_texture_backend) {
    use_promise = false;
  }

  auto image_resource = image_manager_->GetImageResourceFromCache(url);
  if (image_resource) {
    // if image has been cached, there is no need to invoke callback
    return image_resource;
  }

  image_resource = image_manager_->CreateImageResourceFromCachedData(
      url, false, use_texture_backend, use_promise, false, false, false);
  if (image_resource) {
    // if image data has been downloaded, there is no need to invoke callback
    return image_resource;
  }

  // image data is not prepared and initiate an asynchronous load request
  FetchImageAsyncInternal(url, callback, use_texture_backend, false, false,
                          need_redirect, false, true, false);

  // create a clay::Image without actual data in it, when download finished,
  // put the download data to the image.
  auto image = image_manager_->CreateAndCacheImage(
      url, nullptr, false, use_texture_backend, use_promise, false, false,
      false);
  return image->GetAccessor();
}

void ImageResourceFetcher::GetImageResource(
    const std::string& url, const ImageResourceCallback& callback,
    const uint8_t* source, const int len, bool use_texture_backend,
    bool is_deferred, bool decode_with_priority,
    bool enable_low_quality_image) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  std::string trim_url = TrimUrl(url);
  std::string final_url = USE_TRIM_URL_IF_COULD(trim_url, url);

  image_manager_->GetImageResource(
      final_url, callback, source, len, use_texture_backend, is_deferred,
      decode_with_priority, enable_low_quality_image);
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

void ImageResourceFetcher::TryCancelAsyncFetch(const std::string& url,
                                               ImageFetchID fetch_id) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (fetch_id == kDefaultImageFetchID) {
    return;
  }

  std::string trim_url = TrimUrl(url);
  std::string final_url = USE_TRIM_URL_IF_COULD(trim_url, url);

  // Remove the ImageResourceCallback from the map.
  auto range = image_resource_callback_map_.equal_range(final_url);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second.first == fetch_id) {
      image_resource_callback_map_.erase(it);
      break;
    }
  }

  // Cancel the loading only when the number of ImageResourceCallbacks reaches
  // zero, because multiple ImageResourceCallbacks may exist for the same url.
  if (!image_resource_callback_map_.count(final_url)) {
    auto it = url_loader_map_.find(final_url);
    if (it != url_loader_map_.end()) {
      FML_LOG(INFO) << "Cancel image fetch: " << fetch_id
                    << ", url: " << final_url;
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
    bool success, const std::string& url, GrDataPtr data, bool is_svg,
    bool use_texture_backend, bool enable_low_quality_image, bool is_deferred,
    bool decode_with_priority, bool is_promise) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
  // remove loader first.
  auto loader_it = url_loader_map_.find(url);
  if (loader_it != url_loader_map_.end()) {
    url_loader_map_.erase(loader_it);
  }

  if (!success) {
    RunImageResourceCallback(url);
    return;
  }

  if (!image_manager_->UpdateCachedImageData(url, data)) {
    // If the image is not cached, create a new image and cache it.
#if defined(ENABLE_PLATFORM_DECODE)
    image_manager_->CreateAndCacheImageAsync(
        url, data, is_svg, use_texture_backend, is_promise,
        enable_low_quality_image, is_deferred,
        [weak_self = weak_factory_.GetWeakPtr(),
         ui_task_runner = task_runners_.GetUITaskRunner(), url]() {
          FML_DCHECK(ui_task_runner->RunsTasksOnCurrentThread());
          if (!weak_self) {
            return;
          }
          weak_self->RunImageResourceCallback(url);
        });
#else
    image_manager_->CreateAndCacheImage(url, data, is_svg, use_texture_backend,
                                        is_promise, enable_low_quality_image,
                                        is_deferred, decode_with_priority);
    RunImageResourceCallback(url);
#endif
  } else {
    RunImageResourceCallback(url);
  }
}

void ImageResourceFetcher::RunImageResourceCallback(const std::string& url) {
  auto range = image_resource_callback_map_.equal_range(url);
  for (auto it = range.first; it != range.second; ++it) {
    it->second.second(image_manager_->GetImageResourceFromCache(url), false);
  }
  image_resource_callback_map_.erase(url);
}

bool ImageResourceFetcher::SameImage(const std::string& lhs,
                                     const std::string& rhs) {
  auto trim_lhs = TrimUrl(lhs);
  auto trim_rhs = TrimUrl(rhs);
  return (USE_TRIM_URL_IF_COULD(trim_lhs, lhs)) ==
         (USE_TRIM_URL_IF_COULD(trim_rhs, rhs));
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
