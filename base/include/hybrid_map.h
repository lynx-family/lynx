// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_HYBRID_MAP_H_
#define BASE_INCLUDE_HYBRID_MAP_H_

#include <algorithm>
#include <functional>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

#ifdef _MSC_VER
#define HYBRID_MAP_UNLIKELY(x) x
#define HYBRID_MAP_LIKELY(x) x

#define HYBRID_MAP_INLINE inline __forceinline
#define HYBRID_MAP_NEVER_INLINE __declspec(noinline)
#else
#define HYBRID_MAP_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define HYBRID_MAP_LIKELY(x) __builtin_expect(!!(x), 1)

#define HYBRID_MAP_INLINE inline __attribute__((always_inline))
#define HYBRID_MAP_NEVER_INLINE __attribute__((noinline))
#endif

namespace lynx {
namespace base {

/// Checks if map type T has a method with name "reserve".
template <typename T, typename = void>
struct has_reserve_method : std::false_type {};

template <typename T>
struct has_reserve_method<T, std::void_t<decltype(std::declval<T&>().reserve(
                                 std::declval<size_t>()))>> : std::true_type {};

template <typename T>
inline constexpr bool has_reserve_method_v = has_reserve_method<T>::value;

/// Checks if map type T has a method with name "contains".
template <typename T, typename = void>
struct has_contains_method : std::false_type {};

template <typename T>
struct has_contains_method<T, std::void_t<decltype(std::declval<T&>().contains(
                                  std::declval<typename T::key_type>()))>>
    : std::true_type {};

template <typename T>
inline constexpr bool has_contains_method_v = has_contains_method<T>::value;

/// Checks if map type T has a method with name "for_each".
template <typename T, typename Callback, typename = void>
struct has_for_each_method : std::false_type {};

template <typename T, typename Callback>
struct has_for_each_method<T, Callback,
                           std::void_t<decltype(std::declval<T&>().for_each(
                               std::declval<Callback>()))>> : std::true_type {};

template <typename T, typename Callback>
inline constexpr bool has_for_each_method_v =
    has_for_each_method<T, Callback>::value;

/// Maps store `std::pair<const Key, T>` by default. This default implementation
/// of KeyExtractor extracts the Key from the pair instance.
struct KeyExtractor {
  template <typename V>
  const auto& operator()(V&& v) {
    return v.first;
  }
};

/// Generic map policy which only accepts additional arguments of types.
/// Usage: using ReverseMapPolicy = MapPolicy<std::map, std::greater<int>>;
template <template <typename...> class T, typename... Args>
struct MapPolicy {
  template <typename Key, typename Value>
  using type = T<Key, Value, Args...>;
};

/// The default logic of migrating data from small map to big map and destruct
/// the small map afterwards. This transfer policy will not return pointer to
/// the last transferred value.
struct DefaultTransferPolicy {
  template <typename SmallMap, typename BigMap>
  void operator()(SmallMap& small_map, BigMap& big_map) {
    for (auto& p : small_map) {
      big_map.emplace(
          std::move(const_cast<typename SmallMap::key_type&>(p.first)),
          std::move(p.second));
    }
  }
};

/// HybridMap discourages the use of iterators because using a unified iterator
/// to encapsulate iterators for different map types incurs additional overhead,
/// such as requiring additional fields to record the iterator type and checking
/// the iterator type each time data is retrieved. By default, only the
/// following iterator implementations are provided. You can implement more
/// efficient iterators yourself.
template <typename Key, typename T, typename SmallMapPolicy,
          typename BigMapPolicy>
struct DefaultIteratorPolicy {
  using small_map_type = typename SmallMapPolicy::template type<Key, T>;
  using big_map_type = typename BigMapPolicy::template type<Key, T>;

  static constexpr auto isomorphic_value_type =
      std::is_same_v<typename small_map_type::value_type,
                     typename big_map_type::value_type>;

  static constexpr auto using_same_iterator =
      std::is_same_v<typename small_map_type::iterator,
                     typename big_map_type::iterator>;

  using value_type = typename small_map_type::value_type;

