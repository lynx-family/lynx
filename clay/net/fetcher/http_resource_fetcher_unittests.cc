// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include <string>
#include <vector>

#include "clay/net/fetcher/http_resource_fetcher.h"
#include "clay/net/fetcher/http_resource_fetcher_factory.h"
#include "clay/net/net_loader_callback.h"
#include "clay/net/net_loader_manager.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(HttpResourceFetcherTest, Load) {
  std::vector<std::pair<std::string, std::string>> url_set = {
      {
          "https://pics.freeicons.io",
          "/uploads/icons/png/"
          "4398893391579605854-512.png",
      },
      {
          "https://secure.gravatar.com",
          "/avatar/"
          "e402686b4d2ec0899358e5cd311fd12f\?s\\=46\\&d\\=identicon",
      },
      {
          "https://4.bp.blogspot.com",
          "/-uhjF2kC3tFc/U_r3myvwzHI/AAAAAAAACiw/tPQ2XOXFYKY/s1600/"
          "Circles-3.gif",
      },
  };

  for (auto host_path : url_set) {
    std::unique_ptr<HttpResourceFetcher> image_fetcher =
        HttpResourceFetcherFactory::CreateFetcher(
            nullptr, url::Uri(host_path.first + host_path.second), 0);
    auto content = image_fetcher->LoadSync();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_NE(content.length, 0u);
  }
}

}  // namespace testing
}  // namespace clay
