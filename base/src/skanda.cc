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

#include "base/include/skanda.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__)
#define expect(expr, value) (__builtin_expect((expr), (value)))
#define likely(expr) expect((expr) != 0, 1)
#define unlikely(expr) expect((expr) != 0, 0)
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif

// Probably not the correct way to do it but bleh
#define IS_64BIT (UINTPTR_MAX > UINT32_MAX)

#if defined(_MSC_VER)
#if defined(_M_AMD64)
#define x64
#include <intrin.h>
#endif
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__amd64__)
#define x64
#include <x86intrin.h>
#endif
#endif

#define try if (true)
#define catch(...) if (false)
#define throw(...) ((void)0)

namespace lynx {
namespace base {
namespace skanda {

FORCE_INLINE bool is_little_endian() {
  const union {
    uint16_t u;
    uint8_t c[2];
  } LITTLE_ENDIAN_CHECK = {1};
  return LITTLE_ENDIAN_CHECK.c[0];
}

// Undefined behaviour if value == 0
FORCE_INLINE size_t unsafe_int_log2(size_t value) {
#if defined(_MSC_VER)
  unsigned long result;
#if IS_64BIT
  _BitScanReverse64(&result, value);
#else
  _BitScanReverse(&result, value);
#endif
  return result;
#elif defined(__GNUC__) || defined(__clang__)
#if IS_64BIT
  return 63 - __builtin_clzll(value);
#else
  return 31 - __builtin_clz(value);
#endif
#else
#if IS_64BIT
  const uint8_t tab64[64] = {0,  58, 1,  59, 47, 53, 2,  60, 39, 48, 27, 54, 33,
                             42, 3,  61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34,
                             11, 43, 14, 22, 4,  62, 57, 46, 52, 38, 26, 32, 41,
                             50, 36, 17, 19, 29, 10, 13, 21, 56, 45, 25, 31, 35,
                             16, 9,  12, 44, 24, 15, 8,  23, 7,  6,  5,  63};
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value |= value >> 32;
  return tab64[value * 0x03f6eaf2cd271461 >> 58];
#else
  const uint8_t tab32[32] = {0,  9,  1,  10, 13, 21, 2,  29, 11, 14, 16,
                             18, 22, 25, 3,  30, 8,  12, 20, 28, 15, 17,
                             24, 7,  19, 27, 23, 6,  26, 5,  4,  31};
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  return tab32[value * 0x07C4ACDDU >> 27];
#endif
#endif
}

// Way faster than using log2(double), also returns 0 for a value of 0
FORCE_INLINE size_t int_log2(const size_t value) {
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  if (likely(value != 0)) return unsafe_int_log2(value);
  return 0;
#endif  // Fallback already returns 0 when value == 0
  return unsafe_int_log2(value);
}

// Undefined behaviour when value == 0
FORCE_INLINE size_t unsafe_bit_scan_forward(const size_t value) {
#if defined(_MSC_VER)
  unsigned long result;
#if IS_64BIT
  _BitScanForward64(&result, value);
#else
  _BitScanForward(&result, value);
#endif
  return result;
#elif defined(__GNUC__) || defined(__clang__)
#if IS_64BIT
  return __builtin_ctzll(value);
#else
  return __builtin_ctz(value);
#endif
#else
#if IS_64BIT
  const unsigned int tab64[64] = {
      0,  1,  2,  53, 3,  7,  54, 27, 4,  38, 41, 8,  34, 55, 48, 28,
      62, 5,  39, 46, 44, 42, 22, 9,  24, 35, 59, 56, 49, 18, 29, 11,
      63, 52, 6,  26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
      51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12};
  return tab64[(value & (0 - value)) * 0x022FDD63CC95386D >> 58];
#else
  static uint8_t tab32[32] = {0,  1,  28, 2,  29, 14, 24, 3,  30, 22, 20,
                              15, 25, 17, 4,  8,  31, 27, 13, 23, 21, 19,
                              16, 7,  26, 12, 18, 6,  11, 5,  10, 9};
  return tab32[(value & (0 - value)) * 0x077CB531U >> 27];
#endif
#endif
}

// Returns the index of the first set bit, starting from the least significant,
// or 0 if the input is null
FORCE_INLINE size_t bit_scan_forward(const size_t value) {
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  if (likely(value != 0)) return unsafe_bit_scan_forward(value);
  return 0;
#endif  // Fallback already returns 0 when value == 0
  return unsafe_bit_scan_forward(value);
}

struct FastIntHash {
  // Use top bits
  size_t operator()(const size_t value) {
#if IS_64BIT
    return value * 0xff51afd7ed558ccd;
#else
    return value * 0x27d4eb2d;
#endif
  }
};

// These functions are used to obtain the data for the hash. If the number of
// bytes is higher than the word size, they will be mixed. Also note that these
// might read more bytes than necessary.
FORCE_INLINE size_t read_hash4(const uint8_t* const ptr) {
  uint32_t value;
  memcpy(&value, ptr, 4);
  return value;
}
FORCE_INLINE size_t read_hash8(const uint8_t* const ptr) {
#if IS_64BIT
  uint64_t value;
  memcpy(&value, ptr, 8);
#else
  uint32_t value = read_hash4(ptr) ^ read_hash4(ptr + 4);
#endif
  return value;
}
FORCE_INLINE size_t read_hash6(const uint8_t* const ptr) {
#if IS_64BIT
  if (is_little_endian()) return read_hash8(ptr) << 16;
  return read_hash8(ptr) >> 16;
#else
  uint16_t b;
  memcpy(&b, ptr + 4, 2);
  return read_hash4(ptr) ^ b;
#endif
}
FORCE_INLINE size_t read_hash5(const uint8_t* const ptr) {
#if IS_64BIT
  if (is_little_endian()) return read_hash8(ptr) << 24;
  return read_hash8(ptr) >> 24;
#else
  return read_hash4(ptr) ^ ptr[4];
#endif
}
FORCE_INLINE size_t read_hash3(const uint8_t* const ptr) {
  if (is_little_endian())
#if IS_64BIT
    return read_hash4(ptr) << 40;
#else
    return read_hash4(ptr) << 8;
#endif
  return read_hash4(ptr) >> 8;
}

FORCE_INLINE uint64_t read_uint64le(const uint8_t* const ptr) {
  if (is_little_endian()) {
    uint64_t value;
    memcpy(&value, ptr, 8);
    return value;
  }
  uint64_t value = 0;
  for (int i = 0; i < 8; i++) value |= (uint64_t)ptr[i] << i * 8;
  return value;
}
FORCE_INLINE void write_uint64le(uint8_t* const ptr, const uint64_t value) {
  if (is_little_endian())
    memcpy(ptr, &value, 8);
  else {
    for (int i = 0; i < 8; i++) ptr[i] = value >> i * 8;
  }
}
FORCE_INLINE uint32_t read_uint32le(const uint8_t* const ptr) {
  if (is_little_endian()) {
    uint32_t value;
    memcpy(&value, ptr, 4);
    return value;
  }
  uint32_t value = 0;
  for (int i = 0; i < 4; i++) value |= (uint32_t)ptr[i] << i * 8;
  return value;
}
FORCE_INLINE void write_uint32le(uint8_t* const ptr, const uint32_t value) {
  if (is_little_endian())
    memcpy(ptr, &value, 4);
  else {
    for (int i = 0; i < 4; i++) ptr[i] = value >> i * 8;
  }
}
FORCE_INLINE uint32_t read_uint24le(const uint8_t* const ptr) {
  uint32_t value = 0;
  for (int i = 0; i < 3; i++) value |= (uint32_t)ptr[i] << i * 8;
  return value;
}
FORCE_INLINE void write_uint24le(uint8_t* const ptr, const uint32_t value) {
  for (int i = 0; i < 3; i++) ptr[i] = value >> i * 8;
}
FORCE_INLINE uint16_t read_uint16le(const uint8_t* const ptr) {
  uint16_t value;
  if (is_little_endian())
    memcpy(&value, ptr, 2);
  else
    value = ptr[0] | (ptr[1] << 8);
  return value;
}
FORCE_INLINE void write_uint16le(uint8_t* const ptr, const uint16_t value) {
  if (is_little_endian())
    memcpy(ptr, &value, 2);
  else {
    for (int i = 0; i < 2; i++) ptr[i] = value >> i * 8;
  }
}

// https://github.com/romeric/fastapprox/blob/master/fastapprox/src/fastlog.h
FORCE_INLINE float fast_log2(float x) {
  union {
    float f;
    uint32_t i;
  } vx = {x};
  float y = vx.i;
  y *= 1.1920928955078125e-7f;
  return y - 126.94269504f;
}

const int SKANDA_MIN_MATCH_LENGTH = 2;
// These are written uncompressed
const int SKANDA_LAST_BYTES = 32;
const int SKANDA_MAX_WINDOW_LOG = 24;

const int MAX_HUFFMAN_CODE_LENGTH = 11;
const int HUFFMAN_CODE_SPACE = 1 << MAX_HUFFMAN_CODE_LENGTH;
const int MAX_PRECODE_CODE_LENGTH = 7;
const int PRECODE_CODE_SPACE = 1 << MAX_PRECODE_CODE_LENGTH;
const size_t MAX_BLOCK_SIZE = 262143;

enum {
  GREEDY1,
  GREEDY2,
  LAZY1,
  LAZY2,
  OPTIMAL1,
  OPTIMAL2,
  OPTIMAL3,
};

struct CompressorOptions {
  int parser;
  int maxHashLog;
  int maxElementsLog;
  int niceLength;
  int optimalBlockSize;  // (Optimal)
  int maxArrivals;       // (Optimal)
  int parserIterations;  // (Optimal)
};

// Tries to find a match between the two locations (front and back), and returns
// the length Note that this function should be called with at least MIN_LENGTH
// bytes of buffer after limit Set check window to false for those searches that
// won't go outside skanda's window (rep matches for example)
template <bool CHECK_WINDOW>
FORCE_INLINE size_t test_match(const uint8_t* front, const uint8_t* back,
                               const uint8_t* const limit,
                               const size_t minLength, const int windowLog) {
  // Test first bytes
  // Usually compilers will optimize std::equal as 2 comparisons for minLength
  // 5, 6, etc We can only use one comparison to make it faster. For powers of
  // 2, std::equal should be good
  if (minLength == 3) {
    uint32_t a, b;
    memcpy(&a, front, sizeof(uint32_t));
    memcpy(&b, back, sizeof(uint32_t));
    if ((is_little_endian() && (a << 8) != (b << 8)) ||
        (!is_little_endian() && (a >> 8) != (b >> 8)))
      return 0;
  }
#if IS_64BIT
  else if (minLength == 5) {
    uint64_t a, b;
    memcpy(&a, front, sizeof(uint64_t));
    memcpy(&b, back, sizeof(uint64_t));
    if ((is_little_endian() && (a << 24) != (b << 24)) ||
        (!is_little_endian() && (a >> 24) != (b >> 24)))
      return 0;
  } else if (minLength == 6) {
    uint64_t a, b;
    memcpy(&a, front, sizeof(uint64_t));
    memcpy(&b, back, sizeof(uint64_t));
    if ((is_little_endian() && (a << 16) != (b << 16)) ||
        (!is_little_endian() && (a >> 16) != (b >> 16)))
      return 0;
  }
#endif
  else {
    if (!std::equal(back, back + minLength, front)) return 0;
  }

  // Usually there is a higher chance that the first bytes are not equal
  //  than being outside the window, so separating both cases is a speed win
  if (CHECK_WINDOW && (size_t)(front - back) >> windowLog) return 0;

  const uint8_t* const matchOrigin = front;
  front += minLength;
  back += minLength;

  while (true) {
    if (unlikely(front + sizeof(size_t) > limit)) {
      if (front > limit) return 0;

      while (*front == *back && front < limit) {
        front++;
        back++;
      }
      return front - matchOrigin;
    }

    // Compare 4 or 8 bytes at a time using xor. It has the property of
    // returning 0 if the two values are equal. In case they differ, we can get
    // the first byte that differs using a bit scan.
    size_t a, b;
    memcpy(&a, front, sizeof(size_t));
    memcpy(&b, back, sizeof(size_t));
    const size_t xorVal = a ^ b;

    if (xorVal) {
      if (is_little_endian())
        front += unsafe_bit_scan_forward(xorVal) >> 3;
      else
        front += unsafe_int_log2(xorVal) >> 3;
      return front - matchOrigin;
    }

    front += sizeof(size_t);
    back += sizeof(size_t);
  }
}

// A hash table which does not check for collisions
template <class Value, class Hash>
class HashTable {
  Value* arr = nullptr;
  int hashShift;

 public:
  // Use 2^x sizes to avoid the use of modulo operator
  HashTable() {}
  ~HashTable() { delete[] arr; }
  void init(const int logSize) {
    arr = new Value[(size_t)1 << logSize]();
    hashShift = (IS_64BIT ? 64 : 32) - logSize;
  }
  Value& operator[](const size_t value) {
    return arr[Hash{}(value) >> hashShift];
  }
  void clear() {
    size_t arrSize = ((size_t)1 << (IS_64BIT ? 64 : 32)) - hashShift;
    memset(arr, 0, arrSize * sizeof(Value));
  }
};

// Used for easier implementation
template <class Value>
class LZCacheBucket {
  Value* it;
  Value* last;

 public:
  LZCacheBucket() {}
  LZCacheBucket(Value* _begin, Value* _end) {
    it = _begin;
    last = _end;
  }
  // Pushes a new value into the bucket. Used when skipping bytes
  void push(const Value newValue) {
    // An std::move can be faster, but only for big buckets (8 elements or more)
    for (last--; last != it; last--) last[0] = last[-1];
    it[0] = newValue;
  }
  // Whether all positions in the bucket have been loaded
  bool ended() { return it == last; }
  // Loads the next position, and at the same time, stores the previous position
  // in the same spot The first time value must have the current position (so it
  // basically pushes the new value into the bucket)
  void next(Value* value) {
    const Value tmp = *it;
    *it = *value;
    *value = tmp;
    it++;
  }
  // Loads the next position, but without updating the bucket as we go
  Value next() { return *it++; }
};

// This works like the basic hash table, except that it stores N positions per
// bucket
template <class Value, class Hash>
class LZCacheTable {
  Value* arr = nullptr;
  int hashShift;
  int elementsLog;

 public:
  // Use 2^x sizes to avoid the use of modulo and multiplication
  LZCacheTable() {}
  LZCacheTable(const int logSize, const int numElementsLog) {
    init(logSize, numElementsLog);
  }
  ~LZCacheTable() { delete[] arr; }
  void init(const int logSize, const int numElementsLog) {
    arr = new Value[(size_t)1 << logSize << numElementsLog]();
    hashShift = (IS_64BIT ? 64 : 32) - logSize;
    elementsLog = numElementsLog;
  }
  LZCacheBucket<Value> operator[](size_t value) {
    value = Hash{}(value) >> hashShift;
    Value* bucket = arr + (value << elementsLog);
    return LZCacheBucket<Value>(bucket, bucket + ((size_t)1 << elementsLog));
  }
};

struct LZStructure {
  uint32_t matchLength;
  uint32_t matchDistance;
  uint32_t literalRunLength;
};

struct LZMatch {
  uint32_t length;
  uint32_t distance;
};

// Simple and fast
class HashTableMatchFinder {
  HashTable<uint32_t, FastIntHash> lzdict3;
  LZCacheTable<uint32_t, FastIntHash> lzdict4;
  LZCacheTable<uint32_t, FastIntHash> lzdict8;

 public:
  void init(const int windowLog, const CompressorOptions& compressorOptions) {
    const int hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
    lzdict3.init(std::min(hashLog, 14));
    lzdict4.init(hashLog, compressorOptions.maxElementsLog);
    lzdict8.init(hashLog, compressorOptions.maxElementsLog);
  }

  LZMatch* find_matches_and_update(const uint8_t* const input,
                                   const uint8_t* const inputStart,
                                   const uint8_t* const limit, LZMatch* matches,
                                   size_t minLength, const int windowLog,
                                   const CompressorOptions& compressorOptions) {
    uint32_t& chain3 = lzdict3[read_hash3(input)];
    size_t nextExpectedLength = minLength;

    if (nextExpectedLength <= 3) {
      const uint8_t* where = inputStart + chain3;
      size_t length = test_match<true>(input, where, limit, 3, windowLog);

      if (length >= nextExpectedLength) {
        matches->distance = input - where;
        matches->length = length;
        matches++;

        if (length >= compressorOptions.niceLength) {
          chain3 = input - inputStart;
          return matches;
        }
        nextExpectedLength = length + 1;
      }
    }
    chain3 = input - inputStart;

    if (nextExpectedLength <= 7) {
      LZCacheBucket<uint32_t> chain4 = lzdict4[read_hash4(input)];
      uint32_t pos = input - inputStart;
      while (!chain4.ended()) {
        chain4.next(&pos);

        const uint8_t* where = inputStart + pos;

        if (*(input + nextExpectedLength - 1) !=
            *(where + nextExpectedLength - 1))
          continue;

        const size_t length =
            test_match<true>(input, where, limit, 4, windowLog);

        if (length >= nextExpectedLength) {
          matches->distance = input - where;

          matches->length = length;
          matches++;

          nextExpectedLength = length + 1;
          if (length >= 7) {
            while (!chain4.ended()) chain4.next(&pos);
            break;
          }
        }
      }
    }

    if (nextExpectedLength >= 4 &&
        nextExpectedLength <= compressorOptions.niceLength) {
      LZCacheBucket<uint32_t> chain8 = lzdict8[read_hash8(input)];
      uint32_t pos = input - inputStart;
      while (!chain8.ended()) {
        chain8.next(&pos);

        const uint8_t* where = inputStart + pos;
        if (*(input + nextExpectedLength - 1) !=
            *(where + nextExpectedLength - 1))
          continue;

        const size_t length =
            test_match<true>(input, where, limit, 8, windowLog);

        if (length >= nextExpectedLength) {
          matches->distance = input - where;
          matches->length = length;
          matches++;

          nextExpectedLength = length + 1;
          if (length >= compressorOptions.niceLength) {
            while (!chain8.ended()) chain8.next(&pos);
            break;
          }
        }
      }
    }

    return matches;
  }

  void update_position(const uint8_t* const input,
                       const uint8_t* const inputStart) {
    const size_t pos = input - inputStart;
    lzdict3[read_hash3(input)] = pos;
    lzdict4[read_hash4(input)].push(pos);
    lzdict8[read_hash8(input)].push(pos);
  }
};

const int NO_MATCH_POS = 0;
// Original match finder implementation from BriefLZ
class BinaryMatchFinder {
  HashTable<uint32_t, FastIntHash> lzdict3;
  HashTable<uint32_t, FastIntHash> nodeLookup;
  uint32_t* nodes = nullptr;
  size_t nodeListSize;
  size_t nodeListMask;

 public:
  ~BinaryMatchFinder() { delete[] nodes; }
  BinaryMatchFinder() {}

  void init(const size_t inputSize, const int windowLog,
            const CompressorOptions& compressorOptions) {
    const size_t binaryTreeWindow =
        std::min(compressorOptions.maxHashLog, windowLog);

    // Input size is smaller than maximum binary tree size
    if (inputSize < ((size_t)1 << binaryTreeWindow)) {
      nodes = new uint32_t[(size_t)2 * inputSize];
      nodeListSize = inputSize;
      nodeListMask = -1;
    } else {
      nodes = new uint32_t[(size_t)2 << binaryTreeWindow];
      nodeListSize = (size_t)1 << binaryTreeWindow;
      nodeListMask = nodeListSize - 1;
    }
    if (compressorOptions.parser < OPTIMAL3)
      lzdict3.init(std::min(14, windowLog - 3));
    nodeLookup.init(std::min(20, windowLog - 3));
  }

  LZMatch* find_matches_and_update(const uint8_t* const input,
                                   const uint8_t* const inputStart,
                                   const uint8_t* const compressionLimit,
                                   const uint8_t* const blockLimit,
                                   LZMatch* matches, size_t minLength,
                                   const int windowLog,
                                   const CompressorOptions& compressorOptions) {
    const size_t inputPosition = input - inputStart;
    size_t nextExpectedLength = minLength;

    // First try to get a length 3 match
    if (compressorOptions.parser < OPTIMAL3) {
      uint32_t& chain3 = lzdict3[read_hash3(input)];
      if (nextExpectedLength <= 3) {
        const uint8_t* where = inputStart + chain3;
        const size_t length =
            test_match<true>(input, where, blockLimit, 3, windowLog);

        if (length >= nextExpectedLength) {
          matches->distance = input - where;
          matches->length = length;
          matches++;

          if (length >= compressorOptions.niceLength) {
            update_position(input, inputStart, compressionLimit, windowLog,
                            compressorOptions);
            return matches;
          }
          nextExpectedLength = length + 1;
        }
      }
      chain3 = inputPosition;
    }

    // If we reach this position stop the search
    const size_t btEnd =
        inputPosition < nodeListSize ? 0 : inputPosition - nodeListSize;

    uint32_t& lookupEntry =
        nodeLookup[compressorOptions.parser == OPTIMAL3 ? read_hash3(input)
                                                        : read_hash4(input)];
    size_t backPosition = lookupEntry;
    lookupEntry = inputPosition;

    uint32_t* lesserNode = &nodes[2 * (lookupEntry & nodeListMask)];
    uint32_t* greaterNode = lesserNode + 1;
    const uint8_t* lesserFront = input;
    const uint8_t* greaterFront = input;

    size_t depth = (size_t)1 << compressorOptions.maxElementsLog;

    // Check matches
    while (true) {
      if (backPosition <= btEnd || depth-- == 0) {
        *lesserNode = NO_MATCH_POS;
        *greaterNode = NO_MATCH_POS;
        break;
      }

      const uint8_t* front = std::min(lesserFront, greaterFront);
      const uint8_t* back = front - (inputPosition - backPosition);

      const size_t extraLength =
          test_match<false>(front, back, compressionLimit, 0, 0);
      front += extraLength;
      back += extraLength;

      size_t length = front - input;
      // Match cant go outside of block boundaries
      const size_t effectiveLength =
          std::min(length, (size_t)(blockLimit - input));
      uint32_t* const nextNode = &nodes[2 * (backPosition & nodeListMask)];
      if (effectiveLength >= nextExpectedLength) {
        nextExpectedLength = effectiveLength;
        if (compressorOptions.parser != OPTIMAL3) nextExpectedLength++;
        matches->distance = front - back;
        matches->length = effectiveLength;
        matches++;
      }

      if (length >= compressorOptions.niceLength) {
        *lesserNode = nextNode[0];
        *greaterNode = nextNode[1];
        return matches;
      }

      if (*back < *front) {
        *lesserNode = backPosition;
        lesserNode = &nextNode[1];
        backPosition = *lesserNode;
        lesserFront = front;
      } else {
        *greaterNode = backPosition;
        greaterNode = &nextNode[0];
        backPosition = *greaterNode;
        greaterFront = front;
      }
    }

    return matches;
  }