  template <bool Const>
  struct Iterator {
    using small_map_iterator =
        std::conditional_t<Const, typename small_map_type::const_iterator,
                           typename small_map_type::iterator>;
    using big_map_iterator =
        std::conditional_t<Const, typename big_map_type::const_iterator,
                           typename big_map_type::iterator>;

    using pointer = std::conditional_t<Const, const value_type*, value_type*>;
    using reference = std::conditional_t<Const, const value_type&, value_type&>;

    Iterator(small_map_iterator it)
        : using_small_map_(true), small_map_it_(it) {}
    Iterator(big_map_iterator it) : using_small_map_(false), big_map_it_(it) {}

    explicit operator small_map_iterator() const { return small_map_it_; }
    explicit operator big_map_iterator() const { return big_map_it_; }

    reference operator*() const {
      return using_small_map_ ? *small_map_it_ : *big_map_it_;
    }

    pointer operator->() const {
      return using_small_map_ ? &(*small_map_it_) : &(*big_map_it_);
    }

    Iterator& operator++() {
      if (using_small_map_) {
        small_map_it_++;
      } else {
        big_map_it_++;
      }
      return *this;
    }

    Iterator operator++(int) {
      Iterator t(*this);
      ++(*this);
      return t;
    }

    friend bool operator==(const Iterator& x, const Iterator& y) {
      if (x.using_small_map_) {
        return y.using_small_map_ && x.small_map_it_ == y.small_map_it_;
      } else {
        return !y.using_small_map_ && x.big_map_it_ == y.big_map_it_;
      }
    }

    friend bool operator!=(const Iterator& x, const Iterator& y) {
      return !(x == y);
    }

   private:
    bool using_small_map_;
    union {
      small_map_iterator small_map_it_;
      big_map_iterator big_map_it_;
    };
  };

  // DefaultIteratorPolicy only supports isomorphic map types.
  using iterator = std::conditional_t<
      isomorphic_value_type,
      std::conditional_t<using_same_iterator, typename small_map_type::iterator,
                         Iterator<false>>,
      void>;
  using const_iterator = std::conditional_t<
      isomorphic_value_type,
      std::conditional_t<using_same_iterator,
                         typename small_map_type::const_iterator,
                         Iterator<true>>,
      void>;
};

/**
 * About the design ideas of HybridMap.
 * 1. Discourage iterator because iterators compatible with different maps will
 * inevitably incur performance overhead of frequently checking if it is of
 * small map or big map. During testing, it was found that this part of the
 * overhead cannot be ignored. Use `for_each` to iterate elements. You can still
 * wrap iterators outside using `using_small_map()`, `small_map()` and
 * `big_map()` methods or provide `IteratorPolicy` template argument such as
 * `DefaultIteratorPolicy`.
 * 2. `std::variant` is not used to manage different maps. Some additional
 * performance overhead cannot be avoided. For example, you must first use
 * `std::holds_alternative` to determine the type of the map, and then use
 * `std::get`, but the internal implementation of `std::get` must determine the
 * type again.
 *
 * `TransferPolicy`: a callable object providing custom migration logic of data
 * from small map to big map. If not provided DefaultTransferPolicy would be
 * used. If the `()` operator returns a pointer to mapped_type, it would be
 * treated as the last inserted value which triggers map data transfer.
 */
template <typename Key, typename T, size_t MaxSmallMapSize,
          typename SmallMapPolicy, typename BigMapPolicy,
          typename TransferPolicy = DefaultTransferPolicy,
          typename IteratorPolicy =
              DefaultIteratorPolicy<Key, T, SmallMapPolicy, BigMapPolicy>>
class HybridMap {
 public:
  // Use the policies to generate the actual map types.
  using small_map_type = typename SmallMapPolicy::template type<Key, T>;
  using big_map_type = typename BigMapPolicy::template type<Key, T>;

  static_assert(
      std::is_same_v<typename small_map_type::key_type,
                     typename big_map_type::key_type> &&
          std::is_same_v<typename small_map_type::mapped_type,
                         typename big_map_type::mapped_type>,
      "Requires map types have identical internal defined value types.");

  using size_type = size_t;
  using key_type = typename small_map_type::key_type;        // Key
  using mapped_type = typename small_map_type::mapped_type;  // T

