// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_VALUE_TABLE_H_
#define BASE_INCLUDE_VALUE_TABLE_H_

#include <algorithm>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "base/include/base_defines.h"
#include "base/include/base_export.h"
#include "base/include/boost/unordered.h"
#include "base/include/hybrid_map.h"
#include "base/include/type_traits_addon.h"
#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/value/ref_type.h"
#include "base/include/vector.h"

namespace lynx {
namespace lepus {

class BASE_EXPORT Dictionary : public RefCountedBase {
 public:
  // Stores up-to 4 key-value pairs on inline memory.
  static constexpr size_t kInlineStorageSize = 4;

  // Stores up-to 16 elements in small map.
  static constexpr size_t kSmallMapMaximumSize = 16;

  using MapValueType = std::pair<const base::String, Value>;

  // Use LinearFlatMap as small map type of final hybrid map.
  // 1. Use inline memory.
  // 2. Custom key policy which enables hash quick find instead of plain linear
  // find and use `base::String::EqualWhenHashEqual` as comparer when two
  // `base::String` objects are known to have the same hash values.
  using SmallMapPolicy =
      base::InlineFlatMapPolicy<base::InlineLinearFlatMap, kInlineStorageSize>;
  using SmallMapType =
      typename SmallMapPolicy::template type<base::String, Value>;

  static_assert(
      std::is_same_v<typename base::String::equal_when_hash_equal,
                     typename SmallMapType::key_policy::equal_when_hash_equal>,
      "Should use `base::String::equal_when_hash_equal` for best performance.");

  // Use boost::unordered_flat_map as big map type of final hybrid map.
  using BigMapPolicy =
      boost::MapPolicy<boost::unordered_flat_map, std::hash, std::equal_to>;
  using BigMapType = typename BigMapPolicy::template type<base::String, Value>;

  // Custom data transfer method from small map to big map. Utilize the
  // relocatable type-trait of key value types.
  struct PlainBytesTransferPolicy {
    template <typename SmallMap, typename BigMap>
    typename SmallMap::mapped_type* operator()(SmallMap& small_map,
                                               BigMap& big_map) {
      static_assert(
          base::IsTriviallyRelocatable<typename BigMap::key_type, false>::value,
          "Requires `base::String` to be trivially relocatable.");
      static_assert(base::IsTriviallyRelocatable<typename BigMap::mapped_type,
                                                 false>::value,
                    "Requires `lepus::Value` to be trivially relocatable.");

      typename SmallMap::mapped_type* last_value_ptr;
      small_map.for_each([&](const auto& key, auto& value) {
        // Treat big_map as a map containing plain bytes of key and value so
        // that we can do trivial copy and then ignores the destruction of
        // original objects.
        using PlainBytesBigMap =
            BigMapPolicy::plain_bytes_type<typename BigMap::key_type,
                                           typename BigMap::mapped_type>;
        using PlainBytesMappedType = typename PlainBytesBigMap::mapped_type;
        last_value_ptr =
            &big_map.try_emplace_plain_bytes_key(key).first->second;
        auto& target_value =
            reinterpret_cast<PlainBytesMappedType&>(*last_value_ptr);
        target_value = reinterpret_cast<const PlainBytesMappedType&>(value);
      });
      // Force set small_map_ as empty to skip destruction of objects.
      std::decay_t<decltype(small_map)>::Unsafe::SetSize(small_map, 0);
      return last_value_ptr;
    }
  };

  // HybridMap uses DefaultIteratorPolicy as default iterator implementation
  // which is not efficient. Given the types of small and big maps, we can
  // implement more efficient iterator types.
  struct IteratorPolicy {
    template <bool Const>
    struct Iterator {
     private:
      // We use iterator type of boost map(the big map) as a unified iterator
      // because it contains two pointer members and can be used to express
      // small map's iterator which is a single pointer.
      using UnderlyingIterator =
          std::conditional_t<Const, typename BigMapType::const_iterator,
                             typename BigMapType::iterator>;

      UnderlyingIterator it_;

