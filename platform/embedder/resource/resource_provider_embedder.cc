// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/resource/resource_provider_embedder.h"

#include <memory>
#include <string>
#include <utility>

#include "platform/embedder/fetcher/lynx_resource_fetcher_holder.h"
#include "platform/embedder/fetcher/lynx_resource_request_priv.h"
#include "platform/embedder/fetcher/lynx_resource_response_priv.h"
#include "platform/embedder/public/capi/lynx_memory_capi.h"

namespace lynx::embedder {
namespace {
class ResourceProviderEmbedder final
    : public ResourceProvider,
      public std::enable_shared_from_this<ResourceProviderEmbedder> {
 public:
  explicit ResourceProviderEmbedder(lynx_generic_resource_fetcher_t* fetcher)
      : fetcher_(fetcher) {}
  void Load(const std::string& url, lynx_resource_type_e type,
            Callback callback) override {
    auto* request = lynx_resource_request_create_internal(url, type);
    auto* response = lynx_resource_response_create_internal(
        [self = shared_from_this(),
         callback = std::move(callback)](lynx_resource_response_t* response) {
          // Keep the fetcher alive until this request completes.
          (void)self;
          auto owned = std::shared_ptr<lynx_resource_response_t>(
              lynx_resource_response_create_swap(response),
              lynx_resource_response_release);
          callback({std::shared_ptr<const uint8_t>(owned, owned->data.content),
                    owned->data.length});
        });
    lynx_generic_resource_fetcher_fetch_resource(fetcher_.GenericFetcher(),
                                                 request, response);
  }
  std::string Intercept(const std::string& url, bool decode) override {
    char* result = lynx_generic_resource_fetcher_intercept(
        fetcher_.GenericFetcher(), url.c_str(), decode);
    std::string redirected = result ? result : url;
    lynx_free(result);
    return redirected;
  }

 private:
  LynxResourceFetcherHolder fetcher_;
};
}  // namespace
std::shared_ptr<ResourceProvider> CreateResourceProvider(
    lynx_generic_resource_fetcher_t* fetcher) {
  return std::make_shared<ResourceProviderEmbedder>(fetcher);
}
}  // namespace lynx::embedder
