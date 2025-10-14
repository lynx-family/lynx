// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_ENDIANNESS_H_
#define CLAY_FML_ENDIANNESS_H_

#include <cstdint>
#include <type_traits>
#if defined(_MSC_VER)
#include "base/include/intrin.h"  // NOLINT
#endif

#include "build/build_config.h"

// Compiler intrinsics for flipping endianness.
#if defined(_MSC_VER)
#define FML_BYTESWAP_16(n) _byteswap_ushort(n)
#define FML_BYTESWAP_32(n) _byteswap_ulong(n)
#define FML_BYTESWAP_64(n) _byteswap_uint64(n)
#else
#define FML_BYTESWAP_16(n) __builtin_bswap16(n)
#define FML_BYTESWAP_32(n) __builtin_bswap32(n)
#define FML_BYTESWAP_64(n) __builtin_bswap64(n)
#endif

namespace fml {

/// @brief  Flips the endianness of the given value.
///         The given value must be an integral type of size 1, 2, 4, or 8.
template <typename T, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr T ByteSwap(T n) {
  if constexpr (sizeof(T) == 1) {
    return n;
  } else if constexpr (sizeof(T) == 2) {
    return (T)FML_BYTESWAP_16((uint16_t)n);
  } else if constexpr (sizeof(T) == 4) {
    return (T)FML_BYTESWAP_32((uint32_t)n);
  } else if constexpr (sizeof(T) == 8) {
    return (T)FML_BYTESWAP_64((uint64_t)n);
  } else {
    static_assert(!sizeof(T), "Unsupported size");
  }
}

/// @brief  Convert a known big endian value to match the endianness of the
///         current architecture. This is effectively a cross platform
///         ntohl/ntohs (as network byte order is always Big Endian).
///         The given value must be an integral type of size 1, 2, 4, or 8.
template <typename T, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr T BigEndianToArch(T n) {
#if FML_ARCH_CPU_LITTLE_ENDIAN
  return ByteSwap<T>(n);
#else
  return n;
#endif
}

/// @brief  Convert a known little endian value to match the endianness of the
///         current architecture.
///         The given value must be an integral type of size 1, 2, 4, or 8.
template <typename T, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr T LittleEndianToArch(T n) {
#if !FML_ARCH_CPU_LITTLE_ENDIAN
  return ByteSwap<T>(n);
#else
  return n;
#endif
}

}  // namespace fml

#endif  // CLAY_FML_ENDIANNESS_H_