     public:
      using SmallMapIterator =
          std::conditional_t<Const, typename SmallMapType::const_iterator,
                             typename SmallMapType::iterator>;
      using BigMapIterator = UnderlyingIterator;

      using Pointer =
          std::conditional_t<Const, const MapValueType*, MapValueType*>;
      using Reference =
          std::conditional_t<Const, const MapValueType&, MapValueType&>;

     public:
      Iterator(SmallMapIterator it) : it_(nullptr, it) {}
      Iterator(BigMapIterator it) : it_(it) {}

      explicit operator SmallMapIterator() const { return it_.inner_p(); }
      explicit operator BigMapIterator() const { return it_; }

      Reference operator*() const { return *it_; }

      Pointer operator->() const { return &(*it_); }

      Iterator& operator++() {
        if (HYBRID_MAP_LIKELY(it_.inner_pc() == nullptr)) {
          // for small map, just step element pointer
          it_.inner_p()++;
        } else {
          // for big map, call original increment
          it_++;
        }
        return *this;
      }

      Iterator operator++(int) {
        Iterator t(*this);
        ++(*this);
        return t;
      }

      friend bool operator==(const Iterator& x, const Iterator& y) {
        return x.it_ == y.it_;
      }

      friend bool operator!=(const Iterator& x, const Iterator& y) {
        return !(x == y);
      }
    };

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
  };

  using Map =
      base::HybridMap<base::String, Value, kSmallMapMaximumSize, SmallMapPolicy,
                      BigMapPolicy, PlainBytesTransferPolicy, IteratorPolicy>;

  static_assert(Map::TransferPolicyReturnsPointer,
                "Requires optimized transfer policy.");

  /// Use ValueWrapper as result of Dictionary's GetValue() method to
  /// reduce the chance that user cache pointer address of inner Value
  /// objects of the dictionary.
  struct ValueWrapper {
   public:
    explicit ValueWrapper(const Value* value) : value_(value) {}

    BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ValueWrapper);

    const Value& value() const { return *value_; }
    const Value& operator*() const { return *value_; }
    const Value* operator->() const { return value_; }
    const Value* get() const { return value_; }

    operator const Value&() const { return *value_; }

    explicit operator bool() const { return value_ != nullptr; }
    bool has_value() const { return value_ != nullptr; }

