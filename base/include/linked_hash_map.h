// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LINKED_HASH_MAP_H_
#define BASE_INCLUDE_LINKED_HASH_MAP_H_

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <tuple>
#include <utility>

#include "base/include/boost/unordered.h"

namespace lynx {
namespace base {

/**
 LinkedHashMap works like Java LinkedHashMap.
 It is a hash table and linked list implementation with limited map interface,
 and with predictable iteration order. It maintains a doubly-linked list running
 through all of its entries. This linked list defines the iteration ordering,
 which is normally the order in which keys were inserted into the map
 (insertion-order).

 The inner hash table is not created until element count reaches
 LinearFindThreshold. Before that, finding algorithm is linear search.
 This contributes to performance in two ways:
 1. Linear search for few elements is faster than hash table.
 2. Saves the time of creating and maintaining HashTable nodes.

 LinkedHashMap is node based and guarantees pointer stability.

 Performance Tips:
 If the final number of elements can be estimated, calling the reserve() method
 in advance can allocate a whole block of memory(the pool) for the nodes. No
 additional memory allocation system calls are made when the capacity is not
 exceeded.

 Debugger Tips:
 Currently only LLDB script is provided. Reference //tools/lldb/lynx_lldb.py for
 global settings. You can also execute LLDB command
    `command script import {full_path_of linked_hash_map_lldb.py}`
 when LLDB stops for each LLDB session.
 */
template <class Key, class T,
          uint32_t InsertionBuildMapThreshold =
              (std::is_integral_v<Key> || std::is_enum_v<Key>) ? 32 : 12,
          uint32_t FindBuildMapThreshold =
              (std::is_integral_v<Key> || std::is_enum_v<Key>) ? 12 : 6,
          class Hash = std::hash<Key>, class Pred = std::equal_to<Key>>
class LinkedHashMap {
 public:
  using size_type = size_t;
  using key_type = Key;
  using value_type = std::pair<Key, T>;
  using reference = value_type&;
  using const_reference = const value_type&;

  struct Node;
  struct NodeBase {
    Node* prev;
    Node* next;

    NodeBase() : prev(AsNode()), next(AsNode()) {}

    void Reset() { prev = next = AsNode(); }

    Node* AsNode() { return static_cast<Node*>(this); }

    const Node* AsNode() const { return static_cast<const Node*>(this); }
  };

  struct Node : public NodeBase {
    value_type value;

    template <class... Args>
    Node(const Key& key, Args&&... args)
        : NodeBase(),
          value(std::piecewise_construct, std::forward_as_tuple(key),
                std::forward_as_tuple(std::forward<Args>(args)...)) {}

    template <class... Args>
    Node(Key&& key, Args&&... args)
        : NodeBase(),
          value(std::piecewise_construct, std::forward_as_tuple(std::move(key)),
                std::forward_as_tuple(std::forward<Args>(args)...)) {}
  };

  struct Iterator {
    NodeBase* ptr;
    Iterator() : ptr(nullptr) {}
    explicit Iterator(NodeBase* ptr) : ptr(ptr) {}

    Node* node_ptr() const { return static_cast<Node*>(ptr); }

    value_type& operator*() const { return ptr->AsNode()->value; }

    value_type* operator->() const { return &ptr->AsNode()->value; }

    Iterator& operator++() {
      ptr = ptr->next;
      return *this;
    }

    Iterator operator++(int) {
      Iterator t(*this);
      ++(*this);
      return t;
    }

    Iterator& operator--() {
      ptr = ptr->prev;
      return *this;
    }

    Iterator operator--(int) {
      Iterator t(*this);
      --(*this);
      return t;
    }

    friend bool operator==(const Iterator& x, const Iterator& y) {
      return x.ptr == y.ptr;
    }

    friend bool operator!=(const Iterator& x, const Iterator& y) {
      return !(x == y);
    }
  };

