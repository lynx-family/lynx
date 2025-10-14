// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/net_loader_callback.h"
#define FML_USED_ON_EMBEDDER

#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include <utility>

#include "base/include/fml/synchronization/count_down_latch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/fml/file.h"
#include "clay/fml/logging.h"
#include "clay/net/cache/cache_stats.h"
#include "clay/net/cache/net_disk_cache.h"
#include "clay/net/cache/net_resource_cache_key.h"
#include "clay/net/net_loader_manager.h"
#include "clay/net/resource_type.h"
#include "clay/testing/thread_test.h"

namespace clay {

class NetLoaderManagerTest : public clay::testing::ThreadTest {
 public:
  NetLoaderManagerTest() = default;

  void SetUpCache(int64_t max_size, const std::string& cache_path) {
    clay::PersistentCache::SetCacheDirectoryPath(cache_path);
    clay::PersistentCache::ResetCacheForProcess();

    NetLoaderManager::Instance().UnsetCache();
    NetLoaderManager::Instance().EnsureSetup(
        clay::testing::ThreadTest::CreateNewThread("io"), max_size);
  }
  std::chrono::seconds sec_ = std::chrono::seconds(1);

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(NetLoaderManagerTest);
};

TEST_F(NetLoaderManagerTest, Load) {
  std::vector<std::string> url_set = {
      "https://via.placeholder.com/200.png",
      "https://via.placeholder.com/300.png",
      "https://via.placeholder.com/400.png",
  };
  fml::CountDownLatch latch(url_set.size());
  std::set<int> pending;
  for (auto src : url_set) {
    NetLoaderCallback callback;
    callback.set_failed_func(
        [&latch](size_t request_seq, const std::string& reason) {
          latch.CountDown();
          EXPECT_TRUE(false);
        });
    callback.set_succeeded_func(
        [&latch](size_t request_seq, RawResource&& raw_resource) {
          latch.CountDown();
          EXPECT_NE(raw_resource.length, 0u);
        });
    int seq = NetLoaderManager::Instance().Request(src, std::move(callback));
    pending.insert(seq);
  }
  latch.Wait();
}

TEST_F(NetLoaderManagerTest, LoadSync) {
  std::vector<std::string> url_set = {
      "https://via.placeholder.com/200.png",

      "https://via.placeholder.com/300.png",
  };

  for (auto src : url_set) {
    RawResource resource = NetLoaderManager::Instance().RequestSync(src);
    EXPECT_NE(resource.length, 0u);
  }
}

TEST_F(NetLoaderManagerTest, Cancel) {
  std::vector<std::string> url_set = {
      "https://via.placeholder.com/200.png",

      "https://via.placeholder.com/300.png",
  };

  std::set<int> pending;
  for (auto src : url_set) {
    NetLoaderCallback callback;
    callback.set_failed_func(
        [](size_t request_seq, const std::string& reason) {});
    callback.set_succeeded_func(
        [](size_t request_seq, RawResource&& raw_resource) {});
    int seq = NetLoaderManager::Instance().Request(src, std::move(callback));
    pending.insert(seq);

    for (int seq : pending) {
      NetLoaderManager::Instance().CancelBySeq(seq);
    }
  }
}

TEST_F(NetLoaderManagerTest, Cache) {
  fml::ScopedTemporaryDirectory dir;
  SetUpCache(10 * 1024 * 1024, dir.path());

  std::vector<std::string> url_set = {
      "https://via.placeholder.com/200.png",
      "https://via.placeholder.com/300.png",
      "https://via.placeholder.com/400.png",
  };

  fml::CountDownLatch latch(url_set.size());

  std::atomic<int64_t> total_received_length = 0;

  std::this_thread::sleep_for(sec_);
  NetDiskCache* disk_cache = NetLoaderManager::Instance().disk_cache_.get();
  // Ensure cache_stats_ successfully initialized.
  while (!disk_cache->Available()) {
    std::this_thread::sleep_for(sec_);
  }
  for (auto src : url_set) {
    NetLoaderCallback callback;
    callback.set_failed_func(
        [&latch](size_t request_seq, const std::string& reason) {
          latch.CountDown();
          EXPECT_TRUE(false);
        });
    callback.set_succeeded_func(
        [&total_received_length, &latch](size_t request_seq,
                                         RawResource&& raw_resource) {
          latch.CountDown();

          total_received_length += raw_resource.length;
          EXPECT_NE(raw_resource.length, 0u);
        });
    NetLoaderManager::Instance().Request(src, std::move(callback));
  }
  latch.Wait();

  std::this_thread::sleep_for(sec_ * 3);
  const NetLoaderManager& loader = NetLoaderManager::Instance();
  EXPECT_NE(loader.disk_cache_->cache_stats_, nullptr);
  EXPECT_EQ(loader.disk_cache_->cache_stats_->GetCount(),
            static_cast<size_t>(3));
  EXPECT_GE(loader.disk_cache_->cache_stats_->GetSize(), total_received_length);

  // Check whether cache hit
  fml::CountDownLatch latch2(url_set.size());
  for (auto src : url_set) {
    NetLoaderCallback callback;
    callback.set_failed_func(
        [&latch2](size_t request_seq, const std::string& reason) {
          latch2.CountDown();
          EXPECT_TRUE(false);
        });
    callback.set_succeeded_func(
        [&latch2](size_t request_seq, RawResource&& raw_resource) {
          latch2.CountDown();

          EXPECT_NE(raw_resource.length, 0u);
        });
    NetLoaderManager::Instance().Request(src, std::move(callback));
  }
  latch2.Wait();

  EXPECT_EQ(loader.disk_cache_->cache_stats_->GetCount(),
            static_cast<size_t>(3));
  EXPECT_GE(loader.disk_cache_->cache_stats_->GetSize(), total_received_length);

  NetLoaderManager::Instance().UnsetCache();
}

TEST_F(NetLoaderManagerTest, TrimCacheWhileAccess) {
  fml::ScopedTemporaryDirectory dir;
  int64_t cache_size = 110;  // Less than sum of first two resources.
  SetUpCache(cache_size, dir.path());

  auto make_resource = [](size_t length) -> RawResource {
    return {length, std::shared_ptr<uint8_t>(new uint8_t[length],
                                             std::default_delete<uint8_t[]>())};
  };
  std::this_thread::sleep_for(sec_);
  NetDiskCache* disk_cache = NetLoaderManager::Instance().disk_cache_.get();
  // Ensure cache_stats_ successfully initialized.
  while (!disk_cache->Available()) {
    std::this_thread::sleep_for(sec_);
  }
  std::string uri1 = "https://via.placeholder.com/200.png";

  std::string uri2 = "https://via.placeholder.com/300.png";

  std::string uri3 = "https://www.example.com";

  NetResourceCacheKey key1 = NetResourceCacheKey(uri1);
  NetResourceCacheKey key2 = NetResourceCacheKey(uri2);

  NetResourceCacheKey tmp_key1 = NetResourceCacheKey(uri1);
  NetResourceCacheKey tmp_key2 = NetResourceCacheKey(uri2);
  NetResourceCacheKey tmp_key3 = NetResourceCacheKey(uri3);

  EXPECT_TRUE(key1.Valid());
  EXPECT_TRUE(key2.Valid());
  EXPECT_TRUE(tmp_key1.Valid());
  EXPECT_TRUE(tmp_key2.Valid());
  EXPECT_TRUE(tmp_key3.Valid());

  // Create 2 entries.
  disk_cache->WriteContent(tmp_key1.ResourceId(), make_resource(100));
  disk_cache->WriteContent(tmp_key2.ResourceId(), make_resource(20));

  // Wait for write done.
  std::this_thread::sleep_for(sec_);
  // Lock the entry 1.
  EXPECT_TRUE(disk_cache->ContainsKey(&key1));

  // Insert third entry. Entry 1 has been locked, Entry 2 should be delete.
  disk_cache->WriteContent(tmp_key3.ResourceId(), make_resource(30));

  // Wait for write done.
  std::this_thread::sleep_for(sec_);

  EXPECT_FALSE(disk_cache->ContainsKey(&key2));
  EXPECT_EQ(disk_cache->cache_stats_->GetSize(), 130);
  EXPECT_EQ(disk_cache->cache_stats_->GetCount(), static_cast<size_t>(2));

  NetLoaderManager::Instance().UnsetCache();
}

}  // namespace clay
