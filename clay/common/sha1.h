// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_SHA1_H_
#define CLAY_COMMON_SHA1_H_

#include <string>

namespace clay {
std::string SHA1HashBytes(const void* data, size_t size);

static inline std::string SHA1HashString(const std::string& str) {
  return SHA1HashBytes(str.c_str(), str.size());
}
}  // namespace clay

#endif  // CLAY_COMMON_SHA1_H_