  struct ConstIterator {
    NodeBase* ptr;
    ConstIterator() : ptr(nullptr) {}
    explicit ConstIterator(const NodeBase* ptr)
        : ptr(const_cast<NodeBase*>(ptr)) {}
    ConstIterator(const Iterator& other) : ptr(other.ptr) {}

    Node* node_ptr() const { return static_cast<Node*>(ptr); }

    const value_type& operator*() const { return ptr->AsNode()->value; }

    const value_type* operator->() const { return &ptr->AsNode()->value; }

    ConstIterator& operator++() {
      ptr = ptr->next;
      return *this;
    }

    ConstIterator operator++(int) {
      ConstIterator t(*this);
      ++(*this);
      return t;
    }

    ConstIterator& operator--() {
      ptr = ptr->prev;
      return *this;
    }

    ConstIterator operator--(int) {
      ConstIterator t(*this);
      --(*this);
      return t;
    }

    friend bool operator==(const ConstIterator& x, const ConstIterator& y) {
      return x.ptr == y.ptr;
    }

    friend bool operator!=(const ConstIterator& x, const ConstIterator& y) {
      return !(x == y);
    }
  };

  using iterator = Iterator;
  using const_iterator = ConstIterator;

  using map_type = boost::unordered_flat_map<Key, iterator, Hash, Pred>;

 public:
  /// The default initial pool size.
  static constexpr size_t kInitialAllocationSize = 2;

  /// @param initial_allocation_size Capacity for nodes memory for first
  /// allocation.
  explicit LinkedHashMap(
      size_t initial_allocation_size = kInitialAllocationSize)
      : pool_size_(static_cast<uint32_t>(initial_allocation_size)) {}

  ~LinkedHashMap() {
#if 0
    PrintElements("dtor");
#endif

    if (is_perfect_) {
      // Fast path
      for (size_t i = 0; i < count_; i++) {
        pool_[i].~Node();
      }
    } else {
      list_clear();
    }

    if (pool_ != nullptr) {
      std::free(pool_);
    }
    if (map_ != nullptr) {
      delete map_;
    }
  }

#if 0
  void PrintElements(const char* scene, const char* tag = "[AA]") {
    [[maybe_unused]] bool has_not_on_pool = false;
    std::cout << tag << " " << scene << " " << (void*)this << " " << count_
              << " pool_size: " << pool_size_
              << " pool allocated: " << (pool_ != nullptr ? "1" : "0")
              << " perfect: " << (is_perfect_ ? "1" : "0")
              << " has_map: " << (map_ != nullptr ? "1" : "0") << std::endl;
    if (count_ == 1) {
      auto p = begin().node_ptr();
      if (!ptr_on_pool(p)) {
        std::cout << tag << "    not on pool: 0" << std::endl;
        has_not_on_pool = true;
      }
    } else if (count_ > 1) {
      auto it = begin();
      auto p = it.node_ptr();
      if (ptr_on_pool(p)) {
        std::cout << tag << "    "
                  << "on pool: 0" << std::endl;
      } else {
        std::cout << tag << "    "
                  << "not on pool: 0" << std::endl;
        has_not_on_pool = true;
      }
      ++it;
      for (; it != end(); ++it) {
        auto pIt = it.node_ptr();
        if (ptr_on_pool(pIt)) {
          std::cout << tag << "    "
                    << "on pool: " << (intptr_t)pIt - (intptr_t)p << std::endl;
        } else {
          std::cout << tag << "    "
                    << "not on pool: " << (intptr_t)pIt - (intptr_t)p
                    << std::endl;
          has_not_on_pool = true;
        }
        p = pIt;
      }
    }
//    breakLog(scene, count_, has_not_on_pool);
  }
#endif

  LinkedHashMap(std::initializer_list<value_type> initial_list) {
    reserve(initial_list.size());
    for (auto& value : initial_list) {
      insert_or_assign(std::move(value.first), std::move(value.second));
    }
  }

  LinkedHashMap(const LinkedHashMap& other) {
    if (!other.empty()) {
      reserve(other.size());
      // For copy construct, only copy values, leaves map_ as nullptr.
      for (auto it = other.begin(), e = other.end(); it != e; ++it) {
        construct_node_at_end(it->first, it->second);
      }
    }
  }