  void update_position(const uint8_t* const input,
                       const uint8_t* const inputStart,
                       const uint8_t* const limit, const int windowLog,
                       const CompressorOptions& compressorOptions) {
    const size_t inputPosition = input - inputStart;
    if (compressorOptions.parser < OPTIMAL3)
      lzdict3[read_hash3(input)] = inputPosition;

    // If we reach this position on the front stop the update
    const uint8_t* positionSkip =
        std::min(limit, input + compressorOptions.niceLength);
    // If we reach this position on the back stop the update
    const size_t btEnd =
        inputPosition < nodeListSize ? 0 : inputPosition - nodeListSize;
    uint32_t& lookupEntry =
        nodeLookup[compressorOptions.parser == OPTIMAL3 ? read_hash3(input)
                                                        : read_hash4(input)];
    size_t backPosition = lookupEntry;
    lookupEntry = inputPosition;

    uint32_t* lesserNode = &nodes[2 * (inputPosition & nodeListMask)];
    uint32_t* greaterNode = &nodes[2 * (inputPosition & nodeListMask) + 1];

    const uint8_t* lesserFront = input;
    const uint8_t* greaterFront = input;
    size_t depth = (size_t)1 << compressorOptions.maxElementsLog;

    // Check matches
    while (true) {
      if (backPosition <= btEnd || depth-- == 0) {
        *lesserNode = NO_MATCH_POS;
        *greaterNode = NO_MATCH_POS;
        return;
      }

      const uint8_t* front = std::min(lesserFront, greaterFront);
      const uint8_t* back = front - (inputPosition - backPosition);

      const size_t length = test_match<false>(front, back, positionSkip, 0, 0);
      front += length;
      back += length;

      uint32_t* const nextNode = &nodes[2 * (backPosition & nodeListMask)];
      if (front >= positionSkip) {
        *lesserNode = nextNode[0];
        *greaterNode = nextNode[1];
        return;
      }

      if (*back < *front) {
        *lesserNode = backPosition;
        lesserNode = &nextNode[1];
        backPosition = *lesserNode;
        lesserFront = front;
      } else {
        *greaterNode = backPosition;
        greaterNode = &nextNode[0];
        backPosition = *greaterNode;
        greaterFront = front;
      }
    }
  }
};

enum {
  ENTROPY_RAW = 0,
  ENTROPY_HUFFMAN,
  ENTROPY_RLE,
};

enum {
  LITERAL_STREAM = 0,
  TOKEN_STREAM,
  DISTANCE_STREAM,
  LENGTH_STREAM,
};

enum {
  BLOCK_BASIC = 0,
  BLOCK_LAST,
};

struct HuffmanSymbol {
  uint32_t count;
  uint16_t code;
  uint8_t symbol;
  uint8_t bits;
};

struct CodeError {
  float error;
  int symbol;
};

void fast_huffman_codegen(HuffmanSymbol* symbolData, size_t symbolCount,
                          const size_t uniqueSymbols,
                          const size_t maxCodeLength) {
  // Very fast and precise length-limited huffman code generator based on
  // https://github.com/stbrumme/length-limited-prefix-codes/blob/master/limitedkraftheap.c
  // Modified to remove the need for a heap
  const size_t codeSpace = 1 << maxCodeLength;
  CodeError codeErrors[256];
  int codeErrorsCount = 0;

  float countInv = 1.0f / symbolCount;
  // Round frequencies to the closest power of 2 in log scale (works better than
  // floor)
  int usedCodeSpace = 0;
  int i = 0;
  for (; i < uniqueSymbols; i++) {
    if (symbolData[i].count == 0) {
      symbolData[i].bits = maxCodeLength + 1;
      continue;
    }

    float optimal = -fast_log2(symbolData[i].count * countInv);
    int bits = int(optimal + 0.5f);
    float error = optimal - bits;
    if (bits <= 0) bits = 1;
    if (bits > maxCodeLength) bits = maxCodeLength;

    codeErrors[codeErrorsCount] = {error, i};
    codeErrorsCount++;
    symbolData[i].bits = bits;
    usedCodeSpace += codeSpace >> bits;
  }

  // Sort the code length errors. This is important to later select which codes
  //  we will make longer and which shorter. It does not have to be a very good
  //  sort, a simple radix sort with 16 buckets is enough.
  {
    CodeError sortedErrors[16][256];
    size_t errorCounts[16] = {0};
    for (size_t i = 0; i < codeErrorsCount; i++) {
      size_t bucket = floor((codeErrors[i].error + 0.5) * 16);
      if (bucket >= 16)  // Just in case
        bucket = 15;
      sortedErrors[bucket][errorCounts[bucket]] = codeErrors[i];
      errorCounts[bucket]++;
    }
    size_t count = 0;
    for (int b = 15; b >= 0; b--) {
      for (size_t i = 0; i < errorCounts[b]; i++, count++)
        codeErrors[count] = sortedErrors[b][i];
    }
  }

  i = -1;
  while (usedCodeSpace > codeSpace) {
    i++;
    if (i >= codeErrorsCount) i = 0;

    // List is ordered high to low, that means the symbol that got its number of
    // bits most
    //  lowered is at the start. That is also the best candidate to increase
    //  code lengths.
    if (symbolData[codeErrors[i].symbol].bits == maxCodeLength) continue;
    symbolData[codeErrors[i].symbol].bits++;
    usedCodeSpace -= codeSpace >> symbolData[codeErrors[i].symbol].bits;
    codeErrors[i].error -= 1;
  }

  int remainingCodeSpace = codeSpace - usedCodeSpace;
  // Distribute remaining code space
  while (remainingCodeSpace) {
    i--;
    if (i < 0) i = codeErrorsCount - 1;

    if ((codeSpace >> symbolData[codeErrors[i].symbol].bits) <=
        remainingCodeSpace) {
      remainingCodeSpace -= codeSpace >> symbolData[codeErrors[i].symbol].bits;
      symbolData[codeErrors[i].symbol].bits--;
    }
  }
}

const size_t STACK_SIZE = 16;
struct PackageMergeNode {
  uint8_t stackBuffer[STACK_SIZE];
  size_t heapSize = 0;
  uint8_t* heapBuffer = nullptr;
  size_t symbolCount = 0;
  size_t histogramSum;

  PackageMergeNode() {}
  ~PackageMergeNode() { delete[] heapBuffer; }

  void push_back(uint8_t symbol) {
    if (symbolCount >= STACK_SIZE + heapSize) {
      size_t newHeapSize = std::max(heapSize * 2, (size_t)16);
      uint8_t* tmp = new uint8_t[newHeapSize];
      for (size_t i = 0; i < heapSize; i += 16)
        memcpy(tmp + i, heapBuffer + i, 16);
      delete[] heapBuffer;
      heapBuffer = tmp;
      heapSize = newHeapSize;
    }
    if (symbolCount < STACK_SIZE)
      stackBuffer[symbolCount] = symbol;
    else
      heapBuffer[symbolCount - STACK_SIZE] = symbol;
    symbolCount++;
  }
  void copy(PackageMergeNode* other) {
    if (other->heapSize > heapSize) {
      delete[] heapBuffer;
      heapSize = other->heapSize;
      heapBuffer = new uint8_t[heapSize];
    }
    symbolCount = other->symbolCount;
    memcpy(stackBuffer, other->stackBuffer, STACK_SIZE);
    for (size_t i = STACK_SIZE; i < symbolCount; i += 16)
      memcpy(heapBuffer + i - STACK_SIZE, other->heapBuffer + i - STACK_SIZE,
             16);
  }
  uint8_t at(size_t index) {
    if (index < STACK_SIZE) return stackBuffer[index];
    return heapBuffer[index - STACK_SIZE];
  }
  size_t size() { return symbolCount; }
};

struct {
  bool operator()(PackageMergeNode* a, PackageMergeNode* b) const {
    return a->histogramSum < b->histogramSum;
  }
} SortPackageMergeNodePtr;

// From https://experiencestack.co/length-limited-huffman-codes-21971f021d43
void package_merge(HuffmanSymbol* symbolData, size_t symbolCount,
                   const size_t uniqueSymbols, const size_t maxCodeLength) {
  PackageMergeNode originalList[256];
  size_t originalListSize = 0;
  for (size_t i = 0; i < uniqueSymbols; i++) {
    if (symbolData[i].count) {
      originalList[originalListSize].histogramSum = symbolData[i].count;
      originalList[originalListSize].push_back(i);
      originalListSize++;
      symbolData[i].bits = 0;
    } else
      symbolData[i].bits = maxCodeLength + 1;
  }

  // To avoid the continuous copy of vectors of symbols from the nodes, the
  // merged list will
  //  consist of pointers to the nodes from the original list and the new nodes
  //  from last iteration
  size_t mergedListSize = 0;
  PackageMergeNode* mergedList[256 * 2 - 1];
  for (mergedListSize = 0; mergedListSize < originalListSize; mergedListSize++)
    mergedList[mergedListSize] = &originalList[mergedListSize];

  // We will need 2 buffers for the nodes generated in each iteration: one for
  // the previous iteration,
  //  and one for the current. We will swap them to avoid copying the current
  //  into the previous.
  PackageMergeNode newNodesBufferA[255];
  PackageMergeNode newNodesBufferB[255];

  try {
    for (int i = 1; i < maxCodeLength; i++) {
      std::sort(mergedList, mergedList + mergedListSize,
                SortPackageMergeNodePtr);
      PackageMergeNode* newNodes = (i % 2) ? newNodesBufferA : newNodesBufferB;
      size_t numberNewNodes = 0;

      // If odd drop the last element
      for (size_t j = 0; j < (mergedListSize & ~1); j += 2) {
        newNodes[numberNewNodes].histogramSum =
            mergedList[j]->histogramSum + mergedList[j + 1]->histogramSum;
        newNodes[numberNewNodes].copy(mergedList[j]);
        for (size_t k = 0; k < mergedList[j + 1]->size(); k++)
          newNodes[numberNewNodes].push_back(mergedList[j + 1]->at(k));
        numberNewNodes++;
      }

      for (mergedListSize = 0; mergedListSize < originalListSize;
           mergedListSize++)
        mergedList[mergedListSize] = &originalList[mergedListSize];
      for (size_t j = 0; j < numberNewNodes; j++, mergedListSize++)
        mergedList[mergedListSize] = &newNodes[j];
      mergedListSize = originalListSize + numberNewNodes;
    }
  } catch (std::bad_alloc& e) {
    // Fallback
    fast_huffman_codegen(symbolData, symbolCount, uniqueSymbols, maxCodeLength);
    return;
  }

  // We will only need this number of elements at the end
  size_t maxListLength = originalListSize * 2 - 2;
  for (int i = 0; i < maxListLength; i++) {
    for (int j = 0; j < mergedList[i]->size(); j++)
      symbolData[mergedList[i]->at(j)].bits++;
  }
}

void symbol_histogram(const uint8_t* data, size_t count,
                      uint32_t histogram[8][256]) {
  const uint8_t* const fastLoopEnd = data + (count & ~0x7);
  const uint8_t* const end = data + count;
  for (; data < fastLoopEnd; data += 8) {
    if (IS_64BIT) {
      uint64_t w;
      memcpy(&w, data, 8);
      histogram[0][w >> 0 & 0xFF]++;
      histogram[1][w >> 8 & 0xFF]++;
      histogram[2][w >> 16 & 0xFF]++;
      histogram[3][w >> 24 & 0xFF]++;
      histogram[4][w >> 32 & 0xFF]++;
      histogram[5][w >> 40 & 0xFF]++;
      histogram[6][w >> 48 & 0xFF]++;
      histogram[7][w >> 56 & 0xFF]++;
    } else {
      uint32_t w;
      memcpy(&w, data, 4);
      histogram[0][w >> 0 & 0xFF]++;
      histogram[1][w >> 8 & 0xFF]++;
      histogram[2][w >> 16 & 0xFF]++;
      histogram[3][w >> 24 & 0xFF]++;
      memcpy(&w, data + 4, 4);
      histogram[4][w >> 0 & 0xFF]++;
      histogram[5][w >> 8 & 0xFF]++;
      histogram[6][w >> 16 & 0xFF]++;
      histogram[7][w >> 24 & 0xFF]++;
    }
  }
  for (; data < end; data++) histogram[0][*data]++;
}

size_t encode_huffman_header(HuffmanSymbol huffmanSymbols[256],
                             size_t streamSizes[6], uint8_t* output,
                             bool useFastCodeGen) {
  // Encode stream sizes
  size_t maxStreamSize = 0;
  size_t minStreamSize = 99999999;
  for (int i = 0; i < 6; i++) {
    maxStreamSize = std::max(maxStreamSize, streamSizes[i]);
    minStreamSize = std::min(minStreamSize, streamSizes[i]);
  }
  size_t requiredBits = int_log2(maxStreamSize - minStreamSize) + 1;

  uint8_t* outputIt = output + 128;
  size_t state = minStreamSize;
  size_t bitCount = 16;
  state = (state << 4) | (requiredBits - 1);
  bitCount += 4;

  // Normalize
  write_uint32le(outputIt - 4, state << (32 - bitCount));
  outputIt -= bitCount / 8;
  bitCount &= 7;

  for (int i = 0; i < 6; i++) {
    state = (state << requiredBits) | (streamSizes[i] - minStreamSize);
    bitCount += requiredBits;

    // Normalize
    write_uint32le(outputIt - 4, state << (32 - bitCount));
    outputIt -= bitCount / 8;
    bitCount &= 7;
  }

  size_t zeroEnd = 256;
  for (; huffmanSymbols[zeroEnd - 1].bits == MAX_HUFFMAN_CODE_LENGTH + 1;
       zeroEnd--) {
  }

  HuffmanSymbol precodeSymbols[MAX_HUFFMAN_CODE_LENGTH + 2];
  for (size_t i = 1; i < MAX_HUFFMAN_CODE_LENGTH + 2; i++) {
    precodeSymbols[i].count = 0;
    precodeSymbols[i].symbol = i;
  }

  for (size_t i = 0; i < zeroEnd; i++) {
    int symbol = huffmanSymbols[i].bits;
    precodeSymbols[symbol].count++;

    // Avoid only one unique symbol getting a 0 length code
    if (precodeSymbols[symbol].count == zeroEnd) {
      precodeSymbols[symbol].count--;
      precodeSymbols[1 + symbol % 12].count = 1;
    }
  }

  if (useFastCodeGen)
    fast_huffman_codegen(precodeSymbols + 1, zeroEnd,
                         MAX_HUFFMAN_CODE_LENGTH + 1, MAX_PRECODE_CODE_LENGTH);
  else
    package_merge(precodeSymbols + 1, zeroEnd, MAX_HUFFMAN_CODE_LENGTH + 1,
                  MAX_PRECODE_CODE_LENGTH);

  // Convert bit lengths to codes
  // First sort by code length using radix sort
  {
    HuffmanSymbol sortedSymbols[MAX_PRECODE_CODE_LENGTH + 2]
                               [MAX_HUFFMAN_CODE_LENGTH + 2];
    size_t codeLengthCounts[MAX_PRECODE_CODE_LENGTH + 2] = {0};
    for (size_t i = 1; i <= MAX_HUFFMAN_CODE_LENGTH + 1; i++) {
      size_t bits = precodeSymbols[i].bits;
      sortedSymbols[bits][codeLengthCounts[bits]] = precodeSymbols[i];
      codeLengthCounts[bits]++;
    }
    // Now create the codes
    size_t accumulator = 0;
    for (size_t b = 1; b <= MAX_PRECODE_CODE_LENGTH; b++) {
      for (size_t i = 0; i < codeLengthCounts[b]; i++) {
        HuffmanSymbol& symbol = sortedSymbols[b][i];
        precodeSymbols[symbol.symbol].bits = symbol.bits;
        precodeSymbols[symbol.symbol].code =
            accumulator >> (MAX_PRECODE_CODE_LENGTH - symbol.bits);
        accumulator += PRECODE_CODE_SPACE >> symbol.bits;
      }
    }
  }

  bool zeroRun = true;
  size_t usedCodeSpace = 0;

  for (size_t i = 1; i < MAX_HUFFMAN_CODE_LENGTH + 2; i++) {
    if (zeroRun) {
      if (precodeSymbols[i].bits == MAX_PRECODE_CODE_LENGTH + 1) continue;
      state = (state << 3) | (i - 1);
      bitCount += 3;
      zeroRun = false;
    }

    state = (state << 3) | (precodeSymbols[i].bits - 1);
    bitCount += 3;
    usedCodeSpace += PRECODE_CODE_SPACE >> precodeSymbols[i].bits;

    // Normalize
    write_uint32le(outputIt - 4, state << (32 - bitCount));
    outputIt -= bitCount / 8;
    bitCount &= 7;

    if (usedCodeSpace == PRECODE_CODE_SPACE) break;
  }

  for (size_t i = 0; i < zeroEnd; i++) {
    // First encode
    int symbol = huffmanSymbols[i].bits;
    state =
        (state << precodeSymbols[symbol].bits) | precodeSymbols[symbol].code;
    bitCount += precodeSymbols[symbol].bits;

    // Normalize
    write_uint32le(outputIt - 4, state << (32 - bitCount));
    outputIt -= bitCount / 8;
    bitCount &= 7;
  }

  // Flush
  write_uint32le(outputIt - 4, state << (32 - bitCount));
  outputIt -= bitCount / 8;
  bitCount &= 7;
  if (bitCount > 0) {
    outputIt--;
    *outputIt = state << (8 - bitCount);
  }

  return output + 128 - outputIt;
}

void write_header(uint8_t*& output, size_t blockSize, int blockType) {
  // 1 byte header
  if (blockSize < 48) *output++ = blockType | blockSize << 2;
  // 2 byte header
  else if (blockSize < 3072) {
    *output++ = blockType | ((48 + (blockSize & 0xF)) << 2);
    *output++ = blockSize >> 4;
  }
  // 3 byte header
  else {
    *output++ = blockType | ((48 + (blockSize & 0xF)) << 2);
    *output++ = 192 + (blockSize >> 4 & 0x3F);
    *output++ = blockSize >> 10;
  }
}

// For optimal parse. Generates huffman codes and returns encoded size from a
// pregenerated histogram
size_t get_encoded_huffman_info(HuffmanSymbol symbols[256],
                                float decSpeedBias) {
  size_t symbolCount = 0;
  for (int i = 0; i < 256; i++) symbolCount += symbols[i].count;

  // Not enough symbols to justify compression or disabled huffman coding
  if (symbolCount < 32 || decSpeedBias >= 0.99) {
    for (int i = 0; i < 256; i++) symbols[i].bits = 8;
    return symbolCount;
  }
  float maxBitsPerByte = 8.0 - 4 * decSpeedBias;

  for (int i = 0; i < 256; i++) {
    // Check if there is only one unique symbol. If that is the case, give some
    // probability to
    //  another symbol. Having a probability of 1 might break something.
    if (symbols[i].count == symbolCount) {
      symbols[i].count--;
      symbols[(i + 1) % 256].count = 1;
      break;
    }
  }

  fast_huffman_codegen(symbols, symbolCount, 256, MAX_HUFFMAN_CODE_LENGTH);
  size_t compressedSize = 0;
  for (int i = 0; i < 256; i++)
    compressedSize += symbols[i].bits * symbols[i].count;
  compressedSize = compressedSize / 8;  // Convert to bytes

  // We also need the header size to know if huffman is worth, mostly for small
  // streams
  size_t zeroEnd = 256;
  for (; symbols[zeroEnd - 1].bits == MAX_HUFFMAN_CODE_LENGTH + 1; zeroEnd--) {
  }

  HuffmanSymbol precodeSymbols[MAX_HUFFMAN_CODE_LENGTH + 2];
  for (size_t i = 1; i < MAX_HUFFMAN_CODE_LENGTH + 2; i++)
    precodeSymbols[i].count = 0;

  for (size_t i = 0; i < zeroEnd; i++) {
    int symbol = symbols[i].bits;
    precodeSymbols[symbol].count++;
    // Avoid only one unique symbol getting a 0 length code
    if (precodeSymbols[symbol].count == zeroEnd) {
      precodeSymbols[symbol].count--;
      precodeSymbols[1 + symbol % 12].count = 1;
    }
  }

  fast_huffman_codegen(precodeSymbols + 1, zeroEnd, MAX_HUFFMAN_CODE_LENGTH + 1,
                       MAX_PRECODE_CODE_LENGTH);
  size_t headerSize = 0;
  for (int i = 1; i < MAX_HUFFMAN_CODE_LENGTH + 2; i++)
    headerSize += precodeSymbols[i].bits * precodeSymbols[i].count;
  headerSize = headerSize / 8 + 25;  // Convert to bytes and add some additional
                                     // overhead and inefficiencies

  // Not compressible
  if (headerSize + compressedSize >= symbolCount * maxBitsPerByte / 8) {
    for (int i = 0; i < 256; i++) symbols[i].bits = 8;
    return symbolCount;
  }

  return compressedSize + headerSize;
}

class HuffmanEncoder {
  // store huffman symbols
  uint8_t* symbolBuffer = nullptr;
  uint8_t* symbolBufferIt;
  HuffmanSymbol symbolData[256];
  // Store data in output instead of a separate buffer
  bool disableSymbolBuffer = false;
  int streamType;

 public:
  HuffmanEncoder() {}
  ~HuffmanEncoder() {
    if (!disableSymbolBuffer) delete[] symbolBuffer;
  }

  // Returns whether it fails to initialize
  bool initialize_huffman_encoder(const size_t bufferSize, int type,
                                  bool initializeSymbolBuffer) {
    try {
      if (initializeSymbolBuffer) {
        symbolBuffer = new uint8_t[bufferSize];
        symbolBufferIt = symbolBuffer;
      } else
        disableSymbolBuffer = true;
    } catch (const std::bad_alloc& e) {
      delete[] symbolBuffer;
      return true;
    }
    streamType = type;
    return false;
  }

