// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_VALUE_TABLE_H_
#define BASE_INCLUDE_VALUE_TABLE_H_

#include <algorithm>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "base/include/base_defines.h"
#include "base/include/base_export.h"
#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/value/ref_type.h"

namespace lynx {
namespace lepus {

class BASE_EXPORT_FOR_DEVTOOL Dictionary : public RefCountedBase {
 public:
  // The implementation is not guaranteed to be std::unordered_map only.
  using HashMap = std::unordered_map<base::String, Value>;

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
    /// dictionary more quickly. Implement not using `template<K, V>` and
    /// `std::forward<>` for binary size friendly.
    static Value& SetValueUniqueKey(Dictionary& target,
                                    const base::String& key) {
      // Default construct value for key. This is to optimize table decoding.
      return target.hash_map_.try_emplace(key).first->second;
    }

    static Value& SetValueUniqueKey(Dictionary& target, base::String&& key) {
      // Default construct value for key. This is to optimize table decoding.
      return target.hash_map_.try_emplace(std::move(key)).first->second;
    }

    static void SetValueUniqueKey(Dictionary& target, const base::String& key,
                                  const Value& value) {
      target.hash_map_.try_emplace(key, value);
    }

    static void SetValueUniqueKey(Dictionary& target, const base::String& key,
                                  Value&& value) {
      target.hash_map_.try_emplace(key, std::move(value));
    }

    static void SetValueUniqueKey(Dictionary& target, base::String&& key,
                                  const Value& value) {
      target.hash_map_.try_emplace(std::move(key), value);
    }

    static void SetValueUniqueKey(Dictionary& target, base::String&& key,
                                  Value&& value) {
      target.hash_map_.try_emplace(std::move(key), std::move(value));
    }
  };

  friend class Unsafe;

 private:
  class alignas(Value) ValueNoOpCtor {
   private:
    [[maybe_unused]] uint8_t buffer_[sizeof(Value)];
  };

  using ValueNoOpCtorHashMap = std::unordered_map<base::String, ValueNoOpCtor>;

 public:
  static fml::RefPtr<Dictionary> Create() {
    return fml::AdoptRef<Dictionary>(new Dictionary());
  }
  static fml::RefPtr<Dictionary> Create(HashMap map) {
    return fml::AdoptRef<Dictionary>(new Dictionary(std::move(map)));
  }
  ~Dictionary() = default;

  RefType GetRefType() const override { return RefType::kLepusTable; }

  ///  @Note Why this method is implemented like this.
  ///
  ///  The most primitive and intuitive code to implement this method is as
  ///  follows.
  ///
  ///      hash_map_[key] = Value(std::forward<Args>(args)...);
  ///
  ///  or
  ///
  ///      if (auto result = hash_map_.try_emplace(key,
  ///         std::forward<Args>(args)...); !result.second) {
  ///         result.first->second = Value(std::forward<Args>(args)...);
  ///      }
  ///
  ///  But unfortunately, the first way is not optimal in performance which may
  ///  bring default construction or move assignment of Value.
  ///
  ///  The second way is better in performance but will cause severely binary
  ///  expansion for template specialization of try_emplace() method.
  ///  By introducing ValueNoOpCtor and casting hash_map_ to
  ///  ValueNoOpCtorHashMap, the emplace and construct of ValueNoOpCtor is
  ///  cheap.
  ///
  ///  @return ValueWrapper to Value if success or nullptr on failure.
  template <class... Args>
  ValueWrapper SetValue(const base::String& key, Args&&... args) {
    if (IsConstLog()) {
      return ValueWrapper(nullptr);
    }

    auto& hash_map_no_op = reinterpret_cast<ValueNoOpCtorHashMap&>(hash_map_);
    auto [iterator, inserted] = hash_map_no_op.try_emplace(key);
    Value* target_ptr = reinterpret_cast<Value*>(&iterator->second);
    if (!inserted) {
      // Insertion failed, destruct the existing Value.
      if constexpr (sizeof...(Args) == 1) {
        using single_arg_t = std::remove_cv_t<std::remove_reference_t<
            std::tuple_element_t<0, std::tuple<Args...>>>>;
        if constexpr (std::is_same_v<single_arg_t, Value>) {
          if (target_ptr == &std::get<0>(std::tie(args...))) {
            // Possibile that input args is a 'Value' happens to be exactly
            // the same instance of exsiting one.
            return ValueWrapper(target_ptr);
          }
        }
      }

      target_ptr->~Value();
    }
    // Placement new Value() on target_ptr with variadic args.
    new (target_ptr) Value(std::forward<Args>(args)...);
    return ValueWrapper(target_ptr);
  }

