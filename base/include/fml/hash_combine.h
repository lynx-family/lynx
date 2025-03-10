// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_INCLUDE_FML_HASH_COMBINE_H_
#define BASE_INCLUDE_FML_HASH_COMBINE_H_

#include <functional>

namespace lynx {
namespace fml {

template <class Type>
// NOLINTNEXTLINE
constexpr void HashCombineSeed(std::size_t& seed, Type arg) {
  seed ^= std::hash<Type>{}(arg) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class Type, class... Rest>
// NOLINTNEXTLINE
constexpr void HashCombineSeed(std::size_t& seed, Type arg,
                               Rest... other_args) {
  HashCombineSeed(seed, arg);
  HashCombineSeed(seed, other_args...);
}

constexpr std::size_t HashCombine() { return 0xdabbad00; }

template <class... Type>
[[nodiscard]] constexpr std::size_t HashCombine(Type... args) {
  std::size_t seed = HashCombine();
  HashCombineSeed(seed, args...);
  return seed;
}

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::HashCombine;
using lynx::fml::HashCombineSeed;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_HASH_COMBINE_H_