  LinkedHashMap& operator=(const LinkedHashMap& other) {
    clear();
    if (!other.empty()) {
      reserve(other.size());
      // Copy assignment, if map_ already exists, build map_.
      if (map_ != nullptr) {
        for (auto it = other.begin(), e = other.end(); it != e; ++it) {
          map_->emplace(it->first,
                        construct_node_at_end(it->first, it->second));
        }
      } else {
        for (auto it = other.begin(), e = other.end(); it != e; ++it) {
          construct_node_at_end(it->first, it->second);
        }
      }
    }
    return *this;
  }

  LinkedHashMap(LinkedHashMap&& other) : pool_size_(other.pool_size_) {
    if (!other.empty()) {
      // If other has map, steal it or leaves map_ as nullptr.
      if (other.map_ != nullptr) {
        map_ = other.map_;
        other.map_ = nullptr;
      }

      if (other.pool_ != nullptr) {
        pool_ = other.pool_;
        pool_cursor_ = other.pool_cursor_;
        other.pool_ = nullptr;
        other.pool_size_ = kInitialAllocationSize;
        // No need to reset other.pool_cursor_
      }

      count_ = other.count_;
      other.count_ = 0;

      end_.next = other.end_.next;
      end_.next->prev =
          end_as_link()->AsNode();  // Must use self's end_ as real end.
      end_.prev = other.end_.prev;
      end_.prev->next = end_as_link()->AsNode();
      other.end_.Reset();

      is_perfect_ = other.is_perfect_;
      other.is_perfect_ = true;
    }
  }

  LinkedHashMap& operator=(LinkedHashMap&& other) {
    clear();
    if (pool_ != nullptr) {
      std::free(pool_);
      pool_ = nullptr;
    }
    if (map_ != nullptr) {
      delete map_;
      map_ = nullptr;
    }

    // Reconstruct self.
    return *(new (this) LinkedHashMap(std::move(other)));
  }

  /// @param free_pool When true, also release the memory of pool.
  /// And when next time this map is inserted with any data, a new memory
  /// pool will be allocated.
  void clear(bool free_pool = false) noexcept {
    if (empty()) {
      return;
    }
    list_clear();
    count_ = 0;
    is_perfect_ = true;

    if (free_pool) {
      if (pool_ != nullptr) {
        std::free(pool_);
        pool_ = nullptr;
      }
      pool_size_ = kInitialAllocationSize;
    } else {
      // Reset pool cursor so that pool can be reused or reserved with larger
      // size.
      pool_cursor_ = 0;
    }

    if (map_ != nullptr) {
      map_->clear();
    }
  }

  iterator find(const Key& key) {
    return inner_find(key, FindBuildMapThreshold);
  }

  const_iterator find(const Key& key) const noexcept {
    return (const_cast<LinkedHashMap*>(this))
        ->inner_find(key, FindBuildMapThreshold);
  }

  size_type erase(const Key& key) {
    if (map_ != nullptr) {
      if (auto map_it = map_->find(key); map_it != map_->end()) {
        auto list_it = map_it->second;
        map_->erase(map_it);
        erase_node(list_it);
        return 1;
      }
    } else {
      // map_ is nullptr, call inner_find but do not build map_
      // or we have to erase from map_ again which makes no sense.
      auto it = inner_find(key, std::numeric_limits<uint32_t>::max());
      if (it != end()) {
        erase_node(it);
        return 1;
      }
    }
    return 0;
  }

  iterator erase(iterator pos) { return erase(const_iterator(pos)); }

  iterator erase(const_iterator pos) {
    if (map_ != nullptr) {
      map_->erase(pos->first);
    }
    if (pos != end()) {
      return erase_node(pos);
    } else {
      return end();
    }
  }

