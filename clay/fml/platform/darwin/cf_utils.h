// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_DARWIN_CF_UTILS_H_
#define CLAY_FML_PLATFORM_DARWIN_CF_UTILS_H_

#include <CoreFoundation/CoreFoundation.h>

#include "base/include/fml/macros.h"

namespace fml {

template <class T>
class CFRef {
 public:
  CFRef() : instance_(nullptr) {}

  // NOLINTNEXTLINE
  CFRef(T instance) : instance_(instance) {}

  CFRef(const CFRef& other) : instance_(other.instance_) {
    if (instance_) {
      CFRetain(instance_);
    }
  }

  CFRef(CFRef&& other) : instance_(other.instance_) {
    other.instance_ = nullptr;
  }

  CFRef& operator=(CFRef&& other) {
    Reset(other.Release());
    return *this;
  }

  ~CFRef() {
    if (instance_ != nullptr) {
      CFRelease(instance_);
    }
    instance_ = nullptr;
  }

  void Reset(T instance = nullptr) {
    if (instance_ != nullptr) {
      CFRelease(instance_);
    }

    instance_ = instance;
  }

  [[nodiscard]] T Release() {
    auto instance = instance_;
    instance_ = nullptr;
    return instance;
  }

  // NOLINTNEXTLINE
  operator T() const { return instance_; }

  operator bool() const { return instance_ != nullptr; }

 private:
  T instance_;

  CFRef& operator=(const CFRef&) = delete;
};

}  // namespace fml

#endif  // CLAY_FML_PLATFORM_DARWIN_CF_UTILS_H_