  void start_huffman_block(uint8_t* outputBuffer = nullptr) {
    if (outputBuffer) {
      symbolBufferIt = outputBuffer + 3;
      symbolBuffer = outputBuffer + 3;
    } else
      symbolBufferIt = symbolBuffer;
  }
  FORCE_INLINE void add_variable_length(const uint32_t value, size_t bytes) {
    write_uint32le(symbolBufferIt, value);
    symbolBufferIt += bytes;
  }
  // Up to 8 elements
  FORCE_INLINE void add_literals_short(const uint8_t* literals, size_t count) {
    memcpy(symbolBufferIt, literals, 8);
    symbolBufferIt += count;
  }
  // Any number of elements
  FORCE_INLINE void add_literals_long(const uint8_t* literals, size_t count) {
    uint8_t* end = symbolBufferIt + count;
    do {
      memcpy(symbolBufferIt + 0, literals + 0, 16);
      memcpy(symbolBufferIt + 16, literals + 16, 16);
      literals += 32;
      symbolBufferIt += 32;
    } while (symbolBufferIt < end);
    symbolBufferIt = end;
  }
  FORCE_INLINE void add_byte(const uint8_t literal) {
    *symbolBufferIt++ = literal;
  }
  FORCE_INLINE size_t bytes_in_buffer() {
    return symbolBufferIt - symbolBuffer;
  }

  float calculate_entropy(HuffmanSymbol counts[256], size_t symbolCount) {
    float entropy = 0;
    const float probDiv = 1 / float(symbolCount);
    for (size_t i = 0; i < 256; i++) {
      if (counts[i].count) {
        const float prob = counts[i].count * probDiv;
        entropy -= prob * fast_log2(prob);
      }
    }
    return entropy;
  }
  FORCE_INLINE void encode_op(HuffmanSymbol symbolData[256],
                              const uint8_t symbol, size_t& state,
                              size_t& bitCount) {
    state = (state << symbolData[symbol].bits) | symbolData[symbol].code;
    bitCount += symbolData[symbol].bits;
  }
  FORCE_INLINE void renormalize(size_t& state, size_t& bitCount,
                                uint8_t*& streamBufferIt) {
    // Branchless write
    if (IS_64BIT)
      write_uint64le(streamBufferIt - 8, state << (64 - bitCount));
    else
      write_uint32le(streamBufferIt - 4, state << (32 - bitCount));
    streamBufferIt -= bitCount / 8;
    bitCount &= 7;
  }
  void end_stream(size_t state, size_t bitCount, uint8_t*& streamBufferIt) {
    renormalize(state, bitCount, streamBufferIt);
    if (bitCount > 0) {
      streamBufferIt--;
      *streamBufferIt = state << (8 - bitCount);
    }
  }
  size_t encode_stream(HuffmanSymbol symbolData[256], const uint8_t* symbolIt,
                       const uint8_t* symbolEnd, uint8_t* output) {
    const uint8_t* const fastLoopEnd = symbolEnd - (IS_64BIT ? 30 : 12);
    uint8_t* streamBufferIt = output;
    size_t state = 0;
    size_t bitCount = 0;

    // Fast loop: write 5 symbols per iteration
    for (; symbolIt < fastLoopEnd;) {
      encode_op(symbolData, symbolIt[0], state, bitCount);
      encode_op(symbolData, symbolIt[6], state, bitCount);
      if (IS_64BIT) {
        encode_op(symbolData, symbolIt[12], state, bitCount);
        encode_op(symbolData, symbolIt[18], state, bitCount);
        encode_op(symbolData, symbolIt[24], state, bitCount);
      }
      symbolIt += IS_64BIT ? 30 : 12;
      renormalize(state, bitCount, streamBufferIt);
    }
    for (; symbolIt < symbolEnd;) {
      encode_op(symbolData, symbolIt[0], state, bitCount);
      symbolIt += 6;  // Number of streams
    }
    // Output remaining bits
    end_stream(state, bitCount, streamBufferIt);

    size_t streamSize = output - streamBufferIt;
    return streamSize;
  }
  size_t store_block_raw(const uint8_t* symbols, uint8_t* output,
                         const size_t symbolCount) {
    // Symbols are stored in output
    // This means we just write the header
    if (disableSymbolBuffer) {
      size_t symbolCount = symbolBufferIt - output - 3;
      // Always use 3 byte headers to avoid the memcpy, even if it is wasteful
      *output++ = ENTROPY_RAW | ((48 + (symbolCount & 0xF)) << 2);
      *output++ = 192 + (symbolCount >> 4 & 0x3F);
      *output++ = symbolCount >> 10;
      return symbolCount + 3;
    }

    uint8_t* outputStart = output;
    write_header(output, symbolCount, ENTROPY_RAW);
    memcpy(output, symbols, symbolCount);
    return output - outputStart + symbolCount;
  }
  HuffmanSymbol* get_huffman_codes(float decSpeedBias) {
    const size_t symbolCount = symbolBufferIt - symbolBuffer;
    uint32_t histogram[8][256];
    memset(histogram, 0, 8 * 256 * sizeof(uint32_t));
    symbol_histogram(symbolBuffer, symbolCount, histogram);

    for (int i = 0; i < 256; i++) {
      symbolData[i].symbol = i;
      symbolData[i].count = histogram[0][i] + histogram[1][i] +
                            histogram[2][i] + histogram[3][i] +
                            histogram[4][i] + histogram[5][i] +
                            histogram[6][i] + histogram[7][i];

      // Check if there is only one unique symbol. If that is the case, give
      // some probability to
      //  another symbol. Having a probability of 1 might break something.
      if (symbolData[i].count == symbolCount) {
        symbolData[i].count--;
        symbolData[(i + 1) % 256].count = 1;
        break;
      }
    }

    size_t encodedSize = get_encoded_huffman_info(symbolData, decSpeedBias);
    return symbolData;
  }
  // The pointer to stream buffer is to the end of it! The huffman stream is
  // written backwards.
  size_t encode_symbols(uint8_t* output, uint8_t* streamBuffer,
                        float decSpeedBias, bool useFastCodeGen,
                        bool doCompressibilityCheck) {
    const size_t symbolCount = symbolBufferIt - symbolBuffer;
    // Check RLE coding
    if (symbolCount >= 2) {
      size_t len = test_match<false>(symbolBuffer + 1, symbolBuffer,
                                     symbolBuffer + symbolCount, 0, 0);
      if (len == symbolCount - 1) {
        uint8_t* const outputStart = output;
        write_header(output, symbolCount, ENTROPY_RLE);
        *output++ = symbolBuffer[0];
        return output - outputStart;
      }
    }

    // Not enough symbols to justify compression or disabled huffman coding
    if (symbolCount < 32 || !streamBuffer || decSpeedBias >= 0.99f)
      return store_block_raw(symbolBuffer, output, symbolCount);

    uint8_t* const outputStart = output;
    float maxBitsPerByte = 8.0f - 4.0f * decSpeedBias;

    uint32_t histogram[8][256];
    memset(histogram, 0, 8 * 256 * sizeof(uint32_t));

    // Make histograms and test if stream is compressible enough
    size_t readHistogramSymbols = 0;
    if (doCompressibilityCheck) {
      readHistogramSymbols = std::min((size_t)16384, symbolCount);
      symbol_histogram(symbolBuffer, readHistogramSymbols, histogram);

      for (int i = 0; i < 256; i++) {
        symbolData[i].symbol = i;
        symbolData[i].count = histogram[0][i] + histogram[1][i] +
                              histogram[2][i] + histogram[3][i] +
                              histogram[4][i] + histogram[5][i] +
                              histogram[6][i] + histogram[7][i];
      }

      float entropy = calculate_entropy(symbolData, readHistogramSymbols);
      // Reduce maxBitsPerByte a bit to account for the fact huffman usually
      // does not achieve optimal entropy
      if (readHistogramSymbols * entropy + 128 * 8 >
          readHistogramSymbols * (maxBitsPerByte - 0.05f))
        return store_block_raw(symbolBuffer, output, symbolCount);
    }

    if (symbolCount > readHistogramSymbols) {
      symbol_histogram(symbolBuffer + readHistogramSymbols,
                       symbolCount - readHistogramSymbols, histogram);

      for (int i = 0; i < 256; i++) {
        symbolData[i].symbol = i;
        symbolData[i].count = histogram[0][i] + histogram[1][i] +
                              histogram[2][i] + histogram[3][i] +
                              histogram[4][i] + histogram[5][i] +
                              histogram[6][i] + histogram[7][i];
      }

      if (doCompressibilityCheck) {
        float entropy = calculate_entropy(symbolData, symbolCount);
        if (symbolCount * entropy + 128 * 8 >
            symbolCount * (maxBitsPerByte - 0.05f))
          return store_block_raw(symbolBuffer, output, symbolCount);
      }
    }

    if (useFastCodeGen)
      fast_huffman_codegen(symbolData, symbolCount, 256,
                           MAX_HUFFMAN_CODE_LENGTH);
    else
      package_merge(symbolData, symbolCount, 256, MAX_HUFFMAN_CODE_LENGTH);

    // Convert bit lengths to codes
    // First sort by code length using radix sort
    {
      HuffmanSymbol sortedSymbols[MAX_HUFFMAN_CODE_LENGTH + 2][256];
      size_t codeLengthCounts[MAX_HUFFMAN_CODE_LENGTH + 2] = {0};
      for (size_t i = 0; i < 256; i++) {
        size_t bits = symbolData[i].bits;
        sortedSymbols[bits][codeLengthCounts[bits]] = symbolData[i];
        codeLengthCounts[bits]++;
      }
      // Now create the codes
      size_t accumulator = 0;
      for (size_t b = 1; b <= MAX_HUFFMAN_CODE_LENGTH; b++) {
        for (size_t i = 0; i < codeLengthCounts[b]; i++) {
          HuffmanSymbol& symbol = sortedSymbols[b][i];
          symbolData[symbol.symbol].bits = symbol.bits;
          symbolData[symbol.symbol].code =
              accumulator >> (MAX_HUFFMAN_CODE_LENGTH - symbol.bits);
          accumulator += HUFFMAN_CODE_SPACE >> symbol.bits;
        }
      }
    }

    size_t streamSizes[6];
    size_t cumulativeStreamSize = 0;
    for (int i = 0; i < 6; i++) {
      size_t streamSize = encode_stream(symbolData, symbolBuffer + i,
                                        symbolBuffer + symbolCount,
                                        streamBuffer - cumulativeStreamSize);
      streamSizes[i] = streamSize;
      cumulativeStreamSize += streamSize;
    }

    // Store the tree
    uint8_t huffmanHeader[128];
    size_t headerSize = encode_huffman_header(symbolData, streamSizes,
                                              huffmanHeader, useFastCodeGen);

    // Last size check, to make sure huffman DOES help
    if (headerSize + cumulativeStreamSize + 1 >=
        symbolCount * maxBitsPerByte / 8)
      return store_block_raw(symbolBuffer, outputStart, symbolCount);

    write_header(output, symbolCount, ENTROPY_HUFFMAN);
    *output++ = headerSize;
    memcpy(output, huffmanHeader + 128 - headerSize, headerSize);
    output += headerSize;

    cumulativeStreamSize = 0;
    for (int i = 0; i < 6; i++) {
      cumulativeStreamSize += streamSizes[i];
      memcpy(output, streamBuffer - cumulativeStreamSize, streamSizes[i]);
      output += streamSizes[i];
    }

    return output - outputStart;
  }
};

FORCE_INLINE void encode_length(HuffmanEncoder* lengthEncoder, size_t var,
                                const size_t overflow) {
  var -= overflow;
  if (likely(var <= 223))
    lengthEncoder->add_byte(var);
  else {
    var -= 224;
    lengthEncoder->add_byte(224 | (var & 0x1F));
    var >>= 5;

    if (likely(var <= 223))
      lengthEncoder->add_byte(var);
    else {
      var -= 224;
      lengthEncoder->add_byte(224 | (var & 0x1F));
      lengthEncoder->add_byte(var >> 5);
    }
  }
}

FORCE_INLINE void encode_literal_run(HuffmanEncoder* literalEncoder,
                                     HuffmanEncoder* lengthEncoder,
                                     const uint8_t* const input,
                                     const uint8_t* const literalRunStart,
                                     uint8_t* const controlByte) {
  const size_t literalRunLength = input - literalRunStart;

  if (literalRunLength >= 7) {
    *controlByte = (7 << 5);
    encode_length(lengthEncoder, literalRunLength, 7);
    literalEncoder->add_literals_long(literalRunStart, literalRunLength);
  } else {
    *controlByte = (literalRunLength << 5);
    // It is faster to unconditionally copy 8 bytes
    literalEncoder->add_literals_short(literalRunStart, literalRunLength);
  }
}

// Only encodes matches with rep offset
FORCE_INLINE void encode_rep_match(HuffmanEncoder* tokenEncoder,
                                   HuffmanEncoder* lengthEncoder,
                                   uint8_t* const controlByte,
                                   size_t* matchLength,
                                   const bool disableHuffman) {
  if (*matchLength <= 8) {
    *matchLength -= SKANDA_MIN_MATCH_LENGTH;
    tokenEncoder->add_byte(*controlByte | *matchLength);
  }
  // Send matches with length 9-16 as one match and one rep.
  // This is usually much faster to decode since it avoids a branch.
  // Do this only if huffman is disabled, otherwise it hurts ratio.
  else if (*matchLength <= 16 && disableHuffman) {
    tokenEncoder->add_byte(*controlByte | 6 - (*matchLength == 9));
    tokenEncoder->add_byte(*matchLength - 10 + (*matchLength == 9));
  } else {
    tokenEncoder->add_byte(*controlByte | 7);
    encode_length(lengthEncoder, *matchLength, 9);
  }
}

FORCE_INLINE void encode_match(HuffmanEncoder* tokenEncoder,
                               HuffmanEncoder* lengthEncoder,
                               HuffmanEncoder* distanceEncoder,
                               uint8_t* const controlByte, size_t* matchLength,
                               const size_t& distance, size_t* repOffset,
                               const bool disableHuffman) {
  size_t bytes = unsafe_int_log2(distance) / 8 + 1;
  bytes = distance == *repOffset ? 0 : bytes;
  *repOffset = distance;
  distanceEncoder->add_variable_length(distance, bytes);
  *controlByte |= bytes << 3;

  if (*matchLength <= 8) {
    *matchLength -= SKANDA_MIN_MATCH_LENGTH;
    tokenEncoder->add_byte(*controlByte | *matchLength);
  } else if (*matchLength <= 16 && disableHuffman) {
    tokenEncoder->add_byte(*controlByte | 6 - (*matchLength == 9));
    tokenEncoder->add_byte(*matchLength - 10 + (*matchLength == 9));
  } else {
    tokenEncoder->add_byte(*controlByte | 7);
    encode_length(lengthEncoder, *matchLength, 9);
  }
}

size_t compress_ultra_fast(const uint8_t* input, const size_t size,
                           uint8_t* output, const int windowLog,
                           const float decSpeedBias,
                           const CompressorOptions& compressorOptions,
                           ProgressCallback* progress) {
  // Constants for this encoder
  const size_t accelerationThreshold = 4;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  // The last match or literal run must end before this limit
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffset = 1;
  // Keeps track of how many match searches we have made without success.
  // When we dont find matches, we will skip more or less depending on this
  // variable.
  size_t acceleration = 1 << accelerationThreshold;

  bool disableHuffman = decSpeedBias >= 0.99;
  size_t encoderMaxBlockSize =
      disableHuffman ? MAX_BLOCK_SIZE : MAX_BLOCK_SIZE / 2;
  size_t huffmanBufSize = std::min(encoderMaxBlockSize, size);

  const size_t hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
  HashTable<uint32_t, FastIntHash> lzdict;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  try {
    lzdict.init(hashLog);
    if (!disableHuffman) {
      huffmanStreamBuffer = new uint8_t[huffmanBufSize + 128];
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
    }
  } catch (const std::bad_alloc& e) {
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  HuffmanEncoder literalEncoder;
  HuffmanEncoder tokenEncoder;
  HuffmanEncoder lengthEncoder;
  HuffmanEncoder distanceEncoder;

  if (literalEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                LITERAL_STREAM, false) ||
      tokenEncoder.initialize_huffman_encoder(huffmanBufSize / 2 + 128,
                                              TOKEN_STREAM, true) ||
      lengthEncoder.initialize_huffman_encoder(huffmanBufSize / 8 + 128,
                                               LENGTH_STREAM, true) ||
      distanceEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                 DISTANCE_STREAM, true)) {
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    const size_t thisBlockSize = compressionLimit - input < encoderMaxBlockSize
                                     ? compressionLimit - input
                                     : encoderMaxBlockSize;
    const uint8_t* const thisBlockEnd = input + thisBlockSize;
    const uint8_t* const thisBlockStart = input;

    write_header(output, thisBlockSize, BLOCK_BASIC);

    literalEncoder.start_huffman_block(output);
    tokenEncoder.start_huffman_block();
    lengthEncoder.start_huffman_block();
    distanceEncoder.start_huffman_block();
    const uint8_t* literalRunStart = input;
    // Skip first byte of first block
    if (input == inputStart) input++;

    while (likely(input < thisBlockEnd)) {
      // Get possible match location and update the table
      uint32_t* const dictEntry = &lzdict[read_hash6(input)];
      const uint8_t* match = inputStart + *dictEntry;
      *dictEntry = input - inputStart;
      // Try to find a match
      size_t matchLength =
          test_match<true>(input, match, thisBlockEnd, 6, windowLog);

      // Have we found a match?
      if (matchLength) {
        // Add the next two positions to the table as well
        lzdict[read_hash6(input + 1)] = input + 1 - inputStart;
        lzdict[read_hash6(input + 2)] = input + 2 - inputStart;

        // Try to extend the match to the left
        while (input > literalRunStart && match > inputStart &&
               input[-1] == match[-1]) {
          matchLength++;
          input--;
          match--;
        }
        const size_t distance = input - match;

        // First output the literal run
        uint8_t controlByte;
        encode_literal_run(&literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte);

        input += matchLength;
        literalRunStart = input;
        // Output the match
        encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                     &controlByte, &matchLength, distance, &repOffset,
                     disableHuffman);

        // Try to find further rep matches
        while (true) {
          size_t off = input[1] == input[1 - repOffset] ? 1 : 2;
          matchLength = test_match<false>(input + off, input + off - repOffset,
                                          thisBlockEnd, 3, 0);
          if (matchLength == 0) break;

          input += off;
          uint8_t controlByte;
          encode_literal_run(&literalEncoder, &lengthEncoder, input,
                             literalRunStart, &controlByte);
          // Add two additional positions to the table
          lzdict[read_hash6(input + 0)] = input + 0 - inputStart;
          lzdict[read_hash6(input + 1)] = input + 1 - inputStart;

          input += matchLength;
          literalRunStart = input;
          encode_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                           &matchLength, disableHuffman);
        }

        acceleration = 1 << accelerationThreshold;
      }
      // If there isnt advance the position
      else {
        input += acceleration >> accelerationThreshold;
        acceleration++;
      }
    }

    // Maybe we went beyond block end because of acceleration
    input = thisBlockEnd;
    // Send last literal run
    if (input != literalRunStart) {
      uint8_t controlByte;
      encode_literal_run(&literalEncoder, &lengthEncoder, input,
                         literalRunStart, &controlByte);
      tokenEncoder.add_byte(controlByte);
    }

    output += literalEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                            decSpeedBias, true, true);
    output += distanceEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                             decSpeedBias, true, true);
    output += tokenEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                          decSpeedBias, true, true);
    output += lengthEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                           decSpeedBias, true, true);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        delete[] huffmanStreamBuffer;
        return 0;
      }
    }
  }

  delete[] huffmanStreamBuffer;
  write_header(output, SKANDA_LAST_BYTES, BLOCK_LAST);
  memcpy(output, input, SKANDA_LAST_BYTES);
  if (progress)
    progress->progress(size, output - outputStart + SKANDA_LAST_BYTES);

  return output - outputStart + SKANDA_LAST_BYTES;
}

size_t compress_greedy(const uint8_t* input, const size_t size, uint8_t* output,
                       const int windowLog, const float decSpeedBias,
                       const CompressorOptions& compressorOptions,
                       ProgressCallback* progress) {
  const size_t accelerationThreshold = 5;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffset = 1;
  size_t acceleration = 1 << accelerationThreshold;

  bool disableHuffman = decSpeedBias >= 0.99;
  size_t encoderMaxBlockSize =
      disableHuffman ? MAX_BLOCK_SIZE : MAX_BLOCK_SIZE / 2;
  size_t huffmanBufSize = std::min(encoderMaxBlockSize, size);

  const size_t hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
  HashTable<uint32_t, FastIntHash> lzdict;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  try {
    lzdict.init(hashLog);
    if (!disableHuffman) {
      huffmanStreamBuffer = new uint8_t[huffmanBufSize + 128];
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
    }
  } catch (const std::bad_alloc& e) {
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  HuffmanEncoder literalEncoder;
  HuffmanEncoder tokenEncoder;
  HuffmanEncoder lengthEncoder;
  HuffmanEncoder distanceEncoder;

  if (literalEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                LITERAL_STREAM, false) ||
      tokenEncoder.initialize_huffman_encoder(huffmanBufSize / 2 + 128,
                                              TOKEN_STREAM, true) ||
      lengthEncoder.initialize_huffman_encoder(huffmanBufSize / 8 + 128,
                                               LENGTH_STREAM, true) ||
      distanceEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                 DISTANCE_STREAM, true)) {
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    const size_t thisBlockSize = compressionLimit - input < encoderMaxBlockSize
                                     ? compressionLimit - input
                                     : encoderMaxBlockSize;
    const uint8_t* const thisBlockEnd = input + thisBlockSize;
    const uint8_t* const thisBlockStart = input;

    write_header(output, thisBlockSize, BLOCK_BASIC);

    literalEncoder.start_huffman_block(output);
    tokenEncoder.start_huffman_block();
    lengthEncoder.start_huffman_block();
    distanceEncoder.start_huffman_block();
    const uint8_t* literalRunStart = input;
    // Skip first byte of first block
    if (input == inputStart) input++;

    while (input < thisBlockEnd) {
      // First try to find a rep match. Doing it at position +1 gives better
      // results. If one is found simply take it and skip normal match finding.
      size_t matchLength = test_match<false>(input + 1, input + 1 - repOffset,
                                             thisBlockEnd, 3, 0);
      if (matchLength) {
        input++;
        uint8_t controlByte;
        encode_literal_run(&literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte);

        lzdict[read_hash5(input) + 0] = input + 0 - inputStart;

        input += matchLength;
        literalRunStart = input;
        encode_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                         &matchLength, disableHuffman);
        acceleration = 1 << accelerationThreshold;
        continue;
      }

      // If no rep, try a normal match
      uint32_t* dictEntry = &lzdict[read_hash5(input)];
      const uint8_t* match = inputStart + *dictEntry;
      *dictEntry = input - inputStart;
      matchLength = test_match<true>(input, match, thisBlockEnd, 5, windowLog);

      if (matchLength) {
        // Add as many positions as minimum searched length
        lzdict[read_hash5(input + 1)] = input - inputStart + 1;
        lzdict[read_hash5(input + 2)] = input - inputStart + 2;
        lzdict[read_hash5(input + 3)] = input - inputStart + 3;
        lzdict[read_hash5(input + 4)] = input - inputStart + 4;

        while (input > literalRunStart && match > inputStart &&
               input[-1] == match[-1]) {
          matchLength++;
          input--;
          match--;
        }

        uint8_t controlByte;
        encode_literal_run(&literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte);

        size_t distance = input - match;
        input += matchLength;
        literalRunStart = input;
        encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                     &controlByte, &matchLength, distance, &repOffset,
                     disableHuffman);
        acceleration = 1 << accelerationThreshold;
      } else {
        input += acceleration >> accelerationThreshold;
        acceleration++;
      }
    }

    input = thisBlockEnd;
    if (input != literalRunStart) {
      uint8_t controlByte;
      encode_literal_run(&literalEncoder, &lengthEncoder, input,
                         literalRunStart, &controlByte);
      tokenEncoder.add_byte(controlByte);
    }

    output += literalEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                            decSpeedBias, true, true);
    output += distanceEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                             decSpeedBias, true, true);
    output += tokenEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                          decSpeedBias, true, true);
    output += lengthEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                           decSpeedBias, true, true);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        delete[] huffmanStreamBuffer;
        return 0;
      }
    }
  }

  delete[] huffmanStreamBuffer;
  write_header(output, SKANDA_LAST_BYTES, BLOCK_LAST);
  memcpy(output, input, SKANDA_LAST_BYTES);
  if (progress)
    progress->progress(size, output - outputStart + SKANDA_LAST_BYTES);

  return output - outputStart + SKANDA_LAST_BYTES;
}

