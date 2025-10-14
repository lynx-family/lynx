// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/cache/net_disk_cache.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/fml/mapping.h"
#include "clay/net/cache/net_resource_cache_key.h"
#include "clay/net/macros.h"
#include "clay/net/resource_type.h"

namespace clay {

namespace {

constexpr char kNetDiskCacheDirectory[] = "net-disk-cache";

using ItemEntry = std::pair<std::string, fml::FileInfo>;

}  // namespace

NetDiskCache::NetDiskCache(fml::RefPtr<fml::TaskRunner> io_runner,
                           int64_t max_cache_size)
    : io_runner_(std::move(io_runner)) {
  config_.max_cache_size_ = max_cache_size;

  fml::TaskRunner::RunNowOrPostTask(io_runner_, [this, max_cache_size]() {
    std::shared_ptr<fml::UniqueFD> app_cache_base_dir =
        clay::PersistentCache::GetCacheForProcess()->cache_directory();

    cache_directory_ = std::make_unique<fml::UniqueFD>(
        fml::CreateDirectory(*app_cache_base_dir, {kNetDiskCacheDirectory},
                             fml::FilePermission::kReadWrite));
    if (!cache_directory_->is_valid()) {
      FML_DLOG(ERROR)
          << "Directory of net cache is invalid, cache will not be used";
      return;
    }
    NET_LOG << "Use net disk cache max cache size set to be " << max_cache_size;

    std::vector<ItemEntry> cached_items;
    fml::VisitFiles(
        *cache_directory_, [&cached_items](const fml::UniqueFD& directory,
                                           const std::string& filename) {
          auto file = fml::OpenFileReadOnly(directory, filename.c_str());

          if (!file.is_valid()) {
            FML_DLOG(WARNING) << "Failed to open file " << filename;
            fml::UnlinkFile(directory, filename.c_str());
            return true;
          }

          fml::FileInfo file_info;
          if (!fml::GetFileInfo(file, &file_info)) {
            FML_DLOG(WARNING) << "Unable to get file info for " << filename;
          }
          cached_items.emplace_back(filename, file_info);
          return true;
        });
    // cache_stats_ is created at io thread after check cache_directory_ is
    // valid, cache_directory_ and io_runner_ must be valid if cache_stats_
    // exist.
    cache_stats_ = std::make_unique<CacheStats>(std::move(cached_items));
    alive_ = true;
    NET_LOG << "NetDiskCache create finished.";
  });
}

bool NetDiskCache::ContainsKey(NetResourceCacheKey* cache_key) {
  FML_DCHECK(cache_key != nullptr);
  if (!Available()) {
    return false;
  }
  const std::string& res_id = cache_key->ResourceId();
  if (res_id.empty()) {
    return false;
  }
  return cache_stats_->LockItemIfContains(cache_key->ResourceId());
}

void NetDiskCache::GetCacheContent(
    NetResourceCacheKey&& cache_key,
    const std::function<void(const std::string&, RawResource)>& callback) {
  FML_DCHECK(cache_stats_);
  // Ensure cache item be locked
  FML_DCHECK(cache_stats_->ItemHasLocked(cache_key.ResourceId()));
  FML_DCHECK(cache_directory_->is_valid());

  io_runner_->PostTask([callback, cache_key = std::move(cache_key),
                        cache_stats = cache_stats_.get(),
                        dir = cache_directory_.get()]() mutable {
    const std::string& res_id = cache_key.ResourceId();
    auto file = fml::OpenFileReadOnly(*dir, res_id.c_str());
    if (!file.is_valid()) {
      callback(cache_key.Uri().Spec(), {0, nullptr});
      fml::UnlinkFile(*dir, res_id.c_str());
      cache_stats->OnFileInvalid(res_id);
      return;
    }
    auto mapping = std::make_unique<fml::FileMapping>(file);
    RawResource resource = {mapping->GetSize(), nullptr};
    if (resource.length != 0) {
      NET_LOG << "Cache Content of " << cache_key.Uri().Spec() << " hit";
      auto data = std::shared_ptr<uint8_t>(new uint8_t[resource.length],
                                           std::default_delete<uint8_t[]>());
      ::memcpy(reinterpret_cast<void*>(data.get()), mapping->GetMapping(),
               resource.length);
      resource.data = std::move(data);
    }
    callback(cache_key.Uri().Spec(), std::move(resource));
    cache_stats->RefreshAndUnLockItem(res_id);
  });
}

void NetDiskCache::WriteContent(const std::string& resource_id,
                                RawResource resource) {
  if (!Available() ||
      static_cast<int64_t>(resource.length) >= config_.max_cache_size_) {
    // `cache_stats_` has not created yet or size of resource is too big.
    return;
  }
  FML_DCHECK(cache_directory_->is_valid());
  CheckMayTrim();
  io_runner_->PostTask([resource_id, resource = std::move(resource),
                        cache_stats = cache_stats_.get(),
                        dir = cache_directory_.get()]() mutable {
    auto data = std::make_unique<fml::DataMapping>(std::vector<uint8_t>{
        resource.data.get(), resource.data.get() + resource.length});

    NET_LOG << "Write Cache Content to " << resource_id;
    // Write.
    fml::WriteAtomically(*dir, resource_id.c_str(), *data);
    cache_stats->AddItem(resource_id, resource.length);
  });
}

void NetDiskCache::ClearCaches() {
  if (!Available()) {
    return;
  }
  int64_t old_size = config_.max_cache_size_;
  config_.max_cache_size_ = 0;
  CheckMayTrim();
  config_.max_cache_size_ = old_size;
}

void NetDiskCache::CheckMayTrim() {
  FML_DCHECK(cache_stats_);
  if (cache_stats_->GetSize() > config_.max_cache_size_) {
    TrimToLimit();
  }
}

void NetDiskCache::TrimToLimit() {
  std::vector<ItemEntry> cache_items =
      cache_stats_->GetAllDeletableCachedItem();
  // Sort by last access time reversal
  std::sort(cache_items.begin(), cache_items.end(),
            [](const ItemEntry& lhs, const ItemEntry& rhs) {
              if (lhs.second.last_access_time == rhs.second.last_access_time) {
                return lhs.second.size > rhs.second.size;
              }
              return lhs.second.last_access_time > rhs.second.last_access_time;
            });
  for (const ItemEntry& item : cache_items) {
    if (cache_stats_->GetSize() < config_.max_cache_size_) {
      break;
    }
    RemoveItem(item.first);
  }
}

void NetDiskCache::RemoveItem(const std::string& resource_id) {
  cache_stats_->RemoveItem(resource_id);
  io_runner_->PostTask([dir = cache_directory_.get(), resource_id]() {
    fml::UnlinkFile(*dir, resource_id.data());
  });
}

}  // namespace clay
