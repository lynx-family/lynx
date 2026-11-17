// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_BYTE_UTILS_H_
#define BASE_INCLUDE_BYTE_UTILS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace lynx {
namespace base {

// Copies readable bytes without first value-initializing the destination.
// The source must not overlap the destination. For size 0, it may be null.
inline void AssignByteArray(std::vector<uint8_t>& destination, const void* data,
                            size_t size) {
  if (size == 0) {
    destination.clear();
    return;
  }

  const auto* bytes = static_cast<const uint8_t*>(data);
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 200000
  // libc++ 20's uninitialized allocator copy checks trivial copy assignment
  // of the input type without removing const, disabling bulk copy for
  // const uint8_t*.
  // assign only reads the source, even when the iterator type is mutable.
  auto* begin = const_cast<uint8_t*>(bytes);
#else
  // Older libc++ versions (including 15 and 17) select their bulk-copy
  // specialization only for const input pointers.
  const auto* begin = bytes;
#endif
  destination.assign(begin, begin + size);
}

inline std::vector<uint8_t> CopyByteArray(const void* data, size_t size) {
  std::vector<uint8_t> result;
  AssignByteArray(result, data, size);
  return result;
}

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_BYTE_UTILS_H_