size_t compress_lazy_fast(const uint8_t* input, const size_t size,
                          uint8_t* output, const int windowLog,
                          const float decSpeedBias,
                          const CompressorOptions& compressorOptions,
                          ProgressCallback* progress) {
  const size_t accelerationThreshold = 6;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffset = 1;
  size_t acceleration = 1 << accelerationThreshold;

  bool disableHuffman = decSpeedBias >= 0.99;
  size_t encoderMaxBlockSize =
      disableHuffman ? MAX_BLOCK_SIZE : MAX_BLOCK_SIZE / 2;
  size_t huffmanBufSize = std::min(encoderMaxBlockSize, size);

  const size_t hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
  HashTable<uint32_t, FastIntHash> lzdict4;
  HashTable<uint32_t, FastIntHash> lzdict8;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  try {
    lzdict4.init(hashLog);
    lzdict8.init(hashLog);
    if (!disableHuffman) {
      huffmanStreamBuffer = new uint8_t[huffmanBufSize + 128];
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
    }
  } catch (const std::bad_alloc& e) {
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  HuffmanEncoder literalEncoder;
  HuffmanEncoder tokenEncoder;
  HuffmanEncoder lengthEncoder;
  HuffmanEncoder distanceEncoder;

  if (literalEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                LITERAL_STREAM, false) ||
      tokenEncoder.initialize_huffman_encoder(huffmanBufSize / 2 + 128,
                                              TOKEN_STREAM, true) ||
      lengthEncoder.initialize_huffman_encoder(huffmanBufSize / 8 + 128,
                                               LENGTH_STREAM, true) ||
      distanceEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                 DISTANCE_STREAM, true)) {
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    const size_t thisBlockSize = compressionLimit - input < encoderMaxBlockSize
                                     ? compressionLimit - input
                                     : encoderMaxBlockSize;
    const uint8_t* const thisBlockEnd = input + thisBlockSize;
    const uint8_t* const thisBlockStart = input;

    write_header(output, thisBlockSize, BLOCK_BASIC);

    literalEncoder.start_huffman_block(output);
    tokenEncoder.start_huffman_block();
    lengthEncoder.start_huffman_block();
    distanceEncoder.start_huffman_block();
    const uint8_t* literalRunStart = input;
    // Skip first byte of first block
    if (input == inputStart) input++;

    while (input < thisBlockEnd) {
      // First try finding a lazy rep match. Take it if its sufficiently long,
      // otherwise check if there is
      //  a longer normal match.
      size_t repMatchLength = test_match<false>(
          input + 1, input + 1 - repOffset, thisBlockEnd, 2, 0);
      if (repMatchLength >= 4) {
        // Update table
        lzdict4[read_hash4(input + 0)] = (input + 0 - inputStart);
        lzdict4[read_hash4(input + 1)] = (input + 1 - inputStart);
        lzdict4[read_hash4(input + 2)] = (input + 2 - inputStart);
        lzdict4[read_hash4(input + 3)] = (input + 3 - inputStart);
        lzdict4[read_hash4(input + 4)] = (input + 4 - inputStart);
        lzdict8[read_hash8(input + 1)] = (input + 1 - inputStart);

        input++;
        uint8_t controlByte;
        encode_literal_run(&literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte);

        input += repMatchLength;
        literalRunStart = input;
        encode_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                         &repMatchLength, disableHuffman);
        acceleration = 1 << accelerationThreshold;
        continue;
      }

      uint32_t* dictEntry4 = &(lzdict4)[read_hash4(input)];
      uint32_t pos4 = *dictEntry4;
      *dictEntry4 = input - inputStart;

      // First search for a length 4+ match. If we find a good one, test a
      // length 8+. Otherwise send a short rep match if we found one, or a
      // literal. Do not update the hash8 table if we did not find a match, it
      // somehow improves compression?
      const uint8_t* where = inputStart + pos4;
      size_t matchLength =
          test_match<true>(input, where, thisBlockEnd, 4, windowLog);
      size_t distance = input - where;
      if (matchLength >= repMatchLength + 2 &&
          (matchLength >= 5 || distance < 65536)) {
        uint32_t* dictEntry8 = &(lzdict8)[read_hash8(input)];
        uint32_t pos8 = *dictEntry8;
        *dictEntry8 = input - inputStart;

        where = inputStart + pos8;
        if (*(input + matchLength) == *(where + matchLength)) {
          size_t length =
              test_match<true>(input, where, thisBlockEnd, 8, windowLog);
          if (length > matchLength) {
            matchLength = length;
            distance = input - where;
          }
        }
      }
      // We found earlier a rep match to store
      else if (repMatchLength) {
        input++;
        lzdict4[read_hash4(input + 0)] = (input + 0 - inputStart);
        lzdict4[read_hash4(input + 1)] = (input + 1 - inputStart);

        uint8_t controlByte;
        encode_literal_run(&literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte);

        input += repMatchLength;
        literalRunStart = input;
        encode_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                         &repMatchLength, disableHuffman);
        acceleration = 1 << accelerationThreshold;
        continue;
      }
      // Code a literal and continue
      else {
        input += acceleration >> accelerationThreshold;
        acceleration++;
        continue;
      }

      int lazySteps = 0;  // bytes to skip because of lazy matching
      // Now try to get a better match at pos + 1
      const uint8_t* matchPos = input + 1;
      uint32_t* dictEntry8 = &(lzdict8)[read_hash8(matchPos)];
      dictEntry4 = &(lzdict4)[read_hash4(matchPos)];
      uint32_t pos8 = *dictEntry8;
      pos4 = *dictEntry4;
      *dictEntry4 = matchPos - inputStart;
      *dictEntry8 = matchPos - inputStart;

      // To speed up compression we only want to do one search, either on the
      // hash4 or on the hash8. Because we want a longer match we will check the
      // byte that would make the match found longer,
      //  and select the hash4 or the hash8 based on which of the 2 matched.
      const uint8_t* where4 = inputStart + pos4;
      const uint8_t* where8 = inputStart + pos8;
      bool matched4 = *(matchPos + matchLength) == *(where4 + matchLength);
      bool matched8 = *(matchPos + matchLength) == *(where8 + matchLength);

      if (matched4 || matched8) {
        where =
            matched8 ? where8 : where4;  // Prioritize hash8, bit better results
        size_t length =
            test_match<true>(matchPos, where, thisBlockEnd, 4, windowLog);
        if (length > matchLength) {
          distance = matchPos - where;
          matchLength = length;
          lazySteps = 1;
        }
      }

      // Try to get an even better match at pos + 2
      matchPos++;
      // Searching hash4 here does not seem to improve results
      (lzdict4)[read_hash4(matchPos)] = matchPos - inputStart;
      dictEntry8 = &(lzdict8)[read_hash8(matchPos)];
      pos8 = *dictEntry8;
      *dictEntry8 = matchPos - inputStart;

      where = inputStart + pos8;
      if (*(matchPos + matchLength) == *(where + matchLength)) {
        size_t length =
            test_match<true>(matchPos, where, thisBlockEnd, 8, windowLog);
        if (length > matchLength) {
          distance = matchPos - where;
          matchLength = length;
          lazySteps = 2;
        }
      }

      input += lazySteps;
      // Update hash table. Inserting the last positions of the match gives
      // surprisingly good results (from zstd_double_fast.c)
      lzdict4[read_hash4(matchPos + 1)] = (matchPos + 1 - inputStart);
      lzdict8[read_hash8(matchPos + 1)] = (matchPos + 1 - inputStart);
      lzdict4[read_hash4(input + matchLength - 1)] =
          (input + matchLength - 1 - inputStart);
      lzdict8[read_hash8(input + matchLength - 2)] =
          (input + matchLength - 2 - inputStart);
      lzdict4[read_hash4(input + matchLength - 3)] =
          (input + matchLength - 3 - inputStart);
      lzdict8[read_hash8(input + matchLength - 4)] =
          (input + matchLength - 4 - inputStart);

      const uint8_t* match = input - distance;
      while (input > literalRunStart && match > inputStart &&
             input[-1] == match[-1]) {
        matchLength++;
        input--;
        match--;
      }

      uint8_t controlByte;
      encode_literal_run(&literalEncoder, &lengthEncoder, input,
                         literalRunStart, &controlByte);

      input += matchLength;
      literalRunStart = input;
      encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                   &controlByte, &matchLength, distance, &repOffset,
                   disableHuffman);
      acceleration = 1 << accelerationThreshold;
    }

    input = thisBlockEnd;
    if (input != literalRunStart) {
      uint8_t controlByte;
      encode_literal_run(&literalEncoder, &lengthEncoder, input,
                         literalRunStart, &controlByte);
      tokenEncoder.add_byte(controlByte);
    }

    output += literalEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                            decSpeedBias, true, true);
    output += distanceEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                             decSpeedBias, true, true);
    output += tokenEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                          decSpeedBias, true, true);
    output += lengthEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                           decSpeedBias, true, true);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        delete[] huffmanStreamBuffer;
        return 0;
      }
    }
  }

  delete[] huffmanStreamBuffer;
  write_header(output, SKANDA_LAST_BYTES, BLOCK_LAST);
  memcpy(output, input, SKANDA_LAST_BYTES);
  if (progress)
    progress->progress(size, output - outputStart + SKANDA_LAST_BYTES);

  return output - outputStart + SKANDA_LAST_BYTES;
}

size_t compress_lazy(const uint8_t* input, const size_t size, uint8_t* output,
                     const int windowLog, const float decSpeedBias,
                     const CompressorOptions& compressorOptions,
                     ProgressCallback* progress) {
  const size_t accelerationThreshold = 6;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffset = 1;
  size_t acceleration = 1 << accelerationThreshold;

  bool disableHuffman = decSpeedBias >= 0.99;
  size_t encoderMaxBlockSize =
      disableHuffman ? MAX_BLOCK_SIZE : MAX_BLOCK_SIZE / 2;
  size_t huffmanBufSize = std::min(encoderMaxBlockSize, size);

  const int hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
  LZCacheTable<uint32_t, FastIntHash> lzdict4;
  LZCacheTable<uint32_t, FastIntHash> lzdict8;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  try {
    lzdict4.init(hashLog, compressorOptions.maxElementsLog);
    lzdict8.init(hashLog, compressorOptions.maxElementsLog);
    if (!disableHuffman) {
      huffmanStreamBuffer = new uint8_t[huffmanBufSize + 128];
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
    }
  } catch (const std::bad_alloc& e) {
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  HuffmanEncoder literalEncoder;
  HuffmanEncoder tokenEncoder;
  HuffmanEncoder lengthEncoder;
  HuffmanEncoder distanceEncoder;

  if (literalEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                LITERAL_STREAM, false) ||
      tokenEncoder.initialize_huffman_encoder(huffmanBufSize / 2 + 128,
                                              TOKEN_STREAM, true) ||
      lengthEncoder.initialize_huffman_encoder(huffmanBufSize / 8 + 128,
                                               LENGTH_STREAM, true) ||
      distanceEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                 DISTANCE_STREAM, true)) {
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    const size_t thisBlockSize = compressionLimit - input < encoderMaxBlockSize
                                     ? compressionLimit - input
                                     : encoderMaxBlockSize;
    const uint8_t* const thisBlockEnd = input + thisBlockSize;
    const uint8_t* const thisBlockStart = input;

    write_header(output, thisBlockSize, BLOCK_BASIC);

    literalEncoder.start_huffman_block(output);
    tokenEncoder.start_huffman_block();
    lengthEncoder.start_huffman_block();
    distanceEncoder.start_huffman_block();
    const uint8_t* literalRunStart = input;
    // Skip first byte of first block
    if (input == inputStart) input++;

    while (input < thisBlockEnd) {
      // First try a rep match
      size_t repMatchLength = test_match<false>(
          input + 1, input + 1 - repOffset, thisBlockEnd, 2, 0);
      if (repMatchLength >= 4) {
        // Update table
        lzdict4[read_hash4(input + 0)].push(input + 0 - inputStart);
        lzdict8[read_hash8(input + 0)].push(input + 0 - inputStart);
        lzdict4[read_hash4(input + 1)].push(input + 1 - inputStart);
        lzdict8[read_hash8(input + 1)].push(input + 1 - inputStart);
        lzdict4[read_hash4(input + 2)].push(input + 2 - inputStart);
        lzdict8[read_hash8(input + 2)].push(input + 2 - inputStart);
        lzdict4[read_hash4(input + 3)].push(input + 3 - inputStart);
        lzdict8[read_hash8(input + 3)].push(input + 3 - inputStart);
        lzdict4[read_hash4(input + 4)].push(input + 4 - inputStart);
        lzdict8[read_hash8(input + 4)].push(input + 4 - inputStart);

        input++;
        uint8_t controlByte;
        encode_literal_run(&literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte);

        input += repMatchLength;
        literalRunStart = input;
        encode_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                         &repMatchLength, disableHuffman);
        acceleration = 1 << accelerationThreshold;
        continue;
      }

      size_t matchLength = 0;
      size_t distance;

      LZCacheBucket<uint32_t> chain4 = (lzdict4)[read_hash4(input)];
      int lazySteps = 0;

      uint32_t pos4 = input - inputStart;
      while (!chain4.ended()) {
        chain4.next(&pos4);
        const uint8_t* where = inputStart + pos4;
        if (*(input + matchLength) != *(where + matchLength)) continue;
        size_t length =
            test_match<true>(input, where, thisBlockEnd, 4, windowLog);
        size_t thisDistance = input - where;
        if (length >= repMatchLength + 2 && length > matchLength &&
            (length >= 5 || thisDistance < 65536)) {
          distance = thisDistance;
          matchLength = length;
        }
      }

      if (matchLength >= 4) {
        LZCacheBucket<uint32_t> chain8 = (lzdict8)[read_hash8(input)];
        uint32_t pos8 = input - inputStart;
        while (!chain8.ended()) {
          chain8.next(&pos8);
          const uint8_t* where = inputStart + pos8;
          if (*(input + matchLength) != *(where + matchLength)) continue;
          size_t length =
              test_match<true>(input, where, thisBlockEnd, 8, windowLog);
          if (length > matchLength) {
            distance = input - where;
            matchLength = length;
          }
        }
      } else if (repMatchLength) {
        input++;
        uint8_t controlByte;
        encode_literal_run(&literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte);

        lzdict4[read_hash4(input + 0)].push(input + 0 - inputStart);
        lzdict4[read_hash4(input + 1)].push(input + 1 - inputStart);

        input += repMatchLength;
        literalRunStart = input;
        encode_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                         &repMatchLength, disableHuffman);
        acceleration = 1 << accelerationThreshold;
        continue;
      } else {
        input += acceleration >> accelerationThreshold;
        acceleration++;
        continue;
      }

      // Now try to get a better match at pos + 1
      const uint8_t* matchPos = input + 1;
      uint32_t pos8 = matchPos - inputStart;
      pos4 = matchPos - inputStart;
      chain4 = (lzdict4)[read_hash4(matchPos)];
      LZCacheBucket<uint32_t> chain8 = (lzdict8)[read_hash8(matchPos)];

      while (!chain8.ended()) {
        chain8.next(&pos8);
        chain4.next(&pos4);

        const uint8_t* where8 = inputStart + pos8;
        const uint8_t* where4 = inputStart + pos4;

        bool matched8 = *(matchPos + matchLength) == *(where8 + matchLength);
        bool matched4 = *(matchPos + matchLength) == *(where4 + matchLength);

        if (matched8 || matched4) {
          const uint8_t* where = matched8 ? where8 : where4;
          size_t length =
              test_match<true>(matchPos, where, thisBlockEnd, 4, windowLog);
          if (length > matchLength) {
            distance = matchPos - where;
            matchLength = length;
            lazySteps = 1;
          }
        }
      }

      // Now get an even better match at pos + 2
      matchPos++;
      pos8 = matchPos - inputStart;
      (lzdict4)[read_hash4(matchPos)].push(pos8);
      chain8 = (lzdict8)[read_hash8(matchPos)];

      while (!chain8.ended()) {
        chain8.next(&pos8);
        const uint8_t* where = inputStart + pos8;
        if (*(matchPos + matchLength) != *(where + matchLength)) continue;
        size_t length =
            test_match<true>(matchPos, where, thisBlockEnd, 8, windowLog);
        if (length > matchLength) {
          distance = matchPos - where;
          matchLength = length;
          lazySteps = 2;
        }
      }

      // Output match and update hash table
      matchPos++;
      input += lazySteps;
      const uint8_t* const updateEnd =
          input + std::min(matchLength, (size_t)16);
      for (; matchPos < updateEnd; matchPos++) {
        lzdict4[read_hash4(matchPos)].push(matchPos - inputStart);
        lzdict8[read_hash8(matchPos)].push(matchPos - inputStart);
      }

      const uint8_t* match = input - distance;
      while (input > literalRunStart && match > inputStart &&
             input[-1] == match[-1]) {
        matchLength++;
        input--;
        match--;
      }

      uint8_t controlByte;
      encode_literal_run(&literalEncoder, &lengthEncoder, input,
                         literalRunStart, &controlByte);

      input += matchLength;
      literalRunStart = input;
      encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                   &controlByte, &matchLength, distance, &repOffset,
                   disableHuffman);
      acceleration = 1 << accelerationThreshold;
    }

    input = thisBlockEnd;
    if (input != literalRunStart) {
      uint8_t controlByte;
      encode_literal_run(&literalEncoder, &lengthEncoder, input,
                         literalRunStart, &controlByte);
      tokenEncoder.add_byte(controlByte);
    }

    output += literalEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                            decSpeedBias, true, true);
    output += distanceEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                             decSpeedBias, true, true);
    output += tokenEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                          decSpeedBias, true, true);
    output += lengthEncoder.encode_symbols(output, huffmanStreamBufferBegin,
                                           decSpeedBias, true, true);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        delete[] huffmanStreamBuffer;
        return 0;
      }
    }
  }

  delete[] huffmanStreamBuffer;
  write_header(output, SKANDA_LAST_BYTES, BLOCK_LAST);
  memcpy(output, input, SKANDA_LAST_BYTES);
  if (progress)
    progress->progress(size, output - outputStart + SKANDA_LAST_BYTES);

  return output - outputStart + SKANDA_LAST_BYTES;
}

struct SkandaOptimalParserState {
  // 16 high bits store size cost, 16 low bits the speed cost
  // This way we can compare costs, prioritising one over another in a single
  // operation
  uint32_t cost;
  // Serves as both match distance and rep offset.
  uint32_t distance;
  uint32_t literalRunLength;
  uint16_t matchLength;
  uint16_t path;
};

size_t get_length_cost(size_t length, HuffmanSymbol* lengthSymbols) {
  if (likely(length <= 223)) return lengthSymbols[length].bits;
  length -= 224;
  size_t cost = lengthSymbols[224 | (length & 0x1F)].bits;
  length >>= 5;
  if (likely(length <= 223)) return cost + lengthSymbols[length].bits;
  length -= 224;
  cost += lengthSymbols[224 | (length & 0x1F)].bits;
  return cost + lengthSymbols[length >> 5].bits;
}

FORCE_INLINE size_t get_token_cost(size_t literalRunLength, size_t matchLength,
                                   size_t distanceBytes,
                                   HuffmanSymbol* tokenSymbols) {
  return tokenSymbols[(std::min(literalRunLength, (size_t)7) << 5) |
                      (distanceBytes << 3) |
                      (std::min(matchLength, (size_t)9) - 2)]
      .bits;
}

