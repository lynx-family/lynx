// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <thread>
#include <unordered_set>
#include <vector>

#include "platform/embedder/fetcher/lynx_resource_request_priv.h"
#include "platform/embedder/public/capi/lynx_resource_request_capi.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxResourceRequest, Create) {
  lynx_resource_request_t* request = lynx_resource_request_create(
      "assets://main.lynx.bundle", kLynxResourceTypeGeneric);
  EXPECT_TRUE(request != nullptr);
  EXPECT_EQ(lynx_resource_request_get_type(request), kLynxResourceTypeGeneric);
  EXPECT_STREQ(lynx_resource_request_get_url(request),
               "assets://main.lynx.bundle");
  lynx_resource_request_release(request);
}

TEST(LynxResourceRequest, StableId) {
  auto* request = lynx_resource_request_create(nullptr, kLynxResourceTypeImage);
  const auto id = lynx_resource_request_get_id(request);
  EXPECT_NE(id, kLynxResourceRequestIdInvalid);
  EXPECT_EQ(lynx_resource_request_get_id(request), id);
  EXPECT_STREQ(lynx_resource_request_get_url(request), "");
  EXPECT_EQ(lynx_resource_request_get_id(nullptr),
            kLynxResourceRequestIdInvalid);
  lynx_resource_request_release(request);
}

TEST(LynxResourceRequest, ReleasedRequestsKeepDistinctIds) {
  std::unordered_set<lynx_resource_request_id> ids;
  constexpr size_t kRequestCount = 10000;
  ids.reserve(kRequestCount);
  for (size_t i = 0; i < kRequestCount; ++i) {
    auto* request = i % 2 == 0
                        ? lynx_resource_request_create("assets://image.png",
                                                       kLynxResourceTypeImage)
                        : lynx_resource_request_create_internal(
                              "assets://image.png", kLynxResourceTypeImage);
    const auto id = lynx_resource_request_get_id(request);
    lynx_resource_request_release(request);
    // Async response records can outlive the request object.
    EXPECT_NE(id, kLynxResourceRequestIdInvalid);
    ASSERT_TRUE(ids.insert(id).second);
  }
}

TEST(LynxResourceRequest, ConcurrentRequestsKeepDistinctIds) {
  constexpr size_t kThreadCount = 8;
  constexpr size_t kRequestsPerThread = 1000;
  std::vector<std::vector<lynx_resource_request_id>> batches(kThreadCount);
  std::vector<std::thread> threads;
  for (size_t t = 0; t < kThreadCount; ++t) {
    threads.emplace_back([&batches, t, kRequestsPerThread]() {
      auto& ids = batches[t];
      ids.reserve(kRequestsPerThread);
      for (size_t i = 0; i < kRequestsPerThread; ++i) {
        auto* request = lynx_resource_request_create("assets://image.png",
                                                     kLynxResourceTypeImage);
        ids.push_back(lynx_resource_request_get_id(request));
        lynx_resource_request_release(request);
      }
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }
  std::vector<lynx_resource_request_id> ids;
  for (const auto& batch : batches) {
    ids.insert(ids.end(), batch.begin(), batch.end());
  }
  ASSERT_EQ(ids.size(), kThreadCount * kRequestsPerThread);
  std::sort(ids.begin(), ids.end());
  EXPECT_NE(ids.front(), kLynxResourceRequestIdInvalid);
  EXPECT_EQ(std::adjacent_find(ids.begin(), ids.end()), ids.end());
}