  // The purpose of defining `value_type` is to provide the constructor
  // `HybridMap(std::initializer_list<value_type>)`.
  // Doesn't mean that both SmallMap and BigMap must use the same value_type.
  using value_type = std::pair<const Key, T>;

  using iterator = typename IteratorPolicy::iterator;
  using const_iterator = typename IteratorPolicy::const_iterator;

  // If TransferPolicy returns pointer to last inserted value which caused
  // map data transfer, we can use the returned pointer for optimization.
  static constexpr auto TransferPolicyReturnsPointer =
      std::is_same_v<std::remove_const_t<std::invoke_result_t<
                         TransferPolicy, small_map_type&, big_map_type&>>,
                     mapped_type*>;

 public:
  HybridMap() : using_small_map_(true), small_map_() {}
  ~HybridMap() {
    using_small_map_ ? small_map_.~small_map_type() : big_map_.~big_map_type();
  }

  HybridMap(std::initializer_list<value_type> initial_list) {
    if (initial_list.size() <= MaxSmallMapSize) {
      using_small_map_ = true;
      new (&small_map_) small_map_type(std::move(initial_list));
    } else {
      using_small_map_ = false;
      new (&big_map_) big_map_type(std::move(initial_list));
    }
  }

  HybridMap(const HybridMap& other) {
    if (other.using_small_map_) {
      using_small_map_ = true;
      new (&small_map_) small_map_type(other.small_map_);
    } else {
      using_small_map_ = false;
      new (&big_map_) big_map_type(other.big_map_);
    }
  }

  HybridMap(HybridMap&& other) {
    if (other.using_small_map_) {
      using_small_map_ = true;
      new (&small_map_) small_map_type(std::move(other.small_map_));
    } else {
      using_small_map_ = false;
      new (&big_map_) big_map_type(std::move(other.big_map_));
    }
  }

  HybridMap& operator=(const HybridMap& other) {
    if (this == &other) {
      return *this;
    }
    if (using_small_map_) {
      if (other.using_small_map_) {
        small_map_ = other.small_map_;
      } else {
        small_map_.~small_map_type();
        new (&big_map_) big_map_type(other.big_map_);
        using_small_map_ = false;
      }
    } else {
      if (other.using_small_map_) {
        big_map_.~big_map_type();
        new (&small_map_) small_map_type(other.small_map_);
        using_small_map_ = true;
      } else {
        big_map_ = other.big_map_;
      }
    }
    return *this;
  }

  HybridMap& operator=(HybridMap&& other) {
    if (this == &other) {
      return *this;
    }
    if (using_small_map_) {
      if (other.using_small_map_) {
        small_map_ = std::move(other.small_map_);
      } else {
        small_map_.~small_map_type();
        new (&big_map_) big_map_type(std::move(other.big_map_));
        using_small_map_ = false;
      }
    } else {
      if (other.using_small_map_) {
        big_map_.~big_map_type();
        new (&small_map_) small_map_type(std::move(other.small_map_));
        using_small_map_ = true;
      } else {
        big_map_ = std::move(other.big_map_);
      }
    }
    return *this;
  }

  bool using_small_map() const { return using_small_map_; }

  const auto& small_map() const { return small_map_; }
  auto& small_map() { return small_map_; }
  const auto& big_map() const { return big_map_; }
  auto& big_map() { return big_map_; }

  size_t size() const {
    return HYBRID_MAP_LIKELY(using_small_map_) ? small_map_.size()
                                               : big_map_.size();
  }

  bool empty() const {
    return HYBRID_MAP_LIKELY(using_small_map_) ? small_map_.empty()
                                               : big_map_.empty();
  }

  void clear() {
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      small_map_.clear();
    } else {
      big_map_.~big_map_type();
      new (&small_map_) small_map_type();
      using_small_map_ = true;
    }
  }

  void reserve(size_t count) {
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      if (count > MaxSmallMapSize) {
        _transfer_reserve(count);
      } else if constexpr (has_reserve_method_v<small_map_type>) {
        small_map_.reserve(count);
      }
    } else if constexpr (has_reserve_method_v<big_map_type>) {
      big_map_.reserve(count);
    }
  }

  size_t erase(const Key& key) {
    return HYBRID_MAP_LIKELY(using_small_map_) ? small_map_.erase(key)
                                               : big_map_.erase(key);
  }

