// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_NET_LOADER_MANAGER_H_
#define CLAY_NET_NET_LOADER_MANAGER_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/concurrent_message_loop.h"
#include "clay/net/fetcher/http_resource_fetcher.h"
#include "clay/net/net_loader_callback.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class CacheStats;
class NetDiskCache;
class NetResourceCacheKey;
struct RawResource;

class NetLoaderManager {
 public:
  static NetLoaderManager& Instance();

  NetLoaderManager();

  ~NetLoaderManager();

  constexpr static size_t kInvalidRequestSeq = 0;

  NetLoaderManager(const NetLoaderManager&) = delete;
  NetLoaderManager& operator=(const NetLoaderManager&) = delete;
  NetLoaderManager(NetLoaderManager&&) = delete;
  NetLoaderManager& operator=(NetLoaderManager&&) = delete;

  using HostNetLoader = std::function<void(
      const std::string& uri, const std::string& method,
      const std::string& body,
      const std::map<std::string, std::string>& headers, size_t request_seq)>;

  size_t Request(const std::string& uri, NetLoaderCallback&& callback);

  size_t Request(const std::string& url, const std::string& method,
                 std::string&& body,
                 std::map<std::string, std::string>&& headers,
                 NetLoaderCallback&& callback);
  // This function send request by given fetcher (for testing).
  size_t RequestWithFetcher(const std::string& uri,
                            NetLoaderCallback&& callback,
                            std::unique_ptr<HttpResourceFetcher> fetcher);
  RawResource RequestSync(const std::string& uri);

  void OnSucceeded(const std::string& uri, size_t request_seq,
                   RawResource&& resource, bool from_disk_cache = false);
  void OnFailed(const std::string& uri, size_t request_seq, int failed_time,
                const std::string& reason);

  // OnFinished is only provide to HttpResourceFetcherHost.
  void OnFinished(size_t request_seq, bool success, const uint8_t* data,
                  size_t length);

  void NotifyLowMemory();

  void Clear();

  void CancelBySeq(size_t seqs);

  void EnsureSetup(fml::RefPtr<fml::TaskRunner> task_runner,
                   int64_t max_cache_size);
  void UnsetCache();

  void SetHostNetLoader(HostNetLoader&& host_net_loader) {
    host_net_loader_ = std::move(host_net_loader);
  }

 private:
  FRIEND_TEST(NetLoaderManagerTest, Cache);
  FRIEND_TEST(NetLoaderManagerTest, TrimCacheWhileAccess);

  struct NetWaiterEntity {
    size_t request_seq;
    NetLoaderCallback callback;
  };
  using NetLoaderWaiters = std::vector<NetWaiterEntity>;

  NetLoaderWaiters TakeWaiters(const std::string& uri);

  fml::RefPtr<NetDiskCache> disk_cache_;
  fml::RefPtr<fml::TaskRunner> task_runner_;

  size_t current_request_seq_ = 0;
  std::unordered_map<std::string, NetLoaderWaiters> waiters_;
  std::unordered_map<size_t, std::unique_ptr<HttpResourceFetcher>>
      running_fetchers_;

  HostNetLoader host_net_loader_ = nullptr;

  std::mutex disk_cache_mutex_;
  std::mutex waiters_mutex_;
};

}  // namespace clay

#endif  // CLAY_NET_NET_LOADER_MANAGER_H_
