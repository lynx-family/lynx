// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_CAST_UTIL_H_
#define BASE_INCLUDE_CAST_UTIL_H_

namespace lynx {
namespace base {

template <typename Dst, typename Src>
Dst BitCast(Src&& value) {
  static_assert(sizeof(Src) == sizeof(Dst), "BitCast sizes must match.");
  Dst result;
  memcpy(&result, &value, sizeof(result));
  return result;
}

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_CAST_UTIL_H_
