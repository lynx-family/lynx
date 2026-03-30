/*
 * Skanda Compression Algorithm v0.9
 * Copyright (c) 2023 Carlos de Diego
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_SKANDA_H_
#define BASE_INCLUDE_SKANDA_H_

#include <stdio.h>  //size_t

#include <cstdint>

#include "base/include/base_export.h"

namespace lynx {
namespace base {
namespace skanda {
// Base class that allows to track progress of compression and decompression of
//  a Skanda stream. You will have to create a child class which implements the
//  functions.
//  - progress(): the algorithm will pass the number of bytes that have been
//  compressed/decompressed,
//				 and its current compressed size. The function
// will return whether to stop encoding/decoding.
class ProgressCallback {
 public:
  virtual bool progress(size_t processedBytes, size_t compressedSize) {
    return false;
  }
};

// Compresses "size" bytes of data present in "input", and stores it in
// "output". "level" is a tradeoff between compressed size and compression
// speed, and must be <= 9. "decSpeedBias" is a tradeoff between compressed size
// and decompressed speed,
//  and must have a value between 0 and 1, with higher sacrificing ratio for
//  speed.
// You may pass a pointer to an object with base class ProgressCallback, to
// track progress. Returns the size of the compressed stream or a negative error
// code on failure.
size_t compress(const uint8_t* input, size_t size, uint8_t* output,
                int level = 2, float decSpeedBias = 0.5f,
                ProgressCallback* progress = nullptr);

// Decompresses contents in "compressed" to "decompressed".
// You may also pass a pointer to an object with base class ProgressCallback, to
// track progress. Returns 0 on success or a negative error code on failure.
BASE_EXPORT size_t decompress(const uint8_t* compressed, size_t compressedSize,
                              uint8_t* decompressed, size_t decompressedSize,
                              ProgressCallback* progress = nullptr);

// For a given input size, returns a size for the output buffer that is big
// enough to
//  contain the compressed stream even if it expands.
size_t compress_bound(size_t size);

// Returns the amount of memory the algorithm will consume on compression.
size_t estimate_memory(size_t size, int level = 2, float decSpeedBias = 0.5f);

// Returns if a return value is actually an error code
bool is_error(size_t errorCode);

const size_t ERROR_NOMEM = 0 - 1;
const size_t ERROR_CORRUPT = 0 - 2;
}  // namespace skanda
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_SKANDA_H_
