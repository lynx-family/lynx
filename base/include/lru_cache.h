// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LRU_CACHE_H_
#define BASE_INCLUDE_LRU_CACHE_H_
#include <stddef.h>

#include <functional>
#include <limits>
#include <list>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace lynx {
namespace base {

#define DEFAULT_CAPACITY 500

namespace internal {

template <bool kTrackCost>
struct LRUCacheCostState {};

template <>
struct LRUCacheCostState<true> {
  LRUCacheCostState() = default;
  LRUCacheCostState(size_t max_units, size_t max_entry_units)
      : max_units(max_units), max_entry_units(max_entry_units) {}

  size_t max_units = std::numeric_limits<size_t>::max();
  size_t max_entry_units = std::numeric_limits<size_t>::max();
  size_t total_units = 0;
};

}  // namespace internal

// Not thread-safe. Cost accounting is opt-in at compile time; the default
// instantiation retains pair entries and has no cost accounting or budget
// checks. Units in the cost-bounded instantiation are caller-defined, not
// always bytes.
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>, bool kTrackCost = false>
class LRUCache {
 public:
  LRUCache() : LRUCache(DEFAULT_CAPACITY) {}
  explicit LRUCache(size_t capacity) : capacity_(capacity) {}

  // Available only for the cost-bounded instantiation. Zero-cost entries are
  // allowed, but zero capacity disables admission in either instantiation.
  template <bool kEnabled = kTrackCost,
            std::enable_if_t<kEnabled && kTrackCost, int> = 0>
  LRUCache(size_t capacity, size_t max_units, size_t max_entry_units)
      : capacity_(capacity), cost_state_(max_units, max_entry_units) {}

  Value* Get(const Key& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
      return nullptr;
    }
    cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
    return &EntryValue(*it->second);
  }

  // Charges one unit only when cost accounting is enabled. The count-only
  // path does not pass a cost argument or enter the weighted insertion code.
  void Put(const Key& key, Value value) {
    if constexpr (kTrackCost) {
      PutInternal(key, std::move(value), 1);
    } else {
      if (capacity_ == 0) {
        return;
      }
      auto it = cache_map_.find(key);
      if (it != cache_map_.end()) {
        it->second->second = std::move(value);
        cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
        return;
      }
      if (cache_map_.size() == capacity_) {
        EvictLast();
      }
      cache_list_.emplace_front(key, std::move(value));
      cache_map_.emplace(key, cache_list_.begin());
    }
  }

  // Returns false without changing the cache if the entry cannot be admitted.
  template <bool kEnabled = kTrackCost,
            std::enable_if_t<kEnabled && kTrackCost, int> = 0>
  bool Put(const Key& key, Value value, size_t units) {
    return PutInternal(key, std::move(value), units);
  }

  void Clear() {
    cache_list_.clear();
    cache_map_.clear();
    if constexpr (kTrackCost) {
      cost_state_.total_units = 0;
    }
  }

  size_t size() const { return cache_map_.size(); }
  size_t total_units() const {
    if constexpr (kTrackCost) {
      return cost_state_.total_units;
    } else {
      return size();
    }
  }

 private:
  struct CostedEntry {
    Key key;
    Value value;
    size_t units;
  };

  using Entry =
      std::conditional_t<kTrackCost, CostedEntry, std::pair<Key, Value>>;
  using CostState = internal::LRUCacheCostState<kTrackCost>;

  static Value& EntryValue(Entry& entry) {
    if constexpr (kTrackCost) {
      return entry.value;
    } else {
      return entry.second;
    }
  }

  static const Key& EntryKey(const Entry& entry) {
    if constexpr (kTrackCost) {
      return entry.key;
    } else {
      return entry.first;
    }
  }

  bool PutInternal(const Key& key, Value&& value, size_t units) {
    static_assert(kTrackCost, "Weighted insertion requires cost accounting");
    if (capacity_ == 0 || units > cost_state_.max_entry_units ||
        units > cost_state_.max_units) {
      return false;
    }
    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
      cost_state_.total_units -= it->second->units;
      it->second->units = units;
      EntryValue(*it->second) = std::move(value);
      cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
      // Exclude the updated entry until trimming completes. Subtraction
      // avoids overflowing the sum of the current cost and incoming cost.
      while (cost_state_.total_units > cost_state_.max_units - units) {
        EvictLast();
      }
    } else {
      while (cache_map_.size() >= capacity_ ||
             cost_state_.total_units > cost_state_.max_units - units) {
        EvictLast();
      }
      cache_list_.push_front({key, std::move(value), units});
      cache_map_.emplace(key, cache_list_.begin());
    }
    cost_state_.total_units += units;
    return true;
  }

  void EvictLast() {
    const auto& last = cache_list_.back();
    if constexpr (kTrackCost) {
      cost_state_.total_units -= last.units;
    }
    cache_map_.erase(EntryKey(last));
    cache_list_.pop_back();
  }

  size_t capacity_;
  std::list<Entry> cache_list_;
  std::unordered_map<Key, typename std::list<Entry>::iterator, Hash, Equal>
      cache_map_;
  CostState cost_state_;
};

// Shares the LRU implementation while opting into per-entry and total costs.
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
using CostBoundedLRUCache = LRUCache<Key, Value, Hash, Equal, true>;

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_LRU_CACHE_H_
