// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VECTOR_HELPER_H_
#define BASE_INCLUDE_VECTOR_HELPER_H_

#include <istream>
#include <string>

#include "base/include/vector.h"

// This file is separated from 'Array.h' for usage in other repositories like
// Lynx.

namespace lynx {
namespace base {

/**
 * @brief Convert std::string to ByteArray without tailing '\0'.
 *
 * @param s The input string.
 * @return ByteArray
 */
inline ByteArray ByteArrayFromString(const std::string& s) {
  return ByteArray(s.length(), s.c_str());
}

/**
 * @brief Convert std::basic_istream to ByteArray. This function reads from the
 * current position to the end of stream.
 *
 * @param input Input stream to read.
 * @return ByteArray
 */
inline ByteArray ByteArrayFromStream(std::basic_istream<char>& input) {
  const auto current = input.tellg();
  input.seekg(0, std::ios::end);
  const auto length = input.tellg();
  if (current == -1 && length == -1) {
    BASE_VECTOR_DCHECK(false);
    return {};
  }
  ByteArray result(length - current);
  input.seekg(current);
  input.read(result.data<char>(), result.size());
  return result;
}

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_VECTOR_HELPER_H_