    // The following methods may not cover all those of lepus::Value.
    // Add missing ones if you need, or you should use '->'.
    ValueType Type() const { return value_->Type(); }
    bool IsCDate() const { return value_->IsCDate(); }
    bool IsRegExp() const { return value_->IsRegExp(); }
    bool IsClosure() const { return value_->IsClosure(); }
    bool IsCallable() const { return value_->IsCallable(); }
    bool IsReference() const { return value_->IsReference(); }
    bool IsBool() const { return value_->IsBool(); }
    bool IsString() const { return value_->IsString(); }
    bool IsInt64() const { return value_->IsInt64(); }
    bool IsNumber() const { return value_->IsNumber(); }
    bool IsDouble() const { return value_->IsDouble(); }
    bool IsArray() const { return value_->IsArray(); }
    bool IsTable() const { return value_->IsTable(); }
    bool IsObject() const { return value_->IsObject(); }
    bool IsArrayOrJSArray() const { return value_->IsArrayOrJSArray(); }
    bool IsCPointer() const { return value_->IsCPointer(); }
    bool IsRefCounted() const { return value_->IsRefCounted(); }
    bool IsInt32() const { return value_->IsInt32(); }
    bool IsUInt32() const { return value_->IsUInt32(); }
    bool IsUInt64() const { return value_->IsUInt64(); }
    bool IsNil() const { return value_->IsNil(); }
    bool IsUndefined() const { return value_->IsUndefined(); }
    bool IsCFunction() const { return value_->IsCFunction(); }
    bool IsJSObject() const { return value_->IsJSObject(); }
    bool IsByteArray() const { return value_->IsByteArray(); }
    bool IsNaN() const { return value_->IsNaN(); }
    bool IsJSValue() const { return value_->IsJSValue(); }
    bool IsJSCPointer() const { return value_->IsJSCPointer(); }
    bool IsJSArray() const { return value_->IsJSArray(); }
    bool IsJSTable() const { return value_->IsJSTable(); }
    bool IsJSBool() const { return value_->IsJSBool(); }
    bool LEPUSBool() const { return value_->LEPUSBool(); }
    bool IsJSString() const { return value_->IsJSString(); }
    bool IsJSUndefined() const { return value_->IsJSUndefined(); }
    bool IsJSNumber() const { return value_->IsJSNumber(); }
    bool IsJsNull() const { return value_->IsJsNull(); }
    double LEPUSNumber() const { return value_->LEPUSNumber(); }
    bool IsJSInteger() const { return value_->IsJSInteger(); }
    bool IsJSFunction() const { return value_->IsJSFunction(); }
    int GetJSLength() const { return value_->GetJSLength(); }
    bool IsJSFalse() const { return value_->IsJSFalse(); }
    int64_t JSInteger() const { return value_->JSInteger(); }
    std::string ToString() const { return value_->ToString(); }
    bool IsTrue() const { return value_->IsTrue(); }
    bool IsFalse() const { return value_->IsFalse(); }
    bool IsEmpty() const { return value_->IsEmpty(); }
    bool IsEqual(const Value& value) const { return value_->IsEqual(value); }
    bool Bool() const { return value_->Bool(); }
    double Double() const { return value_->Double(); }
    int32_t Int32() const { return value_->Int32(); }
    uint32_t UInt32() const { return value_->UInt32(); }
    int64_t Int64() const { return value_->Int64(); }
    uint64_t UInt64() const { return value_->UInt64(); }
    double Number() const { return value_->Number(); }
    base::String String() const { return value_->String(); }
    std::string_view StringView() const { return value_->StringView(); }
    const char* CString() const { return value_->CString(); }
    const std::string& StdString() const { return value_->StdString(); }
    fml::WeakRefPtr<CArray> Array() const { return value_->Array(); }
    fml::WeakRefPtr<Dictionary> Table() const { return value_->Table(); }
    CFunction Function() const { return value_->Function(); }
    void* CPoint() const { return value_->CPoint(); }
    void* LEPUSCPointer() const { return value_->LEPUSCPointer(); }
    fml::WeakRefPtr<class RefCounted> RefCounted() const {
      return value_->RefCounted();
    }
    Value GetProperty(uint32_t idx) const { return value_->GetProperty(idx); }
    Value GetProperty(const base::String& key) const {
      return value_->GetProperty(key);
    }
    int GetLength() const { return value_->GetLength(); }
    bool Contains(const base::String& key) const {
      return value_->Contains(key);
    }

   private:
    const Value* value_;
  };

  class Unsafe {
   public:
    /// These methods are usually used to copy data between dictionaries. When
    /// it is known that the data to be added will not have duplicate keys,
    /// calling these methods can complete the construction of the entire
    /// dictionary more quickly. Implementation notes:
    /// 1. Using a lot of method overloading instead of templat<K, V> and
    /// `std::forward` can reduce binary size expansion. The cost is to use the
    /// move semantics of `Value` instead of true in-place construction from
    /// arguments.
    /// 2. When using big-map, call `SetValue` directly to avoid specializing
    /// too many big-map's `emplace` and related methods. The cost is that
    /// `SetValue` will finally check again whether it is a big-map or a
    /// small-map.
    static Value& SetValueUniqueKey(Dictionary& target,
                                    const base::String& key) {
      // Default construct value for key. This is to optimize table decoding.
      auto& map = target.map_;
      if (HYBRID_MAP_LIKELY(map.using_small_map())) {
        return map.small_map().emplace_unique(key)->second;
      } else {
        return const_cast<Value&>(*target.SetValue(key));
      }
    }

