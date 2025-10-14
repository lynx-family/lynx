// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_CACHE_NET_DISK_CACHE_H_
#define CLAY_NET_CACHE_NET_DISK_CACHE_H_

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/concurrent_message_loop.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "clay/fml/file.h"
#include "clay/net/cache/cache_stats.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class NetResourceCacheKey;
struct RawResource;

// This class handle disk cache, must access from platform thread.
class NetDiskCache : public fml::RefCountedThreadSafe<NetDiskCache> {
 public:
  struct NetDiskCacheConfig {
    int64_t max_cache_size_;
  };
  NetDiskCache(fml::RefPtr<fml::TaskRunner> io_runner, int64_t max_cache_size);

  // Whether has `cache_key` in disk cache
  bool ContainsKey(NetResourceCacheKey* cache_key);

  // Get cached data, make sure call after `ContainsKey` returns true
  void GetCacheContent(
      NetResourceCacheKey&& cache_key,
      const std::function<void(const std::string&, RawResource)>& callback);

  // Trim cache and write resource into disk.
  void WriteContent(const std::string& resource_id, RawResource resource);

  void ClearCaches();

  fml::RefPtr<fml::TaskRunner> GetIOTaskRunner() { return io_runner_; }
  void WillDestory() { alive_ = false; }

 private:
  FRIEND_TEST(NetLoaderManagerTest, Cache);
  FRIEND_TEST(NetLoaderManagerTest, TrimCacheWhileAccess);

  void CheckMayTrim();
  void TrimToLimit();
  void RemoveItem(const std::string& resource_id);

  // Before cache_stats_ has initialized and after WillDestory() cache is also
  // unavailable
  bool Available() const { return alive_; }

  std::atomic<bool> alive_ = false;

  fml::RefPtr<fml::TaskRunner> io_runner_;
  NetDiskCacheConfig config_;

  std::unique_ptr<CacheStats> cache_stats_;
  std::unique_ptr<fml::UniqueFD> cache_directory_;
};

}  // namespace clay

#endif  // CLAY_NET_CACHE_NET_DISK_CACHE_H_
