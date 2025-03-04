// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_TABLE_H_
#define CORE_RUNTIME_VM_LEPUS_TABLE_H_

#include <algorithm>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "base/include/base_export.h"
#include "base/include/value/base_string.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/ref_type.h"

namespace lynx {
namespace lepus {
class BASE_EXPORT_FOR_DEVTOOL Dictionary : public lepus::RefCounted {
 public:
  using HashMap = std::unordered_map<base::String, Value>;

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
  template <class... Args>
  bool SetValue(const base::String& key, Args&&... args) {
    if (IsConstLog()) {
      return false;
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
            return true;
          }
        }
      }

      target_ptr->~Value();
    }
    // Placement new Value() on target_ptr with variadic args.
    new (target_ptr) Value(std::forward<Args>(args)...);
    return true;
  }

  const Value& GetValue(const base::String& key, bool forUndef = false);

  std::optional<Value> GetProperty(const base::String& key) {
    if (const auto& result = hash_map_.find(key); result != hash_map_.end()) {
      return std::make_optional(result->second);
    }
    return std::nullopt;
  }

  // Default construct or get the value by key. For const tables this function
  // may returns nullptr.
  Value* At(const base::String& key);
  Value* At(base::String&& key);

  bool Contains(const base::String& key) const;

  HashMap::const_iterator find(const base::String& key) const {
    return hash_map_.find(key);
  }

  HashMap::iterator find(const base::String& key) {
    return hash_map_.find(key);
  }

  bool Erase(const base::String& key);

  size_t size() const { return hash_map_.size(); }

  HashMap::iterator begin() { return hash_map_.begin(); }

  HashMap::const_iterator cbegin() const { return hash_map_.cbegin(); }

  HashMap::const_iterator cend() const { return hash_map_.cend(); }

  HashMap::iterator end() { return hash_map_.end(); }
  HashMap::const_iterator begin() const { return hash_map_.begin(); }
  HashMap::const_iterator end() const { return hash_map_.end(); }

  void dump();
  void ReleaseSelf() const override;

  friend bool operator==(const Dictionary& left, const Dictionary& right);

  friend bool operator!=(const Dictionary& left, const Dictionary& right) {
    return !(left == right);
  }

  bool IsConst() const override { return is_const_; }

  bool MarkConst() {
    if (is_const_) return true;
    for (const auto& [key, value] : hash_map_) {
      if (!value.MarkConst()) return false;
    }
    return (is_const_ = true);
  }

  int32_t EraseKey(const base::String& key) {
    if (is_const_) {
      return -1;
    }
    return static_cast<int32_t>(hash_map_.erase(key));
  }

 protected:
  Dictionary() = default;
  Dictionary(HashMap map);

 private:
  HashMap hash_map_;
  bool is_const_ = false;

  LEPUS_INLINE bool IsConstLog() const {
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

#endif  // CORE_RUNTIME_VM_LEPUS_TABLE_H_
