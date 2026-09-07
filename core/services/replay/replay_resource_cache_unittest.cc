// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/replay_resource_cache.h"

#include <memory>
#include <string>
#include <vector>

#include "core/public/lynx_resource_loader.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace replay {

class ReplayResourceCacheTest : public ::testing::Test {
 protected:
  ReplayResourceCache cache_;
};

class TestResourceLoader : public pub::LynxResourceLoader {
 public:
  int load_resource_count() const { return load_resource_count_; }

 protected:
  void LoadResourceInternal(
      const pub::LynxResourceRequest&,
      base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback)
      override {
    ++load_resource_count_;
    pub::LynxResourceResponse response;
    response.data = {'p'};
    callback(response);
  }

 private:
  int load_resource_count_{0};
};

TEST_F(ReplayResourceCacheTest, ResolveExternalScriptExactMatch) {
  std::vector<uint8_t> data = {'a', 'b', 'c'};
  cache_.AddExternalScript("http://example.com/app.js", data);

  pub::LynxResourceRequest request;
  request.url = "http://example.com/app.js";
  request.type = pub::LynxResourceType::kExternalJs;

  pub::LynxResourceResponse response;
  EXPECT_TRUE(cache_.ResolveResource(request, &response));
  EXPECT_EQ(response.data, data);
}

TEST_F(ReplayResourceCacheTest, ResolveViaPathFallback) {
  std::vector<uint8_t> data = {7, 8, 9};
  cache_.AddExternalScript("http://cdn.example.com/path/app.js", data);

  pub::LynxResourceRequest request;
  request.url = "/path/app.js";
  request.type = pub::LynxResourceType::kExternalJs;

  pub::LynxResourceResponse response;
  EXPECT_TRUE(cache_.ResolveResource(request, &response));
  EXPECT_EQ(response.data, data);
}

TEST_F(ReplayResourceCacheTest, PathAliasDoesNotOverwriteExactEntry) {
  std::vector<uint8_t> exact_data = {'r'};
  std::vector<uint8_t> alias_data = {'a'};
  cache_.AddExternalScript("/path/app.js", exact_data);
  cache_.AddExternalScript("https://cdn.example.com/path/app.js", alias_data);

  pub::LynxResourceRequest request;
  request.url = "/path/app.js";
  request.type = pub::LynxResourceType::kExternalJs;

  pub::LynxResourceResponse response;
  EXPECT_TRUE(cache_.ResolveResource(request, &response));
  EXPECT_EQ(response.data, exact_data);
}

TEST_F(ReplayResourceCacheTest, ClearResetsState) {
  cache_.AddExternalScript("url", {'a'});
  EXPECT_FALSE(cache_.IsEmpty());

  cache_.Clear();
  EXPECT_TRUE(cache_.IsEmpty());
}

#if ENABLE_TESTBENCH_REPLAY
TEST_F(ReplayResourceCacheTest, ResourceLoaderOnlyUsesExplicitReplayCache) {
  auto replay_cache = std::make_shared<ReplayResourceCache>();
  replay_cache->AddExternalScript("https://example.com/app.js", {'r'});

  pub::LynxResourceRequest request;
  request.url = "https://example.com/app.js";
  request.type = pub::LynxResourceType::kExternalJs;

  TestResourceLoader replay_loader;
  replay_loader.SetReplayResourceCache(replay_cache);
  pub::LynxResourceResponse replay_response;
  replay_loader.LoadResource(
      request, [&replay_response](pub::LynxResourceResponse& response) {
        replay_response = response;
      });
  EXPECT_EQ(replay_response.data, std::vector<uint8_t>({'r'}));
  EXPECT_EQ(replay_loader.load_resource_count(), 0);

  TestResourceLoader normal_loader;
  pub::LynxResourceResponse normal_response;
  normal_loader.LoadResource(
      request, [&normal_response](pub::LynxResourceResponse& response) {
        normal_response = response;
      });
  EXPECT_EQ(normal_response.data, std::vector<uint8_t>({'p'}));
  EXPECT_EQ(normal_loader.load_resource_count(), 1);
}
#endif  // ENABLE_TESTBENCH_REPLAY

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
