// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/public/capi/lynx_resource_response_capi.h"
#include "platform/embedder/resource/resource_provider_embedder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx::embedder {
TEST(ResourceProviderContract, AsyncBufferAndFetcherOwnership) {
  struct Pending {
    lynx_resource_request_t* request = nullptr;
    lynx_resource_response_t* response = nullptr;
    int releases = 0;
    int buffers_released = 0;
  } pending;
  auto* fetcher = lynx_generic_resource_fetcher_create_with_finalizer(
      &pending,
      [](auto*, void* opaque) { ++static_cast<Pending*>(opaque)->releases; });
  lynx_generic_resource_fetcher_bind_fetch_resource(
      fetcher, [](auto* fetcher, auto* request, auto* response) {
        auto* p = static_cast<Pending*>(
            lynx_generic_resource_fetcher_get_user_data(fetcher));
        p->request = request;
        p->response = response;
      });
  auto provider = CreateResourceProvider(fetcher);
  lynx_generic_resource_fetcher_release(fetcher);
  ResourceData result;
  provider->Load("https://example.invalid/image", kLynxResourceTypeImage,
                 [&](ResourceData data) { result = std::move(data); });
  provider.reset();
  EXPECT_EQ(pending.releases, 0);
  uint8_t bytes[] = {1, 2, 3};
  lynx_resource_response_set_data(
      pending.response, bytes, sizeof(bytes),
      [](uint8_t*, size_t, void* opaque) {
        ++static_cast<Pending*>(opaque)->buffers_released;
      },
      &pending);
  lynx_resource_response_callback(pending.response);
  lynx_resource_response_release(pending.response);
  lynx_resource_request_release(pending.request);
  ASSERT_EQ(result.size, 3u);
  EXPECT_EQ(result.bytes.get(), bytes);
  EXPECT_EQ(pending.releases, 1);
  EXPECT_EQ(pending.buffers_released, 0);
  result = {};
  EXPECT_EQ(pending.buffers_released, 1);
}
TEST(ResourceProviderContract, UnboundFetcherCompletesWithEmptyData) {
  auto* fetcher = lynx_generic_resource_fetcher_create(nullptr);
  auto provider = CreateResourceProvider(fetcher);
  lynx_generic_resource_fetcher_release(fetcher);
  int callbacks = 0;
  provider->Load("https://example.invalid/error", kLynxResourceTypeGeneric,
                 [&](ResourceData data) {
                   ++callbacks;
                   EXPECT_EQ(data.size, 0u);
                   EXPECT_EQ(data.bytes.get(), nullptr);
                 });
  EXPECT_EQ(callbacks, 1);
  EXPECT_EQ(provider->Intercept("https://example.invalid/original", false),
            "https://example.invalid/original");
}
}  // namespace lynx::embedder
