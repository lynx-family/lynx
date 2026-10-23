// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/resource_loader_embedder.h"

#include <future>
#include <utility>

#include "base/include/fml/make_copyable.h"
#include "clay/fml/mapping.h"
#include "clay/fml/paths.h"
#include "clay/net/url/url_helper.h"
#include "clay/ui/common/isolate.h"

namespace {

static lynx_resource_type_e ConvertToLynxResourceType(
    clay::ResourceType resource_type) {
  switch (resource_type) {
    case clay::ResourceType::kImage:
      return kLynxResourceTypeImage;
    case clay::ResourceType::kFont:
      return kLynxResourceTypeFont;
    case clay::ResourceType::kLottie:
      return kLynxResourceTypeLottie;
    case clay::ResourceType::kVideo:
      return kLynxResourceTypeVideo;
    case clay::ResourceType::kSvg:
      return kLynxResourceTypeSVG;
    case clay::ResourceType::kTemplate:
      return kLynxResourceTypeTemplate;
    case clay::ResourceType::kLynxCoreJs:
      return kLynxResourceTypeLynxCoreJS;
    case clay::ResourceType::kDynamicComponent:
      return kLynxResourceTypeLazyBundle;
    case clay::ResourceType::kI18nText:
      return kLynxResourceTypeI18NText;
    case clay::ResourceType::kExternalJs:
      return kLynxResourceTypeExternalJSSource;
    default:
      return kLynxResourceTypeGeneric;
  }
}

}  // namespace

namespace clay {

ResourceLoaderEmbedder::ResourceLoaderEmbedder(
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    std::shared_ptr<lynx::embedder::ResourceProvider> provider)
    : ui_task_runner_(task_runner),
      intercept_(intercept),
      provider_(std::move(provider)) {}

void ResourceLoaderEmbedder::Load(
    const std::string& src,
    const std::function<void(const uint8_t*, size_t)>& callback,
    const ResourceType resource_type, bool need_redirect) {
  std::string url = src;
  if (need_redirect && intercept_) {
    url = intercept_->ShouldInterceptUrl(src, false);
  }
  url::UriSchemeType scheme_type = url::ParseUriScheme(url);
  if (scheme_type == url::UriSchemeType::kLocalFile) {
    fml::TaskRunner::RunNowOrPostTask(
        ui_task_runner_,
        [url = std::move(url), callback = std::move(callback)]() {
          std::string file_path =
              fml::paths::AbsolutePath(fml::paths::FromURI(url));
          auto mapping = fml::FileMapping::CreateReadOnly(file_path);
          if (mapping && mapping->IsValid()) {
            callback(mapping->GetMapping(), mapping->GetSize());
          } else {
            callback(nullptr, 0);
          }
        });
  } else {
    LoadByNet(url, callback, resource_type);
  }
}

void ResourceLoaderEmbedder::LoadByNet(
    const std::string& url,
    const std::function<void(const uint8_t*, size_t)>& callback,
    const ResourceType resource_type) {
  auto provider = provider_.lock();
  if (!provider) {
    if (callback) callback(nullptr, 0);
    return;
  }
  provider->Load(
      url, ConvertToLynxResourceType(resource_type),
      [callback, runner = ui_task_runner_](lynx::embedder::ResourceData data) {
        runner->PostTask([callback, data = std::move(data)]() {
          if (callback) callback(data.bytes.get(), data.size);
        });
      });
}

RawResource ResourceLoaderEmbedder::LoadSyncByNet(
    const std::string& url, const ResourceType resource_type) {
  std::promise<RawResource> promise;
  std::future<RawResource> future = promise.get_future();
  auto provider = provider_.lock();
  if (!provider) return RawResource::MakeWithCopy(nullptr, 0);
  provider->Load(url, ConvertToLynxResourceType(resource_type),
                 [&promise](lynx::embedder::ResourceData data) {
                   promise.set_value(
                       RawResource::MakeWithCopy(data.bytes.get(), data.size));
                 });
  return future.get();
}

RawResource ResourceLoaderEmbedder::LoadSync(const std::string& src,
                                             const ResourceType resource_type,
                                             bool need_redirect) {
  std::string url = src;
  if (need_redirect && intercept_) {
    url = intercept_->ShouldInterceptUrl(src, false);
  }
  if (url::ParseUriScheme(url) == url::UriSchemeType::kNet) {
    return LoadSyncByNet(url, resource_type);
  }

  std::promise<RawResource> promise;
  std::future<RawResource> future = promise.get_future();
  Load(
      url,
      [&promise](const uint8_t* data, size_t size) {
        promise.set_value(RawResource::MakeWithCopy(data, size));
      },
      resource_type, false);
  return future.get();
}

}  // namespace clay