  /// Return a default nil value if key not found.
  ValueWrapper GetValue(const base::String& key) const;

  /// Return a default undefined value if key not found.
  ValueWrapper GetValueOrUndefined(const base::String& key) const;

  /// Return a nullable ValueWrapper and you should check before use.
  ValueWrapper GetValueOrNull(const base::String& key) const;

  /// Insert a nil value and return it if key not found.
  ValueWrapper GetValueOrInsert(const base::String& key);
  ValueWrapper GetValueOrInsert(base::String&& key);

  /// Return false if the dictionary is const, or always true.
  bool Erase(const base::String& key);

  /// Return -1 if the dictionary is const, or returns number of elements
  /// erased(0 or 1).
  int32_t EraseKey(const base::String& key);

  bool Contains(const base::String& key) const;

  auto find(const base::String& key) const { return hash_map_.find(key); }

  auto find(const base::String& key) { return hash_map_.find(key); }

  size_t size() const { return hash_map_.size(); }
  bool empty() const { return hash_map_.empty(); }

  void reserve(size_t count) {
    // TODO(yuyang), implement after refactored.
  }

  /// @note Do not cache pointer to value using `&(it->second)`
  /// to other variables. Later the underlying implementation
  /// of this map will be changed to flat based instead of node
  /// based.
  auto cbegin() const { return hash_map_.cbegin(); }
  auto cend() const { return hash_map_.cend(); }
  auto begin() { return hash_map_.begin(); }
  auto end() { return hash_map_.end(); }
  auto begin() const { return hash_map_.begin(); }
  auto end() const { return hash_map_.end(); }

  void Dump();

  friend bool operator==(const Dictionary& left, const Dictionary& right);

  friend bool operator!=(const Dictionary& left, const Dictionary& right) {
    return !(left == right);
  }

  bool IsConst() const override { return __padding_chars__[0]; }

  bool MarkConst() {
    if (IsConst()) return true;
    for (const auto& [key, value] : hash_map_) {
      if (!value.MarkConst()) return false;
    }
    __padding_chars__[0] = 1;
    return true;
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
  template <typename Callback,
            typename = std::enable_if_t<std::is_invocable_v<
                Callback, const base::String&, const Value&>>>
  void for_each(Callback&& callback) const {
    using callback_ret_type =
        std::invoke_result_t<Callback, const base::String&, const Value&>;
    constexpr bool callback_returns_bool =
        std::is_same_v<callback_ret_type, bool>;

    if constexpr (callback_returns_bool) {
      for (auto& p : hash_map_) {
        if (callback(p.first, p.second)) {
          break;
        }
      }
    } else {
      for (auto& p : hash_map_) {
        callback(p.first, p.second);
      }
    }
  }

  template <typename Callback, typename = std::enable_if_t<std::is_invocable_v<
                                   Callback, const base::String&, Value&>>>
  void for_each(Callback&& callback) {
    using callback_ret_type =
        std::invoke_result_t<Callback, const base::String&, Value&>;
    constexpr bool callback_returns_bool =
        std::is_same_v<callback_ret_type, bool>;

    if constexpr (callback_returns_bool) {
      for (auto& p : hash_map_) {
        if (callback(p.first, p.second)) {
          break;
        }
      }
    } else {
      for (auto& p : hash_map_) {
        callback(p.first, p.second);
      }
    }
  }

 protected:
  Dictionary() = default;
  Dictionary(HashMap map);

  friend class Value;

  void Reset() {
    hash_map_.clear();
    __padding__ = 0;
  }

 private:
  HashMap hash_map_;

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
};

using DictionaryPtr = fml::RefPtr<Dictionary>;

}  // namespace lepus
}  // namespace lynx

#endif  // BASE_INCLUDE_VALUE_TABLE_H_
