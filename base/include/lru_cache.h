// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LRU_CACHE_H_
#define BASE_INCLUDE_LRU_CACHE_H_
#include <stddef.h>

#include <functional>
#include <list>
#include <unordered_map>
#include <utility>

namespace lynx {
namespace base {

#define DEFAULT_CAPACITY 500

template <typename Key, typename Value>
class LRUCache {
 public:
  LRUCache() : capacity_(DEFAULT_CAPACITY) {}
  explicit LRUCache(size_t capacity) : capacity_(capacity) {}

  Value* Get(const Key& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
      return nullptr;
    }
    cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
    return &(it->second->second);
  }

  void Put(const Key& key, Value value) {
    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
      it->second->second = std::move(value);
      cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
      return;
    }
    if (cache_map_.size() == capacity_) {
      auto last = cache_list_.back();
      cache_map_.erase(last.first);
      cache_list_.pop_back();
    }
    cache_list_.emplace_front(key, std::move(value));
    cache_map_[key] = cache_list_.begin();
  }

  void Clear() {
    cache_list_.clear();
    cache_map_.clear();
  }

 private:
  size_t capacity_;
  std::list<std::pair<Key, Value>> cache_list_;
  std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator>
      cache_map_;
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_LRU_CACHE_H_
