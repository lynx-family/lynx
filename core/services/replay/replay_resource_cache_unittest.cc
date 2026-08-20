// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/replay_resource_cache.h"

#include <string>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace replay {

class ReplayResourceCacheTest : public ::testing::Test {
 protected:
  ReplayResourceCache cache_;
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

TEST_F(ReplayResourceCacheTest, ClearResetsState) {
  cache_.AddExternalScript("url", {'a'});
  EXPECT_FALSE(cache_.IsEmpty());

  cache_.Clear();
  EXPECT_TRUE(cache_.IsEmpty());
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
