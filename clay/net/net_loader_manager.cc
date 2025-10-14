// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/net_loader_manager.h"

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/no_destructor.h"
#include "clay/fml/logging.h"
#include "clay/net/cache/net_disk_cache.h"
#include "clay/net/cache/net_resource_cache_key.h"
#include "clay/net/fetcher/http_resource_fetcher_factory.h"
#include "clay/net/fetcher/http_resource_fetcher_host.h"
#include "clay/net/macros.h"
#include "clay/net/url/url_parse.h"

namespace clay {

// static
NetLoaderManager& NetLoaderManager::Instance() {
  static fml::NoDestructor<NetLoaderManager> instance;
  return *(instance.get());
}

NetLoaderManager::NetLoaderManager() {}

void NetLoaderManager::EnsureSetup(fml::RefPtr<fml::TaskRunner> task_runner,
                                   int64_t max_cache_size) {
  if (task_runner_) {
    return;
  }
  task_runner_ = task_runner;
  FML_CHECK(task_runner_);

  {
    std::scoped_lock lock(disk_cache_mutex_);
    if (!disk_cache_) {
      disk_cache_ =
          fml::MakeRefCounted<NetDiskCache>(task_runner_, max_cache_size);
      FML_LOG(WARNING) << "NetLoaderManager::SetupCache!";
    }
  }
}

NetLoaderManager::~NetLoaderManager() = default;

void NetLoaderManager::UnsetCache() {
  std::scoped_lock lock(disk_cache_mutex_);
  if (!disk_cache_) {
    FML_DLOG(WARNING)
        << "Call NetLoaderManager::UnsetCache without disk_cache_!";
    return;
  }
  disk_cache_->WillDestory();
  auto task_runner = disk_cache_->GetIOTaskRunner();
  auto destroy_task =
      fml::MakeCopyable([disk_cache = std::move(disk_cache_)]() mutable {});
  task_runner->PostTask([destroy_task]() { destroy_task(); });
}

size_t NetLoaderManager::Request(const std::string& url,
                                 NetLoaderCallback&& callback) {
  return Request(url, "", "", {}, std::move(callback));
}

size_t NetLoaderManager::Request(const std::string& uri,
                                 const std::string& method, std::string&& body,
                                 std::map<std::string, std::string>&& headers,
                                 NetLoaderCallback&& callback) {
  size_t request_seq = kInvalidRequestSeq;
  {
    std::scoped_lock lock(waiters_mutex_);
    ++current_request_seq_;
    if (current_request_seq_ == kInvalidRequestSeq) {
      ++current_request_seq_;
    }
    request_seq = current_request_seq_;
    callback.SetRequestSeq(request_seq);

    auto iter = waiters_.find(uri);
    if (iter != waiters_.end()) {
      NET_LOG << " add to pending requests " << uri << " seq "
              << current_request_seq_;
      iter->second.push_back({request_seq, callback});
      return request_seq;
    }

    NET_LOG << " insert a new request " << uri << " seq "
            << current_request_seq_;
    waiters_.insert({uri, {{request_seq, callback}}});
  }

  // 1. Make a cache key by uri
  // 2. Check if cache key hit => get content by cache key
  // 3. Else send request, update cache after request finish
  NetResourceCacheKey cache_key(uri);
  if (!cache_key.Valid()) {
    callback.OnFailed("invalid url");
    return kInvalidRequestSeq;
  }

  {
    std::unique_lock lock(disk_cache_mutex_);
    if (disk_cache_ && disk_cache_->ContainsKey(&cache_key)) {
      disk_cache_->GetCacheContent(
          std::move(cache_key),
          std::bind(&NetLoaderManager::OnSucceeded, this, std::placeholders::_1,
                    request_seq, std::placeholders::_2, true));
      return request_seq;
    }
  }

  // Fetch resource
  auto fetcher = HttpResourceFetcherFactory::CreateFetcher(
      host_net_loader_, cache_key.Uri(), request_seq);
  if (!fetcher) {
    OnFailed(uri, request_seq, HttpResourceFetcher::kMaxRetryTime,
             "No available fetcher.");
    return kInvalidRequestSeq;
  }
  fetcher->SetMethod(method);
  fetcher->SetHeaders(std::move(headers));
  fetcher->SetBody(std::move(body));
  fetcher->Load();
  {
    std::scoped_lock lock(waiters_mutex_);
    running_fetchers_.emplace(request_seq, std::move(fetcher));
  }

  return request_seq;
}

size_t NetLoaderManager::RequestWithFetcher(
    const std::string& uri, NetLoaderCallback&& callback,
    std::unique_ptr<HttpResourceFetcher> fetcher) {
  size_t request_seq = kInvalidRequestSeq;
  {
    std::scoped_lock lock(waiters_mutex_);
    ++current_request_seq_;
    if (current_request_seq_ == kInvalidRequestSeq) {
      ++current_request_seq_;
    }
    request_seq = current_request_seq_;
    callback.SetRequestSeq(request_seq);

    auto iter = waiters_.find(uri);
    if (iter == waiters_.end()) {
      waiters_.insert({uri, {{request_seq, callback}}});
    } else {
      return kInvalidRequestSeq;
    }
  }

  fetcher->Load();
  {
    std::scoped_lock lock(waiters_mutex_);
    running_fetchers_.emplace(request_seq, std::move(fetcher));
  }
  return request_seq;
}

// Synchronous request won't use cache and waiting queue.
RawResource NetLoaderManager::RequestSync(const std::string& uri) {
  url::Uri parsed_uri(uri);
  if (!parsed_uri.Valid()) {
    return {0, nullptr};
  }

  size_t request_sep = kInvalidRequestSeq;
  {
    std::scoped_lock lock(waiters_mutex_);
    ++current_request_seq_;
    if (current_request_seq_ == kInvalidRequestSeq) {
      ++current_request_seq_;
    }
    request_sep = current_request_seq_;
  }

  RawResource resource;
  std::unique_ptr<HttpResourceFetcher> fetcher =
      HttpResourceFetcherFactory::CreateFetcher(host_net_loader_, parsed_uri,
                                                request_sep);
  resource = fetcher->LoadSync();
  NET_LOG << " request finish " << parsed_uri.Spec()
          << "\tlength:" << resource.length;
  return resource;
}

void NetLoaderManager::NotifyLowMemory() {
  FML_CHECK(task_runner_->RunsTasksOnCurrentThread());
  std::unique_lock lock(disk_cache_mutex_);
  if (disk_cache_) {
    disk_cache_->ClearCaches();
  }
}

void NetLoaderManager::Clear() {
  std::scoped_lock lock(waiters_mutex_);
  waiters_.clear();
}

void NetLoaderManager::CancelBySeq(size_t seq) {
  // Iterate all pending entry.
  std::scoped_lock lock(waiters_mutex_);

  auto runner_iter = running_fetchers_.find(seq);
  if (runner_iter != running_fetchers_.end()) {
    // Cancel should trigger OnFailed callback
    runner_iter->second->Cancel();
  }

  auto it = waiters_.begin();

  while (it != waiters_.end()) {
    NetLoaderWaiters& waiter = it->second;
    size_t old_size = waiter.size();
    waiter.erase(std::remove_if(waiter.begin(), waiter.end(),
                                [seq](const NetWaiterEntity& entity) {
                                  return entity.request_seq == seq;
                                }),
                 waiter.end());
    if (waiter.size() != old_size) {
      if (waiter.empty()) {
        waiters_.erase(it);
      }
      break;
    } else {
      ++it;
    }
  }
}

NetLoaderManager::NetLoaderWaiters NetLoaderManager::TakeWaiters(
    const std::string& uri) {
  NetLoaderWaiters uri_waiters;

  {
    std::scoped_lock lock(waiters_mutex_);

    auto iter = waiters_.find(uri);
    if (iter != waiters_.end()) {
      uri_waiters = std::move(iter->second);
      waiters_.erase(iter);
    }
  }
  return uri_waiters;
}

void NetLoaderManager::OnSucceeded(const std::string& uri, size_t request_seq,
                                   RawResource&& resource,
                                   bool from_disk_cache) {
  NetLoaderWaiters uri_waiters = TakeWaiters(uri);
  for (auto& waiter : uri_waiters) {
    waiter.callback.OnSucceeded(resource);
  }

// TODO(zuojinglong.9) Now open on Harmony platform, gradually expanding to
// other platforms.
#if defined(OS_HARMONY)
  if (!from_disk_cache) {
    std::scoped_lock lock(disk_cache_mutex_);
    if (disk_cache_) {
      // Write cache if not exist.
      NetResourceCacheKey cache_key(uri);

      fml::TaskRunner::RunNowOrPostTask(
          task_runner_, [disk_cache = disk_cache_, cache_key,
                         resource = std::move(resource)]() mutable {
            if (!disk_cache->ContainsKey(&cache_key)) {
              disk_cache->WriteContent(cache_key.ResourceId(),
                                       std::move(resource));
            }
          });
    }
  }
#endif

  size_t removed;
  {
    std::scoped_lock lock(waiters_mutex_);
    removed = running_fetchers_.erase(request_seq);
  }
  if (removed == 0) {
    FML_LOG(ERROR) << "Invalid status to fetch " << uri
                   << ", request_seq: " << request_seq;
  }
}

void NetLoaderManager::OnFailed(const std::string& uri, size_t request_seq,
                                int failed_time, const std::string& reason) {
  if (failed_time < HttpResourceFetcher::kMaxRetryTime) {
    // Fetch resource
    FML_LOG(WARNING) << "Failed to fetch " << uri << " for " << reason
                     << " . Going to retry.";
    auto fetcher = HttpResourceFetcherFactory::CreateFetcher(
        host_net_loader_, url::Uri(uri), request_seq, failed_time + 1);
    auto fetcher_ptr = fetcher.get();
    {
      std::scoped_lock lock(waiters_mutex_);
      running_fetchers_.erase(request_seq);
      running_fetchers_.emplace(request_seq, std::move(fetcher));
    }
    fetcher_ptr->Load();

    return;
  }
  NetLoaderWaiters uri_waiters = TakeWaiters(uri);
  for (auto& waiter : uri_waiters) {
    waiter.callback.OnFailed(reason);
  }
  {
    std::scoped_lock lock(waiters_mutex_);
    running_fetchers_.erase(request_seq);
  }
}

void NetLoaderManager::OnFinished(size_t request_seq, bool success,
                                  const uint8_t* raw_data, size_t length) {
  std::unique_lock lock(waiters_mutex_);
  auto it = running_fetchers_.find(request_seq);
  if (it == running_fetchers_.end()) {
    return;
  }
  if (!it->second->IsHostLoader()) {
    return;
  }

  HttpResourceFetcherHost* fetcher =
      reinterpret_cast<HttpResourceFetcherHost*>(it->second.get());
  lock.unlock();

  fetcher->OnFinished(success, raw_data, length);
}

}  // namespace clay
