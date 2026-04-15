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
    std::shared_ptr<lynx::embedder::LynxResourceFetcherHolder> fetcher_holder)
    : ui_task_runner_(task_runner),
      intercept_(intercept),
      fetcher_holder_(fetcher_holder) {}

void ResourceLoaderEmbedder::Load(
    const std::string& src,
    const std::function<void(const uint8_t*, size_t)>& callback,
    const ResourceType resource_type, bool need_redirect) {
  std::string url = src;
  if (need_redirect && intercept_) {
    url = intercept_->ShouldInterceptUrl(src, false);
  }
  url::UriSchemeType scheme_type = url::ParseUriScheme(url);
  if (scheme_type == url::UriSchemeType::kNet) {
    LoadByNet(url, callback, resource_type);
  } else if (scheme_type == url::UriSchemeType::kLocalFile) {
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
    fml::TaskRunner::RunNowOrPostTask(
        ui_task_runner_,
        [callback = std::move(callback)]() { callback(nullptr, 0); });
  }
}

void ResourceLoaderEmbedder::LoadByNet(
    const std::string& url,
    const std::function<void(const uint8_t*, size_t)>& callback,
    const ResourceType resource_type) {
  lynx_resource_request_t* fetcher_request =
      lynx_resource_request_create_internal(
          url, ConvertToLynxResourceType(resource_type));
  lynx_resource_response_t* fetcher_response =
      lynx_resource_response_create_internal(fml::MakeCopyable(
          [callback = std::move(callback), ui_task_runner = ui_task_runner_](
              lynx_resource_response_t* fetcher_response) mutable {
            // Swap the response to make sure the response is valid when
            // callback is called.
            lynx_resource_response_t* response =
                lynx_resource_response_create_swap(fetcher_response);
            ui_task_runner->PostTask([callback, response]() mutable {
              if (callback) {
                callback(response->data.content, response->data.length);
              }
              lynx_resource_response_release(response);
            });
          }));
  lynx_generic_resource_fetcher_fetch_resource(
      fetcher_holder_->GenericFetcher(), fetcher_request, fetcher_response);
}

RawResource ResourceLoaderEmbedder::LoadSyncByNet(
    const std::string& url, const ResourceType resource_type) {
  std::promise<RawResource> promise;
  std::future<RawResource> future = promise.get_future();
  lynx_resource_request_t* fetcher_request =
      lynx_resource_request_create_internal(
          url, ConvertToLynxResourceType(resource_type));
  lynx_resource_response_t* fetcher_response =
      lynx_resource_response_create_internal(fml::MakeCopyable(
          [promise = std::move(promise)](
              lynx_resource_response_t* fetcher_response) mutable {
            lynx_resource_response_t* response =
                lynx_resource_response_create_swap(fetcher_response);
            promise.set_value(RawResource::MakeWithCopy(response->data.content,
                                                        response->data.length));
            lynx_resource_response_release(response);
          }));
  lynx_generic_resource_fetcher_fetch_resource(
      fetcher_holder_->GenericFetcher(), fetcher_request, fetcher_response);
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
