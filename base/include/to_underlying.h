// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_TO_UNDERLYING_H_
#define BASE_INCLUDE_TO_UNDERLYING_H_

#include <type_traits>

namespace lynx::base {

// Implementation of C++23's std::to_underlying.
//
// Note: This has an additional `std::is_enum<EnumT>` requirement to be SFINAE
// friendly prior to C++20.
//
// Reference: https://en.cppreference.com/w/cpp/utility/to_underlying
template <typename EnumT, typename = std::enable_if_t<std::is_enum<EnumT>{}>>
constexpr std::underlying_type_t<EnumT> to_underlying(EnumT e) noexcept {
  return static_cast<std::underlying_type_t<EnumT>>(e);
}

}  // namespace lynx::base

#endif  // BASE_INCLUDE_TO_UNDERLYING_H_
