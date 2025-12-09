// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <__config>

#include "platform/embedder/fetcher/lynx_generic_resource_fetcher_priv.h"
#include "platform/embedder/public/lynx_generic_resource_fetcher.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxGenericResourceFetcher, Create) {
  lynx_generic_resource_fetcher_t* fetcher =
      lynx_generic_resource_fetcher_create(nullptr);
  EXPECT_TRUE(fetcher != nullptr);
  lynx_generic_resource_fetcher_release(fetcher);
}

TEST(LynxGenericResourceFetcher, BindFunction) {
  lynx_generic_resource_fetcher_t* fetcher =
      lynx_generic_resource_fetcher_create(nullptr);
  EXPECT_TRUE(fetcher != nullptr);

  fetch_resource_func fetch_resource =
      [](lynx_generic_resource_fetcher_t* fetcher,
         lynx_resource_request_t* request, lynx_resource_response_t* response) {
        lynx_resource_response_release(response);
        lynx_resource_request_release(request);
      };
  lynx_generic_resource_fetcher_bind_fetch_resource(fetcher, fetch_resource);
  EXPECT_EQ(fetcher->fetch_resource, fetch_resource);

  fetch_resource_func fetch_resource_path =
      [](lynx_generic_resource_fetcher_t* fetcher,
         lynx_resource_request_t* request, lynx_resource_response_t* response) {
        lynx_resource_response_release(response);
        lynx_resource_request_release(request);
      };
  lynx_generic_resource_fetcher_bind_fetch_resource_path(fetcher,
                                                         fetch_resource_path);
  EXPECT_EQ(fetcher->fetch_resource_path, fetch_resource_path);

  lynx_generic_resource_fetcher_release(fetcher);
}

TEST(LynxGenericResourceFetcher, FetchResource) {
  lynx_generic_resource_fetcher_t* fetcher =
      lynx_generic_resource_fetcher_create(nullptr);
  EXPECT_TRUE(fetcher != nullptr);

  fetch_resource_func fetch_resource =
      [](lynx_generic_resource_fetcher_t* fetcher,
         lynx_resource_request_t* request, lynx_resource_response_t* response) {
        EXPECT_EQ(lynx_resource_request_get_type(request),
                  kLynxResourceTypeGeneric);
        EXPECT_STREQ(lynx_resource_request_get_url(request),
                     "assets://main.lynx.bundle");
        lynx_resource_response_set_code(response, 0);
        uint8_t data[] = {1, 2, 3};
        lynx_resource_response_set_data(response, data, sizeof(data), nullptr,
                                        nullptr);
        lynx_resource_response_callback(response);
        lynx_resource_response_release(response);
        lynx_resource_request_release(request);
      };
  lynx_generic_resource_fetcher_bind_fetch_resource(fetcher, fetch_resource);
  EXPECT_EQ(fetcher->fetch_resource, fetch_resource);

  lynx_resource_request_t* request = lynx_resource_request_create(
      "assets://main.lynx.bundle", kLynxResourceTypeGeneric);
  EXPECT_TRUE(request != nullptr);
  lynx_resource_response_t* response =
      lynx_resource_response_create([](lynx_resource_response_t* response) {
        EXPECT_EQ(response->code, 0);
        EXPECT_EQ(response->data.length, 3);

        uint8_t test_base[] = {1, 2, 3};
        ASSERT_EQ(memcmp(test_base, response->data.content, sizeof(test_base)),
                  0);
      });
  EXPECT_TRUE(response != nullptr);

  lynx_generic_resource_fetcher_release(fetcher);
}

class LynxGenericResourceFetcherTest
    : public lynx::pub::LynxGenericResourceFetcher {
 public:
  void FetchResource(
      std::shared_ptr<lynx::pub::LynxResourceRequest> request,
      std::shared_ptr<lynx::pub::LynxResourceResponse> response) override {
    EXPECT_EQ(request->GetType(), kLynxResourceTypeGeneric);
    EXPECT_STREQ(request->GetUrl(), "assets://main.lynx.bundle");
    response->SetCode(0);
    uint8_t data[] = {1, 2, 3};
    response->SetData(data, sizeof(data));
    response->Complete();
  }

  void FetchResourcePath(
      std::shared_ptr<lynx::pub::LynxResourceRequest> request,
      std::shared_ptr<lynx::pub::LynxResourceResponse> response) override {
    // UNIMPLEMENTED
  }
};

TEST(LynxGenericResourceFetcherCPP, FetchResource) {
  auto fetcher = std::make_shared<LynxGenericResourceFetcherTest>();
  fetcher->InitIfNeeded();
  lynx_resource_request_t* c_request = lynx_resource_request_create(
      "assets://main.lynx.bundle", kLynxResourceTypeGeneric);
  auto request = std::make_shared<lynx::pub::LynxResourceRequest>(c_request);
  lynx_resource_response_t* c_response =
      lynx_resource_response_create([](lynx_resource_response_t* response) {
        EXPECT_EQ(response->code, 0);
        EXPECT_EQ(response->data.length, 3);

        uint8_t test_base[] = {1, 2, 3};
        ASSERT_EQ(memcmp(test_base, response->data.content, sizeof(test_base)),
                  0);
      });
  auto response = std::make_shared<lynx::pub::LynxResourceResponse>(c_response);
  fetcher->FetchResource(request, response);
}