  /// @brief Compared with for range loop, it is recommended to use foreach
  /// method to traverse nodes. The foreach method will detect whether the data
  /// nodes are continuous and completely located in the memory pool. If this
  /// condition is met, it will traverse the nodes in an array manner. When the
  /// for range loop uses begin() and end() iterators, the iterators use a
  /// double linked list to access nodes, and the performance is slightly worse
  /// than that of an array. However, the foreach method may bring a little
  /// binary increment and does not allow the map to be modified while
  /// iterating.
  template <typename Callback, typename = std::enable_if_t<std::is_invocable_v<
                                   Callback, const Key&, const T&>>>
  void foreach (Callback&& callback) const {
    if (is_perfect_) {
      // Traverse like array.
      for (size_t i = 0; i < count_; i++) {
        const Node& n = pool_[i];
        callback(n.value.first, n.value.second);
      }
    } else {
      // Fallback to default iterators which are linked nodes.
      for (const auto& it : *this) {
        callback(it.first, it.second);
      }
    }
  }

  template <typename Callback,
            typename =
                std::enable_if_t<std::is_invocable_v<Callback, const Key&, T&>>>
  void foreach (Callback&& callback) {
    if (is_perfect_) {
      // Traverse like array.
      for (size_t i = 0; i < count_; i++) {
        Node& n = pool_[i];
        callback(n.value.first, n.value.second);
      }
    } else {
      // Fallback to default iterators which are linked nodes.
      for (auto& it : *this) {
        callback(it.first, it.second);
      }
    }
  }

  /// @brief Merge other map into self. This method provides optimizations
  /// when self is empty and use other's foreach method to accelerate the
  /// iteration.
  void merge(const LinkedHashMap& other) {
    if (empty()) {
      // If self is empty, assigning from source is more efficient
      // because it does not check key existence when inserting nodes.
      *this = other;
    } else {
      other.foreach (
          [=](const Key& k, const T& v) { this->insert_or_assign(k, v); });
    }
  }

  const_iterator begin() const noexcept { return const_iterator(end_.next); }

  const_iterator end() const noexcept { return const_iterator(end_as_link()); }

  iterator begin() noexcept { return iterator(end_.next); }

  iterator end() noexcept { return iterator(end_as_link()); }

  const_reference front() const noexcept { return end_.next->value; }

  reference front() noexcept { return end_.next->value; }

  const_reference back() const noexcept { return end_.prev->value; }

  reference back() noexcept { return end_.prev->value; }

  T& operator[](const Key& key) { return at(key); }

  T& operator[](Key&& key) { return at(std::move(key)); }

  T& at(const Key& key) { return insert_default_if_absent(key).first->second; }

  T& at(Key&& key) {
    return insert_default_if_absent(std::move(key)).first->second;
  }

  bool contains(const Key& key) const {
    return (const_cast<LinkedHashMap*>(this))
               ->inner_find(key, FindBuildMapThreshold) != end();
  }

