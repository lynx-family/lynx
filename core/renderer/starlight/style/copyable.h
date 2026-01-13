// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_COPYABLE_H_
#define CORE_RENDERER_STARLIGHT_STYLE_COPYABLE_H_

#include <utility>

#include "base/include/fml/memory/ref_counted.h"

namespace lynx {
namespace starlight {

// A template wrapper that makes any type compatible with DataRef<T>
// by providing Create() and Copy() methods required by DataRef.
// XXX(zhongyr): this could be a common COW util template and move to base with
// data_ref.h
template <typename T>
class Copyable : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }

  static fml::RefPtr<Copyable<T>> Create() {
    return fml::AdoptRef(new Copyable<T>());
  }

  static fml::RefPtr<Copyable<T>> Create(T&& value) {
    return fml::AdoptRef(new Copyable<T>(std::move(value)));
  }

  static fml::RefPtr<Copyable<T>> Create(const T& value) {
    return fml::AdoptRef(new Copyable<T>(value));
  }

  fml::RefPtr<Copyable<T>> Copy() const {
    return fml::AdoptRef(new Copyable<T>(value_));
  }

  T& Value() { return value_; }
  const T& Value() const { return value_; }

  T* operator->() { return &value_; }
  const T* operator->() const { return &value_; }

  T& operator*() { return value_; }
  const T& operator*() const { return value_; }

  bool operator==(const Copyable<T>& other) const {
    return value_ == other.value_;
  }

  bool operator!=(const Copyable<T>& other) const {
    return value_ != other.value_;
  }

 private:
  Copyable() = default;
  explicit Copyable(const T& value) : value_(value) {}
  explicit Copyable(T&& value) : value_(std::move(value)) {}

  T value_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_COPYABLE_H_
