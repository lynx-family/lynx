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

std::string NetLoaderManager::TakeLastResponse(const std::string& uri) {
  std::scoped_lock lock(response_mutex_);
  auto it = response_by_uri_.find(uri);
  if (it == response_by_uri_.end()) {
    return {};
  }
  std::string response = std::move(it->second);
  response_by_uri_.erase(it);
  return response;
}

void NetLoaderManager::SetResponse(const std::string& uri,
                                   std::string&& response) {
  std::scoped_lock lock(response_mutex_);
  const size_t max_response_size = 128;
  if (response_by_uri_.size() >= max_response_size &&
      response_by_uri_.find(uri) == response_by_uri_.end()) {
    response_by_uri_.erase(response_by_uri_.begin());
  }
  response_by_uri_[uri] = std::move(response);
}

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

    auto iter = request_groups_.find(uri);
    if (iter != request_groups_.end()) {
      NET_LOG << " add to pending requests " << uri << " seq "
              << current_request_seq_;
      iter->second.waiters.push_back({request_seq, callback});
      return request_seq;
    }

    NET_LOG << " insert a new request " << uri << " seq "
            << current_request_seq_;
    request_groups_.emplace(
        uri, NetRequestGroup{request_seq, {{request_seq, callback}}});
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

    auto iter = request_groups_.find(uri);
    if (iter == request_groups_.end()) {
      request_groups_.emplace(
          uri, NetRequestGroup{request_seq, {{request_seq, callback}}});
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
  {
    std::scoped_lock lock(waiters_mutex_);
    request_groups_.clear();
  }
  {
    std::scoped_lock lock(response_mutex_);
    response_by_uri_.clear();
  }
}

void NetLoaderManager::CancelBySeq(size_t seq) {
  std::unique_lock lock(waiters_mutex_);

  std::string canceled_url;
  auto it = request_groups_.begin();

  while (it != request_groups_.end()) {
    NetRequestGroup& group = it->second;
    size_t old_size = group.waiters.size();
    group.waiters.erase(
        std::remove_if(group.waiters.begin(), group.waiters.end(),
                       [seq](const NetWaiterEntity& entity) {
                         return entity.request_seq == seq;
                       }),
        group.waiters.end());
    if (group.waiters.size() != old_size) {
      if (group.waiters.empty()) {
        canceled_url = it->first;
        size_t owner_request_seq = group.owner_request_seq;
        // Retire this waiter generation before canceling. The physical request
        // may report failure asynchronously after a new group for the same URL
        // has already been created.
        request_groups_.erase(it);

        auto runner_iter = running_fetchers_.find(owner_request_seq);
        if (runner_iter != running_fetchers_.end()) {
          // Cancel is asynchronous. TakeWaiters() will reject its eventual
          // callback because this request group has already been retired.
          runner_iter->second->Cancel();
        }
      }
      break;
    } else {
      ++it;
    }
  }

  lock.unlock();
  std::unique_lock response_lock(response_mutex_);
  if (!canceled_url.empty()) {
    response_by_uri_.erase(canceled_url);
  }
}

NetLoaderManager::NetLoaderWaiters NetLoaderManager::TakeWaiters(
    const std::string& uri, size_t owner_request_seq) {
  NetLoaderWaiters uri_waiters;

  {
    std::scoped_lock lock(waiters_mutex_);

    auto iter = request_groups_.find(uri);
    bool owner_matches = iter != request_groups_.end() &&
                         iter->second.owner_request_seq == owner_request_seq;
    if (owner_matches) {
      uri_waiters = std::move(iter->second.waiters);
      request_groups_.erase(iter);
    }
  }
  return uri_waiters;
}

void NetLoaderManager::OnSucceeded(const std::string& uri, size_t request_seq,
                                   RawResource&& resource,
                                   bool from_disk_cache) {
  NetLoaderWaiters uri_waiters = TakeWaiters(uri, request_seq);
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
  {
    std::scoped_lock lock(waiters_mutex_);
    auto group_iter = request_groups_.find(uri);
    if (group_iter == request_groups_.end() ||
        group_iter->second.owner_request_seq != request_seq) {
      // This physical request belongs to a canceled or replaced generation.
      // Only clean up its own fetcher; never retry it or notify the current
      // waiter group for the same URL.
      running_fetchers_.erase(request_seq);
      return;
    }
  }

  if (failed_time < HttpResourceFetcher::kMaxRetryTime) {
    auto fetcher = HttpResourceFetcherFactory::CreateFetcher(
        host_net_loader_, url::Uri(uri), request_seq, failed_time + 1);
    if (fetcher) {
      auto fetcher_ptr = fetcher.get();
      bool request_still_active = false;
      {
        std::scoped_lock lock(waiters_mutex_);
        auto group_iter = request_groups_.find(uri);
        request_still_active =
            group_iter != request_groups_.end() &&
            group_iter->second.owner_request_seq == request_seq;
        if (request_still_active) {
          running_fetchers_.erase(request_seq);
          running_fetchers_.emplace(request_seq, std::move(fetcher));
        } else {
          running_fetchers_.erase(request_seq);
        }
      }

      if (!request_still_active) {
        return;
      }

      FML_LOG(WARNING) << "Failed to fetch " << uri << " for " << reason
                       << " . Going to retry.";
      fetcher_ptr->Load();
      return;
    }
  }

  NetLoaderWaiters uri_waiters = TakeWaiters(uri, request_seq);
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