LZStructure* forward_optimal_parse(
    const uint8_t* const input, const uint8_t* const inputStart,
    const uint8_t* const blockLimit, HashTableMatchFinder* matchFinder,
    SkandaOptimalParserState* parser, LZStructure* stream,
    const size_t startRepOffset, size_t* acceleration,
    const size_t accelerationThreshold, HuffmanSymbol* literalSymbols,
    HuffmanSymbol* tokenSymbols, HuffmanSymbol* lengthSymbols,
    HuffmanSymbol* distanceSymbols, const bool disableHuffman,
    const CompressorOptions& compressorOptions, const int windowLog) {
  const size_t blockLength = std::min(
      (size_t)(blockLimit - input), (size_t)compressorOptions.optimalBlockSize);
  // Initialize positions cost to maximum. We dont need to initialize ALL, only
  // enough ahead
  //  to cover the maximum match length we can write, which is niceLength - 1,
  //  otherwise we would simply skip.
  // This speeds up compression on data with a lot of long matches.
  for (size_t i = 1; i < compressorOptions.niceLength; i++)
    parser[i].cost = UINT32_MAX;
  parser[0].cost = 64 << 16;
  parser[0].distance = startRepOffset;
  parser[0].literalRunLength = 0;

  size_t lastMatchLength = 0;
  size_t lastMatchDistance;

  size_t position = 0;
  bool positionSkip = false;
  for (; position < blockLength; position++) {
    const uint8_t* inputPosition = input + position;
    SkandaOptimalParserState* const parserPosition = parser + position;
    // Make sure we have enough positions ahead initialized
    parserPosition[compressorOptions.niceLength].cost = UINT32_MAX;

    // From Zstd: skip unpromising position
    if (parserPosition[0].cost >= parserPosition[1].cost || positionSkip) {
      matchFinder->update_position(inputPosition, inputStart);
      positionSkip = false;
      continue;
    }

    size_t literalCost = parserPosition->cost;
    // Size cost
    if (disableHuffman)
      literalCost += 0x80000 << (parserPosition->literalRunLength == 6);
    else {
      literalCost += literalSymbols[*inputPosition].bits << 16;
      if (parserPosition->literalRunLength >= 6) {
        literalCost += get_length_cost(parserPosition->literalRunLength + 1 - 7,
                                       lengthSymbols)
                       << 16;
        if (parserPosition->literalRunLength >= 7)
          literalCost -= get_length_cost(parserPosition->literalRunLength - 7,
                                         lengthSymbols)
                         << 16;
      }
    }
    // Speed cost
    literalCost += (parserPosition->literalRunLength == 6) * 2;
    SkandaOptimalParserState* nextPosition = parserPosition + 1;
    if (literalCost < nextPosition->cost) {
      nextPosition->cost = literalCost;
      nextPosition->distance =
          parserPosition->distance;  // Carry forward the rep offset
      nextPosition->literalRunLength = parserPosition->literalRunLength + 1;
    }

    size_t repMatchLength = test_match<false>(
        inputPosition, inputPosition - parserPosition->distance, blockLimit, 2,
        0);
    if (repMatchLength) {
      // Rep matches can be unconditionally taken with lower lengths
      if (repMatchLength >= compressorOptions.niceLength / 2) {
        matchFinder->update_position(inputPosition, inputStart);
        lastMatchLength = repMatchLength;
        lastMatchDistance = parserPosition->distance;
        break;
      }

      size_t repMatchCost = parserPosition->cost;
      // size cost
      if (disableHuffman) {
        // For decoding speed
        if (repMatchLength > 8 && repMatchLength <= 16) repMatchLength = 8;
        repMatchCost += 0x80000 << (repMatchLength > 8);  // size cost
      } else {
        repMatchCost += get_token_cost(parserPosition->literalRunLength,
                                       repMatchLength, 0, tokenSymbols)
                        << 16;
        if (repMatchLength > 8)
          repMatchCost += get_length_cost(repMatchLength - 9, lengthSymbols)
                          << 16;
      }
      // speed cost
      repMatchCost += 1 + (repMatchLength > 8) * 2;  // speed cost

      nextPosition = parserPosition + repMatchLength;
      if (repMatchCost < nextPosition->cost) {
        nextPosition->cost = repMatchCost;
        nextPosition->matchLength = repMatchLength;
        nextPosition->distance = parserPosition->distance;
        nextPosition->literalRunLength = 0;
      }

      // When finding a rep match skip the next position
      positionSkip = true;
      if (repMatchLength >= 3) *acceleration = 1 << accelerationThreshold;
    }

    LZMatch matches[17];
    LZMatch* matchesEnd = matchFinder->find_matches_and_update(
        inputPosition, inputStart, blockLimit, matches, repMatchLength + 1,
        windowLog, compressorOptions);

    // At least one match was found
    if (matchesEnd != matches) {
      // The last match should be the longest
      LZMatch* const longestMatch = matchesEnd - 1;
      if (longestMatch->length >= compressorOptions.niceLength) {
        lastMatchLength = longestMatch->length;
        lastMatchDistance = longestMatch->distance;
        break;
      }

      for (LZMatch* matchIt = longestMatch; matchIt >= matches; matchIt--) {
        size_t matchCost = parserPosition->cost;
        // size cost
        size_t distanceBytes = unsafe_int_log2(matchIt->distance) / 8;
        if (disableHuffman) {
          // For decoding speed
          if (matchIt->length > 8 && matchIt->length <= 16) matchIt->length = 8;
          matchCost += (2 + distanceBytes + (matchIt->length > 8)) << 19;
        } else {
          matchCost +=
              get_token_cost(parserPosition->literalRunLength, matchIt->length,
                             distanceBytes + 1, tokenSymbols)
              << 16;
          for (int i = 0; i <= distanceBytes; i++)
            matchCost +=
                distanceSymbols[(matchIt->distance >> i * 8) & 0xFF].bits << 16;
          if (matchIt->length > 8)
            matchCost += get_length_cost(matchIt->length - 9, lengthSymbols)
                         << 16;
        }
        // speed cost
        matchCost +=
            1 + (matchIt->length > 8) * 2 + (matchIt->distance > (1 << 16));

        nextPosition = parserPosition + matchIt->length;
        if (matchCost < nextPosition->cost) {
          nextPosition->cost = matchCost;
          nextPosition->matchLength = matchIt->length;
          nextPosition->distance = matchIt->distance;
          nextPosition->literalRunLength = 0;
        }
        // If this match does not help, the shorter ones probably wont
        else
          break;
      }

      if (longestMatch->length >= 4) *acceleration = 1 << accelerationThreshold;
    } else {
      (*acceleration)++;
      // If acceleration goes too high go to greedy parse
      if (*acceleration >= (2 << accelerationThreshold)) {
        position++;
        break;
      }
    }
  }

  // Backward pass, pick best option at each step.
  const SkandaOptimalParserState* backwardParse;
  const SkandaOptimalParserState* const parseEnd = parser;

  if (lastMatchLength) {
    stream->literalRunLength = 0;
    stream++;
    stream->matchDistance = lastMatchDistance;
    stream->matchLength = lastMatchLength;
    backwardParse = parser + position;
    stream->literalRunLength = backwardParse->literalRunLength;
    backwardParse -= backwardParse->literalRunLength;
    stream++;

    // Update only first positions if distance < length
    const uint8_t* const updateEnd =
        input + position +
        std::min(std::min(lastMatchDistance, lastMatchLength),
                 (size_t)compressorOptions.niceLength);
    const uint8_t* inputPosition = input + position + 1;
    for (; inputPosition < updateEnd; inputPosition++)
      matchFinder->update_position(inputPosition, inputStart);
  } else {
    size_t bestPos = position;
    size_t bestSize = parser[position].cost;
    for (size_t i = 1; i < compressorOptions.niceLength; i++) {
      if (parser[position + i].cost <= bestSize) {
        bestPos = position + i;
        bestSize = parser[position + i].cost;
      }
    }
    backwardParse = parser + bestPos;
    stream->literalRunLength = backwardParse->literalRunLength;
    backwardParse -= backwardParse->literalRunLength;
    stream++;
  }
  while (backwardParse > parseEnd) {
    stream->matchDistance = backwardParse->distance;
    stream->matchLength = backwardParse->matchLength;
    backwardParse -= backwardParse->matchLength;
    stream->literalRunLength = backwardParse->literalRunLength;
    backwardParse -= backwardParse->literalRunLength;
    stream++;
  }

  return stream - 1;
}

class MatchBuffer {
  LZMatch* matches = nullptr;
  int* matchesCount = nullptr;
  size_t maxMatchesPerPosition;

 public:
  MatchBuffer() {}
  ~MatchBuffer() {
    delete[] matches;
    delete[] matchesCount;
  }
  void init(size_t inSize, const CompressorOptions& compressorOptions) {
    maxMatchesPerPosition = (1 << compressorOptions.maxElementsLog) + 1;
    matches =
        new LZMatch[maxMatchesPerPosition * std::min(inSize, MAX_BLOCK_SIZE)];
    matchesCount = new int[std::min(inSize, MAX_BLOCK_SIZE)];
  }
  void find_matches(const uint8_t* in, const uint8_t* const inputStart,
                    const uint8_t* const blockLimit,
                    const uint8_t* const compressionLimit,
                    BinaryMatchFinder* matchFinder, const int windowLog,
                    const CompressorOptions& compressorOptions) {
    const size_t accelerationThreshold = 6;
    size_t acceleration = 1 << accelerationThreshold;

    size_t blockSize = blockLimit - in;
    size_t pos =
        in == inputStart ? 1 : 0;  // Skip first byte of uncompressed data
    for (; pos < blockSize;) {
      LZMatch* matchesEnd = matchFinder->find_matches_and_update(
          in + pos, inputStart, compressionLimit, blockLimit,
          matches + pos * maxMatchesPerPosition, 3, windowLog,
          compressorOptions);
      matchesCount[pos] = matchesEnd - (matches + pos * maxMatchesPerPosition);

      if (matchesCount[pos] != 0) {
        if (matchesEnd[-1].length >= compressorOptions.niceLength) {
          // Fill all the following positions with the match we have found
          size_t length = matchesEnd[-1].length - 1;
          for (pos++; length > 2; pos++, length--) {
            matches[pos * maxMatchesPerPosition].length = length;
            matches[pos * maxMatchesPerPosition].distance =
                matchesEnd[-1].distance;
            matchesCount[pos] = 1;
            matchFinder->update_position(in + pos, inputStart, compressionLimit,
                                         windowLog, compressorOptions);
          }
          continue;
        }
        if (matchesEnd[-1].length >= 4)
          acceleration = 1 << accelerationThreshold;
      }
      // Disable acceleration on level 9
      else if (compressorOptions.parser < OPTIMAL3)
        acceleration++;

      pos++;
      // Skip positions
      for (size_t skip = 1;
           skip < (acceleration >> accelerationThreshold) && pos < blockSize;
           skip++, pos++)
        matchesCount[pos] = 0;
    }
  }
  void get_position_matches(LZMatch** matchesBegin, LZMatch** matchesEnd,
                            size_t pos) {
    *matchesBegin = matches + pos * maxMatchesPerPosition;
    *matchesEnd = *matchesBegin + matchesCount[pos];
  }
};

LZStructure* multi_arrivals_parse(
    const uint8_t* input, const uint8_t* const inputStart,
    const uint8_t* const blockStart, const uint8_t* const compressionLimit,
    const uint8_t* blockLimit, BinaryMatchFinder* matchFinder,
    MatchBuffer* matchBuffer, SkandaOptimalParserState* parser,
    LZStructure* stream, const size_t startRepOffset, size_t* acceleration,
    const size_t accelerationThreshold, HuffmanSymbol* literalSymbols,
    HuffmanSymbol* tokenSymbols, HuffmanSymbol* lengthSymbols,
    HuffmanSymbol* distanceSymbols, const bool disableHuffman,
    const CompressorOptions& compressorOptions, const int windowLog) {
  const size_t MAX_LENGTH_REDUCTION = disableHuffman ? 0 : 3;

  const size_t blockLength =
      std::min((size_t)(blockLimit - input),
               (size_t)compressorOptions.optimalBlockSize - 1);
  for (size_t i = 0;
       i < compressorOptions.niceLength * compressorOptions.maxArrivals; i++)
    parser[i].cost = UINT32_MAX;
  // Fill only the first arrival of the first position
  parser[0].cost = 64 << 16;
  parser[0].distance = startRepOffset;
  parser[0].literalRunLength = 0;

  size_t lastMatchLength = 0;
  size_t lastMatchDistance;
  size_t lastMatchPath;

  size_t position = 0;
  for (; position < blockLength; position++) {
    const size_t blockPos = input + position - blockStart;
    const uint8_t* inputPosition = input + position;
    SkandaOptimalParserState* const parserPosition =
        parser + position * compressorOptions.maxArrivals;
    // Initialize position ahead
    for (size_t i = 0; i < compressorOptions.maxArrivals; i++)
      parserPosition[compressorOptions.niceLength *
                         compressorOptions.maxArrivals +
                     i]
          .cost = UINT32_MAX;

    size_t nextExpectedLength = 2;  // Only take matches as long as this

    for (size_t i = 0; i < compressorOptions.maxArrivals; i++) {
      SkandaOptimalParserState* const currentArrival = parserPosition + i;
      // Break literal/rep parsing if current arrival has a very high cost
      // Will also break if there are no more arrivals for this position
      if (currentArrival->cost - 0x10000 * compressorOptions.maxArrivals >=
          parserPosition[compressorOptions.maxArrivals].cost)
        break;

      size_t literalCost = currentArrival->cost;
      // size cost
      if (disableHuffman)
        literalCost += 0x80000 << (currentArrival->literalRunLength == 6);
      else {
        literalCost += literalSymbols[*inputPosition].bits << 16;
        if (currentArrival->literalRunLength >= 6) {
          literalCost +=
              get_length_cost(currentArrival->literalRunLength + 1 - 7,
                              lengthSymbols)
              << 16;
          if (currentArrival->literalRunLength >= 7)
            literalCost -= get_length_cost(currentArrival->literalRunLength - 7,
                                           lengthSymbols)
                           << 16;
        }
      }
      // speed cost
      literalCost += (currentArrival->literalRunLength == 6) * 2;
      SkandaOptimalParserState* arrivalIt =
          parserPosition + compressorOptions.maxArrivals;
      SkandaOptimalParserState* lastArrival =
          arrivalIt + compressorOptions.maxArrivals;

      for (; arrivalIt < lastArrival; arrivalIt++) {
        if (literalCost < arrivalIt->cost) {
          for (SkandaOptimalParserState* it = lastArrival - 1; it != arrivalIt;
               it--)
            memcpy(&it[0], &it[-1], sizeof(SkandaOptimalParserState));

          arrivalIt->cost = literalCost;
          arrivalIt->matchLength = 1;
          arrivalIt->distance = currentArrival->distance;
          arrivalIt->literalRunLength = currentArrival->literalRunLength + 1;
          arrivalIt->path = i;
          break;
        }
      }

      size_t repMatchLength = test_match<false>(
          inputPosition, inputPosition - currentArrival->distance, blockLimit,
          2, 0);
      // Since we go from lowest cost arrival to highest, it makes sense that
      // the rep match
      //  should be at least as long as the best found so far
      if (repMatchLength >= nextExpectedLength) {
        // Rep matches can be unconditionally taken with lower lengths
        if (repMatchLength >= compressorOptions.niceLength / 2) {
          lastMatchLength = repMatchLength;
          lastMatchDistance = currentArrival->distance;
          lastMatchPath = i;
          goto doBackwardParse;
        }

        const size_t maxLengthReduction =
            repMatchLength - nextExpectedLength > MAX_LENGTH_REDUCTION
                ? repMatchLength - MAX_LENGTH_REDUCTION
                : nextExpectedLength;

        for (size_t length = repMatchLength; length >= maxLengthReduction;
             length--) {
          size_t repMatchCost = currentArrival->cost;
          // size cost
          if (disableHuffman) {
            // Sending match + rep match has the same size cost as sending an
            // additional length byte, but it avoids a branch in the
            // decompressor
            if (length > 8 && length <= 16) length = 8;
            repMatchCost += 0x80000 << (length > 8);
          } else {
            repMatchCost += get_token_cost(currentArrival->literalRunLength,
                                           length, 0, tokenSymbols)
                            << 16;
            if (length > 8)
              repMatchCost += get_length_cost(length - 9, lengthSymbols) << 16;
          }
          // speed cost
          repMatchCost += 1 + (length > 8) * 2;

          SkandaOptimalParserState* arrivalIt =
              parserPosition + length * compressorOptions.maxArrivals;
          SkandaOptimalParserState* lastArrival =
              arrivalIt + compressorOptions.maxArrivals;

          for (; arrivalIt < lastArrival; arrivalIt++) {
            if (repMatchCost < arrivalIt->cost) {
              for (SkandaOptimalParserState* it = lastArrival - 1;
                   it != arrivalIt; it--)
                memcpy(&it[0], &it[-1], sizeof(SkandaOptimalParserState));

              arrivalIt->cost = repMatchCost;
              arrivalIt->matchLength = length;
              arrivalIt->distance = currentArrival->distance;
              arrivalIt->literalRunLength = 0;
              arrivalIt->path = i;
              break;
            }
            // If there is already an arrival with this offset, and it has lower
            // cost,
            //  skip this match. This keeps more interesting offsets in the
            //  arrivals.
            if (arrivalIt->distance == currentArrival->distance) break;
          }
        }

        nextExpectedLength = repMatchLength;
        if (repMatchLength >= 3) *acceleration = 1 << accelerationThreshold;
      }
    }

    // Unpromising position
    if (parserPosition[0].cost >=
        parserPosition[compressorOptions.maxArrivals].cost) {
      if (disableHuffman)
        matchFinder->update_position(inputPosition, inputStart,
                                     compressionLimit, windowLog,
                                     compressorOptions);
      continue;
    }

    LZMatch matchFinderBuf[65];
    LZMatch *matches, *matchesEnd;
    if (disableHuffman) {
      matchesEnd = matchFinder->find_matches_and_update(
          inputPosition, inputStart, compressionLimit, blockLimit,
          matchFinderBuf, 3, windowLog, compressorOptions);
      matches = matchFinderBuf;
    } else
      matchBuffer->get_position_matches(&matches, &matchesEnd, blockPos);

    if (matchesEnd != matches) {
      // We have the guarantee that matches are in increasing order
      const LZMatch* const longestMatch = matchesEnd - 1;
      if (longestMatch->length >= compressorOptions.niceLength) {
        lastMatchLength = longestMatch->length;
        lastMatchDistance = longestMatch->distance;
        lastMatchPath = 0;
        break;
      }

      size_t previousMatchLength = 3;

      for (LZMatch* matchIt = matches; matchIt != matchesEnd; matchIt++) {
        if (matchIt->distance == parserPosition->distance) continue;

        size_t matchLengthReductionLimit =
            matchIt->length == previousMatchLength ? previousMatchLength
                                                   : previousMatchLength + 1;
        if (matchIt->length - matchLengthReductionLimit > MAX_LENGTH_REDUCTION)
          matchLengthReductionLimit = matchIt->length - MAX_LENGTH_REDUCTION;
        previousMatchLength = matchIt->length;

        // Start search at the highest length. Stop when we reach the limit,
        for (size_t length = matchIt->length;
             length >= matchLengthReductionLimit; length--) {
          size_t matchCost = parserPosition->cost;
          // size cost
          size_t distanceBytes = unsafe_int_log2(matchIt->distance) / 8;
          if (disableHuffman) {
            if (length > 8 && length <= 16) length = 8;
            matchCost += (2 + distanceBytes + (length > 8)) << 19;
          } else {
            matchCost += get_token_cost(parserPosition->literalRunLength,
                                        length, distanceBytes + 1, tokenSymbols)
                         << 16;
            for (int i = 0; i <= distanceBytes; i++)
              matchCost +=
                  distanceSymbols[(matchIt->distance >> i * 8) & 0xFF].bits
                  << 16;
            if (length > 8)
              matchCost += get_length_cost(length - 9, lengthSymbols) << 16;
          }
          // speed cost
          matchCost += 1 + (length > 8) * 2 + (matchIt->distance > (1 << 16));

          SkandaOptimalParserState* arrivalIt =
              parserPosition + length * compressorOptions.maxArrivals;
          SkandaOptimalParserState* lastArrival =
              arrivalIt + compressorOptions.maxArrivals;

          for (; arrivalIt < lastArrival; arrivalIt++) {
            if (matchCost < arrivalIt->cost) {
              for (SkandaOptimalParserState* it = lastArrival - 1;
                   it != arrivalIt; it--)
                memcpy(&it[0], &it[-1], sizeof(SkandaOptimalParserState));

              arrivalIt->cost = matchCost;
              arrivalIt->matchLength = length;
              arrivalIt->distance = matchIt->distance;
              arrivalIt->literalRunLength = 0;
              arrivalIt->path = 0;
              break;
            }

            if (arrivalIt->distance == matchIt->distance) break;
          }
        }
      }

      if (longestMatch->length >= 4) *acceleration = 1 << accelerationThreshold;
    }
    // Disable acceleration on level 9
    else if (compressorOptions.parser < OPTIMAL3) {
      (*acceleration)++;
      // If acceleration goes too high go to greedy parse
      if (*acceleration >= (2 << accelerationThreshold)) {
        position++;
        break;
      }
    }
  }

doBackwardParse:

  // Backward pass, pick best option at each step.
  const SkandaOptimalParserState* backwardParse;
  const SkandaOptimalParserState* const parseEnd = parser;
  size_t path = 0;

  if (lastMatchLength) {
    stream->literalRunLength = 0;
    stream++;
    stream->matchDistance = lastMatchDistance;
    stream->matchLength = lastMatchLength;
    stream->literalRunLength = 0;
    stream++;
    backwardParse = parser + position * compressorOptions.maxArrivals;
    path = lastMatchPath;

    if (disableHuffman) {
      const uint8_t* const updateEnd =
          input + position +
          std::min(std::min(lastMatchDistance, lastMatchLength),
                   (size_t)compressorOptions.niceLength);
      const uint8_t* updatePos = input + position + 1;
      for (; updatePos < updateEnd; updatePos++)
        matchFinder->update_position(updatePos, inputStart, compressionLimit,
                                     windowLog, compressorOptions);
    }
  } else {
    size_t bestPos = position;
    size_t bestSize = parser[position * compressorOptions.maxArrivals].cost;
    for (size_t i = 1; i < compressorOptions.niceLength; i++) {
      if (parser[(position + i) * compressorOptions.maxArrivals].cost <=
          bestSize) {
        bestPos = position + i;
        bestSize = parser[(position + i) * compressorOptions.maxArrivals].cost;
      }
    }
    backwardParse = parser + bestPos * compressorOptions.maxArrivals;
    stream->literalRunLength = 0;
    stream++;
  }
  while (backwardParse > parseEnd) {
    if (backwardParse[path].matchLength > 1) {
      stream->matchDistance = backwardParse[path].distance;
      stream->matchLength = backwardParse[path].matchLength;
      stream->literalRunLength = 0;
      path = backwardParse[path].path;
      backwardParse -= stream->matchLength * compressorOptions.maxArrivals;
      stream++;
    } else {
      (stream - 1)->literalRunLength++;
      path = backwardParse[path].path;
      backwardParse -= compressorOptions.maxArrivals;
    }
  }

  return stream - 1;
}

FORCE_INLINE void aproximator_encode_length(uint32_t lengthSymbols[256],
                                            size_t var, const size_t overflow) {
  var -= overflow;
  if (likely(var <= 223))
    lengthSymbols[var]++;
  else {
    var -= 224;
    lengthSymbols[224 | (var & 0x1F)]++;
    var >>= 5;

    if (likely(var <= 223))
      lengthSymbols[var]++;
    else {
      var -= 224;
      lengthSymbols[224 | (var & 0x1F)]++;
      lengthSymbols[var >> 5]++;
    }
  }
}

FORCE_INLINE void aproximator_encode_literal_run(uint32_t literalSymbols[256],
                                                 uint32_t lengthSymbols[256],
                                                 const uint8_t* const input,
                                                 const uint8_t* literalRun,
                                                 uint8_t* const controlByte) {
  const size_t literalRunLength = input - literalRun;

  if (literalRunLength >= 7) {
    *controlByte = (7 << 5);
    aproximator_encode_length(lengthSymbols, literalRunLength, 7);
  } else
    *controlByte = (literalRunLength << 5);

  for (; literalRun < input; literalRun++) literalSymbols[*literalRun]++;
}

