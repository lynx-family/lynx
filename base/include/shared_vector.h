// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_SHARED_VECTOR_H_
#define BASE_INCLUDE_SHARED_VECTOR_H_

#include <vector>

namespace lynx {
namespace base {
template <typename T>
class SharedVector {
 public:
  SharedVector(const std::vector<T>& source)
      : data_(source.data()), size_(source.size()) {}

  SharedVector(const T* data, size_t size) : data_(data), size_(size) {}

  const T* Data() const { return data_; }

  const T& operator[](std::size_t idx) const { return data_[idx]; }

  size_t Size() const { return size_; }

 private:
  const T* data_;
  const size_t size_;
};
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_SHARED_VECTOR_H_
