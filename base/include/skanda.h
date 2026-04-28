/*
MIT License

Skanda Compression Algorithm v1.0
Copyright (c) 2023-2024 Carlos de Diego

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef BASE_INCLUDE_SKANDA_H_
#define BASE_INCLUDE_SKANDA_H_
#define __SKANDA__
// cspell:ignore lzdict downmix storeu cvtepi probs castps hskip aproximator
// cspell:ignore aproximate substream subblock lzargs lzthread THREADSTART
// cspell:ignore threadstart lzerror clzll ctzll ACDDU

#if defined(__clang__)
#pragma clang system_header
#endif

#include <stdio.h>  //size_t

#include <cstdint>
#include <future>
#include <utility>

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

// Base class that allows the usage of a multithreading to accelerate decoding.
// You will have to create
//  a child class which implements the enqueue function, which will return a
//  future with the thread task or throw an std::bad_alloc or std::system_error
//  exception.
// While you can use this class as is, which will spawn a new thread, the
// creation overhead
//  can be counterproductive in some cases.
class ThreadCallback {
 public:
  virtual std::future<size_t> enqueue(size_t (*func)(void*), void* arg) {
    return std::async(std::launch::async, func, arg);
  }
};

// Base class that allows the usage of a custom memory allocator. You will have
// to create
//  a child class which implements the allocate and free functions. If
//  allocate() fails it must throw an std::bad_alloc exception. There is also
//  the constant skanda::AllocatorCallback::DECOMPRESSOR_MAX_ALLOC, which
//  indicates the maximum amount of memory the decoder will pass to allocate().
class AllocatorCallback {
 public:
  static const int DECOMPRESSOR_MAX_ALLOC = 524288;
  virtual void* allocate(size_t size) { return new uint8_t[size]; }
  virtual void free(void* ptr) { delete[] (uint8_t*)ptr; }
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
                AllocatorCallback* allocator = nullptr,
                ProgressCallback* progress = nullptr);
// Decompresses contents in "compressed" to "decompressed".
// You may pass a pointer to an object with base class ThreadCallback, which
// allows decoding
//  using 2 threads. This only helps if the file is relatively big (512KB) and
//  decSpeedBias was not set to 1 during encoding.
// You may also pass a pointer to an object with base class ProgressCallback, to
// track progress. Returns 0 on success or a negative error code on failure.
size_t decompress(const uint8_t* compressed, size_t compressedSize,
                  uint8_t* decompressed, size_t decompressedSize,
                  ThreadCallback* threadPool = nullptr,
                  AllocatorCallback* allocator = nullptr,
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
const size_t ERROR_THREADSTART = 0 - 2;
const size_t ERROR_CORRUPT = 0 - 3;
}  // namespace skanda

#ifdef SKANDA_IMPLEMENTATION

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

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
#include <immintrin.h>
#include <intrin.h>
#if defined(__SSE4_1__) || defined(__AVX__) || defined(__AVX2__)
#define COMPILE_SSE41
#endif
#endif
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__amd64__)
#define x64
#include <x86intrin.h>
#if __SSE4_1__
#define COMPILE_SSE41
#endif
#endif
#endif

namespace skanda {

#ifdef COMPILE_SSE41
bool hasSSE41() {
#if defined(_MSC_VER)
  int cpu_id[4];
  __cpuid(cpu_id, 0x00000001);
  return (cpu_id[2] >> 19) & 1;
#elif defined(__GNUC__) || defined(__clang__)
  return __builtin_cpu_supports("sse4.1");
#else
  return 0;
#endif
}
const bool HAS_SSE41 = hasSSE41();
#endif

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
const int SKANDA_LAST_BYTES = 31;
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
  // 5, 6, etc We can use only one comparison to make it faster. For powers of
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
  AllocatorCallback* allocator;
  Value* arr = nullptr;
  int hashShift;

 public:
  // Use 2^x sizes to avoid the use of modulo operator
  HashTable(AllocatorCallback* alloc) { allocator = alloc; }
  ~HashTable() { allocator->free(arr); }
  void init(const int logSize) {
    arr = (Value*)allocator->allocate(sizeof(Value) * ((size_t)1 << logSize));
    memset(arr, 0, sizeof(Value) * ((size_t)1 << logSize));
    hashShift = (IS_64BIT ? 64 : 32) - logSize;
  }
  Value& operator[](const size_t value) {
    return arr[Hash{}(value) >> hashShift];
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
  AllocatorCallback* allocator;
  Value* arr = nullptr;
  int hashShift;
  int elementsLog;

 public:
  LZCacheTable(AllocatorCallback* alloc) { allocator = alloc; }
  ~LZCacheTable() { allocator->free(arr); }
  void init(const int logSize, const int numElementsLog) {
    arr = (Value*)allocator->allocate(sizeof(Value) *
                                      (1 << logSize << numElementsLog));
    memset(arr, 0, sizeof(Value) * (1 << logSize << numElementsLog));
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
  HashTableMatchFinder(AllocatorCallback* alloc)
      : lzdict3(alloc), lzdict4(alloc), lzdict8(alloc) {}

  void init(const int windowLog, const CompressorOptions& compressorOptions) {
    const int hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
    lzdict3.init(std::min(hashLog, 14));
    lzdict4.init(hashLog, compressorOptions.maxElementsLog);
    lzdict8.init(hashLog, compressorOptions.maxElementsLog);
  }

  LZMatch* find_matches_and_update(const uint8_t* const input,
                                   const uint8_t* const inputStart,
                                   const uint8_t* const limit, LZMatch* matches,
                                   size_t lastLength, const int windowLog) {
    uint32_t& chain3 = lzdict3[read_hash3(input)];
    if (lastLength < 3) {
      const uint8_t* where = inputStart + chain3;
      size_t length = test_match<true>(input, where, limit, 3, windowLog);

      if (length > lastLength) {
        lastLength = length;
        matches->distance = input - where;
        matches->length = length;
        matches++;
      }
    }
    chain3 = input - inputStart;

    LZCacheBucket<uint32_t> chain4 = lzdict4[read_hash4(input)];
    LZCacheBucket<uint32_t> chain8 = lzdict8[read_hash8(input)];
    uint32_t pos4 = input - inputStart;
    uint32_t pos8 = input - inputStart;

    do {
      chain4.next(&pos4);
      chain8.next(&pos8);
      const uint8_t* where4 = inputStart + pos4;
      const uint8_t* where8 = inputStart + pos8;

      bool matched4 = *(input + lastLength) == *(where4 + lastLength);
      bool matched8 = *(input + lastLength) == *(where8 + lastLength);

      if (matched4 || matched8) {
        const uint8_t* where =
            matched8 ? where8 : where4;  // Prioritize hash8, bit better results
        size_t length = test_match<true>(input, where, limit, 4, windowLog);
        if (length > lastLength) {
          lastLength = length;
          matches->distance = input - where;
          matches->length = length;
          matches++;
        }
      }
    } while (!chain8.ended());

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
  AllocatorCallback* allocator;
  LZCacheTable<uint32_t, FastIntHash> lzdict3chain;
  HashTable<uint32_t, FastIntHash> nodeLookup;
  uint32_t* nodes = nullptr;
  size_t nodeListSize;
  size_t nodeListMask;

 public:
  ~BinaryMatchFinder() { allocator->free(nodes); }
  BinaryMatchFinder(AllocatorCallback* alloc)
      : lzdict3chain(alloc), nodeLookup(alloc) {
    allocator = alloc;
  }

  void init(const size_t inputSize, const int windowLog,
            const CompressorOptions& compressorOptions) {
    const int binaryTreeWindow =
        std::min(compressorOptions.maxHashLog, windowLog);

    // Input size is smaller than maximum binary tree size
    if (inputSize < ((size_t)1 << binaryTreeWindow)) {
      nodes =
          (uint32_t*)allocator->allocate(sizeof(uint32_t) * (2 * inputSize));
      nodeListSize = inputSize;
      nodeListMask = -1;
    } else {
      nodes = (uint32_t*)allocator->allocate(sizeof(uint32_t) *
                                             (2 << binaryTreeWindow));
      nodeListSize = (size_t)1 << binaryTreeWindow;
      nodeListMask = nodeListSize - 1;
    }
    if (compressorOptions.parser < OPTIMAL3)
      lzdict3chain.init(std::min(14, windowLog - 3), 0);
    else
      lzdict3chain.init(std::min(16, windowLog - 3),
                        compressorOptions.maxElementsLog - 4);
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
    LZCacheBucket<uint32_t> chain3 = lzdict3chain[read_hash3(input)];
    uint32_t pos = input - inputStart;
    while (!chain3.ended()) {
      chain3.next(&pos);
      const uint8_t* where = inputStart + pos;

      if (*(input + nextExpectedLength - 1) !=
          *(where + nextExpectedLength - 1))
        continue;

      const size_t length =
          test_match<true>(input, where, blockLimit, 3, windowLog);

      if (length >= nextExpectedLength) {
        matches->distance = input - where;
        matches->length = length;
        matches++;

        nextExpectedLength = length;
      }
    }

    // If we reach this position stop the search
    const size_t btEnd =
        inputPosition < nodeListSize ? 0 : inputPosition - nodeListSize;

    uint32_t& lookupEntry = nodeLookup[read_hash4(input)];
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
    lzdict3chain[read_hash3(input)].push(inputPosition);

    // If we reach this position on the front stop the update
    const uint8_t* positionSkip =
        std::min(limit, input + compressorOptions.niceLength);
    // If we reach this position on the back stop the update
    const size_t btEnd =
        inputPosition < nodeListSize ? 0 : inputPosition - nodeListSize;
    uint32_t& lookupEntry = nodeLookup[read_hash4(input)];
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
  DISTANCE_STREAM,
  TOKEN_STREAM,
  LENGTH_STREAM,
};

enum {
  BLOCK_COMPRESSED = 0,
  BLOCK_RAW,
};

enum {
  BLOCK_NO_FLAGS = 0,
  BLOCK_LAST = 1,
  STREAM_DISTANCE_ADVANCED = 1,
  STREAM_LITERALS_DELTA = 1,
  STREAM_LITERALS_POS_MASK3 = 4,
};

struct HuffmanSymbol {
  uint16_t code;
  uint8_t bits;
};

// output has 8 bits of decimal precision
int fixed_log2(size_t c) {
  int base = unsafe_int_log2(c);

  uint8_t table[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   11,  22,  33,  43,  53,  63,
      73,  82,  91,  100, 109, 117, 125, 134, 141, 149, 157, 164, 172,
      179, 186, 193, 200, 206, 213, 219, 225, 232, 238, 244, 250,
  };

  c <<= 5;
  c >>= base;
  return (base << 8) | (table[c]);
}

void fast_huffman_codegen(uint32_t* symbolHistogram, HuffmanSymbol* symbolData,
                          size_t symbolCount, const size_t uniqueSymbols,
                          const int maxCodeLength) {
  // Very fast and precise length-limited huffman code generator based on
  // https://github.com/stbrumme/length-limited-prefix-codes/blob/master/limitedkraftheap.c
  // Modified to remove the need for a heap
  const size_t codeSpace = 1 << maxCodeLength;

  // We start by making a first guess about the code length, which
  //  is just a round(log2(prob)).
  // At the same time sort the code length errors. This is important to later
  // select which codes
  //  we will make longer and which shorter. It does not have to be a very good
  //  sort, a simple radix sort with 16 buckets is enough.
  uint8_t sortedErrors[16][256];
  size_t errorCounts[16] = {0};

  int symbolCountLog = fixed_log2(symbolCount);
  size_t usedCodeSpace = 0;

  for (size_t i = 0; i < uniqueSymbols; i++) {
    if (symbolHistogram[i] == 0) {
      symbolData[i].bits = maxCodeLength + 1;
      continue;
    }

    //-log(a/b) = log(b) - log(a)
    int optimal = symbolCountLog - fixed_log2(symbolHistogram[i]);
    int bits = (optimal + 128) >> 8;
    int error = optimal - (bits << 8);

    if (bits <= 0) bits = 1;
    if (bits > maxCodeLength) bits = maxCodeLength;

    symbolData[i].bits = bits;
    usedCodeSpace += codeSpace >> bits;

    size_t bucket = (error + 128) >> 4;
    if (bucket >= 16)  // Just in case
      bucket = 15;
    sortedErrors[bucket][errorCounts[bucket]] = i;
    errorCounts[bucket]++;
  }

  // The symbols that got their number of bits most heavily lowered
  //  are in the last bucket.
  size_t bucket = 15;
  size_t index = 0;

  // First pass: increase code lengths until we satisfy kraft
  while (usedCodeSpace > codeSpace) {
    while (index >= errorCounts[bucket]) {
      index = 0;
      bucket = (bucket - 1) % 16;
    }

    size_t symbol = sortedErrors[bucket][index];
    index++;

    if (symbolData[symbol].bits == maxCodeLength) continue;
    symbolData[symbol].bits++;
    usedCodeSpace -= codeSpace >> symbolData[symbol].bits;
  }

  // Second pass: we still have available space, so reduce some lengths
  // Note that the errors are still sorted! But instead of the list
  //  beginning in the first bucket it begins in the middle of another.
  size_t remainingCodeSpace = codeSpace - usedCodeSpace;
  while (remainingCodeSpace) {
    while (index == 0) {
      bucket = (bucket + 1) % 16;
      index = errorCounts[bucket];
    }
    index--;

    size_t symbol = sortedErrors[bucket][index];
    if ((codeSpace >> symbolData[symbol].bits) <= remainingCodeSpace) {
      remainingCodeSpace -= codeSpace >> symbolData[symbol].bits;
      symbolData[symbol].bits--;
    }
  }
}

const size_t STACK_SIZE = 16;
struct PackageMergeNode {
  AllocatorCallback* allocator;
  uint8_t stackBuffer[STACK_SIZE];
  size_t heapSize = 0;
  uint8_t* heapBuffer = nullptr;
  size_t symbolCount = 0;
  size_t histogramSum;

  PackageMergeNode() {}
  ~PackageMergeNode() { allocator->free(heapBuffer); }

  void set_allocator(AllocatorCallback* alloc) { allocator = alloc; }
  void push_back(uint8_t symbol) {
    if (symbolCount >= STACK_SIZE + heapSize) {
      size_t newHeapSize = std::max(heapSize * 2, (size_t)16);
      uint8_t* tmp = (uint8_t*)allocator->allocate(newHeapSize);
      for (size_t i = 0; i < heapSize; i += 16)
        memcpy(tmp + i, heapBuffer + i, 16);
      allocator->free(heapBuffer);
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
      allocator->free(heapBuffer);
      heapSize = other->heapSize;
      heapBuffer = (uint8_t*)allocator->allocate(heapSize);
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
void package_merge(uint32_t* symbolHistogram, HuffmanSymbol* symbolData,
                   size_t symbolCount, const size_t uniqueSymbols,
                   const size_t maxCodeLength, AllocatorCallback* allocator) {
  PackageMergeNode originalList[256];
  // We will need 2 buffers for the nodes generated in each iteration: one for
  // the previous iteration,
  //  and one for the current. We will swap them to avoid copying the current
  //  into the previous.
  PackageMergeNode newNodesBufferA[256];
  PackageMergeNode newNodesBufferB[256];

  for (int i = 0; i < 256; i++) {
    originalList[i].set_allocator(allocator);
    newNodesBufferA[i].set_allocator(allocator);
    newNodesBufferB[i].set_allocator(allocator);
  }

  size_t originalListSize = 0;
  for (int i = 0; i < uniqueSymbols; i++) {
    if (symbolHistogram[i]) {
      originalList[originalListSize].histogramSum = symbolHistogram[i];
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
    fast_huffman_codegen(symbolHistogram, symbolData, symbolCount,
                         uniqueSymbols, maxCodeLength);
    return;
  }

  // We will only need this number of elements at the end
  size_t maxListLength = originalListSize * 2 - 2;
  for (int i = 0; i < maxListLength; i++) {
    for (int j = 0; j < mergedList[i]->size(); j++)
      symbolData[mergedList[i]->at(j)].bits++;
  }
}

void create_huffman_codes(HuffmanSymbol* symbolData, size_t uniqueSymbols,
                          size_t maxHuffmanCodeLength,
                          size_t huffmanCodeSpace) {
  size_t codeLengthCounts[MAX_HUFFMAN_CODE_LENGTH + 2] = {0};
  size_t accumulatedCodeSpace[MAX_HUFFMAN_CODE_LENGTH + 2];
  for (size_t i = 0; i < uniqueSymbols; i++)
    codeLengthCounts[symbolData[i].bits]++;
  size_t accumulator = 0;
  for (size_t i = 1; i <= maxHuffmanCodeLength; i++) {
    accumulatedCodeSpace[i] = accumulator;
    accumulator += (huffmanCodeSpace >> i) * codeLengthCounts[i];
  }
  for (size_t i = 0; i < uniqueSymbols; i++) {
    size_t bits = symbolData[i].bits;
    symbolData[i].code =
        accumulatedCodeSpace[bits] >> (maxHuffmanCodeLength - bits);
    accumulatedCodeSpace[bits] += huffmanCodeSpace >> bits;
  }
}

void symbol_histogram(const uint8_t* data, size_t count,
                      uint32_t* histogramBuckets) {
  const uint8_t* const fastLoopEnd = data + (count & ~0x7);
  const uint8_t* const end = data + count;
  for (; data < fastLoopEnd; data += 8) {
    if (IS_64BIT) {
      uint64_t w;
      memcpy(&w, data, 8);
      histogramBuckets[0 | (w >> 0 & 0xFF)]++;
      histogramBuckets[256 | (w >> 8 & 0xFF)]++;
      histogramBuckets[512 | (w >> 16 & 0xFF)]++;
      histogramBuckets[768 | (w >> 24 & 0xFF)]++;
      histogramBuckets[1024 | (w >> 32 & 0xFF)]++;
      histogramBuckets[1280 | (w >> 40 & 0xFF)]++;
      histogramBuckets[1536 | (w >> 48 & 0xFF)]++;
      histogramBuckets[1792 | (w >> 56 & 0xFF)]++;
    } else {
      uint32_t w;
      memcpy(&w, data, 4);
      histogramBuckets[0 | (w >> 0 & 0xFF)]++;
      histogramBuckets[256 | (w >> 8 & 0xFF)]++;
      histogramBuckets[512 | (w >> 16 & 0xFF)]++;
      histogramBuckets[768 | (w >> 24 & 0xFF)]++;
      memcpy(&w, data + 4, 4);
      histogramBuckets[1024 | (w >> 0 & 0xFF)]++;
      histogramBuckets[1280 | (w >> 8 & 0xFF)]++;
      histogramBuckets[1536 | (w >> 16 & 0xFF)]++;
      histogramBuckets[1792 | (w >> 24 & 0xFF)]++;
    }
  }
  for (; data < end; data++) histogramBuckets[*data]++;
}

void downmix_histogram(uint32_t* histogramBuckets, uint32_t* histogramDownmix) {
#ifdef x64
  for (size_t i = 0; i < 256; i += 4) {
    __m128i a =
        _mm_add_epi32(_mm_loadu_si128((__m128i*)(histogramBuckets + 0 + i)),
                      _mm_loadu_si128((__m128i*)(histogramBuckets + 256 + i)));
    __m128i b =
        _mm_add_epi32(_mm_loadu_si128((__m128i*)(histogramBuckets + 512 + i)),
                      _mm_loadu_si128((__m128i*)(histogramBuckets + 768 + i)));
    __m128i c =
        _mm_add_epi32(_mm_loadu_si128((__m128i*)(histogramBuckets + 1024 + i)),
                      _mm_loadu_si128((__m128i*)(histogramBuckets + 1280 + i)));
    __m128i d =
        _mm_add_epi32(_mm_loadu_si128((__m128i*)(histogramBuckets + 1536 + i)),
                      _mm_loadu_si128((__m128i*)(histogramBuckets + 1792 + i)));
    __m128i counts = _mm_add_epi32(_mm_add_epi32(a, b), _mm_add_epi32(c, d));
    _mm_storeu_si128((__m128i*)&histogramDownmix[i], counts);
  }
#else
  for (size_t i = 0; i < 256; i++)
    histogramDownmix[i] =
        histogramBuckets[0 + i] + histogramBuckets[256 + i] +
        histogramBuckets[512 + i] + histogramBuckets[768 + i] +
        histogramBuckets[1024 + i] + histogramBuckets[1280 + i] +
        histogramBuckets[1536 + i] + histogramBuckets[1792 + i];
#endif
}

float calculate_entropy(uint32_t histogram[256], size_t symbolCount) {
  const float probDiv = 1 / float(symbolCount);
#ifdef x64
  union {
    __m128 entropySSE = _mm_set1_ps(0.0f);
    float entropyScalar[4];
  };
  const __m128 probDivSSE = _mm_set1_ps(probDiv);

  for (size_t i = 0; i < 256; i += 4) {
    __m128i counts = _mm_loadu_si128((__m128i*)&histogram[i]);
    __m128 countsFloat = _mm_cvtepi32_ps(counts);
    __m128 probs = _mm_mul_ps(countsFloat, probDivSSE);
    // Fast log2 algorithm, see the fast_log2() function in this file
    __m128i cast = _mm_castps_si128(probs);
    __m128 logs = _mm_cvtepi32_ps(cast);
    // The multiplication and substraction are common factors
    entropySSE = _mm_sub_ps(entropySSE, _mm_mul_ps(logs, probs));
  }

  // Join all lanes, add common factors of the log2 approximation
  return (entropyScalar[0] + entropyScalar[1] + entropyScalar[2] +
          entropyScalar[3]) *
             1.1920928955078125e-7f +
         126.94269504f;
#else
  float entropy = 0;
  for (size_t i = 0; i < 256; i++) {
    // fast_log2 wont return -inf with an input of 0. This means we dont need an
    // if (count). This does not necessarily make the loop faster, but allows
    // easier vectorization.
    const float prob = histogram[i] * probDiv;
    entropy -= prob * fast_log2(prob);
  }
  return entropy;
#endif
}

struct PrecodeStreamSymbol {
  uint8_t codeLength;
  uint8_t runBit;
};
struct HuffmanHeaderData {
  size_t codeLengthCount;  // Number of code lengths to encode, after run length
  HuffmanSymbol precodeSymbols[MAX_HUFFMAN_CODE_LENGTH + 2];
  uint32_t precodeHistogram[MAX_HUFFMAN_CODE_LENGTH + 2];
  size_t precodeSymbolCount;
  PrecodeStreamSymbol precodeStream[256];
  size_t extraRawBits;
};

HuffmanHeaderData generate_huffman_header_data(
    HuffmanSymbol huffmanSymbols[256], bool useFastCodeGen,
    AllocatorCallback* allocator) {
  HuffmanHeaderData data;

  data.codeLengthCount = 256;
  // We dont have to encode the last trailing code lengths if they have a value
  // of 12
  for (; huffmanSymbols[data.codeLengthCount - 1].bits ==
         MAX_HUFFMAN_CODE_LENGTH + 1;
       data.codeLengthCount--) {
  }
  if (data.codeLengthCount == 256)
    data.codeLengthCount =
        255;  // Dont code last length, as it will be inferred by the decoder
  memset(data.precodeHistogram, 0,
         (MAX_HUFFMAN_CODE_LENGTH + 2) * sizeof(uint32_t));

  int runLen = 1;
  int prevLen = huffmanSymbols[0].bits;
  data.precodeSymbolCount = 0;
  data.extraRawBits = 0;
  for (size_t i = 1; i <= data.codeLengthCount; i++) {
    if (huffmanSymbols[i].bits == prevLen && i != data.codeLengthCount) {
      runLen++;
      continue;
    }

    data.precodeHistogram[prevLen]++;
    data.precodeStream[data.precodeSymbolCount].codeLength = prevLen;
    data.precodeSymbolCount++;

    size_t bits = unsafe_int_log2(runLen);
    // Add multiple of the same symbol for run lengths
    for (size_t j = 0; j < bits; j++) {
      data.precodeHistogram[prevLen]++;
      data.precodeStream[data.precodeSymbolCount].codeLength = prevLen;
      data.precodeStream[data.precodeSymbolCount].runBit = (runLen >> j) & 1;
      data.precodeSymbolCount++;
      data.extraRawBits++;
    }

    runLen = 1;
    prevLen = huffmanSymbols[i].bits;
  }

  // Avoid one code length getting a probability of 1
  for (size_t i = 1; i <= MAX_HUFFMAN_CODE_LENGTH + 1; i++) {
    if (data.precodeHistogram[i] == data.precodeSymbolCount) {
      data.precodeHistogram[i]--;
      data.precodeHistogram[1 + i % 12]++;
    }
  }

  if (useFastCodeGen)
    fast_huffman_codegen(data.precodeHistogram + 1, data.precodeSymbols + 1,
                         data.precodeSymbolCount, MAX_HUFFMAN_CODE_LENGTH + 1,
                         MAX_PRECODE_CODE_LENGTH);
  else
    package_merge(data.precodeHistogram + 1, data.precodeSymbols + 1,
                  data.precodeSymbolCount, MAX_HUFFMAN_CODE_LENGTH + 1,
                  MAX_PRECODE_CODE_LENGTH, allocator);

  return data;
}

FORCE_INLINE void normalize_huffman_header(uint8_t*& outputIt, size_t* state,
                                           size_t* bitCount) {
  write_uint32le(outputIt - 4, *state << (32 - *bitCount));
  outputIt -= *bitCount / 8;
  *bitCount &= 7;
}

size_t encode_huffman_header(HuffmanSymbol huffmanSymbols[256],
                             size_t streamSizes[6], uint8_t* output,
                             const HuffmanHeaderData& headerData) {
  uint8_t* outputIt = output + 128;
  size_t state = 0;
  size_t bitCount = 0;

  int virtualFirstSize = streamSizes[0] + 255;
  int firstSizeLog = int_log2(virtualFirstSize);
  state = firstSizeLog - 8;
  bitCount = 3;
  state =
      (state << firstSizeLog) | (virtualFirstSize & (1 << firstSizeLog) - 1);
  bitCount += firstSizeLog;

  int sizeErrors[6];
  int maxError = 0;
  for (int i = 1; i < 6; i++) {
    int16_t error = streamSizes[i] - streamSizes[0];
    // Fold negative and positive numbers into the positive line
    // https://cbloomrants.blogspot.com/2014/03/03-14-14-fold-up-negatives.html
    sizeErrors[i] = (error << 1) ^ (error >> 15);
    if (sizeErrors[i] > maxError) maxError = sizeErrors[i];
  }

  int maxErrorLog = int_log2(maxError) + 1;
  state = (state << 4) | (maxErrorLog - 1);
  bitCount += 4;

  normalize_huffman_header(outputIt, &state, &bitCount);

  for (int i = 1; i < 6; i++) {
    state = (state << maxErrorLog) | sizeErrors[i];
    bitCount += maxErrorLog;
    normalize_huffman_header(outputIt, &state, &bitCount);
  }

  // Output the used distribution
  size_t hskip = 0;
  for (; hskip < 5 && headerData.precodeSymbols[hskip + 1].bits ==
                          MAX_PRECODE_CODE_LENGTH + 1;
       hskip++) {
  }
  state = (state << 3) | hskip;
  bitCount += 3;

  size_t usedCodeSpace = 0;
  for (size_t i = hskip + 1;
       i <= MAX_HUFFMAN_CODE_LENGTH && usedCodeSpace != PRECODE_CODE_SPACE;
       i++) {
    state = (state << 3) | (headerData.precodeSymbols[i].bits - 1);
    bitCount += 3;
    usedCodeSpace += PRECODE_CODE_SPACE >> headerData.precodeSymbols[i].bits;
    normalize_huffman_header(outputIt, &state, &bitCount);
  }

  size_t lastSymbol = 0;
  for (size_t i = 0; i < headerData.precodeSymbolCount; i++) {
    size_t symbol = headerData.precodeStream[i].codeLength;
    state = (state << headerData.precodeSymbols[symbol].bits) |
            headerData.precodeSymbols[symbol].code;
    bitCount += headerData.precodeSymbols[symbol].bits;

    if (symbol == lastSymbol) {
      state = (state << 1) | headerData.precodeStream[i].runBit;
      bitCount++;
    }
    lastSymbol = symbol;

    normalize_huffman_header(outputIt, &state, &bitCount);
  }

  // Flush
  normalize_huffman_header(outputIt, &state, &bitCount);
  if (bitCount > 0) {
    outputIt--;
    *outputIt = state << (8 - bitCount);
  }

  return output + 128 - outputIt;
}

void write_header(uint8_t*& output, size_t blockSize, int type, int flags) {
  uint32_t data = (type) | (flags << 2) | (blockSize << 6);
  output[0] = (data >> 0) & 0xFF;
  output[1] = (data >> 8) & 0xFF;
  output[2] = (data >> 16) & 0xFF;
  output += 3;
}

// For optimal parse. Generates huffman codes and returns encoded size from a
// pregenerated histogram
size_t get_encoded_huffman_info(uint32_t symbolHistogram[256],
                                size_t symbolCount, HuffmanSymbol symbols[256],
                                float decSpeedBias, int streamType) {
  for (size_t i = 0; i < 256; i++) {
    // RLE coding
    if (symbolHistogram[i] == symbolCount) {
      for (size_t j = 0; j < 256; j++)
        symbols[j].bits = MAX_HUFFMAN_CODE_LENGTH + 1;
      symbols[i].bits = 0;
      return symbolCount + 3;  // Stream header also takes space!
    }
    // No RLE possible
    if (symbolHistogram[i] != 0) break;
  }

  // Not enough symbols to justify compression or disabled huffman coding
  if (symbolCount < 32 || decSpeedBias >= 0.99f) {
    for (size_t i = 0; i < 256; i++) symbols[i].bits = 8;
    return symbolCount + 3;  // Stream header also takes space!
  }
  float maxBitsPerByte = 8.0f - 4.0f * decSpeedBias;

  fast_huffman_codegen(symbolHistogram, symbols, symbolCount, 256,
                       MAX_HUFFMAN_CODE_LENGTH);
  size_t compressedSize = 0;
  for (size_t i = 0; i < 256; i++)
    compressedSize += symbols[i].bits * symbolHistogram[i];

  // We also need the header size to know if huffman is worth, mostly for small
  // streams
  HuffmanHeaderData headerData =
      generate_huffman_header_data(symbols, true, nullptr);

  size_t headerSize =
      48 + (int_log2(symbolCount) - 4) * 5;  // Jump table plus precode tree
  for (size_t i = 1; i <= MAX_HUFFMAN_CODE_LENGTH + 1; i++)
    headerSize +=
        headerData.precodeSymbols[i].bits * headerData.precodeHistogram[i];
  headerSize += headerData.extraRawBits;
  compressedSize = compressedSize / 8 + headerSize / 8 +
                   8;  // Convert to bytes and add some additional overhead and
                       // inefficiencies

  // Not compressible
  if (compressedSize >= symbolCount * maxBitsPerByte / 8.0f) {
    for (size_t i = 0; i < 256; i++) symbols[i].bits = 8;
    return symbolCount + 3;  // Stream header also takes space!
  }

  return compressedSize;
}

class EntropyEncoder {
  enum {
    ENTROPY_MODE_UNKNOWN = 0,
    ENTROPY_MODE_PARTIAL = 1,
    ENTROPY_MODE_RLE = 2,
    ENTROPY_MODE_HUFFMAN = 3,
    ENTROPY_MODE_UNCOMPRESSED = 4,
  };

  AllocatorCallback* allocator;
  // store huffman symbols
  uint8_t* symbolBuffer = nullptr;
  uint8_t* symbolBufferIt;
  HuffmanSymbol symbolData[256];
  bool disableSymbolBuffer;
  // Store symbols in output, which will later save a memcpy if no huffman is
  // used
  bool directOutputWrite = false;
  int streamType;
  // Internally used to track state of histogram calculation
  uint32_t* histogramBuckets = nullptr;
  uint32_t histogramCombined[256];
  size_t readHistogramSymbols;
  int entropyMode;

 public:
  EntropyEncoder() {}
  ~EntropyEncoder() {
    if (!disableSymbolBuffer) allocator->free(symbolBuffer);
    allocator->free(histogramBuckets);
  }

  // Initializes the internal buffer for symbols. If bufferSize is set
  //  to 0 it does not allocate memory and assumes an external buffer
  //  will be given when starting a block.
  // Returns true if it fails to initialize
  bool initialize_encoder(const size_t bufferSize, int type,
                          AllocatorCallback* alloc) {
    allocator = alloc;
    try {
      disableSymbolBuffer = bufferSize == 0;
      if (bufferSize) {
        symbolBuffer = (uint8_t*)allocator->allocate(bufferSize);
        symbolBufferIt = symbolBuffer;
      }
      histogramBuckets =
          (uint32_t*)allocator->allocate(sizeof(uint32_t) * 2048);
    } catch (const std::bad_alloc& e) {
      allocator->free(symbolBuffer);
      allocator->free(histogramBuckets);
      return true;
    }
    streamType = type;
    return false;
  }

  void start_block(uint8_t* externalBuffer = nullptr,
                   bool useDirectOutputWrite = false) {
    if (externalBuffer) {
      symbolBufferIt = externalBuffer;
      symbolBuffer = externalBuffer;
    } else
      symbolBufferIt = symbolBuffer;
    directOutputWrite = useDirectOutputWrite;
    entropyMode = ENTROPY_MODE_UNKNOWN;
  }

  // Up to 4 bytes
  FORCE_INLINE void add_variable_length(const uint32_t value, size_t bytes) {
    write_uint32le(symbolBufferIt, value);
    symbolBufferIt += bytes;
  }
  // Up to 8 elements
  FORCE_INLINE void add_literals_short(const uint8_t* literals, size_t count) {
    memcpy(symbolBufferIt, literals, 8);
    symbolBufferIt += count;
  }
  // Up to 6 elements
  FORCE_INLINE void add_delta_literals_short(const uint8_t* literals,
                                             size_t count, size_t distance) {
#ifdef x64
    __m128i literalsReg = _mm_loadu_si128((__m128i*)literals);
    __m128i predictionReg = _mm_loadu_si128((__m128i*)(literals - distance));
    _mm_storeu_si128((__m128i*)symbolBufferIt,
                     _mm_sub_epi8(literalsReg, predictionReg));
#else
    const uint8_t* prediction = literals - distance;
    symbolBufferIt[0] = literals[0] - prediction[0];
    symbolBufferIt[1] = literals[1] - prediction[1];
    symbolBufferIt[2] = literals[2] - prediction[2];
    symbolBufferIt[3] = literals[3] - prediction[3];
    symbolBufferIt[4] = literals[4] - prediction[4];
    symbolBufferIt[5] = literals[5] - prediction[5];
#endif
    symbolBufferIt += count;
  }
  FORCE_INLINE void add_byte(const uint8_t literal) {
    *symbolBufferIt++ = literal;
  }
  // Returns the current buffer pointer and internally advanced it by count
  // Useful for writing more specific code (and possibly faster) to add data to
  // the stream
  FORCE_INLINE uint8_t* add_count_get_ptr(size_t count) {
    uint8_t* ptr = symbolBufferIt;
    symbolBufferIt += count;
    return ptr;
  }
  size_t get_symbol_count() { return symbolBufferIt - symbolBuffer; }

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

  // Okay, this API might be a bit difficult to explain but I'll try.
  // Because skanda has two different ways to send literals, raw and delta, we
  //  want to know which of the 2 produces the smallest result. To make things
  //  fast we want to test only a portion of the stream for size AND reuse the
  //  obtained histogram later for huffman.
  //
  // So what this does: get_entropy_mode_quick() computes the entropy of a small
  //  chunk, based on that considers whether to use huffman, rle or send
  //  uncompressed and returns the estimated stream size based on that entropy.
  //  With that size we consider using delta literals or not. To then complete
  //  the histogram get_entropy_mode_full() is called. Each call to these
  //  functions sets internal flags, so that when ending the stream with
  //  end_block() it knows what work is already done.
  size_t get_entropy_mode_quick(float decSpeedBias,
                                bool doCompressibilityCheck) {
    const size_t symbolCount = get_symbol_count();
    // Check RLE coding
    if (symbolCount >= 2) {
      size_t len = test_match<false>(symbolBuffer + 1, symbolBuffer,
                                     symbolBuffer + symbolCount, 0, 0);
      if (len == symbolCount - 1) {
        entropyMode = ENTROPY_MODE_RLE;
        return 0;
      }
    }

    // Not enough symbols to justify compression or disabled huffman coding
    if (symbolCount < (doCompressibilityCheck ? 256 : 32) ||
        decSpeedBias >= 0.99f) {
      entropyMode = ENTROPY_MODE_UNCOMPRESSED;
      return symbolCount;
    }
    // Reduce maxBitsPerByte a bit to account for the fact huffman usually does
    // not achieve optimal entropy
    float maxBitsPerByte = 8.0f - 4.0f * decSpeedBias - 0.05f;
    float entropy = 0;

    memset(histogramBuckets, 0, 2048 * sizeof(uint32_t));
    // Make histograms and test if stream is compressible enough
    readHistogramSymbols = 0;
    if (doCompressibilityCheck) {
      if (symbolCount <= 8192 + 128) {
        readHistogramSymbols = symbolCount;
        symbol_histogram(symbolBuffer, readHistogramSymbols, histogramBuckets);
      } else {
        readHistogramSymbols = 8192;
        // This is similar to the simpson rule
        symbol_histogram(symbolBuffer, readHistogramSymbols / 4,
                         histogramBuckets);
        symbol_histogram(
            symbolBuffer + symbolCount / 2 - readHistogramSymbols / 4,
            readHistogramSymbols / 2, histogramBuckets);
        symbol_histogram(symbolBuffer + symbolCount - readHistogramSymbols / 4,
                         readHistogramSymbols / 4, histogramBuckets);
      }

      downmix_histogram(histogramBuckets, histogramCombined);
      entropy = calculate_entropy(histogramCombined, readHistogramSymbols);

      size_t headerBias = 1024 * readHistogramSymbols / symbolCount;
      if (readHistogramSymbols * entropy + headerBias >
          readHistogramSymbols * maxBitsPerByte) {
        entropyMode = ENTROPY_MODE_UNCOMPRESSED;
        return symbolCount;
      }
    }

    entropyMode = ENTROPY_MODE_PARTIAL;
    // This might happen because of the approximation used
    if (entropy < 0) return 0;
    return symbolCount * entropy / 8;
  }

  void get_entropy_mode_full(float decSpeedBias, bool doCompressibilityCheck) {
    const size_t symbolCount = get_symbol_count();
    // Histogram is done and get_entropy_mode_partial() did not set
    // ENTROPY_STATUS_UNCOMPRESSED,
    //  so this can be compressed.
    if (readHistogramSymbols == symbolCount) {
      entropyMode = ENTROPY_MODE_HUFFMAN;
      return;
    }

    float maxBitsPerByte = 8.0f - 4.0f * decSpeedBias - 0.05f;
    symbol_histogram(symbolBuffer + readHistogramSymbols / 4,
                     symbolCount / 2 - readHistogramSymbols / 2,
                     histogramBuckets);
    symbol_histogram(symbolBuffer + symbolCount / 2 + readHistogramSymbols / 4,
                     (symbolCount - symbolCount / 2) - readHistogramSymbols / 2,
                     histogramBuckets);

    downmix_histogram(histogramBuckets, histogramCombined);
    float entropy = calculate_entropy(histogramCombined, symbolCount);
    if (doCompressibilityCheck &&
        symbolCount * entropy + 128 * 8 > symbolCount * maxBitsPerByte) {
      entropyMode = ENTROPY_MODE_UNCOMPRESSED;
      return;
    }

    entropyMode = ENTROPY_MODE_HUFFMAN;
  }

  HuffmanSymbol* get_huffman_codes(float decSpeedBias, size_t* size) {
    const size_t symbolCount = get_symbol_count();
    memset(histogramBuckets, 0, 2048 * sizeof(uint32_t));
    symbol_histogram(symbolBuffer, symbolCount, histogramBuckets);
    downmix_histogram(histogramBuckets, histogramCombined);

    *size = get_encoded_huffman_info(histogramCombined, symbolCount, symbolData,
                                     decSpeedBias, streamType);
    return symbolData;
  }

  size_t store_data_raw(uint8_t* output, int flags) {
    size_t symbolCount = get_symbol_count();
    write_header(output, symbolCount, ENTROPY_RAW, flags);
    // If directOutputWrite == true symbols are stored in output
    // This means we just write the header
    if (!directOutputWrite) {
      // We might have stored the symbols in the output file buffer,
      //  so these 2 pointers might overlap. Use memmove instead of memcpy
      memmove(output, symbolBuffer, symbolCount);
    }
    return symbolCount + 3;
  }

  // The pointer to stream buffer is to the end of it! The huffman stream is
  // written backwards.
  size_t end_block(uint8_t* output, uint8_t* streamBuffer, int flags,
                   float decSpeedBias, bool useFastCodeGen,
                   bool doCompressibilityCheck) {
    const size_t symbolCount = get_symbol_count();
    if (entropyMode == ENTROPY_MODE_UNKNOWN)
      get_entropy_mode_quick(decSpeedBias, doCompressibilityCheck);
    if (entropyMode == ENTROPY_MODE_PARTIAL)
      get_entropy_mode_full(decSpeedBias, doCompressibilityCheck);

    if (entropyMode == ENTROPY_MODE_RLE) {
      write_header(output, symbolCount, ENTROPY_RLE, flags);
      *output = symbolBuffer[0];
      return 4;
    }

    if (entropyMode == ENTROPY_MODE_UNCOMPRESSED || !streamBuffer)
      return store_data_raw(output, flags);

    if (useFastCodeGen)
      fast_huffman_codegen(histogramCombined, symbolData, symbolCount, 256,
                           MAX_HUFFMAN_CODE_LENGTH);
    else
      package_merge(histogramCombined, symbolData, symbolCount, 256,
                    MAX_HUFFMAN_CODE_LENGTH, allocator);
    // Convert bit lengths to codes
    create_huffman_codes(symbolData, 256, MAX_HUFFMAN_CODE_LENGTH,
                         HUFFMAN_CODE_SPACE);

    size_t streamSizes[6];
    size_t cumulativeStreamSize = 0;
    for (int i = 5; i >= 0; i--) {
      size_t streamSize = encode_stream(symbolData, symbolBuffer + i,
                                        symbolBuffer + symbolCount,
                                        streamBuffer - cumulativeStreamSize);
      streamSizes[i] = streamSize;
      cumulativeStreamSize += streamSize;
    }

    // Store the tree
    uint8_t huffmanHeader[128];
    size_t headerSize;
    {
      HuffmanHeaderData headerData =
          generate_huffman_header_data(symbolData, useFastCodeGen, allocator);
      create_huffman_codes(headerData.precodeSymbols + 1,
                           MAX_HUFFMAN_CODE_LENGTH + 1, MAX_PRECODE_CODE_LENGTH,
                           PRECODE_CODE_SPACE);
      headerSize = encode_huffman_header(symbolData, streamSizes, huffmanHeader,
                                         headerData);
    }

    // Last size check, to make sure huffman DOES help
    if (headerSize + cumulativeStreamSize + 1 >= symbolCount)
      return store_data_raw(output, flags);

    write_header(output, symbolCount, ENTROPY_HUFFMAN, flags);
    *output++ = headerSize;
    memcpy(output, huffmanHeader + 128 - headerSize, headerSize);
    output += headerSize;
    memcpy(output, streamBuffer - cumulativeStreamSize, cumulativeStreamSize);

    return headerSize + cumulativeStreamSize + 4;
  }
};

class BitEncoder {
  AllocatorCallback* allocator;
  uint8_t* rawCounts = nullptr;
  uint32_t* rawData = nullptr;
  size_t numberData = 0;

 public:
  BitEncoder() {}
  ~BitEncoder() {
    allocator->free(rawCounts);
    allocator->free(rawData);
  }

  bool init(size_t bufferSize, AllocatorCallback* alloc) {
    try {
      allocator = alloc;
      if (bufferSize) {
        rawCounts = (uint8_t*)allocator->allocate(bufferSize);
        rawData = (uint32_t*)allocator->allocate(sizeof(uint32_t) * bufferSize);
      }
      return 0;
    } catch (std::bad_alloc& e) {
      allocator->free(rawCounts);
      allocator->free(rawData);
      return -1;
    }
  }
  void start_block() { numberData = 0; }
  void encode_raw_bits(size_t bits, size_t count) {
    rawCounts[numberData] = count;
    rawData[numberData] = bits;
    numberData++;
  }

  void renormalize(size_t* state, size_t* stateBitCount, uint8_t*& outputIt) {
    if (IS_64BIT)
      write_uint64le(outputIt, *state);
    else
      write_uint32le(outputIt, *state);
    size_t bytes = *stateBitCount / 8;
    outputIt += bytes;
    *state >>= bytes * 8;
    *stateBitCount &= 7;
  }
  // The stream buffer pointer has to be to the end of it as it writes backwards
  size_t end_block(uint8_t* output) {
    size_t state = 0;
    size_t stateBitCount = 0;
    uint8_t* outputIt = output;

#if IS_64BIT
    size_t index = 0;
    // We can have up to 28 raw bits per distance if we limit ourselves to ~4gb
    // distances. On 64 bit we can encode up to 2 distances per refill
    size_t fastLoopEnd = numberData & ~1;

    for (; index < fastLoopEnd; index += 2) {
      state |= (size_t)rawData[index + 0] << stateBitCount;
      stateBitCount += rawCounts[index + 0];
      state |= (size_t)rawData[index + 1] << stateBitCount;
      stateBitCount += rawCounts[index + 1];
      // Renormalize
      renormalize(&state, &stateBitCount, outputIt);
    }
    if (index < numberData) {
      state |= (size_t)rawData[index] << stateBitCount;
      stateBitCount += rawCounts[index];
      renormalize(&state, &stateBitCount, outputIt);
    }
#else
    // For 32 bit big distances need to be encoded in 2 steps
    for (size_t index = 0; index < numberData; index++) {
      if (rawCounts[index] <= 24) {
        state |= rawData[index] << stateBitCount;
        stateBitCount += rawCounts[index];
      } else {
        state |= rawData[index] << stateBitCount;
        stateBitCount += 24;
        renormalize(&state, &stateBitCount, outputIt);
        state |= (rawData[index] >> 24) << stateBitCount;
        stateBitCount += rawCounts[index] - 24;
      }
      // Renormalize
      renormalize(&state, &stateBitCount, outputIt);
    }
#endif
    // Flush remaining data
    if (stateBitCount > 0) *outputIt++ = state;
    return outputIt - output;
  }
};

FORCE_INLINE void encode_length(EntropyEncoder* lengthEncoder, size_t var,
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

struct CodingOptions {
  unsigned int disableHuffman : 1;
  unsigned int advancedDistanceCoding : 1;
  unsigned int advancedLiteralCoding : 1;
  unsigned int posBits : 3;

  CodingOptions(float decSpeedBias) {
    disableHuffman = decSpeedBias >= 0.99f;
    advancedDistanceCoding = decSpeedBias < 0.1f;
    advancedLiteralCoding = decSpeedBias < 0.6f;
    posBits = 0;
  }
};

void encode_literal_run_long(
    EntropyEncoder* literalEncoder, EntropyEncoder* lengthEncoder,
    const uint8_t* const literalRunStart, const size_t literalRunLength,
    uint8_t* const controlByte, const uint8_t* const inputStart,
    const size_t lastDistance, CodingOptions codingOptions) {
  *controlByte = 7 << 5;
  encode_length(lengthEncoder, literalRunLength, 7);

  if (codingOptions.posBits == 0) {
    uint8_t* rawLiteralsPtr =
        literalEncoder[0].add_count_get_ptr(literalRunLength);
    uint8_t* deltaLiteralsPtr =
        literalEncoder[4].add_count_get_ptr(literalRunLength);
    const uint8_t* src = literalRunStart;
    const uint8_t* const end = src + literalRunLength;
#ifdef x64
    const uint8_t* prediction = src - lastDistance;
    do {
      __m128i literals0 = _mm_loadu_si128((__m128i*)(src + 0));
      __m128i literals1 = _mm_loadu_si128((__m128i*)(src + 16));
      _mm_storeu_si128((__m128i*)(rawLiteralsPtr + 0), literals0);
      _mm_storeu_si128((__m128i*)(rawLiteralsPtr + 16), literals1);
      rawLiteralsPtr += 32;
      src += 32;

      if (codingOptions.advancedLiteralCoding) {
        _mm_storeu_si128(
            (__m128i*)(deltaLiteralsPtr + 0),
            _mm_sub_epi8(literals0,
                         _mm_loadu_si128((__m128i*)(prediction + 0))));
        _mm_storeu_si128(
            (__m128i*)(deltaLiteralsPtr + 16),
            _mm_sub_epi8(literals1,
                         _mm_loadu_si128((__m128i*)(prediction + 16))));
        deltaLiteralsPtr += 32;
        prediction += 32;
      }

    } while (src < end);
#else
    do {
      memcpy(rawLiteralsPtr + 0, src + 0, 16);
      memcpy(rawLiteralsPtr + 16, src + 16, 16);
      rawLiteralsPtr += 32;
      src += 32;
    } while (src < end);

    if (codingOptions.advancedLiteralCoding) {
      src = literalRunStart;
      const uint8_t* prediction = src - lastDistance;
      do {
        deltaLiteralsPtr[0] = src[0] - prediction[0];
        deltaLiteralsPtr[1] = src[1] - prediction[1];
        deltaLiteralsPtr[2] = src[2] - prediction[2];
        deltaLiteralsPtr[3] = src[3] - prediction[3];
        deltaLiteralsPtr[4] = src[4] - prediction[4];
        deltaLiteralsPtr[5] = src[5] - prediction[5];
        deltaLiteralsPtr[6] = src[6] - prediction[6];
        deltaLiteralsPtr[7] = src[7] - prediction[7];
        src += 8;
        deltaLiteralsPtr += 8;
        prediction += 8;
      } while (src < end);
    }
#endif
  } else {
    size_t pos = literalRunStart - inputStart;
    uint8_t* stream0 = literalEncoder[(pos + 0) & 3].add_count_get_ptr(
        (literalRunLength + 3) / 4);
    uint8_t* deltaStream0 =
        literalEncoder[4 + ((pos + 0) & 3)].add_count_get_ptr(
            (literalRunLength + 3) / 4);
    uint8_t* stream1 = literalEncoder[(pos + 1) & 3].add_count_get_ptr(
        (literalRunLength + 2) / 4);
    uint8_t* deltaStream1 =
        literalEncoder[4 + ((pos + 1) & 3)].add_count_get_ptr(
            (literalRunLength + 2) / 4);
    uint8_t* stream2 = literalEncoder[(pos + 2) & 3].add_count_get_ptr(
        (literalRunLength + 1) / 4);
    uint8_t* deltaStream2 =
        literalEncoder[4 + ((pos + 2) & 3)].add_count_get_ptr(
            (literalRunLength + 1) / 4);
    uint8_t* stream3 = literalEncoder[(pos + 3) & 3].add_count_get_ptr(
        (literalRunLength + 0) / 4);
    uint8_t* deltaStream3 =
        literalEncoder[4 + ((pos + 3) & 3)].add_count_get_ptr(
            (literalRunLength + 0) / 4);

    const uint8_t* src = literalRunStart;
    const uint8_t* const end = literalRunStart + literalRunLength;
    const uint8_t* prediction = literalRunStart - lastDistance;

#ifdef COMPILE_SSE41
    if (HAS_SSE41) {
      const __m128i shuffleMask =
          _mm_set_epi8(15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);

      do {
        __m128i literals = _mm_loadu_si128((__m128i*)src);
        __m128i delta =
            _mm_sub_epi8(literals, _mm_loadu_si128((__m128i*)prediction));
        __m128i literalShuffle = _mm_shuffle_epi8(literals, shuffleMask);
        __m128i deltaShuffle = _mm_shuffle_epi8(delta, shuffleMask);
        write_uint32le(stream0, _mm_extract_epi32(literalShuffle, 0));
        write_uint32le(stream1, _mm_extract_epi32(literalShuffle, 1));
        write_uint32le(stream2, _mm_extract_epi32(literalShuffle, 2));
        write_uint32le(stream3, _mm_extract_epi32(literalShuffle, 3));
        write_uint32le(deltaStream0, _mm_extract_epi32(deltaShuffle, 0));
        write_uint32le(deltaStream1, _mm_extract_epi32(deltaShuffle, 1));
        write_uint32le(deltaStream2, _mm_extract_epi32(deltaShuffle, 2));
        write_uint32le(deltaStream3, _mm_extract_epi32(deltaShuffle, 3));
        stream0 += 4;
        stream1 += 4;
        stream2 += 4;
        stream3 += 4;
        deltaStream0 += 4;
        deltaStream1 += 4;
        deltaStream2 += 4;
        deltaStream3 += 4;
        src += 16;
        prediction += 16;
      } while (src < end);
    } else
#endif
    {
      do {
        stream0[0] = src[0];
        deltaStream0[0] = src[0] - prediction[0];
        stream1[0] = src[1];
        deltaStream1[0] = src[1] - prediction[1];
        stream2[0] = src[2];
        deltaStream2[0] = src[2] - prediction[2];
        stream3[0] = src[3];
        deltaStream3[0] = src[3] - prediction[3];
        stream0[1] = src[4];
        deltaStream0[1] = src[4] - prediction[4];
        stream1[1] = src[5];
        deltaStream1[1] = src[5] - prediction[5];
        stream2[1] = src[6];
        deltaStream2[1] = src[6] - prediction[6];
        stream3[1] = src[7];
        deltaStream3[1] = src[7] - prediction[7];
        stream0 += 2;
        stream1 += 2;
        stream2 += 2;
        stream3 += 2;
        deltaStream0 += 2;
        deltaStream1 += 2;
        deltaStream2 += 2;
        deltaStream3 += 2;
        src += 8;
        prediction += 8;
      } while (src < end);
    }
  }
}

FORCE_INLINE void encode_literal_run_short(EntropyEncoder* literalEncoder,
                                           const uint8_t* const literalRunStart,
                                           const size_t literalRunLength,
                                           uint8_t* const controlByte,
                                           const uint8_t* const inputStart,
                                           const size_t lastDistance,
                                           CodingOptions codingOptions) {
  *controlByte = (literalRunLength << 5);
  if (codingOptions.posBits == 0) {
    // It is faster to unconditionally copy 8 bytes
    literalEncoder[0].add_literals_short(literalRunStart, literalRunLength);
    if (codingOptions.advancedLiteralCoding)
      literalEncoder[4].add_delta_literals_short(
          literalRunStart, literalRunLength, lastDistance);
  } else {
    const uint8_t* src = literalRunStart;
    const uint8_t* const end = literalRunStart + literalRunLength;
    const uint8_t* prediction = literalRunStart - lastDistance;
    size_t pos = (literalRunStart - inputStart) & 3;
    for (; src < end;) {
      literalEncoder[pos].add_byte(*src);
      literalEncoder[4 + pos].add_byte(*src - *prediction);
      prediction++;
      pos = (pos + 1) & 3;
      src++;
    }
  }
}

FORCE_INLINE void encode_literal_run(
    EntropyEncoder* literalEncoder, EntropyEncoder* lengthEncoder,
    const uint8_t* const input, const uint8_t* const literalRunStart,
    uint8_t* const controlByte, const uint8_t* const inputStart,
    const size_t lastDistance, CodingOptions codingOptions) {
  const size_t literalRunLength = input - literalRunStart;
  if (literalRunLength >= 7)
    encode_literal_run_long(literalEncoder, lengthEncoder, literalRunStart,
                            literalRunLength, controlByte, inputStart,
                            lastDistance, codingOptions);
  else
    encode_literal_run_short(literalEncoder, literalRunStart, literalRunLength,
                             controlByte, inputStart, lastDistance,
                             codingOptions);
}

FORCE_INLINE void encode_match_length(EntropyEncoder* tokenEncoder,
                                      EntropyEncoder* lengthEncoder,
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
  else if (disableHuffman && *matchLength <= 16) {
    size_t bias = (*matchLength - 10) >> (sizeof(size_t) * 8 - 1);
    tokenEncoder->add_byte(*controlByte | 6 - bias);
    tokenEncoder->add_byte(*matchLength - 10 + bias);
  } else {
    tokenEncoder->add_byte(*controlByte | 7);
    encode_length(lengthEncoder, *matchLength, 9);
  }
}

// Only encodes matches with last rep offset. It is equivalent for both modes of
// distance coding
FORCE_INLINE void encode_first_rep_match(EntropyEncoder* tokenEncoder,
                                         EntropyEncoder* lengthEncoder,
                                         uint8_t* const controlByte,
                                         size_t* matchLength,
                                         CodingOptions codingOptions) {
  encode_match_length(tokenEncoder, lengthEncoder, controlByte, matchLength,
                      codingOptions.disableHuffman);
}
FORCE_INLINE void encode_second_rep_match(EntropyEncoder* tokenEncoder,
                                          EntropyEncoder* lengthEncoder,
                                          uint8_t* const controlByte,
                                          size_t* matchLength,
                                          size_t* repOffsets) {
  *controlByte |= 1 << 3;
  std::swap(repOffsets[0], repOffsets[1]);
  encode_match_length(tokenEncoder, lengthEncoder, controlByte, matchLength,
                      false);
}
FORCE_INLINE void encode_third_rep_match(EntropyEncoder* tokenEncoder,
                                         EntropyEncoder* lengthEncoder,
                                         uint8_t* const controlByte,
                                         size_t* matchLength,
                                         size_t* repOffsets) {
  *controlByte |= 2 << 3;
  size_t distance = repOffsets[2];
  repOffsets[2] = repOffsets[1];
  repOffsets[1] = repOffsets[0];
  repOffsets[0] = distance;
  encode_match_length(tokenEncoder, lengthEncoder, controlByte, matchLength,
                      false);
}

FORCE_INLINE void encode_distance(EntropyEncoder* distanceEncoder,
                                  BitEncoder* bitDistanceEncoder,
                                  uint8_t* const controlByte, size_t distance,
                                  size_t* repOffsets,
                                  bool advancedDistanceCoding) {
  if (!advancedDistanceCoding) {
    size_t bytes = unsafe_int_log2(distance) / 8 + 1;
    bytes = distance == repOffsets[0] ? 0 : bytes;
    repOffsets[0] = distance;
    distanceEncoder->add_variable_length(distance, bytes);
    *controlByte |= bytes << 3;
  } else {
    // Seems faster than a for loop
    if (repOffsets[0] == distance) return;
    if (repOffsets[1] == distance) {
      *controlByte |= 1 << 3;
      std::swap(repOffsets[0], repOffsets[1]);
      return;
    }
    if (repOffsets[2] == distance) {
      *controlByte |= 2 << 3;
      repOffsets[2] = repOffsets[1];
      repOffsets[1] = repOffsets[0];
      repOffsets[0] = distance;
      return;
    }

    // Encode normal offset
    *controlByte |= 3 << 3;
    repOffsets[2] = repOffsets[1];
    repOffsets[1] = repOffsets[0];
    repOffsets[0] = distance;

    distance += 7;
    size_t bits = unsafe_int_log2(distance >> 3);
    size_t distanceToken = ((distance & 7) ^ 7) | (bits << 3);
    distanceEncoder->add_byte(distanceToken);
    bitDistanceEncoder->encode_raw_bits((distance >> 3) & (1 << bits) - 1,
                                        bits);
  }
}

FORCE_INLINE void encode_match(
    EntropyEncoder* tokenEncoder, EntropyEncoder* lengthEncoder,
    EntropyEncoder* distanceEncoder, BitEncoder* bitDistanceEncoder,
    uint8_t* const controlByte, size_t* matchLength, size_t distance,
    size_t* repOffsets, bool disableHuffman, bool advancedDistanceCoding) {
  encode_distance(distanceEncoder, bitDistanceEncoder, controlByte, distance,
                  repOffsets, advancedDistanceCoding);
  encode_match_length(tokenEncoder, lengthEncoder, controlByte, matchLength,
                      disableHuffman);
}

size_t get_block_pos_bits(const uint8_t* data, size_t count,
                          float decSpeedBias) {
  if (count < 16384) return 0;

  // To speed up we will only make a histogram of 1/16 of the block's symbols
  size_t histBytes = count / 16;
  uint32_t histogramBuckets[2048];
  memset(histogramBuckets, 0, sizeof(uint32_t) * 2048);
  // Results are better if we split up the histogram:
  //  1/4 at the beggining, 1/4 at the end and 1/2 in the middle
  // Keep the alignment with & 3
  symbol_histogram(data, histBytes / 4, histogramBuckets);
  symbol_histogram(data + (count - histBytes / 4 & ~3), histBytes / 4,
                   histogramBuckets);
  symbol_histogram(data + (count / 2 - histBytes / 4 & ~3), histBytes / 2,
                   histogramBuckets);

  uint32_t histogramDownmix[256];
  downmix_histogram(histogramBuckets, histogramDownmix);
  float pb0entropy = calculate_entropy(histogramDownmix, histBytes);

  float pb2entropy = 0;
  for (size_t a = 0; a < 4; a++) {
    for (size_t b = 0; b < 256; b++)
      histogramBuckets[a * 256 + b] += histogramBuckets[1024 + a * 256 + b];
    pb2entropy += calculate_entropy(&histogramBuckets[a * 256], histBytes / 4);
  }
  pb2entropy /= 4;

  if (count / 4 * (pb2entropy + decSpeedBias * 2 + 0.10) + 512 * 8 <
      count / 4 * pb0entropy)
    return 2;
  return 0;
}

int initialize_entropy_coders(
    EntropyEncoder* literalEncoder, EntropyEncoder* tokenEncoder,
    EntropyEncoder* distanceEncoder, EntropyEncoder* lengthEncoder,
    BitEncoder* bitDistanceEncoder, CodingOptions codingOptions,
    size_t huffmanBufSize, AllocatorCallback* allocator) {
  for (size_t i = 0; i < 8; i++)
    literalEncoder[i].initialize_encoder(0, LITERAL_STREAM, allocator);
  size_t distanceBufferDivisor = codingOptions.advancedDistanceCoding ? 3 : 1;
  if (tokenEncoder->initialize_encoder(huffmanBufSize / 2 + 32, TOKEN_STREAM,
                                       allocator) ||
      lengthEncoder->initialize_encoder(huffmanBufSize / 8 + 32, LENGTH_STREAM,
                                        allocator) ||
      distanceEncoder->initialize_encoder(
          huffmanBufSize / distanceBufferDivisor + 32, DISTANCE_STREAM,
          allocator) ||
      bitDistanceEncoder->init(
          codingOptions.advancedDistanceCoding ? huffmanBufSize / 3 + 32 : 0,
          allocator))
    return -1;
  return 0;
}

void start_entropy_blocks(EntropyEncoder* literalEncoder,
                          EntropyEncoder* tokenEncoder,
                          EntropyEncoder* distanceEncoder,
                          EntropyEncoder* lengthEncoder,
                          BitEncoder* bitDistanceEncoder, uint8_t* output,
                          uint8_t* deltaLiteralsBuffer, size_t thisBlockSize) {
  // We are going to store the symbols directly in the output buffer, leaving
  //  enough room for the stream header. If we end up not using huffman we
  //  already have the symbols written in the correct place, saving a memcpy,
  //  plus we save memory.
  for (size_t stream = 0; stream < 4; stream++)
    literalEncoder[stream].start_block(
        output + 3 + thisBlockSize * stream / 4 + 16 * stream, stream == 0);
  for (size_t stream = 0; stream < 4; stream++)
    literalEncoder[4 + stream].start_block(
        deltaLiteralsBuffer + thisBlockSize * stream / 4 + 16 * stream);
  tokenEncoder->start_block();
  lengthEncoder->start_block();
  distanceEncoder->start_block();
  bitDistanceEncoder->start_block();
}

// Used only by the non-optimal levels
void end_entropy_blocks(EntropyEncoder* literalEncoder,
                        EntropyEncoder* tokenEncoder,
                        EntropyEncoder* distanceEncoder,
                        EntropyEncoder* lengthEncoder,
                        BitEncoder* bitDistanceEncoder, uint8_t** output,
                        uint8_t* huffmanStreamBufferBegin, float decSpeedBias,
                        CodingOptions codingOptions) {
  // Test if using delta coding is beneficial for literals
  bool useDelta = false;
  if (codingOptions.advancedLiteralCoding) {
    size_t rawSize[4] = {0};
    size_t deltaSize[4] = {0};
    for (int i = 0; i < (1 << codingOptions.posBits); i++) {
      rawSize[i] =
          literalEncoder[0 + i].get_entropy_mode_quick(decSpeedBias, true);
      deltaSize[i] =
          literalEncoder[4 + i].get_entropy_mode_quick(decSpeedBias, true) +
          (literalEncoder[i + 4].get_symbol_count() *
           (decSpeedBias / 4 + 0.1f / 8));
    }
    useDelta = (rawSize[0] + rawSize[1] + rawSize[2] + rawSize[3]) >
               (deltaSize[0] + deltaSize[1] + deltaSize[2] + deltaSize[3]);
  }

  // Encode all streams
  int literalFlags = (useDelta ? STREAM_LITERALS_DELTA : 0) |
                     (codingOptions.posBits ? STREAM_LITERALS_POS_MASK3 : 0);
  for (int i = 0; i < (1 << codingOptions.posBits); i++)
    *output += literalEncoder[useDelta * 4 + i].end_block(
        *output, huffmanStreamBufferBegin, literalFlags, decSpeedBias, true,
        true);
  *output += tokenEncoder->end_block(*output, huffmanStreamBufferBegin, 0,
                                     decSpeedBias, true, true);
  *output += distanceEncoder->end_block(
      *output, huffmanStreamBufferBegin,
      codingOptions.advancedDistanceCoding ? STREAM_DISTANCE_ADVANCED : 0,
      decSpeedBias, true, true);
  if (codingOptions.advancedDistanceCoding)
    *output += bitDistanceEncoder->end_block(*output);
  *output += lengthEncoder->end_block(*output, huffmanStreamBufferBegin, 0,
                                      decSpeedBias, true, true);
}

size_t compress_ultra_fast(const uint8_t* input, const size_t size,
                           uint8_t* output, const int windowLog,
                           const float decSpeedBias,
                           const CompressorOptions& compressorOptions,
                           AllocatorCallback* allocator,
                           ProgressCallback* progress) {
  // Constants for this encoder
  const size_t accelerationThreshold = 4;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  // The last match or literal run must end before this limit
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffsets[3] = {1, 1, 1};
  // Keeps track of how many match searches we have made without success.
  // When we dont find matches, we will skip more or less depending on this
  // variable.
  size_t acceleration = 1 << accelerationThreshold;

  CodingOptions codingOptions(decSpeedBias);
  size_t encoderMaxBlockSize =
      codingOptions.disableHuffman ? MAX_BLOCK_SIZE : MAX_BLOCK_SIZE / 2;
  size_t huffmanBufSize = std::min(encoderMaxBlockSize, size);

  const size_t hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
  HashTable<uint32_t, FastIntHash> lzdict(allocator);
  uint8_t* deltaLiteralsBuffer = nullptr;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  try {
    lzdict.init(hashLog);
    if (codingOptions.advancedLiteralCoding)
      deltaLiteralsBuffer = (uint8_t*)allocator->allocate(huffmanBufSize + 128);
    if (!codingOptions.disableHuffman) {
      huffmanStreamBuffer = (uint8_t*)allocator->allocate(huffmanBufSize + 128);
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
    }
  } catch (const std::bad_alloc& e) {
    allocator->free(deltaLiteralsBuffer);
    allocator->free(huffmanStreamBuffer);
    return ERROR_NOMEM;
  }

  EntropyEncoder literalEncoder[8];
  EntropyEncoder tokenEncoder;
  EntropyEncoder lengthEncoder;
  EntropyEncoder distanceEncoder;
  BitEncoder bitDistanceEncoder;

  if (initialize_entropy_coders(literalEncoder, &tokenEncoder, &distanceEncoder,
                                &lengthEncoder, &bitDistanceEncoder,
                                codingOptions, huffmanBufSize, allocator)) {
    allocator->free(deltaLiteralsBuffer);
    allocator->free(huffmanStreamBuffer);
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    const size_t thisBlockSize =
        (compressionLimit - input < encoderMaxBlockSize)
            ? compressionLimit - input
            : encoderMaxBlockSize;
    const uint8_t* const thisBlockEnd = input + thisBlockSize;

    if (codingOptions.advancedLiteralCoding)
      codingOptions.posBits =
          get_block_pos_bits(input, thisBlockSize, decSpeedBias);
    write_header(output, thisBlockSize, BLOCK_COMPRESSED, 0);
    // First byte of first block is sent as is, this way we
    //  dont need an extra branch when enc/dec delta literals for the first one.
    if (input == inputStart) *output++ = *input++;

    start_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                         &lengthEncoder, &bitDistanceEncoder, output,
                         deltaLiteralsBuffer, thisBlockSize);
    const uint8_t* literalRunStart = input;

    while (likely(input < thisBlockEnd)) {
      // Get possible match location and update the table
      uint32_t* const dictEntry = &lzdict[read_hash6(input)];
      const uint8_t* match = inputStart + *dictEntry;
      *dictEntry = input - inputStart;
      // Try to find a match
      size_t matchLength =
          test_match<true>(input, match, thisBlockEnd, 6, windowLog);

      // No match? Advance the position and try again
      if (matchLength == 0) {
        input += acceleration >> accelerationThreshold;
        acceleration++;
        continue;
      }

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
      encode_literal_run(literalEncoder, &lengthEncoder, input, literalRunStart,
                         &controlByte, inputStart, repOffsets[0],
                         codingOptions);

      input += matchLength;
      // Output the match
      encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                   &bitDistanceEncoder, &controlByte, &matchLength, distance,
                   repOffsets, codingOptions.disableHuffman,
                   codingOptions.advancedDistanceCoding);

      // Try to find further rep matches
      while (true) {
        size_t off = input[1] == input[1 - repOffsets[0]] ? 1 : 2;
        matchLength = test_match<false>(
            input + off, input + off - repOffsets[0], thisBlockEnd, 3, 0);
        if (matchLength != 0) {
          uint8_t controlByte;
          encode_literal_run_short(literalEncoder, input, off, &controlByte,
                                   inputStart, repOffsets[0], codingOptions);
          // Add two additional positions to the table
          input += off;
          lzdict[read_hash6(input + 0)] = input + 0 - inputStart;
          lzdict[read_hash6(input + 1)] = input + 1 - inputStart;
          input += matchLength;
          encode_first_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                 &matchLength, codingOptions);
          continue;
        }

        if (codingOptions.advancedDistanceCoding) {
          matchLength = test_match<false>(input, input - repOffsets[1],
                                          thisBlockEnd, 4, 0);
          if (matchLength != 0) {
            uint8_t controlByte = 0;
            input += matchLength;
            encode_second_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                    &matchLength, repOffsets);
            continue;
          }
          matchLength = test_match<false>(input, input - repOffsets[2],
                                          thisBlockEnd, 4, 0);
          if (matchLength != 0) {
            uint8_t controlByte = 0;
            input += matchLength;
            encode_third_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                   &matchLength, repOffsets);
            continue;
          }
        }

        break;
      }

      literalRunStart = input;
      acceleration = 1 << accelerationThreshold;
    }

    // Maybe we went beyond block end because of acceleration
    input = thisBlockEnd;

    // Send last literal run
    if (input != literalRunStart) {
      uint8_t controlByte;
      encode_literal_run(literalEncoder, &lengthEncoder, input, literalRunStart,
                         &controlByte, inputStart, repOffsets[0],
                         codingOptions);
      tokenEncoder.add_byte(controlByte);
    }

    end_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                       &lengthEncoder, &bitDistanceEncoder, &output,
                       huffmanStreamBufferBegin, decSpeedBias, codingOptions);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        allocator->free(deltaLiteralsBuffer);
        allocator->free(huffmanStreamBuffer);
        return 0;
      }
    }
  }

  allocator->free(deltaLiteralsBuffer);
  allocator->free(huffmanStreamBuffer);
  write_header(output, SKANDA_LAST_BYTES, BLOCK_RAW, BLOCK_LAST);
  memcpy(output, input, SKANDA_LAST_BYTES);
  if (progress)
    progress->progress(size, output - outputStart + SKANDA_LAST_BYTES);

  return output - outputStart + SKANDA_LAST_BYTES;
}

size_t compress_greedy(const uint8_t* input, const size_t size, uint8_t* output,
                       const int windowLog, const float decSpeedBias,
                       const CompressorOptions& compressorOptions,
                       AllocatorCallback* allocator,
                       ProgressCallback* progress) {
  const size_t accelerationThreshold = 5;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffsets[3] = {1, 1, 1};
  size_t acceleration = 1 << accelerationThreshold;

  CodingOptions codingOptions(decSpeedBias);
  size_t encoderMaxBlockSize =
      codingOptions.disableHuffman ? MAX_BLOCK_SIZE : MAX_BLOCK_SIZE / 2;
  size_t huffmanBufSize = std::min(encoderMaxBlockSize, size);

  const size_t hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
  HashTable<uint32_t, FastIntHash> lzdict(allocator);
  uint8_t* deltaLiteralsBuffer = nullptr;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  try {
    lzdict.init(hashLog);
    if (codingOptions.advancedLiteralCoding)
      deltaLiteralsBuffer = (uint8_t*)allocator->allocate(huffmanBufSize + 128);
    if (!codingOptions.disableHuffman) {
      huffmanStreamBuffer = (uint8_t*)allocator->allocate(huffmanBufSize + 128);
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
    }
  } catch (const std::bad_alloc& e) {
    allocator->free(deltaLiteralsBuffer);
    allocator->free(huffmanStreamBuffer);
    return ERROR_NOMEM;
  }

  EntropyEncoder literalEncoder[8];
  EntropyEncoder tokenEncoder;
  EntropyEncoder lengthEncoder;
  EntropyEncoder distanceEncoder;
  BitEncoder bitDistanceEncoder;

  if (initialize_entropy_coders(literalEncoder, &tokenEncoder, &distanceEncoder,
                                &lengthEncoder, &bitDistanceEncoder,
                                codingOptions, huffmanBufSize, allocator)) {
    allocator->free(deltaLiteralsBuffer);
    allocator->free(huffmanStreamBuffer);
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    const size_t thisBlockSize =
        (compressionLimit - input < encoderMaxBlockSize)
            ? compressionLimit - input
            : encoderMaxBlockSize;
    const uint8_t* const thisBlockEnd = input + thisBlockSize;

    if (codingOptions.advancedLiteralCoding)
      codingOptions.posBits =
          get_block_pos_bits(input, thisBlockSize, decSpeedBias);
    write_header(output, thisBlockSize, BLOCK_COMPRESSED, 0);
    if (input == inputStart) *output++ = *input++;

    start_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                         &lengthEncoder, &bitDistanceEncoder, output,
                         deltaLiteralsBuffer, thisBlockSize);
    const uint8_t* literalRunStart = input;

    while (input < thisBlockEnd) {
      // First try to find a rep match. Doing it at position +1 gives better
      // results. If one is found simply take it and skip normal match finding.
      size_t matchLength = test_match<false>(
          input + 1, input + 1 - repOffsets[0], thisBlockEnd, 3, 0);
      if (matchLength) {
        input++;
        uint8_t controlByte;
        encode_literal_run(literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte, inputStart,
                           repOffsets[0], codingOptions);
        lzdict[read_hash5(input) + 0] = input + 0 - inputStart;
        input += matchLength;
        literalRunStart = input;
        encode_first_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                               &matchLength, codingOptions);
        acceleration = 1 << accelerationThreshold;
        continue;
      }

      // If no rep, try a normal match
      uint32_t* dictEntry = &lzdict[read_hash5(input)];
      const uint8_t* match = inputStart + *dictEntry;
      *dictEntry = input - inputStart;
      matchLength = test_match<true>(input, match, thisBlockEnd, 5, windowLog);

      if (matchLength == 0) {
        input += acceleration >> accelerationThreshold;
        acceleration++;
        continue;
      }

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
      encode_literal_run(literalEncoder, &lengthEncoder, input, literalRunStart,
                         &controlByte, inputStart, repOffsets[0],
                         codingOptions);

      size_t distance = input - match;
      input += matchLength;
      encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                   &bitDistanceEncoder, &controlByte, &matchLength, distance,
                   repOffsets, codingOptions.disableHuffman,
                   codingOptions.advancedDistanceCoding);
      acceleration = 1 << accelerationThreshold;

      if (codingOptions.advancedDistanceCoding) {
        matchLength =
            test_match<false>(input, input - repOffsets[1], thisBlockEnd, 4, 0);
        if (matchLength != 0) {
          uint8_t controlByte = 0;
          input += matchLength;
          encode_second_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                  &matchLength, repOffsets);
          literalRunStart = input;
          continue;
        }
        matchLength =
            test_match<false>(input, input - repOffsets[2], thisBlockEnd, 4, 0);
        if (matchLength != 0) {
          uint8_t controlByte = 0;
          input += matchLength;
          encode_third_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                 &matchLength, repOffsets);
          literalRunStart = input;
          continue;
        }
      }
      literalRunStart = input;
    }

    input = thisBlockEnd;
    if (input != literalRunStart) {
      uint8_t controlByte;
      encode_literal_run(literalEncoder, &lengthEncoder, input, literalRunStart,
                         &controlByte, inputStart, repOffsets[0],
                         codingOptions);
      tokenEncoder.add_byte(controlByte);
    }

    end_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                       &lengthEncoder, &bitDistanceEncoder, &output,
                       huffmanStreamBufferBegin, decSpeedBias, codingOptions);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        allocator->free(deltaLiteralsBuffer);
        allocator->free(huffmanStreamBuffer);
        return 0;
      }
    }
  }

  allocator->free(deltaLiteralsBuffer);
  allocator->free(huffmanStreamBuffer);
  write_header(output, SKANDA_LAST_BYTES, BLOCK_RAW, BLOCK_LAST);
  memcpy(output, input, SKANDA_LAST_BYTES);
  if (progress)
    progress->progress(size, output - outputStart + SKANDA_LAST_BYTES);

  return output - outputStart + SKANDA_LAST_BYTES;
}

size_t compress_lazy_fast(const uint8_t* input, const size_t size,
                          uint8_t* output, const int windowLog,
                          const float decSpeedBias,
                          const CompressorOptions& compressorOptions,
                          AllocatorCallback* allocator,
                          ProgressCallback* progress) {
  const size_t accelerationThreshold = 6;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffsets[3] = {1, 1, 1};
  size_t acceleration = 1 << accelerationThreshold;

  CodingOptions codingOptions(decSpeedBias);
  size_t encoderMaxBlockSize =
      codingOptions.disableHuffman ? MAX_BLOCK_SIZE : MAX_BLOCK_SIZE / 2;
  size_t huffmanBufSize = std::min(encoderMaxBlockSize, size);

  const size_t hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
  HashTable<uint32_t, FastIntHash> lzdict4(allocator);
  HashTable<uint32_t, FastIntHash> lzdict8(allocator);
  uint8_t* deltaLiteralsBuffer = nullptr;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  try {
    lzdict4.init(hashLog);
    lzdict8.init(hashLog);
    if (codingOptions.advancedLiteralCoding)
      deltaLiteralsBuffer = (uint8_t*)allocator->allocate(huffmanBufSize + 128);
    if (!codingOptions.disableHuffman) {
      huffmanStreamBuffer = (uint8_t*)allocator->allocate(huffmanBufSize + 128);
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
    }
  } catch (const std::bad_alloc& e) {
    allocator->free(deltaLiteralsBuffer);
    allocator->free(huffmanStreamBuffer);
    return ERROR_NOMEM;
  }

  EntropyEncoder literalEncoder[8];
  EntropyEncoder tokenEncoder;
  EntropyEncoder lengthEncoder;
  EntropyEncoder distanceEncoder;
  BitEncoder bitDistanceEncoder;

  if (initialize_entropy_coders(literalEncoder, &tokenEncoder, &distanceEncoder,
                                &lengthEncoder, &bitDistanceEncoder,
                                codingOptions, huffmanBufSize, allocator)) {
    allocator->free(deltaLiteralsBuffer);
    allocator->free(huffmanStreamBuffer);
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    const size_t thisBlockSize =
        (compressionLimit - input < encoderMaxBlockSize)
            ? compressionLimit - input
            : encoderMaxBlockSize;
    const uint8_t* const thisBlockEnd = input + thisBlockSize;

    if (codingOptions.advancedLiteralCoding)
      codingOptions.posBits =
          get_block_pos_bits(input, thisBlockSize, decSpeedBias);
    write_header(output, thisBlockSize, BLOCK_COMPRESSED, 0);
    if (input == inputStart) *output++ = *input++;

    start_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                         &lengthEncoder, &bitDistanceEncoder, output,
                         deltaLiteralsBuffer, thisBlockSize);
    const uint8_t* literalRunStart = input;

    while (input < thisBlockEnd) {
      // First try finding a lazy rep match. Take it if its sufficiently long,
      // otherwise check if there is
      //  a longer normal match.
      size_t repMatchLength = test_match<false>(
          input + 1, input + 1 - repOffsets[0], thisBlockEnd, 2, 0);
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
        encode_literal_run(literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte, inputStart,
                           repOffsets[0], codingOptions);

        input += repMatchLength;
        literalRunStart = input;
        encode_first_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                               &repMatchLength, codingOptions);
        acceleration = 1 << accelerationThreshold;
        continue;
      }

      if (codingOptions.advancedDistanceCoding) {
        // Only check second and third rep matches if the literal run is short.
        // This comparison is equivalent to input - literalRunStart < 7,
        //  but saves the sub instruction
        if (acceleration < (1 << accelerationThreshold) + 7) {
          size_t matchLength = test_match<false>(input, input - repOffsets[1],
                                                 thisBlockEnd, 2, 0);
          if (matchLength) {
            lzdict4[read_hash4(input + 0)] = (input + 0 - inputStart);
            lzdict4[read_hash4(input + 1)] = (input + 1 - inputStart);
            lzdict8[read_hash8(input + 0)] = (input + 0 - inputStart);

            uint8_t controlByte;
            encode_literal_run_short(literalEncoder, literalRunStart,
                                     input - literalRunStart, &controlByte,
                                     inputStart, repOffsets[0], codingOptions);
            input += matchLength;
            literalRunStart = input;
            encode_second_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                    &matchLength, repOffsets);
            continue;
          }

          matchLength = test_match<false>(input, input - repOffsets[2],
                                          thisBlockEnd, 2, 0);
          if (matchLength) {
            lzdict4[read_hash4(input + 0)] = (input + 0 - inputStart);
            lzdict4[read_hash4(input + 1)] = (input + 1 - inputStart);
            lzdict8[read_hash8(input + 0)] = (input + 0 - inputStart);

            uint8_t controlByte;
            encode_literal_run_short(literalEncoder, literalRunStart,
                                     input - literalRunStart, &controlByte,
                                     inputStart, repOffsets[0], codingOptions);
            input += matchLength;
            literalRunStart = input;
            encode_third_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                   &matchLength, repOffsets);
            continue;
          }
        }
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
        encode_literal_run(literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte, inputStart,
                           repOffsets[0], codingOptions);

        input += repMatchLength;
        literalRunStart = input;

        // Skip function call, we know repMatchLength does not overflow the
        // token
        controlByte |= repMatchLength - 2;
        tokenEncoder.add_byte(controlByte);

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
      encode_literal_run(literalEncoder, &lengthEncoder, input, literalRunStart,
                         &controlByte, inputStart, repOffsets[0],
                         codingOptions);

      input += matchLength;
      literalRunStart = input;
      encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                   &bitDistanceEncoder, &controlByte, &matchLength, distance,
                   repOffsets, codingOptions.disableHuffman,
                   codingOptions.advancedDistanceCoding);
      acceleration = 1 << accelerationThreshold;
    }

    input = thisBlockEnd;
    if (input != literalRunStart) {
      uint8_t controlByte;
      encode_literal_run(literalEncoder, &lengthEncoder, input, literalRunStart,
                         &controlByte, inputStart, repOffsets[0],
                         codingOptions);
      tokenEncoder.add_byte(controlByte);
    }

    end_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                       &lengthEncoder, &bitDistanceEncoder, &output,
                       huffmanStreamBufferBegin, decSpeedBias, codingOptions);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        allocator->free(deltaLiteralsBuffer);
        allocator->free(huffmanStreamBuffer);
        return 0;
      }
    }
  }

  allocator->free(deltaLiteralsBuffer);
  allocator->free(huffmanStreamBuffer);
  write_header(output, SKANDA_LAST_BYTES, BLOCK_RAW, BLOCK_LAST);
  memcpy(output, input, SKANDA_LAST_BYTES);
  if (progress)
    progress->progress(size, output - outputStart + SKANDA_LAST_BYTES);

  return output - outputStart + SKANDA_LAST_BYTES;
}

size_t compress_lazy(const uint8_t* input, const size_t size, uint8_t* output,
                     const int windowLog, const float decSpeedBias,
                     const CompressorOptions& compressorOptions,
                     AllocatorCallback* allocator, ProgressCallback* progress) {
  const size_t accelerationThreshold = 6;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffsets[3] = {1, 1, 1};
  size_t acceleration = 1 << accelerationThreshold;

  CodingOptions codingOptions(decSpeedBias);
  size_t encoderMaxBlockSize =
      codingOptions.disableHuffman ? MAX_BLOCK_SIZE : MAX_BLOCK_SIZE / 2;
  size_t huffmanBufSize = std::min(encoderMaxBlockSize, size);

  const int hashLog = std::min(compressorOptions.maxHashLog, windowLog - 3);
  LZCacheTable<uint32_t, FastIntHash> lzdict4(allocator);
  LZCacheTable<uint32_t, FastIntHash> lzdict8(allocator);
  uint8_t* deltaLiteralsBuffer = nullptr;
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  try {
    lzdict4.init(hashLog, compressorOptions.maxElementsLog);
    lzdict8.init(hashLog, compressorOptions.maxElementsLog);
    if (codingOptions.advancedLiteralCoding)
      deltaLiteralsBuffer = (uint8_t*)allocator->allocate(huffmanBufSize + 128);
    if (!codingOptions.disableHuffman) {
      huffmanStreamBuffer = (uint8_t*)allocator->allocate(huffmanBufSize + 128);
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
    }
  } catch (const std::bad_alloc& e) {
    allocator->free(deltaLiteralsBuffer);
    allocator->free(huffmanStreamBuffer);
    return ERROR_NOMEM;
  }

  EntropyEncoder literalEncoder[8];
  EntropyEncoder tokenEncoder;
  EntropyEncoder lengthEncoder;
  EntropyEncoder distanceEncoder;
  BitEncoder bitDistanceEncoder;

  if (initialize_entropy_coders(literalEncoder, &tokenEncoder, &distanceEncoder,
                                &lengthEncoder, &bitDistanceEncoder,
                                codingOptions, huffmanBufSize, allocator)) {
    allocator->free(deltaLiteralsBuffer);
    allocator->free(huffmanStreamBuffer);
    return ERROR_NOMEM;
  }

  // Main compression loop
  while (input < compressionLimit) {
    const size_t thisBlockSize =
        (compressionLimit - input < encoderMaxBlockSize)
            ? compressionLimit - input
            : encoderMaxBlockSize;
    const uint8_t* const thisBlockEnd = input + thisBlockSize;

    if (codingOptions.advancedLiteralCoding)
      codingOptions.posBits =
          get_block_pos_bits(input, thisBlockSize, decSpeedBias);
    write_header(output, thisBlockSize, BLOCK_COMPRESSED, 0);
    if (input == inputStart) *output++ = *input++;

    start_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                         &lengthEncoder, &bitDistanceEncoder, output,
                         deltaLiteralsBuffer, thisBlockSize);
    const uint8_t* literalRunStart = input;

    while (input < thisBlockEnd) {
      // First try a rep match
      size_t repMatchLength = test_match<false>(
          input + 1, input + 1 - repOffsets[0], thisBlockEnd, 2, 0);
      if (repMatchLength >= 4) {
        // Update table
        lzdict4[read_hash4(input + 0)].push(input + 0 - inputStart);
        lzdict4[read_hash4(input + 1)].push(input + 1 - inputStart);
        lzdict4[read_hash4(input + 2)].push(input + 2 - inputStart);
        lzdict4[read_hash4(input + 3)].push(input + 3 - inputStart);
        lzdict4[read_hash4(input + 4)].push(input + 4 - inputStart);
        lzdict8[read_hash8(input + 1)].push(input + 1 - inputStart);

        input++;
        uint8_t controlByte;
        encode_literal_run(literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte, inputStart,
                           repOffsets[0], codingOptions);

        input += repMatchLength;
        literalRunStart = input;
        encode_first_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                               &repMatchLength, codingOptions);
        acceleration = 1 << accelerationThreshold;
        continue;
      }

      if (codingOptions.advancedDistanceCoding) {
        if (acceleration < (1 << accelerationThreshold) + 7) {
          size_t matchLength = test_match<false>(input, input - repOffsets[1],
                                                 thisBlockEnd, 2, 0);
          if (matchLength) {
            lzdict4[read_hash4(input + 0)].push(input + 0 - inputStart);
            lzdict4[read_hash4(input + 1)].push(input + 1 - inputStart);
            lzdict8[read_hash8(input + 0)].push(input + 0 - inputStart);

            uint8_t controlByte;
            encode_literal_run_short(literalEncoder, literalRunStart,
                                     input - literalRunStart, &controlByte,
                                     inputStart, repOffsets[0], codingOptions);
            input += matchLength;
            literalRunStart = input;
            encode_second_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                    &matchLength, repOffsets);
            continue;
          }

          matchLength = test_match<false>(input, input - repOffsets[2],
                                          thisBlockEnd, 2, 0);
          if (matchLength) {
            lzdict4[read_hash4(input + 0)].push(input + 0 - inputStart);
            lzdict4[read_hash4(input + 1)].push(input + 1 - inputStart);
            lzdict8[read_hash8(input + 0)].push(input + 0 - inputStart);

            uint8_t controlByte;
            encode_literal_run_short(literalEncoder, literalRunStart,
                                     input - literalRunStart, &controlByte,
                                     inputStart, repOffsets[0], codingOptions);
            input += matchLength;
            literalRunStart = input;
            encode_third_rep_match(&tokenEncoder, &lengthEncoder, &controlByte,
                                   &matchLength, repOffsets);
            continue;
          }
        }
      }

      size_t matchLength = 0;
      size_t distance;

      LZCacheBucket<uint32_t> chain4 = (lzdict4)[read_hash4(input)];
      int lazySteps = 0;

      uint32_t pos4 = input - inputStart;
      do {
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
      } while (!chain4.ended());

      if (matchLength >= 4) {
        LZCacheBucket<uint32_t> chain8 = (lzdict8)[read_hash8(input)];
        uint32_t pos8 = input - inputStart;
        do {
          chain8.next(&pos8);
          const uint8_t* where = inputStart + pos8;
          if (*(input + matchLength) != *(where + matchLength)) continue;
          size_t length =
              test_match<true>(input, where, thisBlockEnd, 8, windowLog);
          if (length > matchLength) {
            distance = input - where;
            matchLength = length;
          }
        } while (!chain8.ended());
      } else if (repMatchLength) {
        input++;
        uint8_t controlByte;
        encode_literal_run(literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte, inputStart,
                           repOffsets[0], codingOptions);

        lzdict4[read_hash4(input + 0)].push(input + 0 - inputStart);
        lzdict4[read_hash4(input + 1)].push(input + 1 - inputStart);

        input += repMatchLength;
        literalRunStart = input;
        controlByte |= repMatchLength - 2;
        tokenEncoder.add_byte(controlByte);
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

      do {
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
      } while (!chain8.ended());

      // Now get an even better match at pos + 2
      matchPos++;
      pos8 = matchPos - inputStart;
      (lzdict4)[read_hash4(matchPos)].push(pos8);
      chain8 = (lzdict8)[read_hash8(matchPos)];

      do {
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
      } while (!chain8.ended());

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
      encode_literal_run(literalEncoder, &lengthEncoder, input, literalRunStart,
                         &controlByte, inputStart, repOffsets[0],
                         codingOptions);

      input += matchLength;
      literalRunStart = input;
      encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                   &bitDistanceEncoder, &controlByte, &matchLength, distance,
                   repOffsets, codingOptions.disableHuffman,
                   codingOptions.advancedDistanceCoding);
      acceleration = 1 << accelerationThreshold;
    }

    input = thisBlockEnd;
    if (input != literalRunStart) {
      uint8_t controlByte;
      encode_literal_run(literalEncoder, &lengthEncoder, input, literalRunStart,
                         &controlByte, inputStart, repOffsets[0],
                         codingOptions);
      tokenEncoder.add_byte(controlByte);
    }

    end_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                       &lengthEncoder, &bitDistanceEncoder, &output,
                       huffmanStreamBufferBegin, decSpeedBias, codingOptions);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        allocator->free(deltaLiteralsBuffer);
        allocator->free(huffmanStreamBuffer);
        return 0;
      }
    }
  }

  allocator->free(deltaLiteralsBuffer);
  allocator->free(huffmanStreamBuffer);
  write_header(output, SKANDA_LAST_BYTES, BLOCK_RAW, BLOCK_LAST);
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
  // repOffsets[0] indicates used distance for this match
  uint32_t repOffsets[3];
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
                                   size_t distanceBits,
                                   HuffmanSymbol* tokenSymbols) {
  return tokenSymbols[(std::min(literalRunLength, (size_t)7) << 5) |
                      (distanceBits << 3) |
                      (std::min(matchLength, (size_t)9) - 2)]
      .bits;
}

LZStructure* forward_optimal_parse(
    const uint8_t* const input, const uint8_t* const inputStart,
    const uint8_t* const blockLimit, HashTableMatchFinder* matchFinder,
    SkandaOptimalParserState* parser, LZStructure* stream,
    const size_t* startRepOffsets, size_t* acceleration,
    const size_t accelerationThreshold, HuffmanSymbol* literalSymbols[4],
    HuffmanSymbol* tokenSymbols, HuffmanSymbol* lengthSymbols,
    HuffmanSymbol* distanceSymbols, bool disableHuffman,
    bool advancedDistanceCoding, int literalMode,
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
  parser[0].cost = 64 << 8;
  parser[0].repOffsets[0] = startRepOffsets[0];
  parser[0].repOffsets[1] = startRepOffsets[1];
  parser[0].repOffsets[2] = startRepOffsets[2];
  parser[0].literalRunLength = 0;

  size_t lastMatchLength = 0;
  size_t lastMatchDistance;

  size_t position = 0;
  int positionSkip = 0;
  for (; position < blockLength; position++) {
    const uint8_t* inputPosition = input + position;
    SkandaOptimalParserState* const parserPosition = parser + position;
    // Make sure we have enough positions ahead initialized
    parserPosition[compressorOptions.niceLength].cost = UINT32_MAX;

    // From Zstd: skip unpromising position
    if (parserPosition[0].cost >= parserPosition[1].cost || positionSkip > 0) {
      matchFinder->update_position(inputPosition, inputStart);
      positionSkip--;
      continue;
    }

    size_t literalCost = parserPosition->cost;
    // Size cost
    if (disableHuffman)
      literalCost += 0x8000 << (parserPosition->literalRunLength == 6);
    else {
      if (parserPosition->literalRunLength >= 6) {
        literalCost += get_length_cost(parserPosition->literalRunLength + 1 - 7,
                                       lengthSymbols)
                       << 12;
        if (parserPosition->literalRunLength >= 7)
          literalCost -= get_length_cost(parserPosition->literalRunLength - 7,
                                         lengthSymbols)
                         << 12;
      }

      if (literalMode == 0)
        literalCost += literalSymbols[0][*inputPosition].bits << 12;
      else if (literalMode == STREAM_LITERALS_DELTA) {
        uint8_t delta =
            inputPosition[0] - *(inputPosition - parserPosition->repOffsets[0]);
        literalCost += literalSymbols[0][delta].bits << 12;
      } else if (literalMode == STREAM_LITERALS_POS_MASK3) {
        size_t pos = (inputPosition - inputStart) & 3;
        literalCost += literalSymbols[pos][*inputPosition].bits << 12;
      } else if (literalMode ==
                 (STREAM_LITERALS_DELTA | STREAM_LITERALS_POS_MASK3)) {
        size_t pos = (inputPosition - inputStart) & 3;
        uint8_t delta =
            inputPosition[0] - *(inputPosition - parserPosition->repOffsets[0]);
        literalCost += literalSymbols[pos][delta].bits << 12;
      }
    }
    // Speed cost
    literalCost += (parserPosition->literalRunLength == 6) * 2;

    SkandaOptimalParserState* nextPosition = parserPosition + 1;
    if (literalCost < nextPosition->cost) {
      nextPosition->cost = literalCost;
      memcpy(nextPosition->repOffsets, parserPosition->repOffsets,
             3 * sizeof(uint32_t));  // Carry forward the rep offsets
      nextPosition->literalRunLength = parserPosition->literalRunLength + 1;
    }

    size_t repMatchLength = 0;
    for (int rep = 0; rep < (advancedDistanceCoding ? 3 : 1); rep++) {
      repMatchLength = test_match<false>(
          inputPosition, inputPosition - parserPosition->repOffsets[rep],
          blockLimit, 2, 0);
      if (repMatchLength) {
        // Rep matches can be unconditionally taken with lower lengths
        if (repMatchLength >= compressorOptions.niceLength / 2) {
          matchFinder->update_position(inputPosition, inputStart);
          lastMatchLength = repMatchLength;
          lastMatchDistance = parserPosition->repOffsets[rep];
          goto doBackwardParse;
        }

        size_t repMatchCost = parserPosition->cost;
        // size cost
        if (disableHuffman) {
          // For decoding speed
          if (repMatchLength > 8 && repMatchLength <= 16) repMatchLength = 8;
          repMatchCost += 0x8000 << (repMatchLength > 8);  // size cost
        } else {
          repMatchCost += get_token_cost(parserPosition->literalRunLength,
                                         repMatchLength, rep, tokenSymbols)
                          << 12;
          if (repMatchLength > 8)
            repMatchCost += get_length_cost(repMatchLength - 9, lengthSymbols)
                            << 12;
        }
        // speed cost
        repMatchCost += 1 + (repMatchLength > 8) * 2;  // speed cost

        nextPosition = parserPosition + repMatchLength;
        if (repMatchCost < nextPosition->cost) {
          nextPosition->cost = repMatchCost;
          nextPosition->matchLength = repMatchLength;
          nextPosition->repOffsets[2] =
              parserPosition->repOffsets[rep < 2 ? 2 : 1];
          nextPosition->repOffsets[1] =
              parserPosition->repOffsets[rep < 1 ? 1 : 0];
          nextPosition->repOffsets[0] = parserPosition->repOffsets[rep];
          nextPosition->literalRunLength = 0;
        }

        // When finding a rep match skip the next position
        positionSkip = 1;
        if (repMatchLength >= 3) *acceleration = 1 << accelerationThreshold;
        break;
      }
    }

    LZMatch matches[9];
    LZMatch* matchesEnd = matchFinder->find_matches_and_update(
        inputPosition, inputStart, blockLimit, matches, repMatchLength + 1,
        windowLog);

    // No match found
    if (matchesEnd == matches) {
      (*acceleration)++;
      // If acceleration goes too high go to greedy parse
      if (*acceleration >= (2 << accelerationThreshold)) {
        position++;
        break;
      }
      continue;
    }

    // The last match should be the longest
    LZMatch* const longestMatch = matchesEnd - 1;
    if (longestMatch->length >= compressorOptions.niceLength) {
      lastMatchLength = longestMatch->length;
      lastMatchDistance = longestMatch->distance;
      // Left match extension
      const uint8_t* match = inputPosition - lastMatchDistance;
      while (parser[position].literalRunLength > 0 && match > inputStart &&
             input[position - 1] == match[-1]) {
        lastMatchLength++;
        position--;
        match--;
      }
      break;
    }
    if (longestMatch->length >= 4) *acceleration = 1 << accelerationThreshold;

    for (LZMatch* matchIt = longestMatch; matchIt >= matches; matchIt--) {
      // Left match extension
      SkandaOptimalParserState* matchPos = parserPosition;
      const uint8_t* leftExtensionFront = inputPosition;
      const uint8_t* leftExtensionBack = inputPosition - matchIt->distance;
      const uint8_t* literalRunStart =
          inputPosition - matchPos->literalRunLength;
      while (leftExtensionBack > inputStart &&
             leftExtensionFront > literalRunStart &&
             leftExtensionBack[-1] == leftExtensionFront[-1]) {
        matchIt->length++;
        leftExtensionBack--;
        leftExtensionFront--;
        matchPos--;
      }

      size_t matchCost = matchPos->cost;
      // size cost
      if (disableHuffman) {
        // For decoding speed
        if (matchIt->length > 8 && matchIt->length <= 16) matchIt->length = 8;

        if (!advancedDistanceCoding) {
          size_t distanceBytes = unsafe_int_log2(matchIt->distance) / 8;
          matchCost += (2 + distanceBytes + (matchIt->length > 8)) << 15;
        } else {
          size_t bits = unsafe_int_log2((matchIt->distance + 7) >> 3);
          matchCost += (16 + bits + (matchIt->length > 8) * 8) << 12;
        }
      } else {
        if (!advancedDistanceCoding) {
          size_t distanceBytes = unsafe_int_log2(matchIt->distance) / 8;
          matchCost +=
              get_token_cost(matchPos->literalRunLength, matchIt->length,
                             distanceBytes + 1, tokenSymbols)
              << 12;
          for (int i = 0; i <= distanceBytes; i++)
            matchCost +=
                distanceSymbols[(matchIt->distance >> i * 8) & 0xFF].bits << 12;
        } else {
          size_t virtualDistance = matchIt->distance + 7;
          size_t bits = unsafe_int_log2(virtualDistance >> 3);
          matchCost += get_token_cost(matchPos->literalRunLength,
                                      matchIt->length, 3, tokenSymbols)
                       << 12;
          matchCost +=
              distanceSymbols[((virtualDistance & 7) ^ 7) | (bits << 3)].bits
              << 12;
          matchCost += bits << 12;
        }
        if (matchIt->length > 8)
          matchCost += get_length_cost(matchIt->length - 9, lengthSymbols)
                       << 12;
      }
      // speed cost
      matchCost +=
          1 + (matchIt->length > 8) * 2 + (matchIt->distance > (1 << 16));

      nextPosition = matchPos + matchIt->length;
      // If this match does not help, the shorter ones probably wont
      if (matchCost >= nextPosition->cost) break;

      nextPosition->cost = matchCost;
      nextPosition->matchLength = matchIt->length;
      nextPosition->repOffsets[2] = matchPos->repOffsets[1];
      nextPosition->repOffsets[1] = matchPos->repOffsets[0];
      nextPosition->repOffsets[0] = matchIt->distance;
      nextPosition->literalRunLength = 0;
    }
  }

doBackwardParse:

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
    stream->matchDistance = backwardParse->repOffsets[0];
    stream->matchLength = backwardParse->matchLength;
    backwardParse -= backwardParse->matchLength;
    stream->literalRunLength = backwardParse->literalRunLength;
    backwardParse -= backwardParse->literalRunLength;
    stream++;
  }

  return stream - 1;
}

class MatchBuffer {
  AllocatorCallback* allocator;
  LZMatch* matches = nullptr;
  int* matchesCount = nullptr;
  size_t maxMatchesPerPosition;

 public:
  MatchBuffer(AllocatorCallback* alloc) { allocator = alloc; }
  ~MatchBuffer() {
    allocator->free(matches);
    allocator->free(matchesCount);
  }
  void init(size_t inSize, const CompressorOptions& compressorOptions) {
    maxMatchesPerPosition = (1 << compressorOptions.maxElementsLog) + 1;
    // To reduce memory usage only store the best 24 matches, even if we are
    // making more searches. This also increases speed on small files because we
    // need to allocate less space.
    if (maxMatchesPerPosition > 24) maxMatchesPerPosition = 24;
    matches =
        (LZMatch*)allocator->allocate(sizeof(LZMatch) * maxMatchesPerPosition *
                                      std::min(inSize, MAX_BLOCK_SIZE));
    matchesCount = (int*)allocator->allocate(sizeof(int) *
                                             std::min(inSize, MAX_BLOCK_SIZE));
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
      LZMatch matchBuffer[160];
      LZMatch* matchesEnd = matchFinder->find_matches_and_update(
          in + pos, inputStart, compressionLimit, blockLimit, matchBuffer, 3,
          windowLog, compressorOptions);
      matchesCount[pos] =
          std::min(size_t(matchesEnd - matchBuffer), maxMatchesPerPosition);
      memcpy(&matches[pos * maxMatchesPerPosition],
             matchesEnd - matchesCount[pos],
             matchesCount[pos] * sizeof(LZMatch));

      if (matchesCount[pos] != 0) {
        if (matchesEnd[-1].length >= compressorOptions.niceLength) {
          // Fill all the following positions with the match we have found
          size_t length = matchesEnd[-1].length - 1;

          for (size_t off = 1; off < compressorOptions.niceLength; off++)
            matchFinder->update_position(in + pos + off, inputStart,
                                         compressionLimit, windowLog,
                                         compressorOptions);

          for (pos++; length != 0; pos++, length--) {
            if (length > 2) {
              matches[pos * maxMatchesPerPosition].length = length;
              matches[pos * maxMatchesPerPosition].distance =
                  matchesEnd[-1].distance;
            }
            matchesCount[pos] = length > 2;
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
    LZStructure* stream, const size_t* startRepOffsets, size_t* acceleration,
    const size_t accelerationThreshold, HuffmanSymbol* literalSymbols[4],
    HuffmanSymbol* tokenSymbols, HuffmanSymbol* lengthSymbols,
    HuffmanSymbol* distanceSymbols, const bool disableHuffman,
    const bool advancedDistanceCoding, int literalMode,
    const bool bufferedMatches, const CompressorOptions& compressorOptions,
    const int windowLog) {
  const size_t MAX_LENGTH_REDUCTION = disableHuffman ? 0 : 15;

  const size_t blockLength =
      std::min((size_t)(blockLimit - input),
               (size_t)compressorOptions.optimalBlockSize - 1);
  // We only need to initialize the first arrival
  for (size_t i = 0; i < compressorOptions.niceLength; i++)
    parser[i * compressorOptions.maxArrivals].cost = UINT32_MAX;
  // Fill only the first arrival of the first position
  parser[0].cost = 64 << 12;
  parser[0].repOffsets[0] = startRepOffsets[0];
  parser[0].repOffsets[1] = startRepOffsets[1];
  parser[0].repOffsets[2] = startRepOffsets[2];
  parser[0].literalRunLength = 0;
  parser[1].cost = UINT32_MAX;

  size_t lastMatchLength = 0;
  size_t lastMatchDistance;
  size_t lastMatchPath;

  size_t position = 0;
  for (; position < blockLength; position++) {
    const size_t blockPos = input + position - blockStart;
    const uint8_t* inputPosition = input + position;
    SkandaOptimalParserState* const parserPosition =
        parser + position * compressorOptions.maxArrivals;
    // Initialize position ahead. Only first arrival is needed
    parserPosition[compressorOptions.niceLength * compressorOptions.maxArrivals]
        .cost = UINT32_MAX;

    size_t nextExpectedLength = 2;  // Only take matches as long as this

    for (size_t path = 0; path < compressorOptions.maxArrivals; path++) {
      SkandaOptimalParserState* const currentArrival = parserPosition + path;
      // Break literal/rep parsing if current arrival has a very high cost
      // Will also break if there are no more arrivals for this position
      if (currentArrival->cost - 0x1000 * compressorOptions.maxArrivals >=
          parserPosition[compressorOptions.maxArrivals].cost)
        break;

      size_t literalCost = currentArrival->cost;
      // size cost
      if (disableHuffman)
        literalCost += 0x8000 << (currentArrival->literalRunLength == 6);
      else {
        if (currentArrival->literalRunLength >= 6) {
          literalCost +=
              get_length_cost(currentArrival->literalRunLength + 1 - 7,
                              lengthSymbols)
              << 12;
          if (currentArrival->literalRunLength >= 7)
            literalCost -= get_length_cost(currentArrival->literalRunLength - 7,
                                           lengthSymbols)
                           << 12;
        }

        if (literalMode == 0)
          literalCost += literalSymbols[0][*inputPosition].bits << 12;
        else if (literalMode == STREAM_LITERALS_DELTA) {
          uint8_t delta = inputPosition[0] -
                          *(inputPosition - currentArrival->repOffsets[0]);
          literalCost += literalSymbols[0][delta].bits << 12;
        } else if (literalMode == STREAM_LITERALS_POS_MASK3) {
          size_t pos = (inputPosition - inputStart) & 3;
          literalCost += literalSymbols[pos][*inputPosition].bits << 12;
        } else if (literalMode ==
                   (STREAM_LITERALS_DELTA | STREAM_LITERALS_POS_MASK3)) {
          size_t pos = (inputPosition - inputStart) & 3;
          uint8_t delta = inputPosition[0] -
                          *(inputPosition - currentArrival->repOffsets[0]);
          literalCost += literalSymbols[pos][delta].bits << 12;
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
          memcpy(arrivalIt->repOffsets, currentArrival->repOffsets,
                 sizeof(uint32_t) * 3);
          arrivalIt->literalRunLength = currentArrival->literalRunLength + 1;
          arrivalIt->path = path;
          break;
        }

        // If there is already an arrival with this offset, and it has lower
        // cost,
        //  skip this match. This keeps more interesting offsets in the
        //  arrivals.
        if (arrivalIt->repOffsets[0] == currentArrival->repOffsets[0]) break;
      }

      size_t startRep = currentArrival->literalRunLength == 0;
      for (int rep = 0; rep < (advancedDistanceCoding ? 3 : 1); rep++) {
        size_t repMatchLength = test_match<false>(
            inputPosition, inputPosition - currentArrival->repOffsets[rep],
            blockLimit, 2, 0);
        // Since we go from lowest cost arrival to highest, it makes sense that
        // the rep match
        //  should be at least as long as the best found so far
        if (repMatchLength >= nextExpectedLength) {
          // Rep matches can be unconditionally taken with lower lengths
          if (repMatchLength >= compressorOptions.niceLength / 2) {
            lastMatchLength = repMatchLength;
            lastMatchDistance = currentArrival->repOffsets[rep];
            lastMatchPath = path;
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
              repMatchCost += 0x8000 << (length > 8);
            } else {
              repMatchCost += get_token_cost(currentArrival->literalRunLength,
                                             length, rep, tokenSymbols)
                              << 12;
              if (length > 8)
                repMatchCost += get_length_cost(length - 9, lengthSymbols)
                                << 12;
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
                arrivalIt->repOffsets[2] =
                    currentArrival->repOffsets[rep < 2 ? 2 : 1];
                arrivalIt->repOffsets[1] =
                    currentArrival->repOffsets[rep < 1 ? 1 : 0];
                arrivalIt->repOffsets[0] = currentArrival->repOffsets[rep];
                arrivalIt->literalRunLength = 0;
                arrivalIt->path = path;
                break;
              }

              // If there is already an arrival with this offset, and it has
              // lower cost,
              //  skip this match. This keeps more interesting offsets in the
              //  arrivals.
              if (arrivalIt->repOffsets[0] == currentArrival->repOffsets[0])
                break;
            }
          }

          nextExpectedLength = repMatchLength;
          if (repMatchLength >= 3) *acceleration = 1 << accelerationThreshold;
        }
      }
    }

    // Unpromising position
    if (parserPosition[0].cost - 0x1000 * (compressorOptions.maxArrivals / 8) >=
        parserPosition[compressorOptions.maxArrivals].cost) {
      if (!bufferedMatches)
        matchFinder->update_position(inputPosition, inputStart,
                                     compressionLimit, windowLog,
                                     compressorOptions);
      continue;
    }

    LZMatch matchFinderBuf[160];
    LZMatch *matches, *matchesEnd;
    if (!bufferedMatches) {
      matchesEnd = matchFinder->find_matches_and_update(
          inputPosition, inputStart, compressionLimit, blockLimit,
          matchFinderBuf, nextExpectedLength, windowLog, compressorOptions);
      matches = matchFinderBuf;
    } else
      matchBuffer->get_position_matches(&matches, &matchesEnd, blockPos);

    if (matchesEnd == matches) {
      // Disable acceleration on level 9
      if (compressorOptions.parser < OPTIMAL3) {
        (*acceleration)++;
        // If acceleration goes too high go to greedy parse
        if (*acceleration >= (2 << accelerationThreshold)) {
          position++;
          break;
        }
      }
      continue;
    }

    // We have the guarantee that matches are in increasing order
    const LZMatch* const longestMatch = matchesEnd - 1;
    if (longestMatch->length >= compressorOptions.niceLength) {
      lastMatchLength = longestMatch->length;
      lastMatchDistance = longestMatch->distance;
      lastMatchPath = 0;
      break;
    }
    if (longestMatch->length >= 4) *acceleration = 1 << accelerationThreshold;

    // Not much gain from updating more than 2 paths for normal matches
    size_t pathMax = std::min(compressorOptions.maxArrivals, 2);
    for (size_t path = 0; path < pathMax; path++) {
      SkandaOptimalParserState* const currentArrival = parserPosition + path;
      if (currentArrival->cost - 0x1000 * compressorOptions.maxArrivals >=
          parserPosition[compressorOptions.maxArrivals].cost)
        break;
      // We have the guarantee that matches are in increasing order
      size_t nextMatchReductionLimit = nextExpectedLength;

      for (LZMatch* matchIt = matches; matchIt != matchesEnd; matchIt++) {
        if (matchIt->distance == currentArrival->repOffsets[0] ||
            matchIt->length < nextExpectedLength)
          continue;

        size_t matchLengthReductionLimit =
            matchIt->length == nextMatchReductionLimit
                ? nextMatchReductionLimit
                : nextMatchReductionLimit + 1;
        if (matchIt->length - matchLengthReductionLimit > MAX_LENGTH_REDUCTION)
          matchLengthReductionLimit = matchIt->length - MAX_LENGTH_REDUCTION;
        nextMatchReductionLimit = matchIt->length;

        size_t baseMatchCost = currentArrival->cost;
        // size cost
        if (disableHuffman) {
          if (!advancedDistanceCoding)
            baseMatchCost += (2 + unsafe_int_log2(matchIt->distance) / 8) << 15;
          else
            baseMatchCost +=
                (16 + unsafe_int_log2((matchIt->distance + 7) >> 3)) << 12;
        } else {
          if (!advancedDistanceCoding) {
            size_t distanceBytes = unsafe_int_log2(matchIt->distance) / 8;
            for (int i = 0; i <= distanceBytes; i++)
              baseMatchCost +=
                  distanceSymbols[(matchIt->distance >> i * 8) & 0xFF].bits
                  << 12;
          } else {
            size_t virtualDistance = matchIt->distance + 7;
            size_t bits = unsafe_int_log2(virtualDistance >> 3);
            baseMatchCost +=
                distanceSymbols[((virtualDistance & 7) ^ 7) | (bits << 3)].bits
                << 12;
            baseMatchCost += bits << 12;
          }
        }
        // speed cost
        baseMatchCost += 1 + (matchIt->distance > (1 << 16));

        // Start search at the highest length. Stop when we reach the limit,
        for (size_t length = matchIt->length;
             length >= matchLengthReductionLimit; length--) {
          size_t matchCost = baseMatchCost;
          // size cost
          if (disableHuffman) {
            // For decoding speed
            if (length > 8 && length <= 16) length = 8;
            matchCost += (length > 8) << 15;
          } else {
            size_t distanceBits =
                (advancedDistanceCoding)
                    ? 3
                    : (unsafe_int_log2(matchIt->distance) / 8 + 1);
            matchCost += get_token_cost(currentArrival->literalRunLength,
                                        length, distanceBits, tokenSymbols)
                         << 12;
            if (length > 8)
              matchCost += get_length_cost(length - 9, lengthSymbols) << 12;
          }
          // speed cost
          matchCost += (length > 8) * 2;

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
              arrivalIt->repOffsets[2] = currentArrival->repOffsets[1];
              arrivalIt->repOffsets[1] = currentArrival->repOffsets[0];
              arrivalIt->repOffsets[0] = matchIt->distance;
              arrivalIt->literalRunLength = 0;
              arrivalIt->path = path;
              break;
            }

            if (arrivalIt->repOffsets[0] == matchIt->distance) break;
          }
        }
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

    if (!bufferedMatches) {
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
      stream->matchDistance = backwardParse[path].repOffsets[0];
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

class BlockSplitter {
  AllocatorCallback* allocator;
  size_t maxSubdivisions;
  size_t subdivisionSize;
  uint32_t* literalBlockHist = nullptr;
  uint32_t* tokenBlockHist = nullptr;
  uint32_t* distanceBlockHist = nullptr;
  uint32_t* lengthBlockHist = nullptr;
  HashTable<uint32_t, FastIntHash> lzdict3;
  HashTable<uint32_t, FastIntHash> lzdict6;
  int reuseSubdivisions;

  int literalMode;
  HuffmanSymbol literalCodes[4][256];
  HuffmanSymbol tokenCodes[256];
  HuffmanSymbol distanceCodes[256];
  HuffmanSymbol lengthCodes[256];

  FORCE_INLINE void aproximator_encode_length(uint32_t* lengthHist, size_t var,
                                              const size_t overflow) {
    var -= overflow;
    if (likely(var <= 223))
      lengthHist[var]++;
    else {
      var -= 224;
      lengthHist[224 | (var & 0x1F)]++;
      var >>= 5;

      if (likely(var <= 223))
        lengthHist[var]++;
      else {
        var -= 224;
        lengthHist[224 | (var & 0x1F)]++;
        lengthHist[var >> 5]++;
      }
    }
  }

  FORCE_INLINE void aproximator_encode_literal_run(
      uint32_t* literalHist, uint32_t* lengthHist, const uint8_t* const input,
      const uint8_t* literalRun, uint8_t* const controlByte,
      const uint8_t* const inputStart, const size_t lastDistance,
      CodingOptions codingOptions) {
    const size_t literalRunLength = input - literalRun;

    if (literalRunLength >= 7) {
      *controlByte = (7 << 5);
      aproximator_encode_length(lengthHist, literalRunLength, 7);
    } else
      *controlByte = (literalRunLength << 5);

    for (; literalRun < input; literalRun++) {
      literalHist[literalRun[0]]++;
      if (codingOptions.advancedLiteralCoding) {
        int pos = (literalRun - inputStart) & 3;
        literalHist[512 + pos * 256 + literalRun[0]]++;
        if ((size_t)(literalRun - inputStart) >= lastDistance) {
          uint8_t deltaLiteral = literalRun[0] - *(literalRun - lastDistance);
          literalHist[256 + deltaLiteral]++;
          literalHist[1536 + pos * 256 + deltaLiteral]++;
        }
      }
    }
  }

  FORCE_INLINE void aproximator_encode_match(
      uint32_t* tokenHist, uint32_t* lengthHist, uint32_t* distanceHist,
      uint8_t* const controlByte, size_t* matchLength, size_t distance,
      size_t* repOffsets, const bool advancedDistanceCoding) {
    if (!advancedDistanceCoding) {
      if (distance != repOffsets[0]) {
        int bytes = unsafe_int_log2(distance) / 8 + 1;
        *controlByte |= bytes << 3;
        repOffsets[0] = distance;
        for (int i = 0; i < bytes; i++)
          distanceHist[(distance >> i * 8) & 0xFF]++;
      }
    } else {
      // Seems faster than a for loop
      if (repOffsets[0] == distance) {
      } else if (repOffsets[1] == distance) {
        *controlByte |= 1 << 3;
        std::swap(repOffsets[0], repOffsets[1]);
      } else if (repOffsets[2] == distance) {
        *controlByte |= 2 << 3;
        repOffsets[2] = repOffsets[1];
        repOffsets[1] = repOffsets[0];
        repOffsets[0] = distance;
      } else {
        // Encode normal offset
        *controlByte |= 3 << 3;
        repOffsets[2] = repOffsets[1];
        repOffsets[1] = repOffsets[0];
        repOffsets[0] = distance;

        distance += 7;
        size_t bits = unsafe_int_log2(distance >> 3);
        size_t distanceToken = ((distance & 7) ^ 7) | (bits << 3);
        distanceHist[distanceToken]++;
      }
    }

    if (*matchLength <= 8) {
      *matchLength -= SKANDA_MIN_MATCH_LENGTH;
      tokenHist[*controlByte | *matchLength]++;
    } else {
      tokenHist[*controlByte | 7]++;
      aproximator_encode_length(lengthHist, *matchLength, 9);
    }
  }

  void aproximate_symbol_histogram(const uint8_t* input,
                                   const uint8_t* inputStart, uint8_t* output,
                                   const size_t size, uint32_t* literalHist,
                                   uint32_t* tokenHist, uint32_t* distanceHist,
                                   uint32_t* lengthHist,
                                   CodingOptions codingOptions) {
    const size_t accelerationThreshold = 6;
    const uint8_t* const thisBlockEnd = input + size;
    size_t repOffsets[3] = {1, 1, 1};
    size_t acceleration = 1 << accelerationThreshold;

    memset(literalHist, 0, 256 * 10 * sizeof(uint32_t));
    memset(tokenHist, 0, 256 * sizeof(uint32_t));
    memset(distanceHist, 0, 256 * sizeof(uint32_t));
    memset(lengthHist, 0, 256 * sizeof(uint32_t));

    const uint8_t* literalRunStart = input;
    // Skip first byte
    input++;

    while (likely(input < thisBlockEnd)) {
      // First try a rep match
      size_t repMatchLength = test_match<false>(
          input + 1, input + 1 - repOffsets[0], thisBlockEnd, 2, 0);
      if (repMatchLength) {
        lzdict3[read_hash3(input + 0)] = (input + 0 - inputStart);
        lzdict3[read_hash3(input + 1)] = (input + 1 - inputStart);
        lzdict3[read_hash3(input + 2)] = (input + 2 - inputStart);
        lzdict6[read_hash6(input + 1)] = (input + 1 - inputStart);

        input++;
        uint8_t controlByte;
        aproximator_encode_literal_run(
            literalHist, lengthHist, input, literalRunStart, &controlByte,
            inputStart, repOffsets[0], codingOptions);

        input += repMatchLength;
        literalRunStart = input;
        aproximator_encode_match(
            tokenHist, lengthHist, distanceHist, &controlByte, &repMatchLength,
            repOffsets[0], repOffsets, codingOptions.advancedDistanceCoding);
        acceleration = 1 << accelerationThreshold;
        continue;
      }

      if (codingOptions.advancedDistanceCoding) {
        for (int rep = 1; rep < 3; rep++) {
          repMatchLength = test_match<false>(input, input - repOffsets[rep],
                                             thisBlockEnd, 2, 0);
          if (repMatchLength) {
            lzdict3[read_hash3(input + 0)] = (input + 0 - inputStart);
            lzdict3[read_hash3(input + 1)] = (input + 1 - inputStart);
            lzdict6[read_hash6(input + 0)] = (input + 0 - inputStart);

            uint8_t controlByte;
            aproximator_encode_literal_run(
                literalHist, lengthHist, input, literalRunStart, &controlByte,
                inputStart, repOffsets[0], codingOptions);

            input += repMatchLength;
            literalRunStart = input;
            aproximator_encode_match(tokenHist, lengthHist, distanceHist,
                                     &controlByte, &repMatchLength,
                                     repOffsets[rep], repOffsets, true);
            acceleration = 1 << accelerationThreshold;
            break;
          }
        }
        if (repMatchLength) continue;
      }

      uint32_t* dictEntry3 = &(lzdict3)[read_hash3(input)];
      uint32_t pos3 = *dictEntry3;
      *dictEntry3 = input - inputStart;

      const uint8_t* where = inputStart + pos3;
      size_t matchLength = test_match<true>(input, where, thisBlockEnd, 3,
                                            SKANDA_MAX_WINDOW_LOG);
      size_t distance = input - where;

      if (matchLength && (matchLength >= 4 || distance < 65536)) {
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
      aproximator_encode_literal_run(literalHist, lengthHist, input,
                                     literalRunStart, &controlByte, inputStart,
                                     repOffsets[0], codingOptions);

      input += matchLength;
      literalRunStart = input;
      aproximator_encode_match(tokenHist, lengthHist, distanceHist,
                               &controlByte, &matchLength, distance, repOffsets,
                               codingOptions.advancedDistanceCoding);
      if (matchLength > 3) acceleration = 1 << accelerationThreshold;
    }

    // Maybe we went beyond block end because of acceleration
    input = thisBlockEnd;
    // Send last literal run
    if (input != literalRunStart) {
      uint8_t controlByte;
      aproximator_encode_literal_run(literalHist, lengthHist, input,
                                     literalRunStart, &controlByte, inputStart,
                                     repOffsets[0], codingOptions);
      tokenHist[controlByte]++;
    }
  }

  size_t get_symbol_count(uint32_t* histogram) {
    size_t c = 0;
    for (size_t i = 0; i < 256; i++) c += histogram[i];
    return c;
  }

  size_t get_encoded_size(
      float decSpeedBias, int* bestLiteralMode, CodingOptions codingOptions,
      bool useDeltaBias, uint32_t* literalHistogram, uint32_t* tokenHistogram,
      uint32_t* distanceHistogram, uint32_t* lengthHistogram,
      HuffmanSymbol* literalSymbols, HuffmanSymbol* tokenSymbols,
      HuffmanSymbol* distanceSymbols, HuffmanSymbol* lengthSymbols) {
    *bestLiteralMode = 0;
    size_t symbolCount = get_symbol_count(&literalHistogram[0]);
    size_t bestLiteralsSize = get_encoded_huffman_info(
        &literalHistogram[0], symbolCount, &literalSymbols[0], decSpeedBias,
        LITERAL_STREAM);

    if (codingOptions.advancedLiteralCoding) {
      // I feel this bias is a bit of a hacky way to get the encoder to use
      // delta literals
      //  more often on level 9. The problem is that the only thing that can
      //  accurately detect when to use them is the same parser that the level
      //  uses, which would be too big of an impact on speeds... So for now this
      //  is the best we have.
      float deltaBias = useDeltaBias ? (decSpeedBias - 0.6f) / 8 : 0;

      size_t deltaLiteralsSize =
          get_encoded_huffman_info(&literalHistogram[256], symbolCount,
                                   &literalSymbols[256], decSpeedBias,
                                   LITERAL_STREAM) +
          symbolCount * (decSpeedBias / 4 + 0.05f / 8 + deltaBias);
      if (deltaLiteralsSize < bestLiteralsSize) {
        bestLiteralsSize = deltaLiteralsSize;
        *bestLiteralMode = STREAM_LITERALS_DELTA;
      }

      size_t pos3LiteralsSize = 0;
      for (int i = 0; i < 4; i++) {
        bool useDelta = *bestLiteralMode & STREAM_LITERALS_DELTA;
        size_t pos3Index = (useDelta ? 1536 : 512) + i * 256;
        size_t streamSymbolCount =
            get_symbol_count(&literalHistogram[pos3Index]);

        pos3LiteralsSize += get_encoded_huffman_info(
            &literalHistogram[pos3Index], streamSymbolCount,
            &literalSymbols[pos3Index], decSpeedBias, LITERAL_STREAM);
        // Pos3 mask and delta literals has a higher penalty because its slower
        // to decode
        pos3LiteralsSize +=
            useDelta
                ? streamSymbolCount * (decSpeedBias / 2 + 0.05f / 8 + deltaBias)
                : streamSymbolCount * (decSpeedBias / 4 + 0.05f / 8);
      }

      if (pos3LiteralsSize < bestLiteralsSize) {
        bestLiteralsSize = pos3LiteralsSize;
        *bestLiteralMode |= STREAM_LITERALS_POS_MASK3;
      }
    }

    size_t encodedSize =
        bestLiteralsSize +
        get_encoded_huffman_info(tokenHistogram,
                                 get_symbol_count(tokenHistogram), tokenSymbols,
                                 decSpeedBias, TOKEN_STREAM) +
        get_encoded_huffman_info(
            distanceHistogram, get_symbol_count(distanceHistogram),
            distanceSymbols, decSpeedBias, DISTANCE_STREAM) +
        get_encoded_huffman_info(lengthHistogram,
                                 get_symbol_count(lengthHistogram),
                                 lengthSymbols, decSpeedBias, LENGTH_STREAM);
    return encodedSize;
  }

  void store_codes(int bestLiteralMode, HuffmanSymbol* literalSymbols,
                   HuffmanSymbol* tokenSymbols, HuffmanSymbol* distanceSymbols,
                   HuffmanSymbol* lengthSymbols) {
    literalMode = bestLiteralMode;
    if (bestLiteralMode == 0)
      memcpy(literalCodes, &literalSymbols[0], 256 * sizeof(HuffmanSymbol));
    else if (bestLiteralMode == STREAM_LITERALS_DELTA)
      memcpy(literalCodes, &literalSymbols[256], 256 * sizeof(HuffmanSymbol));
    else if (bestLiteralMode == STREAM_LITERALS_POS_MASK3)
      memcpy(literalCodes, &literalSymbols[512], 1024 * sizeof(HuffmanSymbol));
    else if (bestLiteralMode ==
             (STREAM_LITERALS_POS_MASK3 | STREAM_LITERALS_DELTA))
      memcpy(literalCodes, &literalSymbols[1536], 1024 * sizeof(HuffmanSymbol));
    memcpy(tokenCodes, tokenSymbols, 256 * sizeof(HuffmanSymbol));
    memcpy(distanceCodes, distanceSymbols, 256 * sizeof(HuffmanSymbol));
    memcpy(lengthCodes, lengthSymbols, 256 * sizeof(HuffmanSymbol));
  }

 public:
  BlockSplitter(AllocatorCallback* alloc) : lzdict3(alloc), lzdict6(alloc) {
    allocator = alloc;
  }
  ~BlockSplitter() {
    allocator->free(literalBlockHist);
    allocator->free(tokenBlockHist);
    allocator->free(distanceBlockHist);
    allocator->free(lengthBlockHist);
  }

  void init(int dictLog, int subdivisions, CodingOptions codingOptions) {
    maxSubdivisions = subdivisions;
    subdivisionSize = (MAX_BLOCK_SIZE + 1) / maxSubdivisions;
    literalBlockHist = (uint32_t*)allocator->allocate(
        sizeof(uint32_t) * maxSubdivisions * 256 * 10);
    tokenBlockHist = (uint32_t*)allocator->allocate(sizeof(uint32_t) *
                                                    maxSubdivisions * 256);
    distanceBlockHist = (uint32_t*)allocator->allocate(sizeof(uint32_t) *
                                                       maxSubdivisions * 256);
    lengthBlockHist = (uint32_t*)allocator->allocate(sizeof(uint32_t) *
                                                     maxSubdivisions * 256);
    if (dictLog > 20) dictLog = 20;
    if (dictLog < 1) dictLog = 1;
    lzdict3.init(dictLog);
    lzdict6.init(dictLog);
    reuseSubdivisions = 0;
  }

  size_t get_block_size(const uint8_t* input, const uint8_t* const inputStart,
                        uint8_t* output, size_t maxSize,
                        const CodingOptions codingOptions, bool useDeltaBias,
                        float decSpeedBias) {
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
          &literalBlockHist[i * 256 * 10], &tokenBlockHist[i * 256],
          &distanceBlockHist[i * 256], &lengthBlockHist[i * 256],
          codingOptions);
    }

    uint32_t literalHistogramLeft[256 * 10], tokenHistogramLeft[256],
        distanceHistogramLeft[256], lengthHistogramLeft[256],
        literalHistogramRight[256 * 10], tokenHistogramRight[256],
        distanceHistogramRight[256], lengthHistogramRight[256];
    HuffmanSymbol literalSymbolsLeft[256 * 10], tokenSymbolsLeft[256],
        distanceSymbolsLeft[256], lengthSymbolsLeft[256],
        literalSymbolsRight[256 * 10], tokenSymbolsRight[256],
        distanceSymbolsRight[256], lengthSymbolsRight[256];

    memcpy(literalHistogramLeft, literalBlockHist, 256 * 10 * sizeof(uint32_t));
    memcpy(tokenHistogramLeft, tokenBlockHist, 256 * sizeof(uint32_t));
    memcpy(distanceHistogramLeft, distanceBlockHist, 256 * sizeof(uint32_t));
    memcpy(lengthHistogramLeft, lengthBlockHist, 256 * sizeof(uint32_t));
    memset(literalHistogramRight, 0, 256 * 10 * sizeof(uint32_t));
    memset(tokenHistogramRight, 0, 256 * sizeof(uint32_t));
    memset(distanceHistogramRight, 0, 256 * sizeof(uint32_t));
    memset(lengthHistogramRight, 0, 256 * sizeof(uint32_t));

    for (size_t i = 1; i < subdivisionCount; i++) {
      for (size_t j = 0; j < 256 * 10; j++)
        literalHistogramLeft[j] += literalBlockHist[i * 256 * 10 + j];
      for (size_t j = 0; j < 256; j++) {
        tokenHistogramLeft[j] += tokenBlockHist[i * 256 + j];
        distanceHistogramLeft[j] += distanceBlockHist[i * 256 + j];
        lengthHistogramLeft[j] += lengthBlockHist[i * 256 + j];
      }
    }

    int bestLiteralMode;
    size_t bestEncodedSize = get_encoded_size(
        decSpeedBias, &bestLiteralMode, codingOptions, useDeltaBias,
        literalHistogramLeft, tokenHistogramLeft, distanceHistogramLeft,
        lengthHistogramLeft, literalSymbolsLeft, tokenSymbolsLeft,
        distanceSymbolsLeft, lengthSymbolsLeft);
    size_t bestBlockSize = maxSize;

    // Store the codes to later use them as initial estimation for optimal parse
    store_codes(bestLiteralMode, literalSymbolsLeft, tokenSymbolsLeft,
                distanceSymbolsLeft, lengthSymbolsLeft);

    for (int i = subdivisionCount - 1; i > 0; i--) {
      // Move the split point; change histograms accordingly
      for (size_t j = 0; j < 256 * 10; j++) {
        literalHistogramLeft[j] -= literalBlockHist[i * 256 * 10 + j];
        literalHistogramRight[j] += literalBlockHist[i * 256 * 10 + j];
      }
      for (size_t j = 0; j < 256; j++) {
        tokenHistogramLeft[j] -= tokenBlockHist[i * 256 + j];
        distanceHistogramLeft[j] -= distanceBlockHist[i * 256 + j];
        lengthHistogramLeft[j] -= lengthBlockHist[i * 256 + j];
        tokenHistogramRight[j] += tokenBlockHist[i * 256 + j];
        distanceHistogramRight[j] += distanceBlockHist[i * 256 + j];
        lengthHistogramRight[j] += lengthBlockHist[i * 256 + j];
      }

      size_t newLeftEncodedSize = get_encoded_size(
          decSpeedBias, &bestLiteralMode, codingOptions, useDeltaBias,
          literalHistogramLeft, tokenHistogramLeft, distanceHistogramLeft,
          lengthHistogramLeft, literalSymbolsLeft, tokenSymbolsLeft,
          distanceSymbolsLeft, lengthSymbolsLeft);

      int dummy;
      size_t newRightEncodedSize = get_encoded_size(
          decSpeedBias, &dummy, codingOptions, useDeltaBias,
          literalHistogramRight, tokenHistogramRight, distanceHistogramRight,
          lengthHistogramRight, literalSymbolsRight, tokenSymbolsRight,
          distanceSymbolsRight, lengthSymbolsRight);

      // Making a split here does help
      if (newLeftEncodedSize + newRightEncodedSize < bestEncodedSize) {
        bestEncodedSize = newLeftEncodedSize;
        bestBlockSize = i * subdivisionSize;
        // Reset right split histogram
        memset(literalHistogramRight, 0, 256 * 10 * sizeof(uint32_t));
        memset(tokenHistogramRight, 0, 256 * sizeof(uint32_t));
        memset(distanceHistogramRight, 0, 256 * sizeof(uint32_t));
        memset(lengthHistogramRight, 0, 256 * sizeof(uint32_t));
        // Store the codes to later use them as initial estimation for optimal
        // parse
        store_codes(bestLiteralMode, literalSymbolsLeft, tokenSymbolsLeft,
                    distanceSymbolsLeft, lengthSymbolsLeft);
      }
    }

    // Save existing histograms for next block splitting
    if (bestBlockSize != maxSize) {
      int usedSubdivisions = bestBlockSize / subdivisionSize;
      reuseSubdivisions = subdivisionCount - usedSubdivisions;
      memmove(&literalBlockHist[0],
              &literalBlockHist[usedSubdivisions * 256 * 10],
              reuseSubdivisions * 256 * 10 * sizeof(uint32_t));
      memmove(&tokenBlockHist[0], &tokenBlockHist[usedSubdivisions * 256],
              reuseSubdivisions * 256 * sizeof(uint32_t));
      memmove(&distanceBlockHist[0], &distanceBlockHist[usedSubdivisions * 256],
              reuseSubdivisions * 256 * sizeof(uint32_t));
      memmove(&lengthBlockHist[0], &lengthBlockHist[usedSubdivisions * 256],
              reuseSubdivisions * 256 * sizeof(uint32_t));
    } else
      reuseSubdivisions = 0;
    return bestBlockSize;
  }

  int get_literal_mode() { return literalMode; }

  HuffmanSymbol* get_huffman_codes(bool* uncompressed, int streamID,
                                   int substream) {
    HuffmanSymbol* symbols = nullptr;
    switch (streamID) {
      case LITERAL_STREAM:
        symbols = literalCodes[substream];
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
                        AllocatorCallback* allocator,
                        ProgressCallback* progress) {
  const size_t accelerationThreshold = 6;
  const uint8_t* const inputStart = input;
  const uint8_t* const outputStart = output;
  const uint8_t* const compressionLimit = input + size - SKANDA_LAST_BYTES;

  size_t repOffsets[3] = {1, 1, 1};
  size_t acceleration = 1 << accelerationThreshold;

  CodingOptions codingOptions(decSpeedBias);
  size_t huffmanBufSize = std::min(MAX_BLOCK_SIZE, size);

  HashTableMatchFinder hashMatchFinder(allocator);
  BinaryMatchFinder binaryMatchFinder(allocator);
  SkandaOptimalParserState* parser = nullptr;
  LZStructure* stream = nullptr;
  MatchBuffer matchBuffer(allocator);
  uint8_t* huffmanStreamBuffer = nullptr;
  uint8_t* huffmanStreamBufferBegin = nullptr;
  uint8_t* deltaLiteralsBuffer = nullptr;
  BlockSplitter blockSplitter(allocator);

  try {
    if (compressorOptions.parser == OPTIMAL1) {
      hashMatchFinder.init(windowLog, compressorOptions);
      parser = (SkandaOptimalParserState*)allocator->allocate(
          sizeof(SkandaOptimalParserState) *
          (compressorOptions.optimalBlockSize + compressorOptions.niceLength));
    } else {
      binaryMatchFinder.init(size, windowLog, compressorOptions);
      parser = (SkandaOptimalParserState*)allocator->allocate(
          sizeof(SkandaOptimalParserState) *
          (compressorOptions.optimalBlockSize + compressorOptions.niceLength) *
          compressorOptions.maxArrivals);
      if (!codingOptions.disableHuffman &&
          compressorOptions.parserIterations > 1)
        matchBuffer.init(size, compressorOptions);
    }
    stream = new LZStructure[compressorOptions.optimalBlockSize];
    if (codingOptions.advancedLiteralCoding)
      deltaLiteralsBuffer = new uint8_t[huffmanBufSize + 128];
    if (!codingOptions.disableHuffman) {
      huffmanStreamBuffer = new uint8_t[huffmanBufSize + 128];
      huffmanStreamBufferBegin = huffmanStreamBuffer + huffmanBufSize + 128;
      blockSplitter.init(std::min(windowLog - 4, compressorOptions.maxHashLog),
                         compressorOptions.parser >= OPTIMAL2 ? 64 : 4,
                         codingOptions);
    }
  } catch (const std::bad_alloc& e) {
    allocator->free(parser);
    allocator->free(stream);
    allocator->free(huffmanStreamBuffer);
    allocator->free(deltaLiteralsBuffer);
    return ERROR_NOMEM;
  }

  EntropyEncoder literalEncoder[8];
  EntropyEncoder tokenEncoder;
  EntropyEncoder lengthEncoder;
  EntropyEncoder distanceEncoder;
  BitEncoder bitDistanceEncoder;

  if (initialize_entropy_coders(literalEncoder, &tokenEncoder, &distanceEncoder,
                                &lengthEncoder, &bitDistanceEncoder,
                                codingOptions, huffmanBufSize, allocator)) {
    allocator->free(parser);
    allocator->free(stream);
    allocator->free(huffmanStreamBuffer);
    allocator->free(deltaLiteralsBuffer);
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
    float literalDecSpeedBias[4];
    std::fill_n(literalDecSpeedBias, 4, decSpeedBias);
    float tokenDecSpeedBias = decSpeedBias;
    float distanceDecSpeedBias = decSpeedBias;
    float lengthDecSpeedBias = decSpeedBias;
    bool blockNoHuffman = codingOptions.disableHuffman;
    bool bufferedMatches = false;
    int literalMode = 0;

    size_t thisBlockSize = (compressionLimit - input < MAX_BLOCK_SIZE)
                               ? compressionLimit - input
                               : MAX_BLOCK_SIZE;
    if (!codingOptions.disableHuffman) {
      bool useDeltaBias = compressorOptions.parser >= OPTIMAL3;
      thisBlockSize = blockSplitter.get_block_size(input, inputStart, output,
                                                   thisBlockSize, codingOptions,
                                                   useDeltaBias, decSpeedBias);
      literalMode = blockSplitter.get_literal_mode();
      codingOptions.posBits = (literalMode & STREAM_LITERALS_POS_MASK3) ? 2 : 0;
    }

    const uint8_t* const thisBlockEnd = input + thisBlockSize;
    write_header(output, thisBlockSize, BLOCK_COMPRESSED, 0);
    // Output first byte of first block
    if (input == inputStart) *output++ = *input++;
    const uint8_t* const thisBlockStart = input;

    size_t startRepOffsets[3];
    memcpy(startRepOffsets, repOffsets, sizeof(size_t) * 3);

    for (int iteration = 0;
         iteration < (compressorOptions.parser < OPTIMAL2 || blockNoHuffman
                          ? 1
                          : compressorOptions.parserIterations);
         iteration++) {
      HuffmanSymbol *literalSymbols[4] = {nullptr}, *tokenSymbols = nullptr,
                    *lengthSymbols = nullptr, *distanceSymbols = nullptr;

      if (!blockNoHuffman) {
        if (iteration == 0) {
          blockNoHuffman = true;
          bool uncompressedStream;

          for (int stream = 0; stream < (1 << codingOptions.posBits);
               stream++) {
            literalSymbols[stream] = blockSplitter.get_huffman_codes(
                &uncompressedStream, LITERAL_STREAM, stream);
            literalDecSpeedBias[stream] = uncompressedStream ? 1 : 0;
            blockNoHuffman &= uncompressedStream;
          }
          tokenSymbols = blockSplitter.get_huffman_codes(&uncompressedStream,
                                                         TOKEN_STREAM, 0);
          tokenDecSpeedBias = uncompressedStream ? 1 : 0;
          blockNoHuffman &= uncompressedStream;
          lengthSymbols = blockSplitter.get_huffman_codes(&uncompressedStream,
                                                          LENGTH_STREAM, 0);
          lengthDecSpeedBias = uncompressedStream ? 1 : 0;
          blockNoHuffman &= uncompressedStream;
          distanceSymbols = blockSplitter.get_huffman_codes(&uncompressedStream,
                                                            DISTANCE_STREAM, 0);
          distanceDecSpeedBias = uncompressedStream ? 1 : 0;
          blockNoHuffman &= uncompressedStream;

          if (!blockNoHuffman && compressorOptions.parser >= OPTIMAL2 &&
              compressorOptions.parserIterations > 1) {
            matchBuffer.find_matches(thisBlockStart, inputStart, thisBlockEnd,
                                     compressionLimit, &binaryMatchFinder,
                                     windowLog, compressorOptions);
            bufferedMatches = true;
          }
        } else {
          // Retest if delta coding helps. This is very dependent on the LZ
          // stage,
          //  so even if the block splitter said one mode was better that can be
          //  false.
          if (codingOptions.advancedLiteralCoding) {
            size_t rawSize = 0;
            size_t deltaSize = 0;

            for (int i = 0; i < (1 << codingOptions.posBits); i++) {
              size_t size;
              literalEncoder[0 + i].get_huffman_codes(literalDecSpeedBias[i],
                                                      &size);
              rawSize += size;
              literalEncoder[4 + i].get_huffman_codes(literalDecSpeedBias[i],
                                                      &size);
              deltaSize += size + literalEncoder[4 + i].get_symbol_count() *
                                      (decSpeedBias / 4 + 0.05f / 8);
            }

            literalMode = (literalMode & STREAM_LITERALS_POS_MASK3) |
                          ((deltaSize < rawSize) ? STREAM_LITERALS_DELTA : 0);
          }

          size_t dummy;
          bool delta = (literalMode & STREAM_LITERALS_DELTA);

          for (int stream = 0; stream < (1 << codingOptions.posBits); stream++)
            literalSymbols[stream] =
                literalEncoder[(delta ? 4 : 0) + stream].get_huffman_codes(
                    literalDecSpeedBias[stream], &dummy);
          tokenSymbols =
              tokenEncoder.get_huffman_codes(tokenDecSpeedBias, &dummy);
          lengthSymbols =
              lengthEncoder.get_huffman_codes(lengthDecSpeedBias, &dummy);
          distanceSymbols =
              distanceEncoder.get_huffman_codes(distanceDecSpeedBias, &dummy);
        }
      }

      start_entropy_blocks(literalEncoder, &tokenEncoder, &distanceEncoder,
                           &lengthEncoder, &bitDistanceEncoder, output,
                           deltaLiteralsBuffer, thisBlockSize);

      input = thisBlockStart;
      memcpy(repOffsets, startRepOffsets, sizeof(size_t) * 3);
      const uint8_t* literalRunStart = input;

      while (input < thisBlockEnd) {
        if (acceleration >= (2 << accelerationThreshold)) {
          LZMatch matchFinderBuf[160];
          LZMatch *matches, *matchesEnd;
          if (compressorOptions.parser >= OPTIMAL2) {
            if (!bufferedMatches) {
              matchesEnd = binaryMatchFinder.find_matches_and_update(
                  input, inputStart, compressionLimit, thisBlockEnd,
                  matchFinderBuf, 4, windowLog, compressorOptions);
              matches = matchFinderBuf;
            } else
              matchBuffer.get_position_matches(&matches, &matchesEnd,
                                               input - thisBlockStart);
          } else {
            matchesEnd = hashMatchFinder.find_matches_and_update(
                input, inputStart, thisBlockEnd, matchFinderBuf, 4, windowLog);
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
            encode_literal_run(literalEncoder, &lengthEncoder, input,
                               literalRunStart, &controlByte, inputStart,
                               repOffsets[0], codingOptions);

            input += length;
            literalRunStart = input;
            encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                         &bitDistanceEncoder, &controlByte, &length, distance,
                         repOffsets, blockNoHuffman,
                         codingOptions.advancedDistanceCoding);
            acceleration = 1 << accelerationThreshold;
          } else {
            input += acceleration >> accelerationThreshold;
            acceleration++;
          }
        } else {
          LZStructure* streamIt;
          if (compressorOptions.parser >= OPTIMAL2) {
            CompressorOptions iterationOptions = compressorOptions;
            // Use a lighter parse for the first iteration
            if (iteration == 0 && compressorOptions.parserIterations > 1 &&
                !blockNoHuffman) {
              iterationOptions.maxArrivals =
                  std::max(iterationOptions.maxArrivals / 4, 1);
              iterationOptions.niceLength =
                  std::max(iterationOptions.niceLength / 8, 32);
            }

            streamIt = multi_arrivals_parse(
                input, inputStart, thisBlockStart, compressionLimit,
                thisBlockEnd, &binaryMatchFinder, &matchBuffer, parser, stream,
                repOffsets, &acceleration, accelerationThreshold,
                literalSymbols, tokenSymbols, lengthSymbols, distanceSymbols,
                blockNoHuffman, codingOptions.advancedDistanceCoding,
                literalMode, bufferedMatches, iterationOptions, windowLog);
          } else {
            streamIt = forward_optimal_parse(
                input, inputStart, thisBlockEnd, &hashMatchFinder, parser,
                stream, repOffsets, &acceleration, accelerationThreshold,
                literalSymbols, tokenSymbols, lengthSymbols, distanceSymbols,
                blockNoHuffman, codingOptions.advancedDistanceCoding,
                literalMode, compressorOptions, windowLog);
          }

          // Main compression loop
          while (true) {
            input += streamIt->literalRunLength;

            if (streamIt == stream) break;

            size_t matchLength = streamIt->matchLength;
            size_t distance = streamIt->matchDistance;

            uint8_t controlByte;
            encode_literal_run(literalEncoder, &lengthEncoder, input,
                               literalRunStart, &controlByte, inputStart,
                               repOffsets[0], codingOptions);

            input += matchLength;
            literalRunStart = input;
            encode_match(&tokenEncoder, &lengthEncoder, &distanceEncoder,
                         &bitDistanceEncoder, &controlByte, &matchLength,
                         distance, repOffsets, blockNoHuffman,
                         codingOptions.advancedDistanceCoding);

            streamIt--;
          }
        }
      }

      input = thisBlockEnd;
      if (input != literalRunStart) {
        uint8_t controlByte;
        encode_literal_run(literalEncoder, &lengthEncoder, input,
                           literalRunStart, &controlByte, inputStart,
                           repOffsets[0], codingOptions);
        tokenEncoder.add_byte(controlByte);
      }
    }

    if (codingOptions.advancedLiteralCoding) {
      size_t rawSize = 0;
      size_t deltaSize = 0;

      for (int i = 0; i < (1 << codingOptions.posBits); i++) {
        size_t size;
        literalEncoder[0 + i].get_huffman_codes(literalDecSpeedBias[i], &size);
        rawSize += size;
        literalEncoder[4 + i].get_huffman_codes(literalDecSpeedBias[i], &size);
        deltaSize += size + literalEncoder[4 + i].get_symbol_count() *
                                (decSpeedBias / 4 + 0.05f / 8);
      }

      literalMode = (literalMode & STREAM_LITERALS_POS_MASK3) |
                    ((deltaSize < rawSize) ? STREAM_LITERALS_DELTA : 0);
    }

    // Encode all streams
    bool fastCodeGen = compressorOptions.parser < OPTIMAL3;
    bool useDelta = literalMode & STREAM_LITERALS_DELTA;

    for (int i = 0; i < (1 << codingOptions.posBits); i++)
      output += literalEncoder[(useDelta ? 4 : 0) + i].end_block(
          output, huffmanStreamBufferBegin, literalMode, literalDecSpeedBias[i],
          fastCodeGen, false);
    output += tokenEncoder.end_block(output, huffmanStreamBufferBegin, 0,
                                     tokenDecSpeedBias, fastCodeGen, false);
    output += distanceEncoder.end_block(
        output, huffmanStreamBufferBegin,
        codingOptions.advancedDistanceCoding ? STREAM_DISTANCE_ADVANCED : 0,
        distanceDecSpeedBias, fastCodeGen, false);
    if (codingOptions.advancedDistanceCoding)
      output += bitDistanceEncoder.end_block(output);
    output += lengthEncoder.end_block(output, huffmanStreamBufferBegin, 0,
                                      lengthDecSpeedBias, fastCodeGen, false);

    if (progress) {
      if (progress->progress(input - inputStart, output - outputStart)) {
        allocator->free(parser);
        allocator->free(stream);
        allocator->free(huffmanStreamBuffer);
        allocator->free(deltaLiteralsBuffer);
        return 0;
      }
    }
  }

  allocator->free(parser);
  allocator->free(stream);
  allocator->free(huffmanStreamBuffer);
  allocator->free(deltaLiteralsBuffer);
  write_header(output, SKANDA_LAST_BYTES, BLOCK_RAW, BLOCK_LAST);
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
    {OPTIMAL1, 18, 2, 32, 4096, NOT_USED, NOT_USED},
    {OPTIMAL1, 19, 3, 64, 4096, NOT_USED, NOT_USED},
    {OPTIMAL2, 24, 4, 128, 4096, 2, 2},
    {OPTIMAL2, 25, 5, 256, 4096, 4, 2},
    {OPTIMAL3, 26, 6, 512, 4096, 8, 2},
    {OPTIMAL3, 27, 7, 1024, 4096, 16, 3},
};

size_t compress(const uint8_t* input, size_t size, uint8_t* output, int level,
                float decSpeedBias, AllocatorCallback* allocator,
                ProgressCallback* progress) {
  if (size <= SKANDA_LAST_BYTES + 32) {
    write_header(output, size, BLOCK_RAW, BLOCK_LAST);
    memcpy(output, input, size);
    if (progress) progress->progress(size, size);
    return size + 3;
  }

  if (level > 10) level = 10;
  if (level < 0) level = 0;
  if (decSpeedBias < 0) decSpeedBias = 0;
  if (decSpeedBias > 1) decSpeedBias = 1;

  const int WINDOW_LOGS[] = {31, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20};
  int maxWindowLog = WINDOW_LOGS[(int)(decSpeedBias * 10)];
  int windowLog = int_log2(size - 1) + 1;
  if (windowLog > maxWindowLog) windowLog = maxWindowLog;
  if (windowLog < 6) windowLog = 6;

  AllocatorCallback defaultAllocator;
  if (!allocator) allocator = &defaultAllocator;

  if (skandaCompressorLevels[level].parser == GREEDY1)
    return compress_ultra_fast(input, size, output, windowLog, decSpeedBias,
                               skandaCompressorLevels[level], allocator,
                               progress);
  else if (skandaCompressorLevels[level].parser == GREEDY2)
    return compress_greedy(input, size, output, windowLog, decSpeedBias,
                           skandaCompressorLevels[level], allocator, progress);
  else if (skandaCompressorLevels[level].parser == LAZY1)
    return compress_lazy_fast(input, size, output, windowLog, decSpeedBias,
                              skandaCompressorLevels[level], allocator,
                              progress);
  else if (skandaCompressorLevels[level].parser == LAZY2)
    return compress_lazy(input, size, output, windowLog, decSpeedBias,
                         skandaCompressorLevels[level], allocator, progress);
  return compress_optimal(input, size, output, windowLog, decSpeedBias,
                          skandaCompressorLevels[level], allocator, progress);
}

size_t compress_bound(size_t size) { return size + size / 1024 + 128; }

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
  int blockSplitterDictLog = std::max(
      std::min(
          std::min(windowLog - 4, skandaCompressorLevels[level].maxHashLog),
          20),
      1);
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
  else
    memory += sizeof(uint32_t)
              << std::min(16, windowLog - 3)
              << (skandaCompressorLevels[level].maxElementsLog -
                  4);  // hash 3 table
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
  if (decSpeedBias < 0.99 &&
      skandaCompressorLevels[level].parserIterations > 1) {
    size_t maxMatchesPerPosition =
        std::min((1 << skandaCompressorLevels[level].maxElementsLog) + 1, 24);
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
  const int WINDOW_LOGS[] = {31, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20};
  int maxWindowLog = WINDOW_LOGS[(int)(decSpeedBias * 10)];
  int windowLog = int_log2(size - 1) + 1;
  if (windowLog > maxWindowLog) windowLog = maxWindowLog;
  if (windowLog < 6) windowLog = 6;

  bool disableDeltaCoding = decSpeedBias > 0.6;
  bool advancedDistanceCoding = decSpeedBias < 0.1;
  bool disableHuffman = decSpeedBias >= 0.99;
  size_t huffmanBufSize = std::min(MAX_BLOCK_SIZE, size);
  size_t huffmanMemoryUsage =
      disableHuffman ? 0 : huffmanBufSize + 128;  // Stream buffer
  huffmanMemoryUsage +=
      disableDeltaCoding ? 0 : huffmanBufSize + 128;  // Delta literals buffer
  huffmanMemoryUsage +=
      2048 * sizeof(uint32_t) *
      11;  // Internal memory used by entropy encoders for histograms
  size_t distanceBufferDivisor = advancedDistanceCoding ? 3 : 1;
  huffmanMemoryUsage += huffmanBufSize / 2 + 128;  // Token stream
  huffmanMemoryUsage += huffmanBufSize / 8 + 128;  // Length stream
  huffmanMemoryUsage +=
      huffmanBufSize / distanceBufferDivisor + 128;  // Distance stream
  if (advancedDistanceCoding)
    huffmanMemoryUsage +=
        (huffmanBufSize / 3 + 32) * 5;  // Raw distance bits stream

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
                             int* type, int* flags) {
  if (input + 3 > inputEnd) return -1;
  uint32_t data = (input[0]) | (input[1] << 8) | (input[2] << 16);
  *type = data & 3;
  *flags = (data >> 2) & 0xF;
  *blockSize = data >> 6;
  input += 3;
  return 0;
}

struct EntropyDecodeBuffer {
  uint8_t* decodeBuffer;
  AllocatorCallback* allocator;
  size_t decodeBufferSize;
  size_t decodeBufferUsed;

 public:
  EntropyDecodeBuffer() {}
  void construct(AllocatorCallback* _allocator, size_t _decodeBufferSize) {
    decodeBuffer = nullptr;
    allocator = _allocator;
    decodeBufferSize = _decodeBufferSize;
    decodeBufferUsed = 0;
  }
  bool initialized() { return decodeBuffer != nullptr; }
  void allocate() {
    decodeBuffer = (uint8_t*)allocator->allocate(decodeBufferSize);
  }
  // Returns nullptr if there is not enough space
  uint8_t* get_ptr(size_t spaceToUse, int alignment, int extraBuffer) {
    alignment -= 1;
    decodeBufferUsed = (decodeBufferUsed + alignment) & ~alignment;
    if (decodeBufferUsed + spaceToUse + extraBuffer > decodeBufferSize)
      return nullptr;
    uint8_t* ptr = decodeBuffer + decodeBufferUsed;
    decodeBufferUsed += spaceToUse;
    return ptr;
  }
  uint8_t* get_buffer_end() { return decodeBuffer + decodeBufferSize; }
  void clear() { decodeBufferUsed = 0; }
  ~EntropyDecodeBuffer() { allocator->free(decodeBuffer); }
};

struct HuffmanDecodeTableEntry {
  uint8_t symbol;
  uint8_t bits;
};

class HuffmanDecoder {
  int streamType;
  HuffmanDecodeTableEntry decodeTable[HUFFMAN_CODE_SPACE];
  bool decodeTableInitialized = false;

 public:
  HuffmanDecoder(int type) { streamType = type; }
  ~HuffmanDecoder() {}

  FORCE_INLINE void renormalize_precode(const uint8_t*& huffIt,
                                        const uint8_t* const compressedStart,
                                        uint32_t* huffState, size_t* bitCount) {
    // We can read up to 4 bytes beyond compressedStart.
    // We have at least 1 from block header, 1 from entropy header and 2 from
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
  FORCE_INLINE size_t precode_decode_op(
      uint32_t& huffState, size_t& bitCount,
      HuffmanDecodeTableEntry* decodeTable, size_t& lastSymbol,
      size_t& readSymbols, size_t& symbolRunLen, size_t& usedCodeSpace,
      uint8_t decodedSymbols[MAX_HUFFMAN_CODE_LENGTH + 2][256],
      int decodedCounts[MAX_HUFFMAN_CODE_LENGTH + 2]) {
    size_t value = huffState >> (32 - MAX_PRECODE_CODE_LENGTH);
    size_t newSymbol = decodeTable[value].symbol;
    huffState <<= decodeTable[value].bits;
    bitCount -= decodeTable[value].bits;

    // Start new run length and put one symbol
    if (newSymbol != lastSymbol) {
      decodedSymbols[newSymbol][decodedCounts[newSymbol]] = readSymbols;
      decodedCounts[newSymbol]++;
      readSymbols++;
      usedCodeSpace += HUFFMAN_CODE_SPACE >> newSymbol;
      lastSymbol = newSymbol;
      symbolRunLen = 0;
    }
    // Continue existing run length
    else {
      size_t bit = extract_bits_precode(&huffState, &bitCount,
                                        1);  // read an extra bit for run length
      size_t extraLen = (1 + bit) << symbolRunLen;
      symbolRunLen++;
      size_t loopEnd = readSymbols + extraLen;
      if (loopEnd > 255)  // too many decoded symbols
        return -1;
      do {
        decodedSymbols[newSymbol][decodedCounts[newSymbol]] = readSymbols;
        decodedCounts[newSymbol]++;
        readSymbols++;
      } while (readSymbols < loopEnd);
      usedCodeSpace += (HUFFMAN_CODE_SPACE >> newSymbol) * extraLen;
    }
    return 0;
  }

  void generate_decode_table(
      const uint8_t decodedSymbols[MAX_HUFFMAN_CODE_LENGTH + 2][256],
      const int decodedCounts[MAX_HUFFMAN_CODE_LENGTH + 2]) {
    HuffmanDecodeTableEntry* it = decodeTable;
    for (size_t bits = 1; bits < MAX_HUFFMAN_CODE_LENGTH - 3; bits++) {
      for (int i = 0; i < decodedCounts[bits]; i++) {
        HuffmanDecodeTableEntry* upperRange = it + (HUFFMAN_CODE_SPACE >> bits);
        const uint64_t filling =
            decode_decodeTable_filler(decodedSymbols[bits][i], bits);
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
      const uint64_t filling = decode_decodeTable_filler(
          decodedSymbols[MAX_HUFFMAN_CODE_LENGTH - 3][i],
          MAX_HUFFMAN_CODE_LENGTH - 3);
      memcpy(it + 0, &filling, sizeof(uint64_t));
      memcpy(it + 4, &filling, sizeof(uint64_t));
      it += 8;
    }

    for (int i = 0; i < decodedCounts[MAX_HUFFMAN_CODE_LENGTH - 2]; i++) {
      const uint64_t filling = decode_decodeTable_filler(
          decodedSymbols[MAX_HUFFMAN_CODE_LENGTH - 2][i],
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

  int decode_huffman_header(size_t streamSizes[6], const uint8_t* compressed,
                            const uint8_t* const compressedStart) {
    uint8_t decodedSymbols[MAX_HUFFMAN_CODE_LENGTH + 2][256];
    int decodedCounts[MAX_HUFFMAN_CODE_LENGTH + 2];

    const uint8_t* huffIt = compressed - 4;
    uint32_t huffState = read_uint32le(huffIt);
    size_t bitCount = 32;

    size_t firstSizeLog = extract_bits_precode(&huffState, &bitCount, 3) + 8;
    streamSizes[0] =
        ((1 << firstSizeLog) |
         extract_bits_precode(&huffState, &bitCount, firstSizeLog)) -
        255;
    size_t errorBits = extract_bits_precode(&huffState, &bitCount, 4) + 1;
    renormalize_precode(huffIt, compressedStart, &huffState, &bitCount);

    for (size_t i = 1; i < 6; i++) {
      size_t foldedError =
          extract_bits_precode(&huffState, &bitCount, errorBits);
      size_t unfoldedError = (foldedError >> 1) ^ (0 - (foldedError & 1));
      streamSizes[i] = (streamSizes[0] + unfoldedError) & 0xFFFF;
      renormalize_precode(huffIt, compressedStart, &huffState, &bitCount);
    }

    size_t hskip = extract_bits_precode(&huffState, &bitCount, 3) + 1;
    memset(decodedCounts + 1, 0, (MAX_HUFFMAN_CODE_LENGTH + 1) * sizeof(int));

    size_t usedCodeSpace = 0;
    for (size_t i = hskip;
         i <= MAX_HUFFMAN_CODE_LENGTH && usedCodeSpace < PRECODE_CODE_SPACE;
         i++) {
      size_t bits = extract_bits_precode(&huffState, &bitCount, 3) + 1;
      decodedSymbols[bits][decodedCounts[bits]] = i;
      decodedCounts[bits]++;
      usedCodeSpace += PRECODE_CODE_SPACE >> bits;
      renormalize_precode(huffIt, compressedStart, &huffState, &bitCount);
    }

    // Bits for code length 12 can be inferred
    if (usedCodeSpace < PRECODE_CODE_SPACE) {
      if (usedCodeSpace <
          PRECODE_CODE_SPACE / 2)  // We would have a code shorter than 1 bit
        return -1;
      size_t bits = MAX_PRECODE_CODE_LENGTH -
                    unsafe_int_log2(PRECODE_CODE_SPACE - usedCodeSpace);
      decodedSymbols[bits][decodedCounts[bits]] = MAX_HUFFMAN_CODE_LENGTH + 1;
      decodedCounts[bits]++;
      usedCodeSpace += PRECODE_CODE_SPACE >> bits;
    }
    if (usedCodeSpace != PRECODE_CODE_SPACE) return -1;

    HuffmanDecodeTableEntry decodeTable[PRECODE_CODE_SPACE + 4];
    HuffmanDecodeTableEntry* decodeTableIt = decodeTable;
    for (size_t bits = 1; bits <= MAX_PRECODE_CODE_LENGTH; bits++) {
      for (int i = 0; i < decodedCounts[bits]; i++) {
        HuffmanDecodeTableEntry* upperRange =
            decodeTableIt + (PRECODE_CODE_SPACE >> bits);
        const uint64_t filling =
            decode_decodeTable_filler(decodedSymbols[bits][i], bits);
        do {
          memcpy(decodeTableIt, &filling, sizeof(uint64_t));
          decodeTableIt += 4;
        } while (decodeTableIt < upperRange);
        decodeTableIt = upperRange;
      }
    }

    memset(decodedCounts + 1, 0, (MAX_HUFFMAN_CODE_LENGTH + 1) * sizeof(int));
    usedCodeSpace = 0;
    size_t lastSymbol = 0;
    size_t symbolRunLen = 0;
    size_t readSymbols = 0;

    // We can decode up to 3 symbols per refill
    while (readSymbols < 255 && usedCodeSpace != HUFFMAN_CODE_SPACE) {
      if (precode_decode_op(huffState, bitCount, decodeTable, lastSymbol,
                            readSymbols, symbolRunLen, usedCodeSpace,
                            decodedSymbols, decodedCounts))
        return -1;

      if (!(readSymbols < 255 && usedCodeSpace != HUFFMAN_CODE_SPACE)) break;

      if (precode_decode_op(huffState, bitCount, decodeTable, lastSymbol,
                            readSymbols, symbolRunLen, usedCodeSpace,
                            decodedSymbols, decodedCounts))
        return -1;

      if (!(readSymbols < 255 && usedCodeSpace != HUFFMAN_CODE_SPACE)) break;

      if (precode_decode_op(huffState, bitCount, decodeTable, lastSymbol,
                            readSymbols, symbolRunLen, usedCodeSpace,
                            decodedSymbols, decodedCounts))
        return -1;

      renormalize_precode(huffIt, compressedStart, &huffState, &bitCount);
    }

    // Bits of symbol 255 can be inferred
    if (usedCodeSpace < HUFFMAN_CODE_SPACE) {
      if (usedCodeSpace <
          HUFFMAN_CODE_SPACE / 2)  // We would have a code shorter than 1 bit
        return -1;
      size_t bits = MAX_HUFFMAN_CODE_LENGTH -
                    unsafe_int_log2(HUFFMAN_CODE_SPACE - usedCodeSpace);
      decodedSymbols[bits][decodedCounts[bits]] = 255;
      decodedCounts[bits]++;
      usedCodeSpace += HUFFMAN_CODE_SPACE >> bits;
    }
    // Invalid decoded huffman lengths
    if (usedCodeSpace != HUFFMAN_CODE_SPACE) return -1;
    generate_decode_table(decodedSymbols, decodedCounts);
    decodeTableInitialized = true;
    return 0;
  }

  // Can move the stream pointer up to 7 bytes.
  FORCE_INLINE void renormalize(size_t& state, const uint8_t*& stream) {
    size_t consumedBits = unsafe_bit_scan_forward(state);
    stream -= consumedBits >> 3;
    state = (IS_64BIT ? read_uint64le(stream) : read_uint32le(stream)) | 1;
    state <<= consumedBits & 7;
  }

  FORCE_INLINE void decode_op(
      size_t& state, uint8_t* const out,
      const HuffmanDecodeTableEntry* const decodeTable) {
    const size_t value =
        state >> ((IS_64BIT ? 64 : 32) - MAX_HUFFMAN_CODE_LENGTH);
    state <<= decodeTable[value].bits;
    *out = decodeTable[value].symbol;
  }

  FORCE_INLINE uint64_t decode_decodeTable_filler(uint8_t symbol,
                                                  uint8_t bits) {
    HuffmanDecodeTableEntry entry = {symbol, bits};
    uint16_t u16;
    memcpy(&u16, &entry, 2);
    return 0x0001000100010001 * u16;
  }

  void decode_symbols(const size_t streamSizes[6], const size_t symbolCount,
                      const uint8_t* const compressed, uint8_t* out) {
    const uint8_t* streamA = compressed + streamSizes[0];
    const uint8_t* streamB = streamA + streamSizes[1];
    const uint8_t* streamC = streamB + streamSizes[2];
    const uint8_t* streamD = streamC + streamSizes[3];
    const uint8_t* streamE = streamD + streamSizes[4];
    const uint8_t* streamF = streamE + streamSizes[5];
    const uint8_t* const streamBase =
        compressed;  // Do not read beyond this point

#if IS_64BIT
    // We have 1 byte from block header, 1 byte from entropy header, 2 byte from
    // huffman header size and 4 bytes of huffman header.
    streamA -= 8;
    size_t stateA = read_uint64le(streamA) | 1;
    streamB -= 8;
    size_t stateB = read_uint64le(streamB) | 1;
    streamC -= 8;
    size_t stateC = read_uint64le(streamC) | 1;
    streamD -= 8;
    size_t stateD = read_uint64le(streamD) | 1;
    streamE -= 8;
    size_t stateE = read_uint64le(streamE) | 1;
    streamF -= 8;
    size_t stateF = read_uint64le(streamF) | 1;
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
        decode_op(stateA, out + 0, decodeTable);
        decode_op(stateB, out + 1, decodeTable);
        decode_op(stateC, out + 2, decodeTable);
        decode_op(stateD, out + 3, decodeTable);
        decode_op(stateE, out + 4, decodeTable);
        decode_op(stateF, out + 5, decodeTable);
        // Second decode
        decode_op(stateA, out + 6, decodeTable);
        decode_op(stateB, out + 7, decodeTable);
        decode_op(stateC, out + 8, decodeTable);
        decode_op(stateD, out + 9, decodeTable);
        decode_op(stateE, out + 10, decodeTable);
        decode_op(stateF, out + 11, decodeTable);
        // Third decode
        decode_op(stateA, out + 12, decodeTable);
        decode_op(stateB, out + 13, decodeTable);
        decode_op(stateC, out + 14, decodeTable);
        decode_op(stateD, out + 15, decodeTable);
        decode_op(stateE, out + 16, decodeTable);
        decode_op(stateF, out + 17, decodeTable);
        // Fourth decode
        decode_op(stateA, out + 18, decodeTable);
        decode_op(stateB, out + 19, decodeTable);
        decode_op(stateC, out + 20, decodeTable);
        decode_op(stateD, out + 21, decodeTable);
        decode_op(stateE, out + 22, decodeTable);
        decode_op(stateF, out + 23, decodeTable);
        // Fifth decode and renormalization
        decode_op(stateA, out + 24, decodeTable);
        if (streamA >= streamBase) renormalize(stateA, streamA);
        decode_op(stateB, out + 25, decodeTable);
        if (streamB >= streamBase) renormalize(stateB, streamB);
        decode_op(stateC, out + 26, decodeTable);
        if (streamC >= streamBase) renormalize(stateC, streamC);
        decode_op(stateD, out + 27, decodeTable);
        if (streamD >= streamBase) renormalize(stateD, streamD);
        decode_op(stateE, out + 28, decodeTable);
        if (streamE >= streamBase) renormalize(stateE, streamE);
        decode_op(stateF, out + 29, decodeTable);
        if (streamF >= streamBase) renormalize(stateF, streamF);
        out += 30;
      } else {
        // First decode
        decode_op(stateA, out + 0, decodeTable);
        decode_op(stateB, out + 1, decodeTable);
        decode_op(stateC, out + 2, decodeTable);
        decode_op(stateD, out + 3, decodeTable);
        decode_op(stateE, out + 4, decodeTable);
        decode_op(stateF, out + 5, decodeTable);
        // Second decode and renormalization
        decode_op(stateA, out + 6, decodeTable);
        if (streamA >= streamBase) renormalize(stateA, streamA);
        decode_op(stateB, out + 7, decodeTable);
        if (streamB >= streamBase) renormalize(stateB, streamB);
        decode_op(stateC, out + 8, decodeTable);
        if (streamC >= streamBase) renormalize(stateC, streamC);
        decode_op(stateD, out + 9, decodeTable);
        if (streamD >= streamBase) renormalize(stateD, streamD);
        decode_op(stateE, out + 10, decodeTable);
        if (streamE >= streamBase) renormalize(stateE, streamE);
        decode_op(stateF, out + 11, decodeTable);
        if (streamF >= streamBase) renormalize(stateF, streamF);
        out += 12;
      }
    }

    // States are already normalized, thus we have enough bits to decode
    // remaining symbols
    while (out < outEnd) {
      decode_op(stateA, out + 0, decodeTable);
      decode_op(stateB, out + 1, decodeTable);
      decode_op(stateC, out + 2, decodeTable);
      decode_op(stateD, out + 3, decodeTable);
      decode_op(stateE, out + 4, decodeTable);
      decode_op(stateF, out + 5, decodeTable);
      out += 6;
    }
  }

  // Returns error code or number of decoded symbols. compressed will point to
  // end of this compressed stream,
  //  streamIt to decompressed data and streamBufferEnd to the end of the buffer
  //  for the decompressed data
  size_t decode(const uint8_t*& compressed,
                const uint8_t* const compressedStart,
                const uint8_t* const compressedEnd,
                EntropyDecodeBuffer* decodeBuffer, const uint8_t*& streamIt,
                const uint8_t*& streamBufferEnd, int* flags) {
    size_t symbolCount;
    int mode;
    if (unlikely(
            read_header(compressed, compressedEnd, &symbolCount, &mode, flags)))
      return ERROR_CORRUPT;

    if (mode == ENTROPY_RAW) {
      if (unlikely(compressedEnd - compressed < symbolCount))
        return ERROR_CORRUPT;
      streamIt = compressed;
      compressed += symbolCount;
      streamBufferEnd = compressedEnd;
      return symbolCount;
    }

    if (!decodeBuffer->initialized()) {
      try {
        decodeBuffer->allocate();
      } catch (const std::bad_alloc& e) {
        return ERROR_NOMEM;
      }
    }
    uint8_t* decodePtr =
        decodeBuffer->get_ptr(symbolCount, 1, SKANDA_LAST_BYTES);
    if (unlikely(!decodePtr))  // Not enough space available in the buffer
      return ERROR_CORRUPT;
    streamIt = decodePtr;
    streamBufferEnd = decodeBuffer->get_buffer_end();

    if (mode == ENTROPY_RLE) {
      uint8_t byte = *compressed++;
      memset(decodePtr, byte, symbolCount);
      return symbolCount;
    }

    if (mode == ENTROPY_HUFFMAN) {
      size_t headerSize = (*compressed++) & 0x7F;
      // 16 bits for base size, 4 bits for number of extra size bits, at least 1
      // bit for extra size for each stream,
      //  3 bits for huffman tree not transmitted.
      //  Total 29 bits or 4 bytes that the header should at least take.
      if (headerSize < 4 || compressed + headerSize > compressedEnd)
        return ERROR_CORRUPT;
      size_t streamSizes[6];

      int err = decode_huffman_header(streamSizes, compressed + headerSize,
                                      compressed);
      if (err) return err;
      compressed += headerSize;

      const uint8_t* const compressedStreamEnd =
          compressed + streamSizes[0] + streamSizes[1] + streamSizes[2] +
          streamSizes[3] + streamSizes[4] + streamSizes[5];
      if (compressedStreamEnd > compressedEnd) return ERROR_CORRUPT;

      decode_symbols(streamSizes, symbolCount, compressed, decodePtr);
      compressed = compressedStreamEnd;
      return symbolCount;
    }

    return ERROR_CORRUPT;
  }
};

FORCE_INLINE void renormalize_advanced_distance_decoder(
    const uint8_t*& compressed, const uint8_t* const compressedEnd,
    size_t* state, size_t* stateBitCount) {
  if (compressed < compressedEnd) {
#if IS_64BIT
    *state |= read_uint64le(compressed) << *stateBitCount;
    compressed += (*stateBitCount ^ 63) >> 3;
    *stateBitCount |= 56;
#else
    *state |= read_uint32le(compressed) << *stateBitCount;
    compressed += (*stateBitCount ^ 31) >> 3;
    *stateBitCount |= 24;
#endif
  }
}

FORCE_INLINE int decode_advanced_distance_op(const uint8_t distanceToken,
                                             uint32_t* distanceBuffer,
                                             const uint8_t*& streamIt,
                                             const uint8_t* const streamEnd,
                                             size_t* state,
                                             size_t* stateBitCount) {
  size_t distanceBits = distanceToken >> 3;
  size_t distanceLow = distanceToken & 7;
#if IS_64BIT
  // Unsupported
  if (distanceBits > 28) return -1;
  size_t highBit = 1 << distanceBits;
  *distanceBuffer = (*state & (highBit - 1) | highBit) * 8 - distanceLow;
  *state >>= distanceBits;
  *stateBitCount -= distanceBits;
#else
  if (distanceBits <= 24) {
    size_t highBit = 1 << distanceBits;
    *distanceBuffer = (*state & (highBit - 1) | highBit) * 8 - distanceLow;
    *state >>= distanceBits;
    *stateBitCount -= distanceBits;
  } else {
    size_t distance = 1 << distanceBits;
    distance |= *state & 0xFFFFFF;
    *state >>= 24;
    *stateBitCount -= 24;
    distanceBits -= 24;
    renormalize_advanced_distance_decoder(streamIt, streamEnd, state,
                                          stateBitCount);

    distance |= (*state & (1 << distanceBits) - 1) << 24;
    *state >>= distanceBits;
    *stateBitCount -= distanceBits;
    *distanceBuffer = distance * 8 - distanceLow;
  }
#endif
  return 0;
}

// Returns error code or 0 on success, and advances compressed pointer to end of
// stream
size_t decode_advanced_distances(const uint8_t*& compressed,
                                 const uint8_t* const compressedEnd,
                                 uint32_t*& decodedDistances,
                                 EntropyDecodeBuffer* decodeBuffer,
                                 const uint8_t* distanceTokens,
                                 size_t numberDistances) {
  if (!decodeBuffer->initialized()) {
    try {
      decodeBuffer->allocate();
    } catch (std::bad_alloc& e) {
      return ERROR_NOMEM;
    }
  }

  decodedDistances =
      (uint32_t*)decodeBuffer->get_ptr(numberDistances * 4, 4, 0);
  if (!decodedDistances) return ERROR_CORRUPT;

#if IS_64BIT
  size_t state = read_uint64le(compressed);
  // Technically we have 64 bits, but there is a chance none is consumed during
  // an iteration
  //  of distance decoding, and the renormalize function wont work if this has a
  //  value of 64.
  size_t stateBitCount = 56;
  compressed += 7;
#else
  size_t state = read_uint32le(compressed);
  size_t stateBitCount = 24;
  compressed += 3;
#endif

  uint32_t* decodedDistancesIt = decodedDistances;
  const uint8_t* distanceTokensEnd = distanceTokens + numberDistances;

  while (distanceTokens < distanceTokensEnd) {
    if (decode_advanced_distance_op(distanceTokens[0], decodedDistancesIt,
                                    compressed, compressedEnd, &state,
                                    &stateBitCount))
      return ERROR_CORRUPT;
    distanceTokens++;
    decodedDistancesIt++;
#if IS_64BIT
    // On 64 bit state will always have at least 56 bits, so we can perform two
    // decodes of distances up to ~4gb. For longer distances an extra refill
    // will be needed.
    if (distanceTokens == distanceTokensEnd) break;
    if (decode_advanced_distance_op(distanceTokens[0], decodedDistancesIt,
                                    compressed, compressedEnd, &state,
                                    &stateBitCount))
      return ERROR_CORRUPT;
    distanceTokens++;
    decodedDistancesIt++;
#endif

    renormalize_advanced_distance_decoder(compressed, compressedEnd, &state,
                                          &stateBitCount);
  }

  if (stateBitCount >= 64) return ERROR_CORRUPT;
  compressed -= stateBitCount / 8;
  if (compressed > compressedEnd) return ERROR_CORRUPT;

  return 0;
}

const int8_t inc32decodeTable[16] = {0, 1, 2, 1, 0, 4, 4, 4,
                                     4, 4, 4, 4, 4, 4, 4, 4};
const int8_t inc64decodeTable[16] = {0, 0, 0, 1, 4, -1, -2, -3,
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
  const uint8_t* lengthStreamIt;
  const uint8_t* distanceStreamIt;
  uint8_t* decompressed;
};

template <int LITERAL_FLAGS>
FORCE_INLINE bool decode_long_literal_run_masked(
    uint8_t*& decompressed, const uint8_t** literalStreamsIt,
    const uint8_t** const literalBuffersEnd, const size_t distance,
    const size_t length) {
  uint8_t* dst = decompressed;
  // Advance stream pointers by the number of bytes we read from each of them,
  //  and check if we overflow.
  const uint8_t* stream0 = literalStreamsIt[intptr_t(decompressed + 0) & 3];
  literalStreamsIt[intptr_t(decompressed + 0) & 3] = stream0 + (length + 3) / 4;
  if (unlikely(literalStreamsIt[intptr_t(decompressed + 0) & 3] >
               literalBuffersEnd[intptr_t(decompressed + 0) & 3]))
    return true;
  const uint8_t* stream1 = literalStreamsIt[intptr_t(decompressed + 1) & 3];
  literalStreamsIt[intptr_t(decompressed + 1) & 3] = stream1 + (length + 2) / 4;
  if (unlikely(literalStreamsIt[intptr_t(decompressed + 1) & 3] >
               literalBuffersEnd[intptr_t(decompressed + 1) & 3]))
    return true;
  const uint8_t* stream2 = literalStreamsIt[intptr_t(decompressed + 2) & 3];
  literalStreamsIt[intptr_t(decompressed + 2) & 3] = stream2 + (length + 1) / 4;
  if (unlikely(literalStreamsIt[intptr_t(decompressed + 2) & 3] >
               literalBuffersEnd[intptr_t(decompressed + 2) & 3]))
    return true;
  const uint8_t* stream3 = literalStreamsIt[intptr_t(decompressed + 3) & 3];
  literalStreamsIt[intptr_t(decompressed + 3) & 3] = stream3 + (length + 0) / 4;
  if (unlikely(literalStreamsIt[intptr_t(decompressed + 3) & 3] >
               literalBuffersEnd[intptr_t(decompressed + 3) & 3]))
    return true;

  uint8_t* const end = decompressed + length;
  if (LITERAL_FLAGS & STREAM_LITERALS_DELTA) {
    const uint8_t* prediction = decompressed - distance;
#ifdef x64
    if (distance >= length) {
      do {
        __m128i lit0 = _mm_loadu_si128((__m128i*)stream0);
        __m128i lit1 = _mm_loadu_si128((__m128i*)stream1);
        __m128i lit2 = _mm_loadu_si128((__m128i*)stream2);
        __m128i lit3 = _mm_loadu_si128((__m128i*)stream3);
        __m128i mix0 = _mm_unpacklo_epi8(lit0, lit2);
        __m128i mix1 = _mm_unpacklo_epi8(lit1, lit3);
        _mm_storeu_si128(
            (__m128i*)(dst + 0),
            _mm_add_epi8(_mm_unpacklo_epi8(mix0, mix1),
                         _mm_loadu_si128((__m128i*)(prediction + 0))));
        _mm_storeu_si128(
            (__m128i*)(dst + 16),
            _mm_add_epi8(_mm_unpackhi_epi8(mix0, mix1),
                         _mm_loadu_si128((__m128i*)(prediction + 16))));
        dst += 32;
        prediction += 32;
        stream0 += 8;
        stream1 += 8;
        stream2 += 8;
        stream3 += 8;
      } while (dst < end);
    } else
#endif
    {
      do {
        dst[0] = stream0[0] + prediction[0];
        dst[1] = stream1[0] + prediction[1];
        dst[2] = stream2[0] + prediction[2];
        dst[3] = stream3[0] + prediction[3];
        dst[4] = stream0[1] + prediction[4];
        dst[5] = stream1[1] + prediction[5];
        dst[6] = stream2[1] + prediction[6];
        dst[7] = stream3[1] + prediction[7];
        stream0 += 2;
        stream1 += 2;
        stream2 += 2;
        stream3 += 2;
        dst += 8;
        prediction += 8;
      } while (dst < end);
    }
  } else {
#ifdef x64
    do {
      __m128i lit0 = _mm_loadu_si128((__m128i*)stream0);
      __m128i lit1 = _mm_loadu_si128((__m128i*)stream1);
      __m128i lit2 = _mm_loadu_si128((__m128i*)stream2);
      __m128i lit3 = _mm_loadu_si128((__m128i*)stream3);
      __m128i mix0 = _mm_unpacklo_epi8(lit0, lit2);
      __m128i mix1 = _mm_unpacklo_epi8(lit1, lit3);
      _mm_storeu_si128((__m128i*)(dst + 0), _mm_unpacklo_epi8(mix0, mix1));
      _mm_storeu_si128((__m128i*)(dst + 16), _mm_unpackhi_epi8(mix0, mix1));
      dst += 32;
      stream0 += 8;
      stream1 += 8;
      stream2 += 8;
      stream3 += 8;
      // We could calculate the mix0 and mix1 variables again with the unpackhi
      // instruction,
      //  which would save the 4 stream loads, but it does not improve
      //  performance?
    } while (dst < end);
#else
    do {
      dst[0] = stream0[0];
      dst[1] = stream1[0];
      dst[2] = stream2[0];
      dst[3] = stream3[0];
      dst[4] = stream0[1];
      dst[5] = stream1[1];
      dst[6] = stream2[1];
      dst[7] = stream3[1];
      stream0 += 2;
      stream1 += 2;
      stream2 += 2;
      stream3 += 2;
      dst += 8;
    } while (dst < end);
#endif
  }
  decompressed = end;

  return false;
}

template <int LITERAL_FLAGS>
FORCE_INLINE bool decode_short_literal_run_masked(
    uint8_t*& decompressed, const uint8_t** literalStreamsIt,
    const uint8_t** const literalBuffersEnd, const size_t distance,
    const size_t length) {
  bool overflow = false;
  const uint8_t* runEnd = decompressed + length;
  if (LITERAL_FLAGS & STREAM_LITERALS_DELTA) {
    const uint8_t* prediction = decompressed - distance;
    for (; decompressed < runEnd;) {
      const size_t stream = intptr_t(decompressed) & 3;
      *decompressed++ = *literalStreamsIt[stream]++ + *prediction;
      overflow |= (literalBuffersEnd[stream] - literalStreamsIt[stream]) >>
                  (sizeof(ptrdiff_t) * 8 - 1);
      prediction++;
    }
  } else {
    for (; decompressed < runEnd;) {
      const size_t stream = intptr_t(decompressed) & 3;
      *decompressed++ = *literalStreamsIt[stream]++;
      overflow |= (literalBuffersEnd[stream] - literalStreamsIt[stream]) >>
                  (sizeof(ptrdiff_t) * 8 - 1);
    }
  }

  return overflow;
}

template <bool REDUCE_CHECKS, int LITERAL_FLAGS>
FORCE_INLINE bool decode_long_literal_run(uint8_t*& decompressed,
                                          const uint8_t*& literalStreamIt,
                                          const uint8_t* const literalBufferEnd,
                                          const size_t distance,
                                          const size_t length) {
  if (!REDUCE_CHECKS && unlikely(literalBufferEnd - literalStreamIt < length))
    return true;

  if (LITERAL_FLAGS & STREAM_LITERALS_DELTA) {
    const uint8_t* prediction = decompressed - distance;
#ifdef x64
    if (distance >= length) {
      _mm_storeu_si128(
          (__m128i*)(decompressed + 0),
          _mm_add_epi8(_mm_loadu_si128((__m128i*)(literalStreamIt + 0)),
                       _mm_loadu_si128((__m128i*)(prediction + 0))));
      _mm_storeu_si128(
          (__m128i*)(decompressed + 16),
          _mm_add_epi8(_mm_loadu_si128((__m128i*)(literalStreamIt + 16)),
                       _mm_loadu_si128((__m128i*)(prediction + 16))));
      if (unlikely(length > 32)) {
        if (REDUCE_CHECKS &&
            unlikely(literalBufferEnd - literalStreamIt < length))
          return true;
        prediction += 32;
        uint8_t* dst = decompressed + 32;
        const uint8_t* src = literalStreamIt + 32;
        uint8_t* const end = decompressed + length;
        do {
          _mm_storeu_si128(
              (__m128i*)(dst + 0),
              _mm_add_epi8(_mm_loadu_si128((__m128i*)(src + 0)),
                           _mm_loadu_si128((__m128i*)(prediction + 0))));
          _mm_storeu_si128(
              (__m128i*)(dst + 16),
              _mm_add_epi8(_mm_loadu_si128((__m128i*)(src + 16)),
                           _mm_loadu_si128((__m128i*)(prediction + 16))));
          prediction += 32;
          src += 32;
          dst += 32;
        } while (dst < end);
      }
    } else
#endif
    {
      if (REDUCE_CHECKS &&
          unlikely(literalBufferEnd - literalStreamIt < length))
        return true;
      uint8_t* dst = decompressed;
      const uint8_t* src = literalStreamIt;
      uint8_t* const end = decompressed + length;
      do {
        dst[0] = src[0] + prediction[0];
        dst[1] = src[1] + prediction[1];
        dst[2] = src[2] + prediction[2];
        dst[3] = src[3] + prediction[3];
        dst[4] = src[4] + prediction[4];
        dst[5] = src[5] + prediction[5];
        dst[6] = src[6] + prediction[6];
        dst[7] = src[7] + prediction[7];
        prediction += 8;
        dst += 8;
        src += 8;
      } while (dst < end);
    }

  } else {
    memcpy(decompressed, literalStreamIt, 16);
    memcpy(decompressed + 16, literalStreamIt + 16, 16);
    if (unlikely(length > 32)) {
      if (REDUCE_CHECKS &&
          unlikely(literalBufferEnd - literalStreamIt < length))
        return true;
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
  }

  decompressed += length;
  literalStreamIt += length;
  return false;
}

template <bool REDUCE_CHECKS, int LITERAL_FLAGS>
FORCE_INLINE bool decode_short_literal_run(
    uint8_t*& decompressed, const uint8_t*& literalStreamIt,
    const uint8_t* const literalBufferEnd, const size_t distance,
    const size_t length) {
  if (LITERAL_FLAGS & STREAM_LITERALS_DELTA) {
#ifdef x64
    if (distance >= length) {
      __m128i prediction = _mm_loadu_si128((__m128i*)(decompressed - distance));
      __m128i literals = _mm_loadu_si128((__m128i*)literalStreamIt);
      _mm_storeu_si128((__m128i*)decompressed,
                       _mm_add_epi8(literals, prediction));
    } else
#endif
    {
      const uint8_t* prediction = decompressed - distance;
      decompressed[0] = literalStreamIt[0] + prediction[0];
      decompressed[1] = literalStreamIt[1] + prediction[1];
      decompressed[2] = literalStreamIt[2] + prediction[2];
      decompressed[3] = literalStreamIt[3] + prediction[3];
      decompressed[4] = literalStreamIt[4] + prediction[4];
      decompressed[5] = literalStreamIt[5] + prediction[5];
    }
  } else {
    memcpy(decompressed, literalStreamIt, 8);
  }

  decompressed += length;
  literalStreamIt += length;
  return (!REDUCE_CHECKS && unlikely(literalStreamIt > literalBufferEnd));
}

template <bool REDUCE_CHECKS, int DISTANCE_FLAGS, int LITERAL_FLAGS>
LZDecodeResults decode_lz_subblock(
    const uint8_t* tokenStreamIt, const uint8_t** literalStreamsIt,
    const uint8_t* lengthStreamIt, const uint8_t* distanceStreamIt,
    const uint8_t* const tokenBufferEnd,
    const uint8_t** const literalBuffersEnd,
    const uint8_t* const lengthBufferEnd,
    const uint8_t* const distanceBufferEnd, uint32_t* repOffsets,
    uint8_t* decompressed, uint8_t* const decompressedStart,
    uint8_t* const thisBlockEnd, uint8_t* const subBlockEnd) {
  // Initialize rep offset for simple distance mode
  size_t distance = repOffsets[3];
  const uint8_t* literalStreamIt0 = literalStreamsIt[0];
  const uint8_t* const literalBufferEnd0 = literalBuffersEnd[0];

  do {
    const size_t token = *tokenStreamIt++;
    if (!REDUCE_CHECKS && unlikely(tokenStreamIt > tokenBufferEnd))
      return LZDecodeResults{ERROR_CORRUPT};
    size_t length = token >> 5;

    if (unlikely(length >= 7)) {
      length += decode_length(lengthStreamIt);
      // Detect buffer overflows
      if (unlikely(thisBlockEnd - decompressed < length))
        return LZDecodeResults{ERROR_CORRUPT};
      if (!REDUCE_CHECKS && unlikely(lengthStreamIt > lengthBufferEnd))
        return LZDecodeResults{ERROR_CORRUPT};

      if (LITERAL_FLAGS & STREAM_LITERALS_POS_MASK3) {
        if (decode_long_literal_run_masked<LITERAL_FLAGS>(
                decompressed, literalStreamsIt, literalBuffersEnd, distance,
                length))
          return LZDecodeResults{ERROR_CORRUPT};
      } else {
        if (decode_long_literal_run<REDUCE_CHECKS, LITERAL_FLAGS>(
                decompressed, literalStreamIt0, literalBufferEnd0, distance,
                length))
          return LZDecodeResults{ERROR_CORRUPT};
      }
    }
    // Short literal run
    else {
      if (LITERAL_FLAGS & STREAM_LITERALS_POS_MASK3) {
        if (decode_short_literal_run_masked<LITERAL_FLAGS>(
                decompressed, literalStreamsIt, literalBuffersEnd, distance,
                length))
          return LZDecodeResults{ERROR_CORRUPT};
      } else {
        if (decode_short_literal_run<REDUCE_CHECKS, LITERAL_FLAGS>(
                decompressed, literalStreamIt0, literalBufferEnd0, distance,
                length))
          return LZDecodeResults{ERROR_CORRUPT};
      }
    }

    if (!(DISTANCE_FLAGS & STREAM_DISTANCE_ADVANCED)) {
      size_t distanceBits = token & 0x18;
      size_t newDistance =
          read_uint32le(distanceStreamIt) & (1 << distanceBits) - 1;
      // branchless selection, hope the compiler does replace this with a branch
      distance ^= (0 - (distanceBits != 0)) & (distance ^ newDistance);
      distanceStreamIt += distanceBits >> 3;
      length = token & 0x7;

      if (!REDUCE_CHECKS && unlikely(distanceStreamIt > distanceBufferEnd))
        return LZDecodeResults{ERROR_CORRUPT};
    } else {
      // Branchless distance/rep offset selection and update
      size_t repToUse = (token >> 3) & 3;
      // Read next non rep distance
      if (likely(distanceStreamIt < distanceBufferEnd))
        memcpy(&repOffsets[6], distanceStreamIt, 4);
      distanceStreamIt += (repToUse + 1) & 4;
      distance = repOffsets[repToUse + 3];
      repOffsets[repToUse + 3] = repOffsets[repToUse + 2];
      repOffsets[repToUse + 2] = repOffsets[repToUse + 1];
      repOffsets[repToUse + 1] = repOffsets[repToUse + 0];
      repOffsets[3] = distance;
      length = token & 0x7;
    }

    const uint8_t* match = decompressed - distance;
    if (unlikely(decompressed - decompressedStart < distance))
      return LZDecodeResults{ERROR_CORRUPT};

    if (length == 7) {
      length = decode_length(lengthStreamIt) + SKANDA_MIN_MATCH_LENGTH + 7;
      // Detect buffer overflows
      if (!REDUCE_CHECKS && unlikely(lengthStreamIt > lengthBufferEnd))
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
        match += inc32decodeTable[distance];
        memcpy(decompressed + 4, match, 4);
        match += inc64decodeTable[distance];
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
      length += SKANDA_MIN_MATCH_LENGTH;
      // If the distance is big enough we can perform a faster copy
      if (likely(distance >= length)) memcpy(decompressed, match, 8);
      // Else it is a run-length type match
      else {
        decompressed[0] = match[0];
        decompressed[1] = match[1];
        decompressed[2] = match[2];
        decompressed[3] = match[3];
        match += inc32decodeTable[distance];
        memcpy(decompressed + 4, match, 4);
      }
      decompressed += length;
    }

  } while (likely(decompressed < subBlockEnd));

  LZDecodeResults results;
  results.error = 0;
  results.tokenStreamIt = tokenStreamIt;
  results.lengthStreamIt = lengthStreamIt;
  results.distanceStreamIt = distanceStreamIt;
  results.decompressed = decompressed;
  repOffsets[3] = distance;
  if (!(LITERAL_FLAGS & STREAM_LITERALS_POS_MASK3))
    literalStreamsIt[0] = literalStreamIt0;
  return results;
}

template <bool REDUCE_CHECKS, int DISTANCE_FLAGS>
LZDecodeResults decode_lz_subblock_literal_mode_selector(
    const uint8_t* tokenStreamIt, const uint8_t** literalStreamsIt,
    const uint8_t* lengthStreamIt, const uint8_t* distanceStreamIt,
    const uint8_t* const tokenBufferEnd,
    const uint8_t** const literalBuffersEnd,
    const uint8_t* const lengthBufferEnd,
    const uint8_t* const distanceBufferEnd, uint32_t* repOffsets,
    uint8_t* decompressed, uint8_t* const decompressedStart,
    uint8_t* const thisBlockEnd, uint8_t* const subBlockEnd, int literalFlags) {
  if (literalFlags == 0) {
    return decode_lz_subblock<REDUCE_CHECKS, DISTANCE_FLAGS, 0>(
        tokenStreamIt, literalStreamsIt, lengthStreamIt, distanceStreamIt,
        tokenBufferEnd, literalBuffersEnd, lengthBufferEnd, distanceBufferEnd,
        repOffsets, decompressed, decompressedStart, thisBlockEnd, subBlockEnd);
  } else if (literalFlags == STREAM_LITERALS_DELTA) {
    return decode_lz_subblock<REDUCE_CHECKS, DISTANCE_FLAGS,
                              STREAM_LITERALS_DELTA>(
        tokenStreamIt, literalStreamsIt, lengthStreamIt, distanceStreamIt,
        tokenBufferEnd, literalBuffersEnd, lengthBufferEnd, distanceBufferEnd,
        repOffsets, decompressed, decompressedStart, thisBlockEnd, subBlockEnd);
  } else if (literalFlags == STREAM_LITERALS_POS_MASK3) {
    return decode_lz_subblock<REDUCE_CHECKS, DISTANCE_FLAGS,
                              STREAM_LITERALS_POS_MASK3>(
        tokenStreamIt, literalStreamsIt, lengthStreamIt, distanceStreamIt,
        tokenBufferEnd, literalBuffersEnd, lengthBufferEnd, distanceBufferEnd,
        repOffsets, decompressed, decompressedStart, thisBlockEnd, subBlockEnd);
  } else if (literalFlags ==
             (STREAM_LITERALS_DELTA | STREAM_LITERALS_POS_MASK3)) {
    return decode_lz_subblock<REDUCE_CHECKS, DISTANCE_FLAGS,
                              STREAM_LITERALS_DELTA |
                                  STREAM_LITERALS_POS_MASK3>(
        tokenStreamIt, literalStreamsIt, lengthStreamIt, distanceStreamIt,
        tokenBufferEnd, literalBuffersEnd, lengthBufferEnd, distanceBufferEnd,
        repOffsets, decompressed, decompressedStart, thisBlockEnd, subBlockEnd);
  } else {
    return LZDecodeResults{ERROR_CORRUPT};
  }
}

size_t decode_lz_block(
    uint8_t* decompressed, uint8_t* const decompressedStart,
    uint8_t* const thisBlockEnd, uint32_t* repOffsets,
    const uint8_t** literalStreamsIt, const uint8_t* tokenStreamIt,
    const uint8_t* distanceStreamIt, const uint8_t* lengthStreamIt,
    const uint8_t** literalBuffersEnd, const uint8_t* tokenBufferEnd,
    const uint8_t* distanceBufferEnd, const uint8_t* lengthBufferEnd,
    int literalFlags, int tokenFlags, int distanceFlags, int lengthFlags) {
  if (literalFlags & STREAM_LITERALS_POS_MASK3) {
    // Shuffle the literal streams so that instead of:
    //  int stream = (decompressed - decompressedStart) & 3;
    // We can do:
    //  int stream = decompressed & 3;
    const uint8_t* shuffledIts[4];
    const uint8_t* shuffledEnds[4];
    for (size_t i = 0; i < 4; i++) {
      shuffledIts[reinterpret_cast<size_t>(decompressedStart + i) & 3] =
          literalStreamsIt[i];
      shuffledEnds[reinterpret_cast<size_t>(decompressedStart + i) & 3] =
          literalBuffersEnd[i];
    }
    memcpy(literalStreamsIt, shuffledIts, sizeof(uint8_t*) * 4);
    memcpy(literalBuffersEnd, shuffledEnds, sizeof(uint8_t*) * 4);
  }

  while (decompressed < thisBlockEnd) {
    size_t subBlockSize = std::min(16384, int(thisBlockEnd - decompressed));
    uint8_t* subBlockEnd = decompressed + subBlockSize;

    // LZ decoding obviously needs checks for malicious input, but they take a
    // 15% performance hit. We will divide them into 3 categories:
    // - Output buffer overflow: we have a 32 byte buffer at the end, so we only
    // have to check
    //   these on long literal runs or matches, which are rare, so this is not a
    //   problem.
    // - Long distances: the only way this check can be avoided is after having
    // decompressed 16MB of data
    // - Compressed streams: we need to make sure we dont read beyond the end of
    // any of the data 4 streams.
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
    //   outside of it. If they come directly from compressed buffer we need to
    //   make sure we are far away from the end.
    // With these we can minimize the performance penalty while keeping the
    // decompressor safe.

    LZDecodeResults results;
    bool reduceChecks =
        ((literalFlags & STREAM_LITERALS_POS_MASK3) ||
         (literalBuffersEnd[0] - literalStreamsIt[0] >= subBlockSize)) &&
        ((distanceFlags & STREAM_DISTANCE_ADVANCED) ||
         (distanceBufferEnd - distanceStreamIt >= subBlockSize * 1.5)) &&
        (tokenBufferEnd - tokenStreamIt >= subBlockSize / 2) &&
        (lengthBufferEnd - lengthStreamIt >= subBlockSize / 8);

    if (distanceFlags & STREAM_DISTANCE_ADVANCED) {
      if (reduceChecks) {
        results =
            decode_lz_subblock_literal_mode_selector<1,
                                                     STREAM_DISTANCE_ADVANCED>(
                tokenStreamIt, literalStreamsIt, lengthStreamIt,
                distanceStreamIt, tokenBufferEnd, literalBuffersEnd,
                lengthBufferEnd, distanceBufferEnd, repOffsets, decompressed,
                decompressedStart, thisBlockEnd, subBlockEnd, literalFlags);
      } else {
        results =
            decode_lz_subblock_literal_mode_selector<0,
                                                     STREAM_DISTANCE_ADVANCED>(
                tokenStreamIt, literalStreamsIt, lengthStreamIt,
                distanceStreamIt, tokenBufferEnd, literalBuffersEnd,
                lengthBufferEnd, distanceBufferEnd, repOffsets, decompressed,
                decompressedStart, thisBlockEnd, subBlockEnd, literalFlags);
      }
    } else {
      if (reduceChecks) {
        results = decode_lz_subblock_literal_mode_selector<1, 0>(
            tokenStreamIt, literalStreamsIt, lengthStreamIt, distanceStreamIt,
            tokenBufferEnd, literalBuffersEnd, lengthBufferEnd,
            distanceBufferEnd, repOffsets, decompressed, decompressedStart,
            thisBlockEnd, subBlockEnd, literalFlags);
      } else {
        results = decode_lz_subblock_literal_mode_selector<0, 0>(
            tokenStreamIt, literalStreamsIt, lengthStreamIt, distanceStreamIt,
            tokenBufferEnd, literalBuffersEnd, lengthBufferEnd,
            distanceBufferEnd, repOffsets, decompressed, decompressedStart,
            thisBlockEnd, subBlockEnd, literalFlags);
      }
    }

    if (unlikely(results.error)) return results.error;
    tokenStreamIt = results.tokenStreamIt;
    lengthStreamIt = results.lengthStreamIt;
    distanceStreamIt = results.distanceStreamIt;
    decompressed = results.decompressed;
  }

  return 0;
}

size_t decompress_single_thread(const uint8_t* compressed,
                                size_t compressedSize, uint8_t* decompressed,
                                size_t decompressedSize,
                                ProgressCallback* progress,
                                AllocatorCallback* allocator) {
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

  // This buffer is supposed to store all the entropy symbols and advanced
  // decoded distances. The advanced decoded distances should be aligned on 4
  // bytes to preserve performance. Let's see how big this buffer should be:
  //  - If using basic distances: theoretically you can have up to 1 million
  //  symbols per block,
  //    but it doesn't make sense. In general we can expect to not have much
  //    more symbols than the block size.
  //  - If using advanced distances: worst case is the entire block consists of
  //  len 3 non rep matches.
  //    In this case we need 6 bytes of buffer (1 token, 1 distance token, 4
  //    distance) for every 3 bytes of decoded data. We also round to the next
  //    multiple of 4 because of the 4 byte alignment.
  // The second option is more restrictive. I made a small program to make sure
  // I am allocating enough memory
  /*
   * for (int i = 1; i < (1 << 18) - 1; i++) {
   *	  int allocatedMemory = (i * 2 + 3) & ~3;
   *	  int numberOfSequences = i / 3; //floor
   *	  int spaceUsed = numberOfSequences * 2;
   *	  //add remaining literals
   *	  spaceUsed += (i - numberOfSequences * 3);
   *	  //round up for alignment
   *	  spaceUsed = (spaceUsed + 3) & ~3;
   *	  spaceUsed += numberOfSequences * 4;
   *	  if (spaceUsed > allocatedMemory)
   *		  printf("\nError, for a size of %d not enough memory is
   *allocated", i);
   * }
   */
  size_t decodeBufferSize =
      (std::min(MAX_BLOCK_SIZE, decompressedSize) * 2 + 3) & ~3;
  EntropyDecodeBuffer entropyDecodeBuffer;
  entropyDecodeBuffer.construct(allocator, decodeBufferSize);

  HuffmanDecoder literalDecoder(LITERAL_STREAM);
  HuffmanDecoder tokenDecoder(TOKEN_STREAM);
  HuffmanDecoder lengthDecoder(LENGTH_STREAM);
  HuffmanDecoder distanceDecoder(DISTANCE_STREAM);
  uint32_t repOffsets[7] = {1, 1, 1, 1, 1, 1, 1};

  while (true) {
    size_t thisBlockSize;
    int blockType, blockFlags;
    if (unlikely(read_header(compressed, compressedEnd, &thisBlockSize,
                             &blockType, &blockFlags)))
      return ERROR_CORRUPT;

    if (blockType == BLOCK_RAW) {
      // Not enough bytes in either buffer
      if (decompressedStart + decompressedSize - decompressed < thisBlockSize ||
          compressedStart + compressedSize - compressed < thisBlockSize)
        return ERROR_CORRUPT;
      memcpy(decompressed, compressed, thisBlockSize);
      decompressed += thisBlockSize;
      compressed += thisBlockSize;

      if (progress) {
        if (progress->progress(decompressed - decompressedStart,
                               compressed - compressedStart))
          return 0;
      }
      if (blockFlags & BLOCK_LAST) return 0;
      continue;
    }

    // Decode LZ+Entropy block
    uint8_t* const thisBlockEnd = decompressed + thisBlockSize;
    if (unlikely(thisBlockEnd > decompressedEnd ||
                 blockType != BLOCK_COMPRESSED || (blockFlags & BLOCK_LAST)))
      return ERROR_CORRUPT;
    // First byte of first block is sent as is
    if (decompressed == decompressedStart) *decompressed++ = *compressed++;

    const uint8_t* literalBuffersEnd[4];
    const uint8_t* tokenBufferEnd;
    const uint8_t* lengthBufferEnd;
    const uint8_t* distanceBufferEnd;
    const uint8_t* literalStreamsIt[4];
    const uint8_t* distanceStreamIt;
    const uint8_t* tokenStreamIt;
    const uint8_t* lengthStreamIt;
    int literalFlags, tokenFlags, distanceFlags, lengthFlags;
    entropyDecodeBuffer.decodeBufferUsed = 0;

    size_t numberLiterals = literalDecoder.decode(
        compressed, compressedStart, compressedEnd, &entropyDecodeBuffer,
        literalStreamsIt[0], literalBuffersEnd[0], &literalFlags);
    if (unlikely(is_error(numberLiterals))) return numberLiterals;

    if (literalFlags & STREAM_LITERALS_POS_MASK3) {
      for (size_t stream = 1; stream < 4; stream++) {
        int dummy;
        numberLiterals = literalDecoder.decode(
            compressed, compressedStart, compressedEnd, &entropyDecodeBuffer,
            literalStreamsIt[stream], literalBuffersEnd[stream], &dummy);
        if (unlikely(is_error(numberLiterals))) return numberLiterals;
      }
    }

    size_t numberTokens = tokenDecoder.decode(
        compressed, compressedStart, compressedEnd, &entropyDecodeBuffer,
        tokenStreamIt, tokenBufferEnd, &tokenFlags);
    if (unlikely(is_error(numberTokens))) return numberTokens;

    size_t numberDistances = distanceDecoder.decode(
        compressed, compressedStart, compressedEnd, &entropyDecodeBuffer,
        distanceStreamIt, distanceBufferEnd, &distanceFlags);
    if (unlikely(is_error(numberDistances))) return numberDistances;

    if (distanceFlags & STREAM_DISTANCE_ADVANCED) {
      uint32_t* newDistanceStreamIt;
      size_t error = decode_advanced_distances(
          compressed, compressedEnd, newDistanceStreamIt, &entropyDecodeBuffer,
          distanceStreamIt, numberDistances);
      if (unlikely(error)) return error;
      distanceStreamIt = (uint8_t*)newDistanceStreamIt;
      distanceBufferEnd = distanceStreamIt + numberDistances * 4;
    }

    size_t numberLengths = lengthDecoder.decode(
        compressed, compressedStart, compressedEnd, &entropyDecodeBuffer,
        lengthStreamIt, lengthBufferEnd, &lengthFlags);
    if (unlikely(is_error(numberLengths))) return numberLengths;

    size_t error = decode_lz_block(
        decompressed, decompressedStart, thisBlockEnd, repOffsets,
        literalStreamsIt, tokenStreamIt, distanceStreamIt, lengthStreamIt,
        literalBuffersEnd, tokenBufferEnd, distanceBufferEnd, lengthBufferEnd,
        literalFlags, tokenFlags, distanceFlags, lengthFlags);
    if (unlikely(error)) return error;

    decompressed = thisBlockEnd;
    if (progress) {
      if (progress->progress(decompressed - decompressedStart,
                             compressed - compressedStart))
        return 0;
    }
  }
}