FORCE_INLINE void aproximator_encode_match(
    uint32_t tokenSymbols[256], uint32_t lengthSymbols[256],
    uint32_t distanceSymbols[256], uint8_t* const controlByte,
    size_t* matchLength, const size_t& distance, size_t* repOffset) {
  if (distance != *repOffset) {
    int bytes = unsafe_int_log2(distance) / 8 + 1;
    *controlByte |= bytes << 3;
    *repOffset = distance;
    for (int i = 0; i < bytes; i++)
      distanceSymbols[(distance >> i * 8) & 0xFF]++;
  }

  if (*matchLength <= 8) {
    *matchLength -= SKANDA_MIN_MATCH_LENGTH;
    tokenSymbols[*controlByte | *matchLength]++;
  } else {
    tokenSymbols[*controlByte | 7]++;
    aproximator_encode_length(lengthSymbols, *matchLength, 9);
  }
}

class BlockSplitter {
  size_t maxSubdivisions;
  size_t subdivisionSize;
  uint32_t* literalSymbols = nullptr;
  uint32_t* tokenSymbols = nullptr;
  uint32_t* distanceSymbols = nullptr;
  uint32_t* lengthSymbols = nullptr;
  HashTable<uint32_t, FastIntHash> lzdict3;
  HashTable<uint32_t, FastIntHash> lzdict6;
  int reuseSubdivisions;

  HuffmanSymbol literalCodes[256];
  HuffmanSymbol tokenCodes[256];
  HuffmanSymbol distanceCodes[256];
  HuffmanSymbol lengthCodes[256];

  void aproximate_symbol_histogram(const uint8_t* input,
                                   const uint8_t* inputStart, uint8_t* output,
                                   const size_t size,
                                   uint32_t literalSymbols[256],
                                   uint32_t tokenSymbols[256],
                                   uint32_t distanceSymbols[256],
                                   uint32_t lengthSymbols[256]) {
    const size_t accelerationThreshold = 6;
    const uint8_t* const thisBlockEnd = input + size;
    size_t repOffset = 1;
    size_t acceleration = 1 << accelerationThreshold;

    memset(literalSymbols, 0, 256 * sizeof(uint32_t));
    memset(tokenSymbols, 0, 256 * sizeof(uint32_t));
    memset(distanceSymbols, 0, 256 * sizeof(uint32_t));
    memset(lengthSymbols, 0, 256 * sizeof(uint32_t));

    const uint8_t* literalRunStart = input;
    // Skip first byte
    input++;

    while (likely(input < thisBlockEnd)) {
      // First try a rep match
      size_t repMatchLength = test_match<false>(
          input + 1, input + 1 - repOffset, thisBlockEnd, 2, 0);
      if (repMatchLength) {
        lzdict3[read_hash3(input + 0)] = (input + 0 - inputStart);
        lzdict3[read_hash3(input + 1)] = (input + 1 - inputStart);
        lzdict3[read_hash3(input + 2)] = (input + 2 - inputStart);
        lzdict6[read_hash6(input + 1)] = (input + 1 - inputStart);

        input++;
        uint8_t controlByte;
        aproximator_encode_literal_run(literalSymbols, lengthSymbols, input,
                                       literalRunStart, &controlByte);

        input += repMatchLength;
        literalRunStart = input;
        aproximator_encode_match(tokenSymbols, lengthSymbols, distanceSymbols,
                                 &controlByte, &repMatchLength, repOffset,
                                 &repOffset);
        acceleration = 1 << accelerationThreshold;
        continue;
      }

      uint32_t* dictEntry3 = &(lzdict3)[read_hash3(input)];
      uint32_t pos3 = *dictEntry3;
      *dictEntry3 = input - inputStart;

      const uint8_t* where = inputStart + pos3;
      size_t matchLength = test_match<true>(input, where, thisBlockEnd, 3,
                                            SKANDA_MAX_WINDOW_LOG);
      size_t distance = input - where;
      if (matchLength) {
        uint32_t* dictEntry6 = &(lzdict6)[read_hash6(input)];
        uint32_t pos6 = *dictEntry6;
        *dictEntry6 = input - inputStart;

        where = inputStart + pos6;
        if (*(input + matchLength) == *(where + matchLength)) {
          size_t length = test_match<true>(input, where, thisBlockEnd, 6,
                                           SKANDA_MAX_WINDOW_LOG);
          if (length > matchLength) {
            matchLength = length;
            distance = input - where;
          }
        }
      } else {
        input += acceleration >> accelerationThreshold;
        acceleration++;
        continue;
      }

      int lazySteps = 0;  // bytes to skip because of lazy matching
      // Now try to get a better match at pos + 1
      const uint8_t* matchPos = input + 1;
      uint32_t* dictEntry6 = &(lzdict6)[read_hash6(matchPos)];
      dictEntry3 = &(lzdict3)[read_hash3(matchPos)];
      uint32_t pos6 = *dictEntry6;
      pos3 = *dictEntry3;
      *dictEntry3 = matchPos - inputStart;
      *dictEntry6 = matchPos - inputStart;

      const uint8_t* where3 = inputStart + pos3;
      const uint8_t* where6 = inputStart + pos6;
      bool matched3 = *(matchPos + matchLength) == *(where3 + matchLength);
      bool matched6 = *(matchPos + matchLength) == *(where6 + matchLength);

      if (matched3 || matched6) {
        where = matched6 ? where6 : where3;
        size_t length = test_match<true>(matchPos, where, thisBlockEnd, 3,
                                         SKANDA_MAX_WINDOW_LOG);
        if (length > matchLength) {
          distance = matchPos - where;
          matchLength = length;
          lazySteps = 1;
        }
      }

      input += lazySteps;
      lzdict3[read_hash3(input + matchLength - 1)] =
          (input + matchLength - 1 - inputStart);
      lzdict6[read_hash6(input + matchLength - 2)] =
          (input + matchLength - 2 - inputStart);
      lzdict3[read_hash3(input + matchLength - 3)] =
          (input + matchLength - 3 - inputStart);

      const uint8_t* match = input - distance;
      while (input > literalRunStart && match > inputStart &&
             input[-1] == match[-1]) {
        matchLength++;
        input--;
        match--;
      }

      uint8_t controlByte;
      aproximator_encode_literal_run(literalSymbols, lengthSymbols, input,
                                     literalRunStart, &controlByte);

      input += matchLength;
      literalRunStart = input;
      aproximator_encode_match(tokenSymbols, lengthSymbols, distanceSymbols,
                               &controlByte, &matchLength, distance,
                               &repOffset);
      if (matchLength > 3) acceleration = 1 << accelerationThreshold;
    }

    // Maybe we went beyond block end because of acceleration
    input = thisBlockEnd;
    // Send last literal run
    if (input != literalRunStart) {
      uint8_t controlByte;
      aproximator_encode_literal_run(literalSymbols, lengthSymbols, input,
                                     literalRunStart, &controlByte);
      tokenSymbols[controlByte]++;
    }
  }

 public:
  BlockSplitter() {}
  ~BlockSplitter() {
    delete[] literalSymbols;
    delete[] tokenSymbols;
    delete[] distanceSymbols;
    delete[] lengthSymbols;
  }

  void init(int windowLog, int subdivisions) {
    maxSubdivisions = subdivisions;
    subdivisionSize = (MAX_BLOCK_SIZE + 1) / maxSubdivisions;
    literalSymbols = new uint32_t[maxSubdivisions * 256];
    tokenSymbols = new uint32_t[maxSubdivisions * 256];
    distanceSymbols = new uint32_t[maxSubdivisions * 256];
    lengthSymbols = new uint32_t[maxSubdivisions * 256];
    int dictLog = windowLog - 6;
    if (dictLog > 17) dictLog = 17;
    if (dictLog < 1) dictLog = 1;
    lzdict3.init(dictLog);
    lzdict6.init(dictLog);
    reuseSubdivisions = 0;
  }

  size_t get_block_size(const uint8_t* input, const uint8_t* const inputStart,
                        uint8_t* output, size_t maxSize, float decSpeedBias) {
    // The algorithm is fairly simple:
    //  - Step 1: set split point and best split point to maximum block size,
    //  set best compressed size to the compressed size of maximum size block
    //  - Step 2: reduce split point by whatever amount you want, for example in
    //  steps of 16KB. Test compressed size
    //		 of data to the left of this split point and to the right. The
    // right data ends at the current best split point. 		If sum
    // of left and right compressed sizes is less than best compressed size, set
    // best split point to 		 current split point and best compressed
    // size to the compressed size of the left data. Repeat step 2 until you
    // reach the minimum block size.
    //
    // To optimize this algorithm we can get histograms for each of the split
    // points we will test, then calculating
    //  compressed sizes is a matter of combining histograms and computing the
    //  huffman codes.

    const size_t subdivisionCount =
        (maxSize / subdivisionSize) + (maxSize % subdivisionSize > 0);
    // Get histograms for each subdivision
    for (int i = reuseSubdivisions; i < subdivisionCount; i++) {
      size_t testSize =
          std::min(subdivisionSize, maxSize - i * subdivisionSize);
      aproximate_symbol_histogram(
          input + subdivisionSize * i, inputStart, output, testSize,
          &literalSymbols[i * 256], &tokenSymbols[i * 256],
          &distanceSymbols[i * 256], &lengthSymbols[i * 256]);
    }

    HuffmanSymbol literalSymbolsLeft[256];
    HuffmanSymbol tokenSymbolsLeft[256];
    HuffmanSymbol distanceSymbolsLeft[256];
    HuffmanSymbol lengthSymbolsLeft[256];
    memset(literalSymbolsLeft, 0, 256 * sizeof(HuffmanSymbol));
    memset(tokenSymbolsLeft, 0, 256 * sizeof(HuffmanSymbol));
    memset(distanceSymbolsLeft, 0, 256 * sizeof(HuffmanSymbol));
    memset(lengthSymbolsLeft, 0, 256 * sizeof(HuffmanSymbol));

    HuffmanSymbol literalSymbolsRight[256];
    HuffmanSymbol tokenSymbolsRight[256];
    HuffmanSymbol distanceSymbolsRight[256];
    HuffmanSymbol lengthSymbolsRight[256];
    memset(literalSymbolsRight, 0, 256 * sizeof(HuffmanSymbol));
    memset(tokenSymbolsRight, 0, 256 * sizeof(HuffmanSymbol));
    memset(distanceSymbolsRight, 0, 256 * sizeof(HuffmanSymbol));
    memset(lengthSymbolsRight, 0, 256 * sizeof(HuffmanSymbol));

    for (int i = 0; i < subdivisionCount; i++) {
      for (int j = 0; j < 256; j++) {
        literalSymbolsLeft[j].count += literalSymbols[i * 256 + j];
        tokenSymbolsLeft[j].count += tokenSymbols[i * 256 + j];
        distanceSymbolsLeft[j].count += distanceSymbols[i * 256 + j];
        lengthSymbolsLeft[j].count += lengthSymbols[i * 256 + j];
      }
    }

    size_t bestEncodedSize =
        get_encoded_huffman_info(literalSymbolsLeft, decSpeedBias) +
        get_encoded_huffman_info(tokenSymbolsLeft, decSpeedBias) +
        get_encoded_huffman_info(distanceSymbolsLeft, decSpeedBias) +
        get_encoded_huffman_info(lengthSymbolsLeft, decSpeedBias);
    size_t bestBlockSize = maxSize;

    // Store the codes to later use them as initial estimation for optimal parse
    memcpy(literalCodes, literalSymbolsLeft, 256 * sizeof(HuffmanSymbol));
    memcpy(tokenCodes, tokenSymbolsLeft, 256 * sizeof(HuffmanSymbol));
    memcpy(distanceCodes, distanceSymbolsLeft, 256 * sizeof(HuffmanSymbol));
    memcpy(lengthCodes, lengthSymbolsLeft, 256 * sizeof(HuffmanSymbol));

    for (int i = subdivisionCount - 1; i > 0; i--) {
      for (int j = 0; j < 256; j++) {
        literalSymbolsLeft[j].count -= literalSymbols[i * 256 + j];
        tokenSymbolsLeft[j].count -= tokenSymbols[i * 256 + j];
        distanceSymbolsLeft[j].count -= distanceSymbols[i * 256 + j];
        lengthSymbolsLeft[j].count -= lengthSymbols[i * 256 + j];
        literalSymbolsRight[j].count += literalSymbols[i * 256 + j];
        tokenSymbolsRight[j].count += tokenSymbols[i * 256 + j];
        distanceSymbolsRight[j].count += distanceSymbols[i * 256 + j];
        lengthSymbolsRight[j].count += lengthSymbols[i * 256 + j];
      }

      size_t newLeftEncodedSize =
          get_encoded_huffman_info(literalSymbolsLeft, decSpeedBias) +
          get_encoded_huffman_info(tokenSymbolsLeft, decSpeedBias) +
          get_encoded_huffman_info(distanceSymbolsLeft, decSpeedBias) +
          get_encoded_huffman_info(lengthSymbolsLeft, decSpeedBias);
      size_t newRightEncodedSize =
          get_encoded_huffman_info(literalSymbolsRight, decSpeedBias) +
          get_encoded_huffman_info(tokenSymbolsRight, decSpeedBias) +
          get_encoded_huffman_info(distanceSymbolsRight, decSpeedBias) +
          get_encoded_huffman_info(lengthSymbolsRight, decSpeedBias);

      // Making a split here does help
      if (newLeftEncodedSize + newRightEncodedSize < bestEncodedSize) {
        bestEncodedSize = newLeftEncodedSize;
        bestBlockSize = i * subdivisionSize;
        // Reset right split histogram
        memset(literalSymbolsRight, 0, 256 * sizeof(HuffmanSymbol));
        memset(tokenSymbolsRight, 0, 256 * sizeof(HuffmanSymbol));
        memset(distanceSymbolsRight, 0, 256 * sizeof(HuffmanSymbol));
        memset(lengthSymbolsRight, 0, 256 * sizeof(HuffmanSymbol));
        // Store the codes to later use them as initial estimation for optimal
        // parse
        memcpy(literalCodes, literalSymbolsLeft, 256 * sizeof(HuffmanSymbol));
        memcpy(tokenCodes, tokenSymbolsLeft, 256 * sizeof(HuffmanSymbol));
        memcpy(distanceCodes, distanceSymbolsLeft, 256 * sizeof(HuffmanSymbol));
        memcpy(lengthCodes, lengthSymbolsLeft, 256 * sizeof(HuffmanSymbol));
      }
    }

    // Save existing histograms for next block splitting
    if (bestBlockSize != maxSize) {
      int usedSubdivisions = bestBlockSize / subdivisionSize;
      reuseSubdivisions = subdivisionCount - usedSubdivisions;
      memmove(&literalSymbols[0], &literalSymbols[usedSubdivisions * 256],
              reuseSubdivisions * 256 * sizeof(uint32_t));
      memmove(&tokenSymbols[0], &tokenSymbols[usedSubdivisions * 256],
              reuseSubdivisions * 256 * sizeof(uint32_t));
      memmove(&distanceSymbols[0], &distanceSymbols[usedSubdivisions * 256],
              reuseSubdivisions * 256 * sizeof(uint32_t));
      memmove(&lengthSymbols[0], &lengthSymbols[usedSubdivisions * 256],
              reuseSubdivisions * 256 * sizeof(uint32_t));
    } else
      reuseSubdivisions = 0;

    return bestBlockSize;
  }

  HuffmanSymbol* get_huffman_codes(float decSpeedBias, bool* uncompressed,
                                   int streamID) {
    HuffmanSymbol* symbols;
    switch (streamID) {
      case LITERAL_STREAM:
        symbols = literalCodes;
        break;
      case TOKEN_STREAM:
        symbols = tokenCodes;
        break;
      case DISTANCE_STREAM:
        symbols = distanceCodes;
        break;
      case LENGTH_STREAM:
        symbols = lengthCodes;
        break;
    }

    *uncompressed = true;
    for (int i = 0; i < 256; i++) {
      if (symbols[i].bits != 8) {
        *uncompressed = false;
        break;
      }
    }

    return symbols;
  }
};

size_t compress_optimal(const uint8_t* input, const size_t size,
                        uint8_t* output, const int windowLog,
                        const float decSpeedBias,
                        const CompressorOptions& compressorOptions,
                        ProgressCallback* progress) {
  const size_t accelerationThreshold = 6;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffset = 1;
  size_t acceleration = 1 << accelerationThreshold;

  bool disableHuffman = decSpeedBias >= 0.99;
  size_t huffmanBufSize = std::min(MAX_BLOCK_SIZE, size);

  HashTableMatchFinder hashMatchFinder;
  BinaryMatchFinder binaryMatchFinder;
  SkandaOptimalParserState* parser = nullptr;
  LZStructure* stream = nullptr;
  MatchBuffer matchBuffer;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  BlockSplitter blockSplitter;

  try {
    if (compressorOptions.parser == OPTIMAL1) {
      hashMatchFinder.init(windowLog, compressorOptions);
      parser = new SkandaOptimalParserState[compressorOptions.optimalBlockSize +
                                            compressorOptions.niceLength];
    } else {
      binaryMatchFinder.init(size, windowLog, compressorOptions);
      parser =
          new SkandaOptimalParserState[(compressorOptions.optimalBlockSize +
                                        compressorOptions.niceLength) *
                                       compressorOptions.maxArrivals];
    }
    stream = new LZStructure[compressorOptions.optimalBlockSize];
    if (!disableHuffman) {
      matchBuffer.init(size, compressorOptions);
      huffmanStreamBuffer = new uint8_t[huffmanBufSize + 128];
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
      blockSplitter.init(windowLog,
                         compressorOptions.parser >= OPTIMAL2 ? 64 : 4);
    }
  } catch (const std::bad_alloc& e) {
    delete[] parser;
    delete[] stream;
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  HuffmanEncoder literalEncoder;
  HuffmanEncoder tokenEncoder;
  HuffmanEncoder lengthEncoder;
  HuffmanEncoder distanceEncoder;

  if (literalEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                LITERAL_STREAM, false) ||
      tokenEncoder.initialize_huffman_encoder(huffmanBufSize / 2 + 128,
                                              TOKEN_STREAM, true) ||
      lengthEncoder.initialize_huffman_encoder(huffmanBufSize / 8 + 128,
                                               LENGTH_STREAM, true) ||
      distanceEncoder.initialize_huffman_encoder(huffmanBufSize + 128,
                                                 DISTANCE_STREAM, true)) {
    delete[] parser;
    delete[] stream;
    delete[] huffmanStreamBuffer;
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    // On first iteration do some approximation of huffman codes for each
    // stream. For that approximation
    //  we will use the given speed bias to decide whether we use huffman or
    //  send the stream uncompressed.
    // On successive iterations and the final codes we will force uncompressed
    // if the approximation used
    //  uncompressed and huffman if the approximation used huffman.
    // This is done to avoid changes from huffman to uncompressed or viceversa
    // on the last iteration that could
    //  make the previous parsing suboptimal.
    // With the default configuration it also tends to use huffman more than the
    // faster levels,
    //  so we tweak the bias to keep a similar decode speed.
    float tweakedBias = sqrt(decSpeedBias);
    float literalHuffmanDecSpeedBias = tweakedBias;
    float tokenHuffmanDecSpeedBias = tweakedBias;
    float distanceHuffmanDecSpeedBias = tweakedBias;
    float lengthHuffmanDecSpeedBias = tweakedBias;
    bool blockNoHuffman = true;

    size_t thisBlockSize = compressionLimit - input < MAX_BLOCK_SIZE
                               ? compressionLimit - input
                               : MAX_BLOCK_SIZE;
    if (!disableHuffman)
      thisBlockSize = blockSplitter.get_block_size(input, inputStart, output,
                                                   thisBlockSize, tweakedBias);

    const uint8_t* const thisBlockEnd = input + thisBlockSize;
    const uint8_t* const thisBlockStart = input;
    write_header(output, thisBlockSize, BLOCK_BASIC);
    const size_t startRepOffset = repOffset;

    for (int iteration = 0;
         iteration < (compressorOptions.parser < OPTIMAL2 || disableHuffman
                          ? 1
                          : compressorOptions.parserIterations);
         iteration++) {
      HuffmanSymbol* literalSymbols = nullptr;
      HuffmanSymbol* tokenSymbols = nullptr;
      HuffmanSymbol* lengthSymbols = nullptr;
      HuffmanSymbol* distanceSymbols = nullptr;

      if (!disableHuffman) {
        if (iteration == 0) {
          bool uncompressedLiteral, uncompressedToken, uncompressedLength,
              uncompressedDistance;
          literalSymbols = blockSplitter.get_huffman_codes(
              literalHuffmanDecSpeedBias, &uncompressedLiteral, LITERAL_STREAM);
          literalHuffmanDecSpeedBias = uncompressedLiteral ? 1 : 0;
          tokenSymbols = blockSplitter.get_huffman_codes(
              tokenHuffmanDecSpeedBias, &uncompressedToken, TOKEN_STREAM);
          tokenHuffmanDecSpeedBias = uncompressedToken ? 1 : 0;
          lengthSymbols = blockSplitter.get_huffman_codes(
              lengthHuffmanDecSpeedBias, &uncompressedLength, LENGTH_STREAM);
          lengthHuffmanDecSpeedBias = uncompressedLength ? 1 : 0;
          distanceSymbols = blockSplitter.get_huffman_codes(
              distanceHuffmanDecSpeedBias, &uncompressedDistance,
              DISTANCE_STREAM);
          distanceHuffmanDecSpeedBias = uncompressedDistance ? 1 : 0;

          blockNoHuffman = uncompressedLiteral & uncompressedToken &
                           uncompressedLength & uncompressedDistance;
          if (!blockNoHuffman && compressorOptions.parser >= OPTIMAL2)
            matchBuffer.find_matches(thisBlockStart, inputStart, thisBlockEnd,
                                     compressionLimit, &binaryMatchFinder,
                                     windowLog, compressorOptions);
        } else {
          literalSymbols =
              literalEncoder.get_huffman_codes(literalHuffmanDecSpeedBias);
          tokenSymbols =
              tokenEncoder.get_huffman_codes(tokenHuffmanDecSpeedBias);
          lengthSymbols =
              lengthEncoder.get_huffman_codes(lengthHuffmanDecSpeedBias);
          distanceSymbols =
              distanceEncoder.get_huffman_codes(distanceHuffmanDecSpeedBias);
        }
      }

      input = thisBlockStart;
      repOffset = startRepOffset;
      const uint8_t* literalRunStart = input;
      // Skip first byte of first block
      if (input == inputStart) input++;

      literalEncoder.start_huffman_block(output);
      tokenEncoder.start_huffman_block();
      lengthEncoder.start_huffman_block();
      distanceEncoder.start_huffman_block();

      while (input < thisBlockEnd) {
        if (acceleration >= (2 << accelerationThreshold)) {
          LZMatch matchFinderBuf[65];
          LZMatch *matches, *matchesEnd;
          if (compressorOptions.parser >= OPTIMAL2) {
            if (disableHuffman || blockNoHuffman) {
              matchesEnd = binaryMatchFinder.find_matches_and_update(
                  input, inputStart, compressionLimit, thisBlockEnd,
                  matchFinderBuf, 4, windowLog, compressorOptions);
              matches = matchFinderBuf;
            } else
              matchBuffer.get_position_matches(&matches, &matchesEnd,
                                               input - thisBlockStart);
          } else {
            matchesEnd = hashMatchFinder.find_matches_and_update(
                input, inputStart, thisBlockEnd, matchFinderBuf, 4, windowLog,
                compressorOptions);
            matches = matchFinderBuf;
          }

          if (matchesEnd != matches) {
            size_t distance = matchesEnd[-1].distance;
            size_t length = matchesEnd[-1].length;
            // Left match extension
            const uint8_t* match = input - distance;
            while (input > literalRunStart && match > inputStart &&
                   input[-1] == match[-1]) {
              length++;
              input--;
              match--;
            }

            uint8_t controlByte;
            encode_literal_run(&literalEncoder, &lengthEncoder, input,
                               literalRunStart, &controlByte);

            input += length;
            literalRunStart = input;
            encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                         &controlByte, &length, distance, &repOffset,
                         blockNoHuffman);
            acceleration = 1 << accelerationThreshold;
          } else {
            input += acceleration >> accelerationThreshold;
            acceleration++;
          }
        } else {
          LZStructure* streamIt;
          if (compressorOptions.parser >= OPTIMAL2) {
            CompressorOptions iterationOptions = compressorOptions;
            // Use a lighter parse for the first iterations
            if (iteration < compressorOptions.parserIterations - 1 &&
                !(disableHuffman | blockNoHuffman)) {
              iterationOptions.maxArrivals = 2;
              iterationOptions.niceLength = 32;
            }

            streamIt = multi_arrivals_parse(
                input, inputStart, thisBlockStart, compressionLimit,
                thisBlockEnd, &binaryMatchFinder, &matchBuffer, parser, stream,
                repOffset, &acceleration, accelerationThreshold, literalSymbols,
                tokenSymbols, lengthSymbols, distanceSymbols,
                disableHuffman | blockNoHuffman, iterationOptions, windowLog);
          } else {
            streamIt = forward_optimal_parse(
                input, inputStart, thisBlockEnd, &hashMatchFinder, parser,
                stream, repOffset, &acceleration, accelerationThreshold,
                literalSymbols, tokenSymbols, lengthSymbols, distanceSymbols,
                disableHuffman | blockNoHuffman, compressorOptions, windowLog);
          }

          // Main compression loop
          while (true) {
            input += streamIt->literalRunLength;

            if (streamIt == stream) break;

            size_t matchLength = streamIt->matchLength;
            size_t distance = streamIt->matchDistance;

            if (compressorOptions.parser == OPTIMAL1 && distance != repOffset) {
              const uint8_t* match = input - distance;
              while (input > literalRunStart && match > inputStart &&
                     input[-1] == match[-1]) {
                matchLength++;
                input--;
                match--;
              }
            }

            uint8_t controlByte;
            encode_literal_run(&literalEncoder, &lengthEncoder, input,
                               literalRunStart, &controlByte);

            input += matchLength;
            literalRunStart = input;
            encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                         &controlByte, &matchLength, distance, &repOffset,
                         blockNoHuffman);

            streamIt--;
          }
        }
      }

      input = thisBlockEnd;
      if (input != literalRunStart) {
        uint8_t controlByte;
        encode_literal_run(&literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte);
        tokenEncoder.add_byte(controlByte);
      }

      // Dont do more than one iteration if we are not going to use huffman on
      // any stream
      if (blockNoHuffman) break;
    }

    output += literalEncoder.encode_symbols(
        output, huffmanStreamBufferBegin, literalHuffmanDecSpeedBias,
        compressorOptions.parser != OPTIMAL3, false);
    output += distanceEncoder.encode_symbols(
        output, huffmanStreamBufferBegin, distanceHuffmanDecSpeedBias,
        compressorOptions.parser != OPTIMAL3, false);
    output += tokenEncoder.encode_symbols(
        output, huffmanStreamBufferBegin, tokenHuffmanDecSpeedBias,
        compressorOptions.parser != OPTIMAL3, false);
    output += lengthEncoder.encode_symbols(
        output, huffmanStreamBufferBegin, lengthHuffmanDecSpeedBias,
        compressorOptions.parser != OPTIMAL3, false);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        delete[] parser;
        delete[] stream;
        delete[] huffmanStreamBuffer;
        return 0;
      }
    }
  }

  delete[] parser;
  delete[] stream;
  delete[] huffmanStreamBuffer;
  write_header(output, SKANDA_LAST_BYTES, BLOCK_LAST);
  memcpy(output, input, SKANDA_LAST_BYTES);
  if (progress)
    progress->progress(size, output - outputStart + SKANDA_LAST_BYTES);

  return output - outputStart + SKANDA_LAST_BYTES;
}

