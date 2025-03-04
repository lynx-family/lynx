// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_PLATFORM_DARWIN_TYPE_UTILS_H_
#define BASE_INCLUDE_PLATFORM_DARWIN_TYPE_UTILS_H_

#include <objc/objc.h>

#include <type_traits>

namespace lynx {
namespace base {

// https://stackoverflow.com/a/23283812
template <typename T, typename V = bool>
struct is_objc_class : std::false_type {};

template <typename T>
struct is_objc_class<
    T, typename std::enable_if<std::is_convertible<T, id>::value, bool>::type>
    : std::true_type {};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_PLATFORM_DARWIN_TYPE_UTILS_H_
