// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_UTILS_ANY_H_
#define CORE_BASE_UTILS_ANY_H_

#include <utility>

namespace lynx {
namespace base {

class BaseErasure {
 public:
  BaseErasure() = default;
  BaseErasure(const BaseErasure&) = delete;
  BaseErasure& operator=(const BaseErasure&) = delete;
  virtual ~BaseErasure() {}

 public:
  virtual BaseErasure* clone() = 0;
};

template <typename T>
class ValueHolder : public BaseErasure {
 public:
  ValueHolder(T t) : value_(t) {}
  ValueHolder* clone() override { return new ValueHolder<T>(value_); }

 public:
  T value_;
};

class any {
 public:
  any() : data_ptr_(nullptr) {}
  ~any() { delete data_ptr_; }

  template <typename T>
  any(T t) : data_ptr_(new ValueHolder<T>(t)) {}

  any(const any& a) : data_ptr_(a.data_ptr_->clone()) {}

  any(any&& a) noexcept : data_ptr_(a.data_ptr_) { a.data_ptr_ = nullptr; }

  any& operator=(any a) {
    std::swap(data_ptr_, a.data_ptr_);
    return *this;
  }

  template <class T>
  friend T any_cast(const any& a);

 private:
  BaseErasure* data_ptr_;
};

template <class T>
T any_cast(const any& a) {
  return static_cast<ValueHolder<T>*>(a.data_ptr_)->value_;
}

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_UTILS_ANY_H_