const int NOT_USED = -1;
const CompressorOptions skandaCompressorLevels[] = {
    //      Parser        Hash log     Elements per hash     Nice length Block
    //      size      Max arrivals     Iterations
    {GREEDY1, 14, NOT_USED, NOT_USED, NOT_USED, NOT_USED, NOT_USED},
    {GREEDY2, 16, NOT_USED, NOT_USED, NOT_USED, NOT_USED, NOT_USED},
    {LAZY1, 17, NOT_USED, NOT_USED, NOT_USED, NOT_USED, NOT_USED},
    {LAZY2, 17, 1, NOT_USED, NOT_USED, NOT_USED, NOT_USED},
    {LAZY2, 18, 2, NOT_USED, NOT_USED, NOT_USED, NOT_USED},
    {OPTIMAL1, 18, 2, 32, 1024, NOT_USED, NOT_USED},
    {OPTIMAL1, 19, 3, 64, 1024, NOT_USED, NOT_USED},
    {OPTIMAL2, 24, 4, 128, 4096, 2, 2},
    {OPTIMAL2, 24, 5, 256, 4096, 4, 2},
    {OPTIMAL3, 24, 6, 512, 4096, 8, 2},
    {OPTIMAL3, 24, 6, 1024, 4096, 16, 3},
};

size_t compress(const uint8_t* input, size_t size, uint8_t* output, int level,
                float decSpeedBias, ProgressCallback* progress) {
  if (size <= SKANDA_LAST_BYTES + 8) {
    write_header(output, size, BLOCK_LAST);
    memcpy(output, input, size);
    if (progress) progress->progress(size, size);
    return size + 1;
  }

  if (level > 10) level = 10;
  if (level < 0) level = 0;
  if (decSpeedBias < 0) decSpeedBias = 0;
  if (decSpeedBias > 1) decSpeedBias = 1;

  const int WINDOW_LOGS[] = {24, 23, 22, 21, 20, 20};
  int maxWindowLog = WINDOW_LOGS[(int)(decSpeedBias * 5)];
  int windowLog = int_log2(size - 1) + 1;
  if (windowLog > maxWindowLog) windowLog = maxWindowLog;
  if (windowLog < 6) windowLog = 6;

  if (skandaCompressorLevels[level].parser == GREEDY1)
    return compress_ultra_fast(input, size, output, windowLog, decSpeedBias,
                               skandaCompressorLevels[level], progress);
  else if (skandaCompressorLevels[level].parser == GREEDY2)
    return compress_greedy(input, size, output, windowLog, decSpeedBias,
                           skandaCompressorLevels[level], progress);
  else if (skandaCompressorLevels[level].parser == LAZY1)
    return compress_lazy_fast(input, size, output, windowLog, decSpeedBias,
                              skandaCompressorLevels[level], progress);
  else if (skandaCompressorLevels[level].parser == LAZY2)
    return compress_lazy(input, size, output, windowLog, decSpeedBias,
                         skandaCompressorLevels[level], progress);
  return compress_optimal(input, size, output, windowLog, decSpeedBias,
                          skandaCompressorLevels[level], progress);
}

size_t compress_bound(size_t size) { return size + size / 1024 + 32; }

size_t lz_memory_usage(size_t size, int windowLog, int level,
                       float decSpeedBias) {
  if (level < 0) return sizeof(uint32_t) << 12;
  if (skandaCompressorLevels[level].parser == GREEDY1)
    return sizeof(uint32_t) << 12;
  if (skandaCompressorLevels[level].parser == GREEDY2)
    return sizeof(uint32_t)
           << std::min(skandaCompressorLevels[level].maxHashLog, windowLog - 3);
  if (skandaCompressorLevels[level].parser == LAZY1)
    return sizeof(uint32_t)
           << std::min(skandaCompressorLevels[level].maxHashLog, windowLog - 3)
           << 1;
  if (skandaCompressorLevels[level].parser == LAZY2)
    // Lazy extra uses 2 tables
    return sizeof(uint32_t)
           << std::min(skandaCompressorLevels[level].maxHashLog, windowLog - 3)
           << skandaCompressorLevels[level].maxElementsLog << 1;

  // Block splitter memory
  const size_t blockSplitterSubdivisions =
      skandaCompressorLevels[level].parser == OPTIMAL1 ? 4 : 64;
  size_t memory = sizeof(uint32_t) * 256 * 4 * blockSplitterSubdivisions;
  int blockSplitterDictLog = std::max(std::min(windowLog - 6, 17), 1);
  memory += (sizeof(uint32_t) * 2) << blockSplitterDictLog;

  if (skandaCompressorLevels[level].parser == OPTIMAL1) {
    const int log2size =
        std::min(skandaCompressorLevels[level].maxHashLog, windowLog - 3);
    memory += sizeof(uint32_t) << std::min(log2size, 14);  // hash 3 table
    // hash 4 and hash 8 tables
    memory += sizeof(uint32_t)
              << log2size << skandaCompressorLevels[level].maxElementsLog << 1;
    memory += sizeof(SkandaOptimalParserState) *
              (skandaCompressorLevels[level].optimalBlockSize +
               skandaCompressorLevels[level].niceLength);
    memory +=
        sizeof(LZStructure) * skandaCompressorLevels[level].optimalBlockSize;
    return memory;
  }
  memory += sizeof(uint32_t)
            << std::min(20, windowLog - 3);  // binary node lookup
  if (skandaCompressorLevels[level].parser < OPTIMAL3)
    memory += sizeof(uint32_t) << std::min(14, windowLog - 3);  // hash 3 table
  const size_t binaryTreeWindow =
      (size_t)1 << std::min(skandaCompressorLevels[level].maxHashLog,
                            windowLog);
  if (size < binaryTreeWindow)
    memory += sizeof(uint32_t) * 2 * size;  // binary tree
  else
    memory += sizeof(uint32_t) * 2 * binaryTreeWindow;  // binary tree
  memory += sizeof(SkandaOptimalParserState) *
            (skandaCompressorLevels[level].optimalBlockSize +
             skandaCompressorLevels[level].niceLength) *
            skandaCompressorLevels[level].maxArrivals;
  memory +=
      sizeof(LZStructure) * skandaCompressorLevels[level].optimalBlockSize;
  // match buffer
  if (decSpeedBias < 0.99) {
    size_t maxMatchesPerPosition =
        (1 << skandaCompressorLevels[level].maxElementsLog) + 1;
    memory += sizeof(LZMatch) * maxMatchesPerPosition *
              std::min(size, MAX_BLOCK_SIZE);
    memory += sizeof(int) * std::min(size, MAX_BLOCK_SIZE);
  }
  return memory;
}

size_t estimate_memory(size_t size, int level, float decSpeedBias) {
  if (size <= SKANDA_LAST_BYTES + 1) return 0;

  // Negative levels use the same memory as level 0
  if (level < 0) level = 0;
  if (level > 10) level = 10;
  const int WINDOW_LOGS[] = {24, 23, 22, 21, 20, 20};
  int maxWindowLog = WINDOW_LOGS[(int)(decSpeedBias * 5)];
  int windowLog = int_log2(size - 1) + 1;
  if (windowLog > maxWindowLog) windowLog = maxWindowLog;
  if (windowLog < 6) windowLog = 6;

  bool disableHuffman = decSpeedBias >= 0.99;
  size_t huffmanBufSize = std::min(MAX_BLOCK_SIZE, size);
  size_t huffmanMemoryUsage =
      disableHuffman ? 0 : huffmanBufSize + 128;   // StreamBuffer
  huffmanMemoryUsage += huffmanBufSize / 2 + 128;  // Token stream
  huffmanMemoryUsage += huffmanBufSize / 8 + 128;  // Length stream
  huffmanMemoryUsage += huffmanBufSize + 128;      // Distance stream

  return huffmanMemoryUsage +
         lz_memory_usage(size, windowLog, level, decSpeedBias);
}

bool is_error(size_t errorCode) {
  return errorCode >
         (SIZE_MAX /
          2);  // Error codes are negative numbers, but size_t is unsigned
}

FORCE_INLINE int read_header(const uint8_t*& input,
                             const uint8_t* const inputEnd, size_t* blockSize,
                             int* blockType) {
  if (input >= inputEnd) return -1;
  uint8_t byte = *input++;
  *blockType = byte & 3;
  *blockSize = byte >> 2;
  if (*blockSize < 48) return 0;
  if (input >= inputEnd) return -1;
  // 2 byte block size
  byte = *input++;
  *blockSize -= 48;
  *blockSize |= byte << 4;
  if (*blockSize < 3072) return 0;
  if (input >= inputEnd) return -1;
  // 3 byte block size
  byte = *input++;
  *blockSize -= 192 << 4;
  *blockSize |= byte << 10;
  return 0;
}

class HuffmanDecoder {
  size_t outBufferSize = 0;
  uint8_t* outBuffer = nullptr;
  int streamType;

 public:
  HuffmanDecoder(size_t maxOutBufferSize, int type) {
    outBufferSize = maxOutBufferSize;
    streamType = type;
  }
  ~HuffmanDecoder() { delete[] outBuffer; }

  struct HuffmanDecodeTableEntry {
    uint8_t symbol;
    uint8_t bits;
  };

  uint8_t* get_output_buffer() { return outBuffer; }

  FORCE_INLINE void renormalize_precode(const uint8_t*& huffIt,
                                        const uint8_t* const compressedStart,
                                        uint32_t* huffState, size_t* bitCount) {
    // We can read up to 3 bytes beyond compressedStart.
    // We have at least 1 from block header, 1 from entropy header and 1 from
    // huffman header size so it's safe.
    if (huffIt > compressedStart) {
      *huffState |= read_uint32le(huffIt - 4) >> *bitCount;
      huffIt -= (*bitCount ^ 31) >> 3;
      *bitCount |= 24;
    }
  }
  FORCE_INLINE size_t extract_bits_precode(uint32_t* huffState,
                                           size_t* bitCount,
                                           const size_t nBits) {
    size_t result = *huffState >> (32 - nBits);
    *huffState <<= nBits;
    *bitCount -= nBits;
    return result;
  }
  int decode_huffman_header(
      uint8_t decodedSymbols[MAX_HUFFMAN_CODE_LENGTH + 2][256],
      int decodedCounts[MAX_HUFFMAN_CODE_LENGTH + 2], size_t streamSizes[6],
      const uint8_t* compressed, const uint8_t* const compressedStart) {
    const uint8_t* huffIt = compressed - 4;
    uint32_t huffState = read_uint32le(huffIt);
    size_t bitCount = 32;

    size_t baseSize = extract_bits_precode(&huffState, &bitCount, 16);
    size_t extraBits = extract_bits_precode(&huffState, &bitCount, 4) + 1;
    renormalize_precode(huffIt, compressedStart, &huffState, &bitCount);

    for (size_t i = 0; i < 6; i++) {
      size_t extraSize = extract_bits_precode(&huffState, &bitCount, extraBits);
      streamSizes[i] = baseSize + extraSize;
      renormalize_precode(huffIt, compressedStart, &huffState, &bitCount);
    }

    size_t initialZeroRun = extract_bits_precode(&huffState, &bitCount, 3) + 1;
    if (initialZeroRun == 8) return -1;
    memset(decodedCounts + 1, 0, 8 * sizeof(int));
    size_t usedCodeSpace = 0;

    for (size_t i = initialZeroRun;
         i <= MAX_HUFFMAN_CODE_LENGTH + 1 && usedCodeSpace < PRECODE_CODE_SPACE;
         i++) {
      size_t bits = extract_bits_precode(&huffState, &bitCount, 3) + 1;
      decodedSymbols[bits][decodedCounts[bits]] = i;
      decodedCounts[bits]++;
      usedCodeSpace += PRECODE_CODE_SPACE >> bits;
      renormalize_precode(huffIt, compressedStart, &huffState, &bitCount);
    }

    if (usedCodeSpace != PRECODE_CODE_SPACE) return -1;

    HuffmanDecodeTableEntry decodeTable[PRECODE_CODE_SPACE + 4];
    HuffmanDecodeTableEntry* decodeTableIt = decodeTable;
    for (size_t bits = 1; bits <= MAX_PRECODE_CODE_LENGTH; bits++) {
      for (int i = 0; i < decodedCounts[bits]; i++) {
        HuffmanDecodeTableEntry* upperRange =
            decodeTableIt + (PRECODE_CODE_SPACE >> bits);
        const uint64_t filling =
            decode_table_filler(decodedSymbols[bits][i], bits);
        do {
          memcpy(decodeTableIt, &filling, sizeof(uint64_t));
          decodeTableIt += 4;
        } while (decodeTableIt < upperRange);
        decodeTableIt = upperRange;
      }
    }

    memset(decodedCounts + 1, 0, (MAX_HUFFMAN_CODE_LENGTH + 1) * sizeof(int));
    usedCodeSpace = 0;
    // We can decode up to 3 symbols per refill
    for (size_t i = 0; i < 256 && usedCodeSpace < HUFFMAN_CODE_SPACE; i += 3) {
      size_t value = huffState >> (32 - MAX_PRECODE_CODE_LENGTH);
      size_t bits = decodeTable[value].symbol;
      huffState <<= decodeTable[value].bits;
      bitCount -= decodeTable[value].bits;

      decodedSymbols[bits][decodedCounts[bits]] = i + 0;
      decodedCounts[bits]++;
      usedCodeSpace += HUFFMAN_CODE_SPACE >> bits;

      if (i >= 255 || usedCodeSpace >= HUFFMAN_CODE_SPACE) break;

      value = huffState >> (32 - MAX_PRECODE_CODE_LENGTH);
      bits = decodeTable[value].symbol;
      huffState <<= decodeTable[value].bits;
      bitCount -= decodeTable[value].bits;

      decodedSymbols[bits][decodedCounts[bits]] = i + 1;
      decodedCounts[bits]++;
      usedCodeSpace += HUFFMAN_CODE_SPACE >> bits;

      if (usedCodeSpace >= HUFFMAN_CODE_SPACE) break;

      value = huffState >> (32 - MAX_PRECODE_CODE_LENGTH);
      bits = decodeTable[value].symbol;
      huffState <<= decodeTable[value].bits;
      bitCount -= decodeTable[value].bits;

      decodedSymbols[bits][decodedCounts[bits]] = i + 2;
      decodedCounts[bits]++;
      usedCodeSpace += HUFFMAN_CODE_SPACE >> bits;

      renormalize_precode(huffIt, compressedStart, &huffState, &bitCount);
    }

    if (usedCodeSpace != HUFFMAN_CODE_SPACE) return -1;
    return 0;
  }

  // Can move the stream pointer up to 7 bytes.
  FORCE_INLINE void renormalize(size_t& state, const uint8_t*& stream) {
    size_t consumedBits = unsafe_bit_scan_forward(state);
    stream -= consumedBits >> 3;
    state = (IS_64BIT ? read_uint64le(stream) : read_uint32le(stream)) | 1;
    state <<= consumedBits & 7;
  }

  FORCE_INLINE void decode_op(size_t& state, uint8_t* const out,
                              const HuffmanDecodeTableEntry* const table) {
    const size_t value =
        state >> ((IS_64BIT ? 64 : 32) - MAX_HUFFMAN_CODE_LENGTH);
    state <<= table[value].bits;
    *out = table[value].symbol;
  }

  FORCE_INLINE uint64_t decode_table_filler(uint8_t symbol, uint8_t bits) {
    HuffmanDecodeTableEntry entry = {symbol, bits};
    uint16_t u16;
    memcpy(&u16, &entry, 2);
    return 0x0001000100010001 * u16;
  }

  void generate_decode_table(
      const uint8_t decodedSymbols[MAX_HUFFMAN_CODE_LENGTH + 2][256],
      const int decodedCounts[MAX_HUFFMAN_CODE_LENGTH + 2],
      HuffmanDecodeTableEntry* table) {
    HuffmanDecodeTableEntry* it = table;
    for (size_t bits = 1; bits < MAX_HUFFMAN_CODE_LENGTH - 3; bits++) {
      for (int i = 0; i < decodedCounts[bits]; i++) {
        HuffmanDecodeTableEntry* upperRange = it + (HUFFMAN_CODE_SPACE >> bits);
        const uint64_t filling =
            decode_table_filler(decodedSymbols[bits][i], bits);
        do {
          memcpy(it + 0, &filling, sizeof(uint64_t));
          memcpy(it + 4, &filling, sizeof(uint64_t));
          memcpy(it + 8, &filling, sizeof(uint64_t));
          memcpy(it + 12, &filling, sizeof(uint64_t));
          it += 16;
        } while (it < upperRange);
        it = upperRange;
      }
    }

    for (int i = 0; i < decodedCounts[MAX_HUFFMAN_CODE_LENGTH - 3]; i++) {
      const uint64_t filling =
          decode_table_filler(decodedSymbols[MAX_HUFFMAN_CODE_LENGTH - 3][i],
                              MAX_HUFFMAN_CODE_LENGTH - 3);
      memcpy(it + 0, &filling, sizeof(uint64_t));
      memcpy(it + 4, &filling, sizeof(uint64_t));
      it += 8;
    }

    for (int i = 0; i < decodedCounts[MAX_HUFFMAN_CODE_LENGTH - 2]; i++) {
      const uint64_t filling =
          decode_table_filler(decodedSymbols[MAX_HUFFMAN_CODE_LENGTH - 2][i],
                              MAX_HUFFMAN_CODE_LENGTH - 2);
      memcpy(it, &filling, sizeof(uint64_t));
      it += 4;
    }

    for (int i = 0; i < decodedCounts[MAX_HUFFMAN_CODE_LENGTH - 1]; i++) {
      it[0].bits = MAX_HUFFMAN_CODE_LENGTH - 1;
      it[0].symbol = decodedSymbols[MAX_HUFFMAN_CODE_LENGTH - 1][i];
      it[1].bits = MAX_HUFFMAN_CODE_LENGTH - 1;
      it[1].symbol = decodedSymbols[MAX_HUFFMAN_CODE_LENGTH - 1][i];
      it += 2;
    }

    for (int i = 0; i < decodedCounts[MAX_HUFFMAN_CODE_LENGTH]; i++) {
      it->bits = MAX_HUFFMAN_CODE_LENGTH;
      it->symbol = decodedSymbols[MAX_HUFFMAN_CODE_LENGTH][i];
      it++;
    }
  }

