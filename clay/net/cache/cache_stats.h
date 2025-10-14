// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_CACHE_CACHE_STATS_H_
#define CLAY_NET_CACHE_CACHE_STATS_H_

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/synchronization/shared_mutex.h"
#include "clay/fml/file.h"

namespace clay {

// Keep all cache statistics info in memory, this class may accessed by
// multi thread, so it should be thread safe.
class CacheStats {
 public:
  explicit CacheStats(
      std::vector<std::pair<std::string, fml::FileInfo>>&& init_cache_item);
  int64_t GetSize() const { return disk_size_; }
  size_t GetCount() const;
  void AddItem(const std::string& item, int64_t item_size);
  void RemoveItem(const std::string& item);

  bool LockItemIfContains(const std::string& item);
  bool ItemHasLocked(const std::string& item);
  void RefreshAndUnLockItem(const std::string& item);
  void OnFileInvalid(const std::string& item);

  std::vector<std::pair<std::string, fml::FileInfo>> GetAllDeletableCachedItem()
      const;

 private:
  void DecreaseItemLock(const std::string& item);

  std::atomic<int64_t> disk_size_ = 0;

  std::unique_ptr<fml::SharedMutex> items_mutex_;
  std::unique_ptr<fml::SharedMutex> locked_items_mutex_;

  std::unordered_map<std::string, fml::FileInfo> items_;
  // Items be visited but not used, keep those to avoid delete by trim.
  std::unordered_map<std::string, int> locked_items_;
};

}  // namespace clay

#endif  // CLAY_NET_CACHE_CACHE_STATS_H_
