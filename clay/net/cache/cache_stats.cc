// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/cache/cache_stats.h"

#include "base/include/fml/time/time_point.h"
#include "clay/fml/file.h"
#include "clay/fml/logging.h"

namespace clay {

CacheStats::CacheStats(
    std::vector<std::pair<std::string, fml::FileInfo>>&& init_cache_items)
    : items_mutex_(fml::SharedMutex::Create()),
      locked_items_mutex_(fml::SharedMutex::Create()) {
  fml::UniqueLock write_lock(*items_mutex_);

  for (auto& item : init_cache_items) {
    items_.emplace(std::move(item.first), item.second);
    disk_size_ += item.second.size;
  }
}

size_t CacheStats::GetCount() const {
  fml::SharedLock read_lock(*items_mutex_);
  return items_.size();
}

void CacheStats::AddItem(const std::string& item, int64_t item_size) {
  fml::UniqueLock write_lock(*items_mutex_);
  fml::FileInfo file_info;
  file_info.size = item_size;
  file_info.last_access_time = fml::TimePoint::Now();
  auto result = items_.emplace(item, file_info);
  // Insert must success. otherwise we should use cached item rather insert.
  FML_DCHECK(result.second);
  disk_size_ += item_size;
}

void CacheStats::RemoveItem(const std::string& item) {
  fml::UniqueLock write_lock(*items_mutex_);

  auto iter = items_.find(item);
  FML_DCHECK(iter != items_.end());
  disk_size_ -= iter->second.size;
  items_.erase(iter);
}

bool CacheStats::LockItemIfContains(const std::string& item) {
  // Quick check if item already be locked.
  {
    fml::UniqueLock lock(*locked_items_mutex_);
    auto iter = locked_items_.find(item);
    if (iter != locked_items_.end()) {
      iter->second++;
      return true;
    }
  }

  fml::SharedLock read_lock(*items_mutex_);
  auto iter = items_.find(item);
  if (iter != items_.end()) {
    {
      // Lock item if item exist to avoid delete when trim cache
      fml::UniqueLock lock(*locked_items_mutex_);
      locked_items_.emplace(item, 1);
    }
    return true;
  }

  return false;
}

bool CacheStats::ItemHasLocked(const std::string& item) {
  fml::SharedLock lock(*locked_items_mutex_);
  return locked_items_.find(item) != locked_items_.end();
}

void CacheStats::RefreshAndUnLockItem(const std::string& item) {
  {
    fml::UniqueLock write_lock(*items_mutex_);
    auto iter = items_.find(item);
    FML_DCHECK(iter != items_.end());
    iter->second.last_access_time = fml::TimePoint::Now();
  }
  DecreaseItemLock(item);
}

void CacheStats::OnFileInvalid(const std::string& item) {
  {
    fml::UniqueLock write_lock(*items_mutex_);
    auto iter = items_.find(item);
    if (iter != items_.end()) {
      disk_size_ -= iter->second.size;
      items_.erase(iter);
    }
  }
  DecreaseItemLock(item);
}

std::vector<std::pair<std::string, fml::FileInfo>>
CacheStats::GetAllDeletableCachedItem() const {
  fml::SharedLock read_lock(*items_mutex_);
  fml::SharedLock locked_items_lock(*locked_items_mutex_);

  std::vector<std::pair<std::string, fml::FileInfo>> cache_items;
  for (const auto& item : items_) {
    if (locked_items_.find(item.first) != locked_items_.end()) {
      continue;
    }
    cache_items.emplace_back(item.first, item.second);
  }
  return cache_items;
}

void CacheStats::DecreaseItemLock(const std::string& item) {
  fml::UniqueLock locked_items_lock(*locked_items_mutex_);
  auto iter = locked_items_.find(item);
  if (iter != locked_items_.end()) {
    if (iter->second == 1) {
      locked_items_.erase(item);
    } else {
      iter->second--;
    }
  }
}

}  // namespace clay