  void decode_symbols(
      const uint8_t decodedSymbols[MAX_HUFFMAN_CODE_LENGTH + 2][256],
      const int decodedCounts[MAX_HUFFMAN_CODE_LENGTH + 2],
      const size_t streamSizes[6], const size_t symbolCount,
      const uint8_t* const compressed, uint8_t* out) {
    HuffmanDecodeTableEntry table[HUFFMAN_CODE_SPACE];
    generate_decode_table(decodedSymbols, decodedCounts, table);

    const uint8_t* streamA = compressed + streamSizes[0];
    const uint8_t* streamB = streamA + streamSizes[1];
    const uint8_t* streamC = streamB + streamSizes[2];
    const uint8_t* streamD = streamC + streamSizes[3];
    const uint8_t* streamE = streamD + streamSizes[4];
    const uint8_t* streamF = streamE + streamSizes[5];
    const uint8_t* const streamBase =
        compressed;  // Do not read beyond this point

#if IS_64BIT
    // We have 1 byte from block header, 1 byte from entropy header, 1 byte from
    // huffman header size and 4 bytes of huffman header. We can only read 7
    // bytes safely, but that is enough to initialize the streams. Renormalize
    // will only read 7 bytes as well.
    streamA -= 7;
    size_t stateA = (read_uint64le(streamA) | 1) << 8;
    streamB -= 7;
    size_t stateB = (read_uint64le(streamB) | 1) << 8;
    streamC -= 7;
    size_t stateC = (read_uint64le(streamC) | 1) << 8;
    streamD -= 7;
    size_t stateD = (read_uint64le(streamD) | 1) << 8;
    streamE -= 7;
    size_t stateE = (read_uint64le(streamE) | 1) << 8;
    streamF -= 7;
    size_t stateF = (read_uint64le(streamF) | 1) << 8;
    uint8_t* const fastLoopEnd = out + (symbolCount - symbolCount % 30);
#else
    streamA -= 4;
    size_t stateA = read_uint32le(streamA) | 1;
    streamB -= 4;
    size_t stateB = read_uint32le(streamB) | 1;
    streamC -= 4;
    size_t stateC = read_uint32le(streamC) | 1;
    streamD -= 4;
    size_t stateD = read_uint32le(streamD) | 1;
    streamE -= 4;
    size_t stateE = read_uint32le(streamE) | 1;
    streamF -= 4;
    size_t stateF = read_uint32le(streamF) | 1;
    uint8_t* const fastLoopEnd = out + (symbolCount - symbolCount % 12);
#endif
    uint8_t* const outEnd = out + symbolCount;

    while (out < fastLoopEnd) {
      if (IS_64BIT) {
        // First decode
        decode_op(stateA, out + 0, table);
        decode_op(stateB, out + 1, table);
        decode_op(stateC, out + 2, table);
        decode_op(stateD, out + 3, table);
        decode_op(stateE, out + 4, table);
        decode_op(stateF, out + 5, table);
        // Second decode
        decode_op(stateA, out + 6, table);
        decode_op(stateB, out + 7, table);
        decode_op(stateC, out + 8, table);
        decode_op(stateD, out + 9, table);
        decode_op(stateE, out + 10, table);
        decode_op(stateF, out + 11, table);
        // Third decode
        decode_op(stateA, out + 12, table);
        decode_op(stateB, out + 13, table);
        decode_op(stateC, out + 14, table);
        decode_op(stateD, out + 15, table);
        decode_op(stateE, out + 16, table);
        decode_op(stateF, out + 17, table);
        // Fourth decode
        decode_op(stateA, out + 18, table);
        decode_op(stateB, out + 19, table);
        decode_op(stateC, out + 20, table);
        decode_op(stateD, out + 21, table);
        decode_op(stateE, out + 22, table);
        decode_op(stateF, out + 23, table);
        // Fifth decode and renormalization
        decode_op(stateA, out + 24, table);
        if (streamA >= streamBase) renormalize(stateA, streamA);
        decode_op(stateB, out + 25, table);
        if (streamB >= streamBase) renormalize(stateB, streamB);
        decode_op(stateC, out + 26, table);
        if (streamC >= streamBase) renormalize(stateC, streamC);
        decode_op(stateD, out + 27, table);
        if (streamD >= streamBase) renormalize(stateD, streamD);
        decode_op(stateE, out + 28, table);
        if (streamE >= streamBase) renormalize(stateE, streamE);
        decode_op(stateF, out + 29, table);
        if (streamF >= streamBase) renormalize(stateF, streamF);
        out += 30;
      } else {
        // First decode
        decode_op(stateA, out + 0, table);
        decode_op(stateB, out + 1, table);
        decode_op(stateC, out + 2, table);
        decode_op(stateD, out + 3, table);
        decode_op(stateE, out + 4, table);
        decode_op(stateF, out + 5, table);
        // Second decode and renormalization
        decode_op(stateA, out + 6, table);
        if (streamA >= streamBase) renormalize(stateA, streamA);
        decode_op(stateB, out + 7, table);
        if (streamB >= streamBase) renormalize(stateB, streamB);
        decode_op(stateC, out + 8, table);
        if (streamC >= streamBase) renormalize(stateC, streamC);
        decode_op(stateD, out + 9, table);
        if (streamD >= streamBase) renormalize(stateD, streamD);
        decode_op(stateE, out + 10, table);
        if (streamE >= streamBase) renormalize(stateE, streamE);
        decode_op(stateF, out + 11, table);
        if (streamF >= streamBase) renormalize(stateF, streamF);
        out += 12;
      }
    }

    // States are already normalized, thus we have enough bits to decode
    // remaining symbols
    while (out < outEnd) {
      decode_op(stateA, out + 0, table);
      decode_op(stateB, out + 1, table);
      decode_op(stateC, out + 2, table);
      decode_op(stateD, out + 3, table);
      decode_op(stateE, out + 4, table);
      decode_op(stateF, out + 5, table);
      out += 6;
    }
  }

  // Returns error code or 0. compressed will point to end of this compressed
  // stream,
  //  streamIt to decompressed data and streamEnd to the end of decompressed
  //  data
  size_t decode(const uint8_t*& compressed,
                const uint8_t* const compressedStart,
                const uint8_t* const compressedEnd, const uint8_t*& streamIt,
                const uint8_t*& streamEnd) {
    size_t symbolCount;
    int mode;
    if (unlikely(read_header(compressed, compressedEnd, &symbolCount, &mode)))
      return ERROR_CORRUPT;
    if (unlikely(symbolCount > outBufferSize)) return ERROR_CORRUPT;

    if (mode == ENTROPY_RAW) {
      if (unlikely(compressedEnd - compressed < symbolCount))
        return ERROR_CORRUPT;
      streamIt = compressed;
      compressed += symbolCount;
      streamEnd = compressed;
      return 0;
    }

    if (!outBuffer) {
      try {
        outBuffer = new uint8_t[outBufferSize];
      } catch (const std::bad_alloc& e) {
        return ERROR_NOMEM;
      }
    }
    streamIt = outBuffer;
    streamEnd = outBuffer + symbolCount;

    if (mode == ENTROPY_RLE) {
      uint8_t byte = *compressed++;
      memset(outBuffer, byte, symbolCount);
      return 0;
    }

    if (mode == ENTROPY_HUFFMAN) {
      size_t headerSize = *compressed++ & 0x7F;
      // 16 bits for base size, 4 bits for number of extra size bits, at least 1
      // bit for extra size for each stream,
      //  3 bits for huffman tree not transmitted, and 1 bit to select a
      //  predefined huffman tree or reuse tree. Total 30 bits or 4 bytes that
      //  the header should at least take.
      // Although predefined or reuse trees are not yet implemented, this could
      // be a reasonable way to do it, so I am taking it into account for the
      // future.
      if (headerSize <= 4 || compressed + headerSize > compressedEnd)
        return ERROR_CORRUPT;
      size_t streamSizes[6];
      uint8_t decodedSymbols[MAX_HUFFMAN_CODE_LENGTH + 2][256];
      int decodedCounts[MAX_HUFFMAN_CODE_LENGTH + 2];

      int err =
          decode_huffman_header(decodedSymbols, decodedCounts, streamSizes,
                                compressed + headerSize, compressed);
      if (err) return err;
      compressed += headerSize;

      const uint8_t* const compressedStreamEnd =
          compressed + streamSizes[0] + streamSizes[1] + streamSizes[2] +
          streamSizes[3] + streamSizes[4] + streamSizes[5];
      if (compressedStreamEnd > compressedEnd) return ERROR_CORRUPT;

      decode_symbols(decodedSymbols, decodedCounts, streamSizes, symbolCount,
                     compressed, outBuffer);
      compressed = compressedStreamEnd;
      return 0;
    }

    return ERROR_CORRUPT;
  }
};

const int8_t inc32table[16] = {0, 1, 2, 1, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
const int8_t inc64table[16] = {0, 0, 0, 1, 4, -1, -2, -3,
                               4, 4, 4, 4, 4, 4,  4,  4};

FORCE_INLINE size_t decode_length(const uint8_t*& compressed) {
  size_t first = *compressed++;
  if (likely(first <= 223)) return first;
  size_t second = *compressed++;
  if (likely(second <= 223)) return (second << 5) + first;
  size_t third = *compressed++;
  return (((third << 5) + second) << 5) + first;
}

struct LZDecodeResults {
  size_t error;
  const uint8_t* tokenStreamIt;
  const uint8_t* literalStreamIt;
  const uint8_t* lengthStreamIt;
  const uint8_t* distanceStreamIt;
  size_t distance;
  uint8_t* decompressed;
};

// CHECK_LEVEL modifies the number of checks performed to guarantee input
// safety. The higher the level, the less checks are performed.
template <int CHECK_LEVEL>
LZDecodeResults decode_lz_block(
    const uint8_t* tokenStreamIt, const uint8_t* literalStreamIt,
    const uint8_t* lengthStreamIt, const uint8_t* distanceStreamIt,
    const uint8_t* const tokenStreamEnd, const uint8_t* const literalStreamEnd,
    const uint8_t* const lengthStreamEnd,
    const uint8_t* const distanceStreamEnd, size_t distance,
    uint8_t* decompressed, uint8_t* const decompressedStart,
    uint8_t* const thisBlockEnd, uint8_t* const subBlockEnd) {
  do {
    const size_t token = *tokenStreamIt++;
    if (CHECK_LEVEL == 0 && unlikely(tokenStreamIt > tokenStreamEnd))
      return LZDecodeResults{ERROR_CORRUPT};
    size_t length = token >> 5;

    if (unlikely(length == 7)) {
      length += decode_length(lengthStreamIt);
      // Detect buffer overflows
      if (CHECK_LEVEL == 0 &&
          unlikely(lengthStreamIt > lengthStreamEnd ||
                   literalStreamEnd - literalStreamIt < length))
        return LZDecodeResults{ERROR_CORRUPT};
      memcpy(decompressed, literalStreamIt, 16);
      memcpy(decompressed + 16, literalStreamIt + 16, 16);

      if (unlikely(length > 32)) {
        if (CHECK_LEVEL != 0 &&
            unlikely(literalStreamEnd - literalStreamIt < length))
          return LZDecodeResults{ERROR_CORRUPT};
        if (unlikely(thisBlockEnd - decompressed < length))
          return LZDecodeResults{ERROR_CORRUPT};

        uint8_t* dst = decompressed + 32;
        const uint8_t* src = literalStreamIt + 32;
        uint8_t* const end = decompressed + length;
        do {
          memcpy(dst, src, 16);
          memcpy(dst + 16, src + 16, 16);
          src += 32;
          dst += 32;
        } while (dst < end);
      }

      decompressed += length;
      literalStreamIt += length;
    } else {
      memcpy(decompressed, literalStreamIt, 8);
      decompressed += length;
      literalStreamIt += length;
      // Detect buffer overflows
      if (CHECK_LEVEL == 0 && unlikely(literalStreamIt > literalStreamEnd))
        return LZDecodeResults{ERROR_CORRUPT};
    }

    size_t distanceBits = token & 0x18;
    size_t newDistance =
        read_uint32le(distanceStreamIt) & (1 << distanceBits) - 1;
    length = token & 0x7;
    distance ^= (0 - (distanceBits != 0)) &
                (distance ^ newDistance);  // branchless selection
    distanceStreamIt += distanceBits >> 3;
    const uint8_t* match = decompressed - distance;

    if (CHECK_LEVEL < 2 &&
        unlikely(decompressed - decompressedStart < distance))
      return LZDecodeResults{ERROR_CORRUPT};
    if (CHECK_LEVEL == 0 && unlikely(distanceStreamIt > distanceStreamEnd))
      return LZDecodeResults{ERROR_CORRUPT};

    if (length == 7) {
      length = decode_length(lengthStreamIt) + SKANDA_MIN_MATCH_LENGTH + 7;
      // Detect buffer overflows
      if (CHECK_LEVEL == 0 && unlikely(lengthStreamIt > lengthStreamEnd))
        return LZDecodeResults{ERROR_CORRUPT};
      if (unlikely(thisBlockEnd - decompressed < (ptrdiff_t)length))
        return LZDecodeResults{ERROR_CORRUPT};
      uint8_t* const copyEnd = decompressed + length;

      // If the distance is big enough we can perform a faster copy
      if (likely(distance >= 16)) {
        // Common case length <= 32: omit some instructions
        memcpy(decompressed, match, 16);
        memcpy(decompressed + 16, match + 16, 16);

        if (length > 32) {
          match += 32;
          decompressed += 32;
          do {
            memcpy(decompressed, match, 16);
            memcpy(decompressed + 16, match + 16, 16);
            match += 32;
            decompressed += 32;
          } while (decompressed < copyEnd);
        }
      }
      // Else it is a run-length type match
      else {
        decompressed[0] = match[0];
        decompressed[1] = match[1];
        decompressed[2] = match[2];
        decompressed[3] = match[3];
        match += inc32table[distance];
        memcpy(decompressed + 4, match, 4);
        match += inc64table[distance];
        memcpy(decompressed + 8, match, 8);

        if (length > 16) {
          decompressed += 16;
          match += 8;
          do {
            memcpy(decompressed, match, 8);
            memcpy(decompressed + 8, match + 8, 8);
            match += 16;
            decompressed += 16;
          } while (decompressed < copyEnd);
        }
      }

      decompressed = copyEnd;
    } else {
      // If the distance is big enough we can perform a faster copy
      if (likely(distance >= 8)) memcpy(decompressed, match, 8);
      // Else it is a run-length type match
      else {
        decompressed[0] = match[0];
        decompressed[1] = match[1];
        decompressed[2] = match[2];
        decompressed[3] = match[3];
        match += inc32table[distance];
        memcpy(decompressed + 4, match, 4);
      }
      decompressed += length + SKANDA_MIN_MATCH_LENGTH;
    }

  } while (likely(decompressed < subBlockEnd));

  return LZDecodeResults{0,
                         tokenStreamIt,
                         literalStreamIt,
                         lengthStreamIt,
                         distanceStreamIt,
                         distance,
                         decompressed};
}

size_t decompress(const uint8_t* compressed, size_t compressedSize,
                  uint8_t* decompressed, size_t decompressedSize,
                  ProgressCallback* progress) {
  if (unlikely(compressedSize == 0)) return ERROR_CORRUPT;

  uint8_t* const decompressedStart = decompressed;
  uint8_t* const decompressedEnd =
      decompressed + (decompressedSize < SKANDA_LAST_BYTES
                          ? 0
                          : decompressedSize - SKANDA_LAST_BYTES);
  const uint8_t* const compressedStart = compressed;
  const uint8_t* const compressedEnd =
      compressed + compressedSize -
      (decompressedSize < SKANDA_LAST_BYTES ? decompressedSize
                                            : SKANDA_LAST_BYTES);

  HuffmanDecoder literalDecoder(std::min(MAX_BLOCK_SIZE, decompressedSize) + 32,
                                LITERAL_STREAM);
  HuffmanDecoder tokenDecoder(
      std::min(MAX_BLOCK_SIZE, decompressedSize) / 2 + 32, TOKEN_STREAM);
  HuffmanDecoder lengthDecoder(
      std::min(MAX_BLOCK_SIZE, decompressedSize) / 8 + 32, LENGTH_STREAM);
  HuffmanDecoder distanceDecoder(
      std::min(MAX_BLOCK_SIZE, decompressedSize) * 1.5 + 32, DISTANCE_STREAM);
  size_t distance = 1;

  while (true) {
    size_t thisBlockSize;
    int blockType;
    if (unlikely(
            read_header(compressed, compressedEnd, &thisBlockSize, &blockType)))
      return ERROR_CORRUPT;

    // Last block always contains uncompressed bytes
    if (blockType == BLOCK_LAST) {
      // Not enough bytes in either buffer
      if (decompressedStart + decompressedSize - decompressed < thisBlockSize ||
          compressedStart + compressedSize - compressed < thisBlockSize)
        return ERROR_CORRUPT;
      memcpy(decompressed, compressed, thisBlockSize);
      return 0;
    }

    // Decode LZ+Huffman block
    uint8_t* const thisBlockEnd = decompressed + thisBlockSize;
    if (unlikely(thisBlockEnd > decompressedEnd || blockType != BLOCK_BASIC))
      return ERROR_CORRUPT;

    const uint8_t* literalStreamEnd;
    const uint8_t* tokenStreamEnd;
    const uint8_t* lengthStreamEnd;
    const uint8_t* distanceStreamEnd;
    const uint8_t* literalStreamIt;
    const uint8_t* distanceStreamIt;
    const uint8_t* tokenStreamIt;
    const uint8_t* lengthStreamIt;
    size_t error;

    error = literalDecoder.decode(compressed, compressedStart, compressedEnd,
                                  literalStreamIt, literalStreamEnd);
    if (unlikely(error)) return error;
    bool huffmanLiterals =
        literalStreamIt == literalDecoder.get_output_buffer();
    error = distanceDecoder.decode(compressed, compressedStart, compressedEnd,
                                   distanceStreamIt, distanceStreamEnd);
    if (unlikely(error)) return error;
    bool huffmanDistances =
        distanceStreamIt == distanceDecoder.get_output_buffer();
    error = tokenDecoder.decode(compressed, compressedStart, compressedEnd,
                                tokenStreamIt, tokenStreamEnd);
    if (unlikely(error)) return error;
    bool huffmanTokens = tokenStreamIt == tokenDecoder.get_output_buffer();
    error = lengthDecoder.decode(compressed, compressedStart, compressedEnd,
                                 lengthStreamIt, lengthStreamEnd);
    if (unlikely(error)) return error;
    bool huffmanLengths = lengthStreamIt == lengthDecoder.get_output_buffer();

    while (decompressed < thisBlockEnd) {
      size_t subBlockSize = std::min(16384, int(thisBlockEnd - decompressed));
      uint8_t* subBlockEnd = decompressed + subBlockSize;

      // LZ decoding obviously needs checks for malicious input, but they take a
      // 15% performance hit. We will divide them into 3 categories:
      // - Output buffer overflow: we have a 32 byte buffer at the end, so we
      // only have to check
      //   these on long literal runs or matches, which are rare, so this is not
      //   a problem.
      // - Long distances: the only way this check can be avoided is after
      // having decompressed 16MB of data
      // - Compressed streams: we need to make sure we dont read beyond the end
      // of any of the data 4 streams.
      //   Note however, that for a given block size you can only read so many
      //   bytes in any stream. Specificaly:
      //   - Distance stream: 3 bytes per match, minimum match length 2 -> 1.5x
      //   block size
      //   - Token stream: minimum 2 bytes decompressed per token -> 0.5x block
      //   size
      //   - Literal stream -> 1x block size
      //   - Length stream: 1 byte read for 7 bytes of literal run length and 1
      //   for 9 match length -> 0.125x block size If the streams are huffman
      //   compressed we just need a big enough buffer to ensure we cant read
      //   outside of it. If they come directly from compressed buffer we need
      //   to make sure we are far away from the end.
      // With these we can minimize the performance penalty while keeping the
      // decompressor safe.

      LZDecodeResults results;
      // Stream was huffman compressed, or is far from the end
      if ((huffmanLiterals ||
           compressedEnd - literalStreamIt >= subBlockSize) &&
          (huffmanDistances ||
           compressedEnd - distanceStreamIt >= subBlockSize * 1.5) &&
          (huffmanTokens ||
           compressedEnd - tokenStreamIt >= subBlockSize / 2) &&
          (huffmanLengths ||
           compressedEnd - lengthStreamIt >= subBlockSize / 8)) {
        if (decompressed - decompressedStart >= (1 << 24)) {
          results = decode_lz_block<2>(
              tokenStreamIt, literalStreamIt, lengthStreamIt, distanceStreamIt,
              tokenStreamEnd, literalStreamEnd, lengthStreamEnd,
              distanceStreamEnd, distance, decompressed, decompressedStart,
              thisBlockEnd, subBlockEnd);
        } else {
          results = decode_lz_block<1>(
              tokenStreamIt, literalStreamIt, lengthStreamIt, distanceStreamIt,
              tokenStreamEnd, literalStreamEnd, lengthStreamEnd,
              distanceStreamEnd, distance, decompressed, decompressedStart,
              thisBlockEnd, subBlockEnd);
        }
      } else {
        results = decode_lz_block<0>(
            tokenStreamIt, literalStreamIt, lengthStreamIt, distanceStreamIt,
            tokenStreamEnd, literalStreamEnd, lengthStreamEnd,
            distanceStreamEnd, distance, decompressed, decompressedStart,
            thisBlockEnd, subBlockEnd);
      }

      if (unlikely(results.error)) return results.error;
      tokenStreamIt = results.tokenStreamIt;
      literalStreamIt = results.literalStreamIt;
      lengthStreamIt = results.lengthStreamIt;
      distanceStreamIt = results.distanceStreamIt;
      distance = results.distance;
      decompressed = results.decompressed;
    }

    decompressed = thisBlockEnd;
    if (progress) {
      if (progress->progress(decompressed - decompressedStart,
                             compressed - (compressedEnd - compressedSize)))
        return 0;
    }
  }
}
}  // namespace skanda
}  // namespace base
}  // namespace lynx

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#undef try
#undef catch
#undef throw
