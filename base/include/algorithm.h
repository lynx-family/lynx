/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_ALGORITHM_H_
#define BASE_INCLUDE_ALGORITHM_H_

#include <utility>

namespace lynx {
namespace base {

/** Insertion Sort algorithm. Use this algorithm when the region to be sorted is
 * a small constant size (e.g., count <= 32)
 *
 * @param left points to the beginning of the region to be sorted
 * @param count number of items to be sorted
 * @param less_than a functor with bool operator()(T a, T b) which returns true
 * if a comes before b.
 *
 * */
template <typename T, typename C>
inline void InsertionSort(T* left, size_t count, const C& less_than) {
  if (count <= 1) {
    return;
  }
  T* right = left + count - 1;
  for (T* next = left + 1; next <= right; ++next) {
    if (!less_than(*next, *(next - 1))) {
      continue;
    }
    T insert = std::move(*next);
    T* hole = next;
    do {
      *hole = std::move(*(hole - 1));
      --hole;
    } while (left < hole && less_than(insert, *(hole - 1)));
    *hole = std::move(insert);
  }
}

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_ALGORITHM_H_