  /// Different with other maps, the result is the pointer of data value in this
  /// map corresponding to key. DO NOT cache the pointer if either small map or
  /// big map is not node based.
  /// @return nullptr if key not found in map.
  T* find(const Key& key) {
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      auto it = small_map_.find(key);
      return it != small_map_.end() ? &it->second : nullptr;
    } else {
      auto it = big_map_.find(key);
      return it != big_map_.end() ? &it->second : nullptr;
    }
  }

  const T* find(const Key& key) const {
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      auto it = small_map_.find(key);
      return it != small_map_.end() ? &it->second : nullptr;
    } else {
      auto it = big_map_.find(key);
      return it != big_map_.end() ? &it->second : nullptr;
    }
  }

  bool contains(const Key& key) const {
    // This implementation is faster than `return find(key) != nullptr;` with
    // less comparisons.
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      if constexpr (has_contains_method_v<small_map_type>) {
        return small_map_.contains(key);
      } else {
        return small_map_.find(key) != small_map_.end();
      }
    } else {
      if constexpr (has_contains_method_v<big_map_type>) {
        return big_map_.contains(key);
      } else {
        return big_map_.find(key) != big_map_.end();
      }
    }
  }

  size_t count(const Key& key) const { return contains(key) ? 1 : 0; }

  T& operator[](const Key& key) {
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      // [] will insert default constructed value and may trigger data transfer.
      auto res = small_map_.try_emplace(key);
      if (res.second) {
        // Insertion took place, may need to transfer data to big map.
        if (HYBRID_MAP_UNLIKELY(small_map_.size() > MaxSmallMapSize)) {
          if constexpr (TransferPolicyReturnsPointer) {
            // Fast path, result of _transfer() is the last inserted value in
            // big map.
            return *_transfer();
          } else {
            // Slow path, must find inserted value in big map again.
            _transfer();
            return big_map_.find(key)->second;
          }
        }
      }
      return res.first->second;
    } else {
      return big_map_[key];
    }
  }

  T& operator[](Key&& key) {
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      // [] will insert default constructed value and may trigger data transfer.
      auto res = small_map_.try_emplace(std::move(key));
      if (res.second) {
        // Insertion took place, may need to transfer data to big map.
        if (HYBRID_MAP_UNLIKELY(small_map_.size() > MaxSmallMapSize)) {
          if constexpr (TransferPolicyReturnsPointer) {
            // Fast path, result of _transfer() is the last inserted value in
            // big map.
            return *_transfer();
          } else {
            // Slow path, must find inserted value in big map again.
            auto key_copy = res.first->first;
            _transfer();
            return big_map_.find(key_copy)->second;
          }
        }
      }
      return res.first->second;
    } else {
      return big_map_[std::move(key)];
    }
  }

  T& at(const Key& key) {
    return HYBRID_MAP_LIKELY(using_small_map_) ? small_map_.at(key)
                                               : big_map_.at(key);
  }

  const T& at(const Key& key) const {
    return HYBRID_MAP_LIKELY(using_small_map_) ? small_map_.at(key)
                                               : big_map_.at(key);
  }

  /// Different with other maps, the result is the pointer of data value in this
  /// map corresponding to key and a flag telling if insertion took place. DO
  /// NOT cache the pointer if either small map or big map is not node based.
  template <class V>
  std::pair<T*, bool> insert_or_assign(const Key& key, V&& value) {
    std::pair<T*, bool> result;
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      auto res = small_map_.insert_or_assign(key, std::forward<V>(value));
      result = {&res.first->second, res.second};
      if (result.second) {
        // Insertion took place, may need to transfer data to big map.
        if (HYBRID_MAP_UNLIKELY(small_map_.size() > MaxSmallMapSize)) {
          if constexpr (TransferPolicyReturnsPointer) {
            // Fast path, result of _transfer() is the last inserted value in
            // big map.
            result.first = _transfer();
          } else {
            // Slow path, must find inserted value in big map again.
            _transfer();
            result.first = &big_map_.find(key)->second;
          }
        }
      }
    } else {
      auto res = big_map_.insert_or_assign(key, std::forward<V>(value));
      result = {&res.first->second, res.second};
    }
    return result;
  }

  template <class V>
  std::pair<T*, bool> insert_or_assign(Key&& key, V&& value) {
    std::pair<T*, bool> result;
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      auto res =
          small_map_.insert_or_assign(std::move(key), std::forward<V>(value));
      result = {&res.first->second, res.second};
      if (result.second) {
        // Insertion took place, may need to transfer data to big map.
        if (HYBRID_MAP_UNLIKELY(small_map_.size() > MaxSmallMapSize)) {
          if constexpr (TransferPolicyReturnsPointer) {
            // Fast path, result of _transfer() is the last inserted value in
            // big map.
            result.first = _transfer();
          } else {
            // Slow path, must find inserted value in big map again. Must copy
            // key beforehand becuase argument `key` is moved and `res` is
            // invalid after transferring.
            auto key_copy = res.first->first;
            _transfer();
            result.first = &big_map_.find(key_copy)->second;
          }
        }
      }
    } else {
      auto res =
          big_map_.insert_or_assign(std::move(key), std::forward<V>(value));
      result = {&res.first->second, res.second};
    }
    return result;
  }

  template <class V>
  std::pair<T*, bool> insert(const Key& key, V&& value) {
    return emplace(key, std::forward<V>(value));
  }

  template <class V>
  std::pair<T*, bool> insert(Key&& key, V&& value) {
    return emplace(std::move(key), std::forward<V>(value));
  }

  template <class... Args>
  std::pair<T*, bool> emplace(const Key& key, Args&&... args) {
    std::pair<T*, bool> result;
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      auto res = small_map_.try_emplace(key, std::forward<Args>(args)...);
      result = {&res.first->second, res.second};
      if (result.second) {
        // Insertion took place, may need to transfer data to big map.
        if (HYBRID_MAP_UNLIKELY(small_map_.size() > MaxSmallMapSize)) {
          if constexpr (TransferPolicyReturnsPointer) {
            // Fast path, result of _transfer() is the last inserted value in
            // big map.
            result.first = _transfer();
          } else {
            // Slow path, must find inserted value in big map again.
            _transfer();
            result.first = &big_map_.find(key)->second;
          }
        }
      }
    } else {
      auto res = big_map_.try_emplace(key, std::forward<Args>(args)...);
      result = {&res.first->second, res.second};
    }
    return result;
  }

  template <class... Args>
  std::pair<T*, bool> emplace(Key&& key, Args&&... args) {
    std::pair<T*, bool> result;
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      auto res =
          small_map_.try_emplace(std::move(key), std::forward<Args>(args)...);
      result = {&res.first->second, res.second};
      if (result.second) {
        // Insertion took place, may need to transfer data to big map.
        if (HYBRID_MAP_UNLIKELY(small_map_.size() > MaxSmallMapSize)) {
          if constexpr (TransferPolicyReturnsPointer) {
            // Fast path, result of _transfer() is the last inserted value in
            // big map.
            result.first = _transfer();
          } else {
            // Slow path, must find inserted value in big map again. Must copy
            // key beforehand becuase argument `key` is moved and `res` is
            // invalid after transferring.
            auto key_copy = res.first->first;
            _transfer();
            result.first = &big_map_.find(key_copy)->second;
          }
        }
      }
    } else {
      auto res =
          big_map_.try_emplace(std::move(key), std::forward<Args>(args)...);
      result = {&res.first->second, res.second};
    }
    return result;
  }

  template <class U, class... Args>
  std::pair<T*, bool> try_emplace(U&& key, Args&&... args) {
    return emplace(std::forward<U>(key), std::forward<Args>(args)...);
  }

  template <typename... Args1, typename... Args2>
  std::pair<T*, bool> emplace(std::piecewise_construct_t,
                              std::tuple<Args1...> args1,
                              std::tuple<Args2...> args2) {
    std::pair<T*, bool> result;
    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      auto res = small_map_.emplace(std::piecewise_construct, std::move(args1),
                                    std::move(args2));
      result = {&res.first->second, res.second};
      if (result.second) {
        // Insertion took place, may need to transfer data to big map.
        if (HYBRID_MAP_UNLIKELY(small_map_.size() > MaxSmallMapSize)) {
          if constexpr (TransferPolicyReturnsPointer) {
            // Fast path, result of _transfer() is the last inserted value in
            // big map.
            result.first = _transfer();
          } else {
            // Slow path, must find inserted value in big map again. Must copy
            // key beforehand becuase argument `key` is moved and `res` is
            // invalid after transferring.
            auto key_copy = res.first->first;
            _transfer();
            result.first = &big_map_.find(key_copy)->second;
          }
        }
      }
    } else {
      auto res = big_map_.emplace(std::piecewise_construct, std::move(args1),
                                  std::move(args2));
      result = {&res.first->second, res.second};
    }
    return result;
  }

  /**
   * @brief Iterates over each key-value pair in the map and applies a
   * user-provided callback.
   *
   * This method supports two types of callbacks for maximum flexibility:
   * 1.  **Callback with `void` return type**: The function will be executed for
   * every element in the map. Signature: `void(const K& key, const T& value)`
   *
   * 2.  **Callback with `bool` return type**: This allows for early termination
   * of the loop. If the callback returns `true`, the iteration stops
   * immediately. If it returns `false`, the iteration continues to the next
   * element. Signature: `bool(const K& key, const T& value)`
   *
   * @tparam Callback The type of the callback function or callable object
   * (e.g., a lambda).
   *
   * @param callback The callable object to be invoked for each key-value pair.
   * It will be perfectly forwarded.
   *
   * @note This method is enabled only if the provided `Callback` is invocable
   * with arguments of type `const K&` and `const T&`.
   * @note The method uses `if constexpr` to create a zero-overhead abstraction,
   * ensuring that the check for a boolean return value happens at compile-time
   * with no runtime cost.
   *
   * @code
   * MyMap<std::string, int> map = {{"a", 1}, {"b", 2}};
   *
   * // Example 1: Print all elements (void return).
   * map.for_each([](const std::string& key, int value) {
   *   std::cout << key << ": " << value << std::endl;
   * });
   *
   * // Example 2: Find an element and stop (bool return).
   * map.for_each([](const std::string& key, int value) -> bool {
   *   if (key == "b") {
   *     std::cout << "Found 'b'!" << std::endl;
   *     return true; // Stop iterating
   *   }
   *   return false; // Continue iterating
   * });
   * @endcode
   */
  template <typename Callback, typename = std::enable_if_t<std::is_invocable_v<
                                   Callback, const Key&, const T&>>>
  void for_each(Callback&& callback) const {
    using callback_ret_type =
        std::invoke_result_t<Callback, const Key&, const T&>;
    constexpr bool callback_returns_bool =
        std::is_same_v<callback_ret_type, bool>;

    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      if constexpr (has_for_each_method_v<small_map_type, Callback>) {
        small_map_.for_each(std::forward<Callback>(callback));
      } else {
        if constexpr (callback_returns_bool) {
          for (auto& p : small_map_) {
            if (callback(p.first, p.second)) {
              break;
            }
          }
        } else {
          for (auto& p : small_map_) {
            callback(p.first, p.second);
          }
        }
      }
    } else {
      if constexpr (has_for_each_method_v<big_map_type, Callback>) {
        big_map_.for_each(std::forward<Callback>(callback));
      } else {
        if constexpr (callback_returns_bool) {
          for (auto& p : big_map_) {
            if (callback(p.first, p.second)) {
              break;
            }
          }
        } else {
          for (auto& p : big_map_) {
            callback(p.first, p.second);
          }
        }
      }
    }
  }

  template <typename Callback,
            typename =
                std::enable_if_t<std::is_invocable_v<Callback, const Key&, T&>>>
  void for_each(Callback&& callback) {
    using callback_ret_type = std::invoke_result_t<Callback, const Key&, T&>;
    constexpr bool callback_returns_bool =
        std::is_same_v<callback_ret_type, bool>;

    if (HYBRID_MAP_LIKELY(using_small_map_)) {
      if constexpr (has_for_each_method_v<small_map_type, Callback>) {
        small_map_.for_each(std::forward<Callback>(callback));
      } else {
        if constexpr (callback_returns_bool) {
          for (auto& p : small_map_) {
            if (callback(p.first, p.second)) {
              break;
            }
          }
        } else {
          for (auto& p : small_map_) {
            callback(p.first, p.second);
          }
        }
      }
    } else {
      if constexpr (has_for_each_method_v<big_map_type, Callback>) {
        big_map_.for_each(std::forward<Callback>(callback));
      } else {
        if constexpr (callback_returns_bool) {
          for (auto& p : big_map_) {
            if (callback(p.first, p.second)) {
              break;
            }
          }
        } else {
          for (auto& p : big_map_) {
            callback(p.first, p.second);
          }
        }
      }
    }
  }

  const_iterator cbegin() const {
    return using_small_map_ ? const_iterator(small_map_.cbegin())
                            : const_iterator(big_map_.cbegin());
  }

  const_iterator cend() const {
    return using_small_map_ ? const_iterator(small_map_.cend())
                            : const_iterator(big_map_.cend());
  }

  iterator begin() {
    return using_small_map_ ? iterator(small_map_.begin())
                            : iterator(big_map_.begin());
  }

  iterator end() {
    return using_small_map_ ? iterator(small_map_.end())
                            : iterator(big_map_.end());
  }

  const_iterator begin() const {
    return using_small_map_ ? const_iterator(small_map_.begin())
                            : const_iterator(big_map_.begin());
  }

  const_iterator end() const {
    return using_small_map_ ? const_iterator(small_map_.end())
                            : const_iterator(big_map_.end());
  }

  const_iterator find_iterator(const Key& key) const {
    return using_small_map_ ? const_iterator(small_map_.find(key))
                            : const_iterator(big_map_.find(key));
  }

  iterator find_iterator(const Key& key) {
    return using_small_map_ ? iterator(small_map_.find(key))
                            : iterator(big_map_.find(key));
  }

  template <typename It,
            typename = std::enable_if_t<!std::is_void_v<It> &&
                                        (std::is_same_v<It, iterator> ||
                                         std::is_same_v<It, const_iterator>)>>
  iterator erase_iterator(It pos) {
    if constexpr (std::is_same_v<It, iterator>) {
      return using_small_map_
                 ? iterator(
                       small_map_.erase(typename small_map_type::iterator(pos)))
                 : iterator(
                       big_map_.erase(typename big_map_type::iterator(pos)));
    } else {
      return using_small_map_
                 ? iterator(small_map_.erase(
                       typename small_map_type::const_iterator(pos)))
                 : iterator(big_map_.erase(
                       typename big_map_type::const_iterator(pos)));
    }
  }

 private:
  std::conditional_t<TransferPolicyReturnsPointer, mapped_type*, void>
  _transfer() {
    big_map_type temp_big_map;
    if constexpr (has_reserve_method_v<big_map_type>) {
      temp_big_map.reserve(small_map_.size() +
                           2);  // size+2 for coming insertion
    }
    if constexpr (TransferPolicyReturnsPointer) {
      auto last_pointer = TransferPolicy()(small_map_, temp_big_map);
      small_map_.~small_map_type();
      new (&big_map_) big_map_type(std::move(temp_big_map));
      using_small_map_ = false;
      return last_pointer;
    } else {
      TransferPolicy()(small_map_, temp_big_map);
      small_map_.~small_map_type();
      new (&big_map_) big_map_type(std::move(temp_big_map));
      using_small_map_ = false;
    }
  }

  void _transfer_reserve(size_t size) {
    if (HYBRID_MAP_LIKELY(small_map_.empty())) {
      small_map_.~small_map_type();
      new (&big_map_) big_map_type();
      if constexpr (has_reserve_method_v<big_map_type>) {
        big_map_.reserve(size);
      }
    } else {
      big_map_type temp_big_map;
      if constexpr (has_reserve_method_v<big_map_type>) {
        temp_big_map.reserve(size);
      }

      TransferPolicy()(small_map_, temp_big_map);
      small_map_.~small_map_type();
      new (&big_map_) big_map_type(std::move(temp_big_map));
    }
    using_small_map_ = false;
  }

  bool using_small_map_;
  union {
    small_map_type small_map_;
    big_map_type big_map_;
  };
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_HYBRID_MAP_H_