enum {
  THREAD_HUFFMAN_READY,  // This queue slot can be used to decode huffman
  THREAD_LZ_READY,       // This queue slot can be used to decode lz
  THREAD_END,  // Used to notify the other thread to stop in case of error or
               // finalising decoding
};

// Size of decoded huffman blocks queue
const int HUFFMAN_DECODE_AHEAD = 4;
struct ThreadedDecoderBlockInfo {
  EntropyDecodeBuffer entropyDecodeBuffer;
  const uint8_t* literalStreamsIt[4];
  const uint8_t* distanceStreamIt;
  const uint8_t* tokenStreamIt;
  const uint8_t* lengthStreamIt;
  const uint8_t* literalBuffersEnd[4];
  const uint8_t* tokenBufferEnd;
  const uint8_t* lengthBufferEnd;
  const uint8_t* distanceBufferEnd;
  size_t blockSize;
  int blockType;
  int blockFlags;
  int literalFlags;
  int tokenFlags;
  int distanceFlags;
  int lengthFlags;
  std::atomic_int blockStatus;
  std::mutex mtx;
  std::condition_variable condvar;
};

struct LZThreadArgs {
  uint8_t* decompressed;
  size_t decompressedSize;
  std::atomic_size_t* readCompressedBytes;
  ProgressCallback* progress;
  ThreadedDecoderBlockInfo* blockInfo;
};

