// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/fetcher/lynx_generic_resource_fetcher_priv.h"
#include "platform/embedder/fetcher/lynx_resource_fetcher_holder.h"
#include "platform/embedder/fetcher/lynx_resource_response_priv.h"
#include "platform/embedder/public/capi/lynx_resource_request_capi.h"
#include "platform/embedder/public/lynx_generic_resource_fetcher.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxGenericResourceFetcher, Create) {
  lynx_generic_resource_fetcher_t* fetcher =
      lynx_generic_resource_fetcher_create(nullptr);
  EXPECT_TRUE(fetcher != nullptr);
  lynx_generic_resource_fetcher_release(fetcher);
}

TEST(LynxGenericResourceFetcher, UnboundFetchCompletesWithErrorOnce) {
  auto* fetcher = lynx_generic_resource_fetcher_create(nullptr);
  for (bool path : {false, true}) {
    SCOPED_TRACE(path);
    int callbacks = 0;
    auto* response = lynx_resource_response_create_internal([&](auto* result) {
      ++callbacks;
      EXPECT_EQ(result->code, -1);
      EXPECT_EQ(result->error_message,
                path ? "fetch_resource_path is unimplemented"
                     : "fetch_resource is unimplemented");
    });
    auto dispatch = path ? lynx_generic_resource_fetcher_fetch_resource_path
                         : lynx_generic_resource_fetcher_fetch_resource;
    dispatch(fetcher,
             lynx_resource_request_create("resource", kLynxResourceTypeGeneric),
             response);
    EXPECT_EQ(callbacks, 1);
  }
  lynx_generic_resource_fetcher_release(fetcher);
}

TEST(LynxGenericResourceFetcher, FetchCanCompleteAfterDispatchReturns) {
  struct Pending {
    lynx_resource_request_t* request = nullptr;
    lynx_resource_response_t* response = nullptr;
  } pending;
  auto* fetcher = lynx_generic_resource_fetcher_create(&pending);
  lynx_generic_resource_fetcher_bind_fetch_resource(
      fetcher, [](auto* fetcher, auto* request, auto* response) {
        auto* pending = static_cast<Pending*>(
            lynx_generic_resource_fetcher_get_user_data(fetcher));
        pending->request = request;
        pending->response = response;
      });
  int callbacks = 0;
  auto* response = lynx_resource_response_create_internal([&](auto* result) {
    ++callbacks;
    EXPECT_EQ(result->code, 204);
  });
  auto* request =
      lynx_resource_request_create("resource", kLynxResourceTypeGeneric);
  lynx_generic_resource_fetcher_fetch_resource(fetcher, request, response);
  EXPECT_EQ(callbacks, 0);
  EXPECT_EQ(pending.request, request);
  ASSERT_EQ(pending.response, response);
  lynx_resource_response_set_code(pending.response, 204);
  lynx_resource_response_callback(pending.response);
  lynx_resource_response_release(pending.response);
  lynx_resource_request_release(pending.request);
  EXPECT_EQ(callbacks, 1);
  lynx_generic_resource_fetcher_release(fetcher);
}

TEST(LynxGenericResourceFetcher, HoldersRetainFetcherUntilLastRelease) {
  int finalizers = 0;
  auto* fetcher = lynx_generic_resource_fetcher_create_with_finalizer(
      &finalizers, [](auto*, void* data) { ++*static_cast<int*>(data); });
  {
    lynx::embedder::LynxResourceFetcherHolder first(fetcher);
    {
      lynx::embedder::LynxResourceFetcherHolder second(fetcher);
      lynx_generic_resource_fetcher_release(fetcher);
      EXPECT_EQ(first.GenericFetcher(), fetcher);
      EXPECT_EQ(second.GenericFetcher(), fetcher);
      EXPECT_EQ(finalizers, 0);
    }
    EXPECT_EQ(finalizers, 0);
  }
  EXPECT_EQ(finalizers, 1);
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
  int callback_count = 0;
  lynx_resource_response_t* response = lynx_resource_response_create_internal(
      [&](lynx_resource_response_t* response) {
        ++callback_count;
        EXPECT_EQ(response->code, 0);
        EXPECT_EQ(response->data.length, 3);

        uint8_t test_base[] = {1, 2, 3};
        ASSERT_EQ(memcmp(test_base, response->data.content, sizeof(test_base)),
                  0);
      });
  EXPECT_TRUE(response != nullptr);

  lynx_generic_resource_fetcher_fetch_resource(fetcher, request, response);
  EXPECT_EQ(callback_count, 1);
  lynx_generic_resource_fetcher_release(fetcher);
}