  /// @brief This method is basically for internal usage. It searches the map
  /// and if key is absent, a default constructed value will be inserted.
  /// This is more efficient than insert_or_assign(key, T()) because it
  /// does not construct a T() instance when key is found.
  std::pair<iterator, bool> insert_default_if_absent(const Key& key) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(key), true};
    } else {
      return {it, false};
    }
  }

  std::pair<iterator, bool> insert_default_if_absent(Key&& key) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(std::move(key)), true};
    } else {
      return {it, false};
    }
  }

  /// @brief Since the total number of elements cannot be calculated from the
  /// last and first iterators, it is recommended to call reserve() in advance
  /// to allocate memory.
  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      insert_or_assign(first->first, first->second);
    }
  }

  template <class... Args>
  std::pair<iterator, bool> emplace_or_assign(const Key& key, Args&&... args) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(key, std::forward<Args>(args)...), true};
    } else {
      it->second.~T();
      new (&(it->second)) T(std::forward<Args>(args)...);
      return {it, false};
    }
  }

  template <class... Args>
  std::pair<iterator, bool> emplace_or_assign(Key&& key, Args&&... args) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {
          construct_node_at_end(std::move(key), std::forward<Args>(args)...),
          true};
    } else {
      it->second.~T();
      new (&(it->second)) T(std::forward<Args>(args)...);
      return {it, false};
    }
  }

  std::pair<iterator, bool> insert_or_assign(const Key& key, const T& obj) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(key, obj), true};
    } else {
      it->second = obj;
      return {it, false};
    }
  }

  std::pair<iterator, bool> insert_or_assign(const Key& key, T&& obj) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(key, std::move(obj)), true};
    } else {
      it->second = std::move(obj);
      return {it, false};
    }
  }

  // For Key of base::String type.
  std::pair<iterator, bool> insert_or_assign(Key&& key, T&& obj) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(std::move(key), std::move(obj)), true};
    } else {
      it->second = std::move(obj);
      return {it, false};
    }
  }

  std::pair<iterator, bool> insert_or_assign(Key&& key, const T& obj) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(std::move(key), obj), true};
    } else {
      it->second = obj;
      return {it, false};
    }
  }

  std::pair<iterator, bool> insert_if_absent(const Key& key, const T& obj) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(key, obj), true};
    } else {
      return {it, false};
    }
  }

  std::pair<iterator, bool> insert_if_absent(const Key& key, T&& obj) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(key, std::move(obj)), true};
    } else {
      return {it, false};
    }
  }

  // For Key of base::String type.
  std::pair<iterator, bool> insert_if_absent(Key&& key, T&& obj) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(std::move(key), std::move(obj)), true};
    } else {
      return {it, false};
    }
  }

  std::pair<iterator, bool> insert_if_absent(Key&& key, const T& obj) {
    auto it = inner_find(key, InsertionBuildMapThreshold);
    if (it == end()) {
      return {construct_node_at_end(std::move(key), obj), true};
    } else {
      return {it, false};
    }
  }

  bool empty() const { return count_ == 0; }

  size_type size() const noexcept { return count_; }

  /// @brief Pre-allocate memory for next nodes to be inserted into the map.
  /// This method only records the size of the memory pool required and does not
  /// actually allocate memory. A whole block of memory is allocated only when
  /// the first node data is inserted. Before the first node is inserted, you
  /// can call this method multiple times, and it will record the maximum
  /// required capacity. Once any node has been added to the map and the memory
  /// pool is used, subsequent reserve() calls will not take effect.
  void reserve(size_type count) {
    if (pool_ == nullptr) {
      if (count > pool_size_) {
        pool_size_ = static_cast<uint32_t>(count);
      }
    } else if (pool_cursor_ == 0 && count > pool_size_) {
      // Pool allocated but not used and new reserving count is larger.
      std::free(pool_);
      pool_ = nullptr;
      pool_size_ = static_cast<uint32_t>(count);
    } else {
      // failed reserve
    }
    // Space size reserved and allocate pool until first node being allocated
    // because user may do reserve but no element pushed.
  }

  /// @brief Unlike the reserve() method, it allows you to reduce the size of
  /// the memory pool that needs to be created. LinkedHashMap uses
  /// **kInitialAllocationSize** as the initial memory pool size by default.
  /// If you only need a smaller capacity (for example, you know that the Map
  /// will only have 1 to 2 elements in the end), you can use this method to
  /// reduce the memory pool and save memory.
  void set_pool_capacity(size_type count) {
    if (count > pool_size_) {
      reserve(count);
    } else if (count < pool_size_) {
      // Allows to set to smaller pool capacity.
      if (pool_ == nullptr) {
        pool_size_ = static_cast<uint32_t>(count);
      } else {
        // Pool already allocated, no need to reset to smaller size.
      }
    }
  }

  /// This class is for testing only.
  class Testing {
   public:
    /// @brief It calculates count of alive nodes which are on pool memory.
    static size_t count_of_nodes_on_pool(const LinkedHashMap& map) {
      size_t result = 0;
      for (auto it = map.begin(); it != map.end(); ++it) {
        if (map.ptr_on_pool(it.node_ptr())) {
          result++;
        }
      }
      return result;
    }

    static bool assume_status(const LinkedHashMap& map, bool has_map,
                              bool is_perfect) {
      return ((map.map_ != nullptr) == has_map) &&
             (map.is_perfect_ == is_perfect);
    }

    static bool assume_end_in_initial_state(const LinkedHashMap& map) {
      return map.end_.prev == map.end_as_link() &&
             map.end_.next == map.end_as_link();
    }

    static bool check_consistency(const LinkedHashMap& map) {
      if (map.empty()) {
        bool end_in_initial_state = map.end_.prev == map.end_as_link() &&
                                    map.end_.next == map.end_as_link();
        if (!end_in_initial_state) {
          return false;
        }
      }

      if (map.map_ != nullptr) {
        if (map.map_->size() != map.count_) {
          return false;
        }
        for (auto it = map.begin(); it != map.end(); ++it) {
          if (map.map_->find(it->first)->second != it) {
            return false;
          }
        }
      }

      size_t count = 0;
      for (auto it = map.begin(); it != map.end(); ++it) {
        count++;
      }
      if (count != map.count_) {
        return false;
      }

      if (map.is_perfect_) {
        for (size_t i = 0; i < map.count_; i++) {
          Node& n = map.pool_[i];
          if (!map.ptr_on_pool(&n)) {
            return false;
          }

          if (i == 0) {
            if (n.prev != map.end_as_link()) {
              return false;
            }
          } else {
            if (n.prev != &map.pool_[i - 1]) {
              return false;
            }
          }

          if (i == map.count_ - 1) {
            if (n.next != map.end_as_link()) {
              return false;
            }
          } else {
            if (n.next != &map.pool_[i + 1]) {
              return false;
            }
          }
        }
      }

      return true;
    }
  };

 private:
  friend class Testing;

  NodeBase end_;
  Node* pool_{nullptr};
  uint32_t pool_size_{kInitialAllocationSize};
  uint32_t pool_cursor_;
  uint32_t count_{0u};

  // A pefect map only contains nodes continuous on pool memory.
  // We can fast iterate nodes like array if self is perfect.
  bool is_perfect_{true};

  // Map is created until element count reaches LinearFindThreshold.
  map_type* map_{nullptr};

  // If map was built, search by map or do linear search and create
  // map if count of elements reaches build_map_threshold.
  iterator inner_find(const Key& key, uint32_t build_map_threshold) {
    iterator result = end();
    if (map_ != nullptr) {
      // map_ not null also means hash search may be better choice.
      if (const auto map_it = map_->find(key); map_it != map_->end()) {
        result = map_it->second;
      }
    } else if (count_ > build_map_threshold) {
      // Do last time linear find and also build the map in the same loop.
      map_ = new map_type();
      map_->reserve(std::max<size_t>(pool_size_, count_));

      if (is_perfect_) {
        // List nodes are in perfect state, loop as an array for
        // better performance.
        size_t i = 0;
        for (; i < count_; i++) {
          Node& n = pool_[i];
          if (n.value.first == key) {
            result = iterator(&n);
            break;
          }
          map_->emplace(std::piecewise_construct,
                        std::forward_as_tuple(n.value.first),
                        std::forward_as_tuple(&n));
        }

        // Loop for remained index to build map, but no need to
        // check equality with key.
        for (; i < count_; i++) {
          Node& n = pool_[i];
          map_->emplace(std::piecewise_construct,
                        std::forward_as_tuple(n.value.first),
                        std::forward_as_tuple(&n));
        }
      } else {
        // Not perfect nodes, loop as linked list.
        auto i = begin();
        const auto e = end();
        for (; i != e; ++i) {
          if (i->first == key) {
            result = i;
            break;
          }
          map_->emplace(i->first, i);
        }

        // Loop for remained index to build map, but no need to
        // check equality with key.
        for (; i != e; ++i) {
          map_->emplace(i->first, i);
        }
      }
    } else {
      // Do linear find.
      if (is_perfect_) {
        for (size_t i = 0; i < count_; i++) {
          Node& n = pool_[i];
          if (n.value.first == key) {
            result = iterator(&n);
            break;
          }
        }
      } else {
        const auto e = end();
        for (auto i = begin(); i != e; ++i) {
          if (i->first == key) {
            result = i;
            break;
          }
        }
      }
    }
    return result;
  }

  inline bool alloc_pool() {
    if (map_ != nullptr) {
      map_->reserve(pool_size_);
    }

    pool_ = static_cast<Node*>(std::malloc(sizeof(Node) * pool_size_));
    pool_cursor_ = 0;
    if (pool_ != nullptr) {
      return true;
    } else {
      // No more try to allocate pool.
      pool_size_ = 0;
      return false;
    }
  }

  Node* alloc_node() {
    if (pool_ == nullptr && pool_size_ > 0) {
      if (alloc_pool()) {
        return &pool_[pool_cursor_++];
      }
    }
    if (pool_ != nullptr && pool_cursor_ < pool_size_) {
      return &pool_[pool_cursor_++];
    } else {
      is_perfect_ = false;
      return static_cast<Node*>(std::malloc(sizeof(Node)));
    }
  }

  inline bool ptr_on_pool(Node* ptr) const {
    return pool_ != nullptr && ptr >= pool_ && ptr < pool_ + pool_size_;
  }

  void free_node(Node* ptr) {
    if (ptr_on_pool(ptr)) {
      // node on pool, do nothing
    } else {
      std::free(ptr);
    }
  }

  // To minimize binary size of construct_node_at_end.
  __attribute__((noinline)) iterator finish_construct_node_at_end(Node* n) {
    link_nodes_at_back(n, n);
    count_++;
    if (map_ != nullptr) {
      return map_
          ->emplace(std::piecewise_construct,
                    std::forward_as_tuple(n->value.first),
                    std::forward_as_tuple(n))
          .first->second;
    } else {
      return iterator(n);
    }
  }

  template <class... Args>
  inline __attribute__((always_inline)) iterator construct_node_at_end(
      const Key& key, Args&&... args) {
    return finish_construct_node_at_end(
        new (alloc_node()) Node(key, std::forward<Args>(args)...));
  }

  template <class... Args>
  inline __attribute__((always_inline)) iterator construct_node_at_end(
      Key&& key, Args&&... args) {
    return finish_construct_node_at_end(
        new (alloc_node()) Node(std::move(key), std::forward<Args>(args)...));
  }

  void link_nodes_at_back(NodeBase* f, NodeBase* l) {
    l->next = end_as_link()->AsNode();
    f->prev = end_.prev;
    f->prev->next = f->AsNode();
    end_.prev = l->AsNode();
  }

  void unlink_nodes(NodeBase* f, NodeBase* l) {
    f->prev->next = l->next;
    l->next->prev = f->prev;
  }

  const NodeBase* end_as_link() const { return &end_; }

  NodeBase* end_as_link() { return &end_; }

  iterator erase_node(const_iterator pos) {
    auto n = pos.ptr;
    auto r = n->next;
    unlink_nodes(n, n);
    auto nptr = n->AsNode();
    nptr->~Node();
    free_node(nptr);
    count_--;
    if (empty()) {
      // All nodes removed, reset pool cursor so that pool can be reused or
      // reserved with larger size.
      pool_cursor_ = 0;
      is_perfect_ = true;
    } else {
      is_perfect_ = false;
    }
    return iterator(r);
  }

  void list_clear() {
    if (is_perfect_) {
      // Fast path
      for (size_t i = 0; i < count_; i++) {
        pool_[i].~Node();
      }
      end_.Reset();
    } else if (!empty()) {
      auto f = end_.next;
      auto l = end_as_link();
      unlink_nodes(f, l->prev);
      while (f != l) {
        auto n = f->next;
        auto nptr = f->AsNode();
        nptr->~Node();
        free_node(nptr);
        f = n;
      }
    }
  }
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_LINKED_HASH_MAP_H_