size_t decompress_lz_thread(void* args) {
  LZThreadArgs* lzargs = (LZThreadArgs*)args;
  uint8_t* decompressed = lzargs->decompressed;
  size_t decompressedSize = lzargs->decompressedSize;
  std::atomic_size_t* readCompressedBytes = lzargs->readCompressedBytes;
  ProgressCallback* progress = lzargs->progress;
  ThreadedDecoderBlockInfo* blockInfo = lzargs->blockInfo;

  uint8_t* const decompressedStart = decompressed;
  uint8_t* const decompressedEnd =
      decompressed + (decompressedSize < SKANDA_LAST_BYTES
                          ? 0
                          : decompressedSize - SKANDA_LAST_BYTES);
  uint32_t repOffsets[7] = {1, 1, 1, 1, 1, 1, 1};

  for (size_t iteration = 0;; iteration++) {
    ThreadedDecoderBlockInfo* thisBlock =
        &blockInfo[iteration % HUFFMAN_DECODE_AHEAD];
    std::unique_lock lock(thisBlock->mtx);
    thisBlock->condvar.wait(lock, [thisBlock] {
      return thisBlock->blockStatus != THREAD_HUFFMAN_READY;
    });

    if (thisBlock->blockStatus == THREAD_END) {
      lock.unlock();
      thisBlock->condvar.notify_one();
      return 0;
    }

    if (thisBlock->blockType == BLOCK_RAW) {
      memcpy(decompressed, thisBlock->literalStreamsIt[0],
             thisBlock->blockSize);
      decompressed += thisBlock->blockSize;
    } else {
      uint8_t* thisBlockEnd = decompressed + thisBlock->blockSize;
      // First byte of first block is sent as is. The other thread already
      // copied the byte
      if (decompressed == decompressedStart) decompressed++;

      size_t error = decode_lz_block(
          decompressed, decompressedStart, thisBlockEnd, repOffsets,
          thisBlock->literalStreamsIt, thisBlock->tokenStreamIt,
          thisBlock->distanceStreamIt, thisBlock->lengthStreamIt,
          thisBlock->literalBuffersEnd, thisBlock->tokenBufferEnd,
          thisBlock->distanceBufferEnd, thisBlock->lengthBufferEnd,
          thisBlock->literalFlags, thisBlock->tokenFlags,
          thisBlock->distanceFlags, thisBlock->lengthFlags);
      if (unlikely(error)) {
        thisBlock->blockStatus = THREAD_END;
        lock.unlock();
        thisBlock->condvar.notify_one();
        return error;
      }
      decompressed = thisBlockEnd;
    }

    if (progress) {
      if (progress->progress(decompressed - decompressedStart,
                             *readCompressedBytes)) {
        thisBlock->blockStatus = THREAD_END;
        lock.unlock();
        thisBlock->condvar.notify_one();
        return 0;
      }
    }

    if (thisBlock->blockFlags & BLOCK_LAST) return 0;

    // Notify this slot is ready to be used to decode huffman symbols
    thisBlock->blockStatus = THREAD_HUFFMAN_READY;
    lock.unlock();
    thisBlock->condvar.notify_one();
  }
}