    static Value& SetValueUniqueKey(Dictionary& target, base::String&& key) {
      // Default construct value for key. This is to optimize table decoding.
      auto& map = target.map_;
      if (HYBRID_MAP_LIKELY(map.using_small_map())) {
        return map.small_map().emplace_unique(std::move(key))->second;
      } else {
        return const_cast<Value&>(*target.SetValue(std::move(key)));
      }
    }

    static void SetValueUniqueKey(Dictionary& target, const base::String& key,
                                  const Value& value) {
      auto& map = target.map_;
      if (HYBRID_MAP_LIKELY(map.using_small_map())) {
        map.small_map().emplace_unique(key, value);
      } else {
        target.SetValue(key, value);
      }
    }

    static void SetValueUniqueKey(Dictionary& target, const base::String& key,
                                  Value&& value) {
      auto& map = target.map_;
      if (HYBRID_MAP_LIKELY(map.using_small_map())) {
        map.small_map().emplace_unique(key, std::move(value));
      } else {
        target.SetValue(key, std::move(value));
      }
    }

    static void SetValueUniqueKey(Dictionary& target, base::String&& key,
                                  const Value& value) {
      auto& map = target.map_;
      if (HYBRID_MAP_LIKELY(map.using_small_map())) {
        map.small_map().emplace_unique(std::move(key), value);
      } else {
        target.SetValue(std::move(key), value);
      }
    }

    static void SetValueUniqueKey(Dictionary& target, base::String&& key,
                                  Value&& value) {
      auto& map = target.map_;
      if (HYBRID_MAP_LIKELY(map.using_small_map())) {
        map.small_map().emplace_unique(std::move(key), std::move(value));
      } else {
        target.SetValue(std::move(key), std::move(value));
      }
    }

    static BASE_INLINE Dictionary* RawCreate() { return new Dictionary(); }
  };

  friend class Unsafe;

 public:
  static fml::RefPtr<Dictionary> Create() {
    return fml::AdoptRef<Dictionary>(new Dictionary());
  }
  static fml::RefPtr<Dictionary> Create(
      std::initializer_list<MapValueType>&& data) {
    return fml::AdoptRef<Dictionary>(new Dictionary(std::move(data)));
  }
  ~Dictionary() = default;

  RefType GetRefType() const override { return RefType::kLepusTable; }

  ///  @note Why this method is implemented like this.
  ///
  ///  The most primitive and intuitive code to implement this method is as
  ///  follows.
  ///
  ///      map_[key] = Value(std::forward<Args>(args)...);
  ///
  ///  or
  ///
  ///      if (auto result = map_.try_emplace(key,
  ///         std::forward<Args>(args)...); !result.second) {
  ///         result.first->second = Value(std::forward<Args>(args)...);
  ///      }
  ///
  ///  But unfortunately, the first way is not optimal in performance which may
  ///  bring default construction and move assignment of Value.
  ///
  ///  The second way is better in performance but will cause severely binary
  ///  expansion for template specialization of try_emplace() method.
  ///
  ///  @return ValueWrapper to Value if success or nullptr on failure.
  template <class... Args>
  ValueWrapper SetValue(const base::String& key, Args&&... args) {
    if (HYBRID_MAP_UNLIKELY(IsConstLog())) {
      return ValueWrapper(nullptr);
    }

    // Possible that input args is a single 'Value' instance happens to be
    // exactly the same instance of exsiting one.
    // Like `table->SetValue(key, *table->GetValue(key))`.
    if constexpr (sizeof...(Args) == 1) {
      using single_arg_t = std::remove_cv_t<std::remove_reference_t<
          std::tuple_element_t<0, std::tuple<Args...>>>>;
      if constexpr (std::is_same_v<single_arg_t, Value>) {
        auto [target_ptr, inserted] =
            map_.try_emplace(key, Value::kUnsafeCreateAsUninitialized);
        if (HYBRID_MAP_UNLIKELY(!inserted)) {
          if (HYBRID_MAP_UNLIKELY(target_ptr ==
                                  &std::get<0>(std::tie(args...)))) {
            return ValueWrapper(target_ptr);  // Do not override.
          }
          target_ptr->~Value();
        }
        // Placement new Value() on target_ptr with variadic args.
        return ValueWrapper(new (target_ptr)
                                Value(std::forward<Args>(args)...));
      }
    }

    // Other cases of args, calling the public method without inlining can
    // optimize binary size, but with a slight performance loss.
    return ValueWrapper(new (SetValuePrepare(key))
                            Value(std::forward<Args>(args)...));
  }

