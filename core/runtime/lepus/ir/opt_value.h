// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_OPT_VALUE_H_
#define CORE_RUNTIME_LEPUS_IR_OPT_VALUE_H_

#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/None.h"

namespace lynx {
namespace lepus {
namespace ir {

template <typename T>
class OptValue {
  static_assert(std::is_trivially_copyable<T>::value,
                "OptValue<> can only be used with trivially copyable types");
  T value_{};
  bool has_value_;

 public:
  typedef T value_type;

  OptValue(llvh::NoneType) : has_value_(false) {}
  explicit OptValue() : has_value_(false) {}
  OptValue(const T& v) : value_(v), has_value_(true) {}

  OptValue(const OptValue&) = default;
  OptValue& operator=(const OptValue&) = default;
  ~OptValue() = default;

  bool HasValue() const { return has_value_; }
  explicit operator bool() const { return has_value_; }

  const T& GetValue() const {
    if (!HasValue()) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: OptValue has no value");
    }
    return value_;
  }
  const T& operator*() const { return GetValue(); }

  const T* operator->() const { return &GetValue(); }
};

static_assert(std::is_trivially_copyable<OptValue<int>>::value,
              "OptValue<int> must be trivially copyable");

/// Specialization for bool that improves codegen by collapsing compares.
template <>
class OptValue<bool> {
  // -1 = none, 0 = false, 1 = true
  int value_;

 public:
  typedef bool value_type;
  OptValue(llvh::NoneType) : value_(-1) {}
  explicit OptValue() : value_(-1) {}
  OptValue(bool v) : value_(v ? 1 : 0) {}

  OptValue(const OptValue&) = default;
  OptValue& operator=(const OptValue&) = default;
  ~OptValue() = default;

  bool HasValue() const { return value_ >= 0; }
  explicit operator bool() const { return HasValue(); }

  bool GetValue() const {
    if (!HasValue()) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: OptValue<bool> has no value");
    }
    return value_ > 0;
  }

  bool operator*() const { return GetValue(); }
};

static_assert(std::is_trivially_copyable<OptValue<bool>>::value,
              "OptValue<bool> must be trivially copyable");

template <typename T, typename U>
bool operator==(const OptValue<T>& a, const OptValue<U>& b) {
  if (a && b) return *a == *b;
  return a.HasValue() == b.HasValue();
}

template <typename T, typename U>
bool operator!=(const OptValue<T>& a, const OptValue<U>& b) {
  return !(a == b);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_OPT_VALUE_H_