void error_cleanup(ThreadedDecoderBlockInfo* thisBlock,
                   std::unique_lock<std::mutex>* lock,
                   std::future<size_t>* lzthread) {
  thisBlock->blockStatus = THREAD_END;
  lock->unlock();
  thisBlock->condvar.notify_one();
  lzthread->wait();
}

size_t decompress_multi_thread(const uint8_t* compressed, size_t compressedSize,
                               uint8_t* decompressed, size_t decompressedSize,
                               ThreadCallback* threadPool,
                               AllocatorCallback* allocator,
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

  // Allow to buffer the decoded huffman symbols of a few blocks ahead
  ThreadedDecoderBlockInfo blockInfo[HUFFMAN_DECODE_AHEAD];
  size_t decodeBufferSize =
      (std::min(MAX_BLOCK_SIZE, decompressedSize) * 2 + 3) & ~3;
  for (int i = 0; i < HUFFMAN_DECODE_AHEAD; i++) {
    blockInfo[i].entropyDecodeBuffer.construct(allocator, decodeBufferSize);
    blockInfo[i].blockStatus = THREAD_HUFFMAN_READY;
  }

  std::atomic_size_t readCompressedBytes;
  std::future<size_t> lzthread;

  LZThreadArgs lzargs;
  lzargs.decompressed = decompressed;
  lzargs.decompressedSize = decompressedSize;
  lzargs.readCompressedBytes = &readCompressedBytes;
  lzargs.progress = progress;
  lzargs.blockInfo = blockInfo;

  try {
    lzthread = threadPool->enqueue(decompress_lz_thread, &lzargs);
  } catch (std::bad_alloc& e) {
    return ERROR_NOMEM;
  } catch (...) {
    return ERROR_THREADSTART;
  }

  HuffmanDecoder literalDecoder(LITERAL_STREAM);
  HuffmanDecoder tokenDecoder(TOKEN_STREAM);
  HuffmanDecoder lengthDecoder(LENGTH_STREAM);
  HuffmanDecoder distanceDecoder(DISTANCE_STREAM);

  for (size_t iteration = 0;; iteration++) {
    ThreadedDecoderBlockInfo* thisBlock =
        &blockInfo[iteration % HUFFMAN_DECODE_AHEAD];
    std::unique_lock lock(thisBlock->mtx);
    thisBlock->condvar.wait(lock, [thisBlock] {
      return thisBlock->blockStatus != THREAD_LZ_READY;
    });

    // An error ocurred during lz decoding or decode operation was halted
    if (thisBlock->blockStatus != THREAD_HUFFMAN_READY) return lzthread.get();

    size_t thisBlockSize;
    int blockType, blockFlags;
    if (unlikely(read_header(compressed, compressedEnd, &thisBlockSize,
                             &blockType, &blockFlags))) {
      error_cleanup(thisBlock, &lock, &lzthread);
      return ERROR_CORRUPT;
    }
    thisBlock->blockSize = thisBlockSize;
    thisBlock->blockType = blockType;
    thisBlock->blockFlags = blockFlags;

    if (blockType == BLOCK_RAW) {
      // Not enough bytes in either buffer
      if (decompressedStart + decompressedSize - decompressed < thisBlockSize ||
          compressedStart + compressedSize - compressed < thisBlockSize) {
        error_cleanup(thisBlock, &lock, &lzthread);
        return ERROR_CORRUPT;
      }
      // Use the literal stream pointer to inform the lz thread where to copy
      // from
      thisBlock->literalStreamsIt[0] = compressed;
      decompressed += thisBlockSize;
      compressed += thisBlockSize;
    } else {
      // Last block must be uncompressed
      if (thisBlock->blockFlags & BLOCK_LAST) {
        error_cleanup(thisBlock, &lock, &lzthread);
        return ERROR_CORRUPT;
      }

      // Decode LZ+Huffman block
      uint8_t* const thisBlockEnd = decompressed + thisBlockSize;
      if (unlikely(thisBlockEnd > decompressedEnd ||
                   blockType != BLOCK_COMPRESSED)) {
        error_cleanup(thisBlock, &lock, &lzthread);
        return ERROR_CORRUPT;
      }
      // First byte of first block is sent as is
      if (decompressed == decompressedStart) *decompressed = *compressed++;
      thisBlock->entropyDecodeBuffer.decodeBufferUsed = 0;

      size_t numberLiterals = literalDecoder.decode(
          compressed, compressedStart, compressedEnd,
          &thisBlock->entropyDecodeBuffer, thisBlock->literalStreamsIt[0],
          thisBlock->literalBuffersEnd[0], &thisBlock->literalFlags);
      if (is_error(numberLiterals)) {
        error_cleanup(thisBlock, &lock, &lzthread);
        return numberLiterals;
      }

      if (thisBlock->literalFlags & STREAM_LITERALS_POS_MASK3) {
        for (size_t stream = 1; stream < 4; stream++) {
          int dummy;
          numberLiterals = literalDecoder.decode(
              compressed, compressedStart, compressedEnd,
              &thisBlock->entropyDecodeBuffer,
              thisBlock->literalStreamsIt[stream],
              thisBlock->literalBuffersEnd[stream], &dummy);
          if (is_error(numberLiterals)) {
            error_cleanup(thisBlock, &lock, &lzthread);
            return numberLiterals;
          }
        }
      }

      size_t numberTokens = tokenDecoder.decode(
          compressed, compressedStart, compressedEnd,
          &thisBlock->entropyDecodeBuffer, thisBlock->tokenStreamIt,
          thisBlock->tokenBufferEnd, &thisBlock->tokenFlags);
      if (is_error(numberTokens)) {
        error_cleanup(thisBlock, &lock, &lzthread);
        return numberTokens;
      }

      size_t numberDistances = distanceDecoder.decode(
          compressed, compressedStart, compressedEnd,
          &thisBlock->entropyDecodeBuffer, thisBlock->distanceStreamIt,
          thisBlock->distanceBufferEnd, &thisBlock->distanceFlags);
      if (is_error(numberDistances)) {
        error_cleanup(thisBlock, &lock, &lzthread);
        return numberDistances;
      }

      if (thisBlock->distanceFlags & STREAM_DISTANCE_ADVANCED) {
        uint32_t* newDistanceStreamIt;
        size_t error = decode_advanced_distances(
            compressed, compressedEnd, newDistanceStreamIt,
            &thisBlock->entropyDecodeBuffer, thisBlock->distanceStreamIt,
            numberDistances);
        if (error) {
          error_cleanup(thisBlock, &lock, &lzthread);
          return error;
        }
        thisBlock->distanceStreamIt = (uint8_t*)newDistanceStreamIt;
        thisBlock->distanceBufferEnd =
            thisBlock->distanceStreamIt + numberDistances * 4;
      }

      size_t numberLengths = lengthDecoder.decode(
          compressed, compressedStart, compressedEnd,
          &thisBlock->entropyDecodeBuffer, thisBlock->lengthStreamIt,
          thisBlock->lengthBufferEnd, &thisBlock->lengthFlags);
      if (is_error(numberLengths)) {
        error_cleanup(thisBlock, &lock, &lzthread);
        return numberLengths;
      }

      decompressed = thisBlockEnd;
    }

    // Notify this slot is ready to be used to decode lz
    readCompressedBytes = compressed - compressedStart;
    thisBlock->blockStatus = THREAD_LZ_READY;
    lock.unlock();
    thisBlock->condvar.notify_one();

    // Wait until lz decoder thread finishes everything
    if (blockFlags & BLOCK_LAST) {
      size_t lzerror = lzthread.get();
      return lzerror;
    }
  }
}

