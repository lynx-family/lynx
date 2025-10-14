/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Copyright 2022 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CLAY_FML_BASE64_H_
#define CLAY_FML_BASE64_H_

#include <cstddef>

namespace fml {
struct Base64 {
 public:
  enum Error { kNoError, kPadError, kBadCharError };

  /**
     Base64 encodes src into dst.

     Normally this is called once with 'dst' nullptr to get the required size,
     then again with an allocated 'dst' pointer to do the actual encoding.

     @param dst nullptr or a pointer to a buffer large enough to receive the
     result

     @param encode nullptr for default encoding or a pointer to at least 65
     chars. encode[64] will be used as the pad character. Encodings other than
     the default encoding cannot be decoded.

     @return the required length of dst for encoding.
  */
  static size_t Encode(const void* src, size_t length, void* dst,
                       const char* encode = nullptr);

  /**
     Base64 decodes src into dst.

     Normally this is called once with 'dst' nullptr to get the required size,
     then again with an allocated 'dst' pointer to do the actual encoding.

     @param dst nullptr or a pointer to a buffer large enough to receive the
     result

     @param dstLength assigned the length dst is required to be. Must not be
     nullptr.
  */
  static Error Decode(const void* src, size_t srcLength, void* dst,
                      size_t* dstLength);
};
}  // namespace fml

#endif  // CLAY_FML_BASE64_H_