  /// Return a default nil value if key not found.
  ValueWrapper GetValue(const base::String& key) const;

  /// Return a default undefined value if key not found.
  ValueWrapper GetValueOrUndefined(const base::String& key) const;

  /// Return a nullable ValueWrapper and you should check before use.
  ValueWrapper GetValueOrNull(const base::String& key) const;

  /// Return false if the dictionary is const, or always true.
  bool Erase(const base::String& key);

  /// Return -1 if the dictionary is const, or returns number of elements
  /// erased(0 or 1).
  int32_t EraseKey(const base::String& key);

  bool Contains(const base::String& key) const {
    return map_.find(key) != nullptr;
  }

  auto find(const base::String& key) const { return map_.find_iterator(key); }

  auto find(const base::String& key) { return map_.find_iterator(key); }

  size_t size() const { return map_.size(); }
  bool empty() const { return size() == 0; }

  void reserve(size_t count) { map_.reserve(count); }

  template <typename Callback>
  void for_each(Callback&& callback) {
    map_.for_each(std::forward<Callback>(callback));
  }

  template <typename Callback>
  void for_each(Callback&& callback) const {
    map_.for_each(std::forward<Callback>(callback));
  }

  /// @note Do not cache pointer to value using `&(it->second)`
  /// to other variables. The underlying map does not guarantee pointer
  /// stability.
  auto cbegin() const { return map_.cbegin(); }
  auto cend() const { return map_.cend(); }
  auto begin() { return map_.begin(); }
  auto end() { return map_.end(); }
  auto begin() const { return map_.begin(); }
  auto end() const { return map_.end(); }

  friend bool operator==(const Dictionary& left, const Dictionary& right);

  friend bool operator!=(const Dictionary& left, const Dictionary& right) {
    return !(left == right);
  }

  bool IsConst() const override { return __padding_chars__[0]; }

  bool MarkConst() {
    if (IsConst()) return true;
    // For best iteration performance
    if (map_.using_small_map()) {
      for (const auto& [key, value] : map_.small_map()) {
        if (!value.MarkConst()) return false;
      }
    } else {
      for (const auto& [key, value] : map_.big_map()) {
        if (!value.MarkConst()) return false;
      }
    }
    __padding_chars__[0] = 1;
    return true;
  }

  bool using_small_map() const {
    // For unittest
    return map_.using_small_map();
  }

 protected:
  Dictionary() {}
  Dictionary(std::initializer_list<MapValueType>&& data)
      : map_(std::move(data)) {}

  friend class Value;

  void Reset() {
    map_.clear();
    __padding__ = 0;
  }

 private:
  Map map_;

  BASE_INLINE bool IsConstLog() const {
    if (IsConst()) {
#ifdef DEBUG
      // TODO(yuyang), Currently LOGD still produce assembly in release mode.
      LOGD("Lepus table is const");
#endif
      return true;
    }
    return false;
  }

  HYBRID_MAP_NEVER_INLINE Value* SetValuePrepare(const base::String& key) {
    // If value does not exist at key, construct but do not initialize any field
    // for best performance. Later placement new on the memory.
    auto [target_ptr, inserted] =
        map_.try_emplace(key, Value::kUnsafeCreateAsUninitialized);
    if (HYBRID_MAP_UNLIKELY(!inserted)) {
      // Insertion failed, destruct the existing Value.
      target_ptr->~Value();
    }
    return target_ptr;
  }
};

using DictionaryPtr = fml::RefPtr<Dictionary>;

}  // namespace lepus
}  // namespace lynx

#endif  // BASE_INCLUDE_VALUE_TABLE_H_