size_t decompress(const uint8_t* compressed, size_t compressedSize,
                  uint8_t* decompressed, size_t decompressedSize,
                  ThreadCallback* threadPool, AllocatorCallback* allocator,
                  ProgressCallback* progress) {
  AllocatorCallback defaultAllocator;
  if (!allocator) allocator = &defaultAllocator;
  // Thread initialization cost outweights speed gain though there is no clear
  // threshold for this. It heavily depends on how the file is compressed and
  // the cost of thread creation
  if (!threadPool || decompressedSize < 524288)
    return decompress_single_thread(compressed, compressedSize, decompressed,
                                    decompressedSize, progress, allocator);
  return decompress_multi_thread(compressed, compressedSize, decompressed,
                                 decompressedSize, threadPool, allocator,
                                 progress);
}
}  // namespace skanda

#endif  // SKANDA_IMPLEMENTATION

namespace lynx {
namespace base {
namespace skanda {

using ProgressCallback = ::skanda::ProgressCallback;

inline size_t compress(const uint8_t* input, size_t size, uint8_t* output,
                       int level = 2, float decSpeedBias = 0.5f,
                       ProgressCallback* progress = nullptr) {
  return ::skanda::compress(input, size, output, level, decSpeedBias, nullptr,
                            progress);
}

inline size_t decompress(const uint8_t* compressed, size_t compressed_size,
                         uint8_t* decompressed, size_t decompressed_size,
                         ProgressCallback* progress = nullptr) {
  return ::skanda::decompress(compressed, compressed_size, decompressed,
                              decompressed_size, nullptr, nullptr, progress);
}

inline size_t compress_bound(size_t size) {
  return ::skanda::compress_bound(size);
}

inline size_t estimate_memory(size_t size, int level = 2,
                              float decSpeedBias = 0.5f) {
  return ::skanda::estimate_memory(size, level, decSpeedBias);
}

inline bool is_error(size_t error_code) {
  return ::skanda::is_error(error_code);
}

}  // namespace skanda
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_SKANDA_H_
