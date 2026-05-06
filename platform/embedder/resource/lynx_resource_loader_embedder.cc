// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/resource/lynx_resource_loader_embedder.h"

#include <vector>

#include "base/include/fml/make_copyable.h"

namespace {
static lynx_resource_type_e ConvertResourceType(
    lynx::pub::LynxResourceType resource_type) {
  switch (resource_type) {
    case lynx::pub::LynxResourceType::kImage:
      return kLynxResourceTypeImage;
    case lynx::pub::LynxResourceType::kFont:
      return kLynxResourceTypeFont;
    case lynx::pub::LynxResourceType::kLottie:
      return kLynxResourceTypeLottie;
    case lynx::pub::LynxResourceType::kVideo:
      return kLynxResourceTypeVideo;
    case lynx::pub::LynxResourceType::kSvg:
      return kLynxResourceTypeSVG;
    case lynx::pub::LynxResourceType::kTemplate:
      return kLynxResourceTypeTemplate;
    case lynx::pub::LynxResourceType::kLynxCoreJs:
      return kLynxResourceTypeLynxCoreJS;
    case lynx::pub::LynxResourceType::kLazyBundle:
      return kLynxResourceTypeLazyBundle;
    case lynx::pub::LynxResourceType::kI18nText:
      return kLynxResourceTypeI18NText;
    case lynx::pub::LynxResourceType::kExternalJs:
      return kLynxResourceTypeExternalJSSource;
    case lynx::pub::LynxResourceType::kAssets:
      return kLynxResourceTypeAssets;
    default:
      return kLynxResourceTypeGeneric;
  }
}
}  // namespace

namespace lynx {
namespace embedder {

void LynxResourceLoaderEmbedder::SetResourceFetcherHolder(
    std::shared_ptr<LynxResourceFetcherHolder> holder) {
  resource_fetcher_holder_ = std::move(holder);
}

void LynxResourceLoaderEmbedder::LoadResourceInternal(
    const pub::LynxResourceRequest& request,
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback) {
  if (request.type == pub::LynxResourceType::kAssets) {
    // For lynx_core.js
    auto assets = js_source_loader_.LoadJSSource(request.url);
    if (!assets.empty()) {
      const uint8_t* bytes = reinterpret_cast<const uint8_t*>(assets.data());
      pub::LynxResourceResponse response{
          .data = std::vector<uint8_t>(bytes, bytes + assets.size())};
      callback(response);
      return;
    }
  }
  // adapt to `lynx_generic_resource_fetcher`, see
  // platform/embedder/public/capi/lynx_generic_resource_fetcher_capi.h
  if (!resource_fetcher_holder_ ||
      !resource_fetcher_holder_->GenericFetcher()) {
    pub::LynxResourceResponse response{.err_code = -1,
                                       .err_msg = "unimplemented"};
    callback(response);
    return;
  }
  lynx_resource_request_t* fetcher_request =
      lynx_resource_request_create_internal(request.url,
                                            ConvertResourceType(request.type));
  lynx_resource_response_t* fetcher_response =
      lynx_resource_response_create_internal(fml::MakeCopyable(
          [callback = std::move(callback)](
              lynx_resource_response_t* fetcher_response) mutable {
            pub::LynxResourceResponse response;
            if (fetcher_response->data.length > 0) {
              response.data =
                  std::vector<uint8_t>(fetcher_response->data.content,
                                       fetcher_response->data.content +
                                           fetcher_response->data.length);
            }
            callback(response);
          }));
  lynx_generic_resource_fetcher_fetch_resource(
      resource_fetcher_holder_->GenericFetcher(), fetcher_request,
      fetcher_response);
}

}  // namespace embedder
}  // namespace lynx
