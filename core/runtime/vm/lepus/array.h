// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_ARRAY_H_
#define CORE_RUNTIME_VM_LEPUS_ARRAY_H_
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/vector.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/ref_type.h"

namespace lynx {
namespace lepus {
class BASE_EXPORT_FOR_DEVTOOL CArray : public lepus::RefCounted {
 public:
  static fml::RefPtr<CArray> Create() {
    return fml::AdoptRef<CArray>(new CArray());
  }

  RefType GetRefType() const override { return RefType::kLepusArray; }

  // Push a default constructed value at back.
  Value* push_back_default() {
    if (IsConstLog()) {
      return nullptr;
    }
    return &vec_.emplace_back();
  }

  bool push_back(const Value& value) {
    if (IsConstLog()) {
      return false;
    }
    vec_.push_back(value);
    return true;
  }

  bool push_back(Value&& value) {
    if (IsConstLog()) {
      return false;
    }
    vec_.push_back(std::move(value));
    return true;
  }

  template <class... Args>
  bool emplace_back(Args&&... args) {
    if (IsConstLog()) {
      return false;
    }
    vec_.emplace_back(std::forward<Args>(args)...);
    return true;
  }

  bool pop_back() {
    if (IsConstLog()) {
      return false;
    }
    if (vec_.size() > 0) vec_.pop_back();
    return true;
  }

  bool Erase(uint32_t idx) {
    if (IsConstLog()) {
      return false;
    }

    if (idx >= 0 && idx < vec_.size()) {
      vec_.erase(vec_.begin() + idx);
    }
    return true;
  }

  bool Erase(size_t start, size_t del_count) {
    if (IsConstLog()) {
      return false;
    }

    auto begin = (start < vec_.size()) ? (vec_.begin() + start) : vec_.end();
    auto end =
        (start + del_count <= vec_.size()) ? (begin + del_count) : vec_.end();

    vec_.erase(begin, end);
    return true;
  }

  bool Insert(size_t pos, size_t ins_count, LEPUSContext* ctx,
              LEPUSValue* argv) {
    if (IsConstLog()) {
      return false;
    }

    auto pos_itr = (pos <= vec_.size()) ? (vec_.begin() + pos) : vec_.end();
    for (size_t i = 0; i < ins_count; ++i) {
      pos_itr = vec_.emplace(pos_itr, ctx, argv[i]) + 1;
    }
    return true;
  }

  bool Insert(uint32_t idx, const lepus::Value& value) {
    if (IsConstLog()) {
      return false;
    }

    if (idx >= 0 && idx <= vec_.size()) {
      vec_.insert(vec_.begin() + idx, value);
    }
    return true;
  }

  Value get_shift() {
    if (vec_.size() > 0) {
      Value ret = std::move(vec_[0]);
      vec_.erase(vec_.begin(), vec_.begin() + 1);
      return ret;
    } else {
      return Value();
    }
  }

  const Value& get(size_t index) const {
    if (index >= vec_.size()) {
      static Value empty;
      empty = Value();
      return empty;
    }
    return vec_[index];
  }

  void resize(long size) { vec_.resize(size); }

  void reserve(long size) { vec_.reserve(size); }

  bool set(size_t index, const Value& v) {
    if (IsConstLog()) {
      return false;
    }
    if (static_cast<size_t>(index) >= vec_.size()) {
      resize(index + 1);
    }
    vec_[index] = v;
    return true;
  }

  bool set(size_t index, Value&& v) {
    if (IsConstLog()) {
      return false;
    }
    if (static_cast<size_t>(index) >= vec_.size()) {
      resize(index + 1);
    }
    vec_[index] = std::move(v);
    return true;
  }

  void SetIsMatchResult() { is_regexp_match_result_ = true; }

  bool GetIsMatchResult() const { return is_regexp_match_result_; }

  size_t size() const { return vec_.size(); }

  void ReleaseSelf() const override;

  ~CArray() override = default;

  friend bool operator==(const CArray& left, const CArray& right) {
    return left.vec_ == right.vec_ &&
           left.is_regexp_match_result_ == right.is_regexp_match_result_;
  }

  friend bool operator!=(const CArray& left, const CArray& right) {
    return !(left == right);
  }

  Value GetMatchIndex() {
    DCHECK(is_regexp_match_result_);
    DCHECK(size() >= 3);
    return get(size() - 3);
  }

  Value GetMatchGroups() {
    DCHECK(is_regexp_match_result_);
    DCHECK(size() >= 3);
    return get(size() - 1);
  }

  Value GetMatchInput() {
    DCHECK(is_regexp_match_result_);
    DCHECK(size() >= 3);
    return get(size() - 2);
  }

  bool IsConst() const override { return is_const_; }

  bool MarkConst() {
    if (is_const_) return true;
    for (const auto& ele : vec_) {
      if (!ele.MarkConst()) return false;
    }
    return (is_const_ = true);
  }

 protected:
  CArray() = default;

 private:
  base::InlineVector<Value, 6> vec_;
  bool is_regexp_match_result_ = false;
  bool is_const_ = false;

  friend class LEPUSValueHelper;

  LEPUS_INLINE bool IsConstLog() const {
    if (IsConst()) {
#ifdef DEBUG
      // TODO(yuyang), Currently LOGD still produce assembly in release mode.
      LOGD("Lepus array is const");
#endif
      return true;
    }
    return false;
  }
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_ARRAY_H_
