// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_DATAURI_UTILS_H_
#define BASE_INCLUDE_DATAURI_UTILS_H_

#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

#include "base/include/closure.h"

namespace lynx {
namespace base {

class DataURIUtil {
 public:
  using BufferFactory = base::MoveOnlyClosure<char *, size_t>;

  /**
   * decode base64 str to binary data
   * @param base64_str  base64 str without any prefix
   * @return size & binary data, binary data's size maybe larger than the size
   * returned, do not assume them is equal, return 0 if failed
   */
  static int32_t DecodeBase64(const std::string_view &base64_str,
                              BufferFactory factory);
  /**
   * decode base64 encoded data uri(data:[<mediatype>];base64,<data>) str to
   * binary data
   * @param uri
   * @return @see DecodeBase64
   */
  static int32_t DecodeDataURI(const std::string_view &uri,
                               BufferFactory factory);

  /**
   * Check if the given string is a data uri, do not handle any leading blank,
   * trim first.
   */
  static bool IsDataURI(const std::string_view &uri);
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_DATAURI_UTILS_H_
