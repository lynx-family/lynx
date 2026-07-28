// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_entity_id_generator.h"

#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::shell {
namespace {

TEST(LynxEntityIdGeneratorTest, UsesOneNamespaceAcrossEntityDomains) {
  const auto view_id = GenerateLynxEntityId();
  const auto engine_id = GenerateLynxEntityId();
  const auto runtime_id = GenerateLynxEntityId();

  EXPECT_GE(view_id, 0);
  EXPECT_GE(engine_id, 0);
  EXPECT_GE(runtime_id, 0);
  EXPECT_LT(view_id, engine_id);
  EXPECT_LT(engine_id, runtime_id);
}

TEST(LynxEntityIdGeneratorTest, AllocatesUniqueIdsConcurrently) {
  constexpr int kThreadCount = 8;
  constexpr int kIdsPerThread = 128;
  constexpr int kIdCount = kThreadCount * kIdsPerThread;
  std::mutex ids_mutex;
  std::vector<base::LynxEntityId> ids;
  ids.reserve(kIdCount);
  std::vector<std::thread> threads;

  for (int thread = 0; thread < kThreadCount; ++thread) {
    threads.emplace_back([&]() {
      std::vector<base::LynxEntityId> local_ids;
      local_ids.reserve(kIdsPerThread);
      for (int id = 0; id < kIdsPerThread; ++id) {
        local_ids.push_back(GenerateLynxEntityId());
      }
      std::lock_guard<std::mutex> lock(ids_mutex);
      ids.insert(ids.end(), local_ids.begin(), local_ids.end());
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }

  std::sort(ids.begin(), ids.end());
  ASSERT_EQ(ids.size(), static_cast<size_t>(kIdCount));
  for (size_t id = 1; id < ids.size(); ++id) {
    EXPECT_LT(ids[id - 1], ids[id]);
  }
}

}  // namespace
}  // namespace lynx::shell
