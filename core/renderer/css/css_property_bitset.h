// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_PROPERTY_BITSET_H_
#define CORE_RENDERER_CSS_CSS_PROPERTY_BITSET_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

#include "base/include/log/logging.h"
#include "core/renderer/css/css_property_id.h"

// clang-format off
#if defined(__clang__) && __has_feature(unsafe_buffer_usage)
// Disable hwasan checking for unsafe buffer accesses.
// Disabling `clang-format` allows each `_Pragma` to be on its own line, as
// recommended by https://gcc.gnu.org/onlinedocs/cpp/Pragmas.html.
#define UNSAFE_BUFFERS(...)                  \
  _Pragma("clang unsafe_buffer_usage begin") \
      __VA_ARGS__                            \
  _Pragma("clang unsafe_buffer_usage end")
#else
#define UNSAFE_BUFFERS(...) __VA_ARGS__
#endif
// clang-format on

#if !defined(UNSAFE_TODO)
#define UNSAFE_TODO(...) UNSAFE_BUFFERS(__VA_ARGS__)
#endif

#if __cplusplus >= 202002L
#include <bit>
#elif defined(__GNUC__) || defined(__clang__)
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

namespace lynx {
namespace tasm {

#if __cplusplus >= 202002L
inline int CountrZero(uint64_t n) { return std::countr_zero(n); }
#elif defined(__GNUC__) || defined(__clang__)
inline int CountrZero(uint64_t n) { return __builtin_ctzll(n); }
#elif defined(_MSC_VER)
inline int CountrZero(uint64_t n) {
  unsigned long index;
  _BitScanForward64(&index, n);
  return static_cast<int>(index);
}
#else
#error "Compiler not supported for CountrZero. Please add a fallback."
#endif

// A bitset designed for CSSPropertyIDs.
template <size_t kBits>
class CSSBitsetBase {
 public:
  static_assert(kBits <= kPropertyEnd,
                "Bit count must not exceed kPropertyEnd");
  static_assert(kBits > 0, "Iterator assumes at least one chunk.");

  static const size_t kChunks = (kBits + 63) / 64;

  CSSBitsetBase() : chunks_() {}
  CSSBitsetBase(const CSSBitsetBase<kBits>& o) { *this = o; }

  // This slightly weird construction helps Clang make an actual
  // compile-time static value, until we have constinit.
  template <int N>
  explicit constexpr CSSBitsetBase(const CSSPropertyID (&list)[N])
      : chunks_(CreateChunks(list)) {}

  CSSBitsetBase& operator=(const CSSBitsetBase& o) = default;

  bool operator==(const CSSBitsetBase& o) const { return chunks_ == o.chunks_; }
  bool operator!=(const CSSBitsetBase& o) const { return !(*this == o); }

  inline void Set(CSSPropertyID id) {
    size_t bit = static_cast<size_t>(static_cast<unsigned>(id));
    UNSAFE_TODO(chunks_.data()[bit / 64]) |= (1ull << (bit % 64));
  }

  inline void Reset(CSSPropertyID id) {
    size_t bit = static_cast<size_t>(static_cast<unsigned>(id));
    UNSAFE_TODO(chunks_.data()[bit / 64]) &= ~(1ull << (bit % 64));
  }

  inline void Or(CSSPropertyID id, bool v) {
    size_t bit = static_cast<size_t>(static_cast<unsigned>(id));
    UNSAFE_TODO(chunks_.data()[bit / 64]) |=
        (static_cast<uint64_t>(v) << (bit % 64));
  }

  void And(const CSSBitsetBase& other) {
    for (size_t i = 0; i < kChunks; ++i) {
      UNSAFE_TODO(chunks_.data()[i] &= other.chunks_.data()[i]);
    }
  }

  void Xor(const CSSBitsetBase& other) {
    for (size_t i = 0; i < kChunks; ++i) {
      UNSAFE_TODO(chunks_.data()[i] ^= other.chunks_.data()[i]);
    }
  }

  void Or(const CSSBitsetBase& other) {
    for (size_t i = 0; i < kChunks; ++i) {
      UNSAFE_TODO(chunks_.data()[i] |= other.chunks_.data()[i]);
    }
  }

  CSSBitsetBase& operator&=(const CSSBitsetBase& other) {
    And(other);
    return *this;
  }

  CSSBitsetBase& operator^=(const CSSBitsetBase& other) {
    Xor(other);
    return *this;
  }

  CSSBitsetBase& operator|=(const CSSBitsetBase& other) {
    Or(other);
    return *this;
  }

  inline bool Has(CSSPropertyID id) const {
    size_t bit = static_cast<size_t>(static_cast<unsigned>(id));
    return UNSAFE_TODO(chunks_.data()[bit / 64]) & (1ull << (bit % 64));
  }

  inline bool HasAny() const {
    for (uint64_t chunk : chunks_) {
      if (chunk) {
        return true;
      }
    }
    return false;
  }

  inline void Reset() {
    UNSAFE_TODO(std::memset(chunks_.data(), 0, sizeof(chunks_)));
  }

  // Yields the CSSPropertyIDs which are set.
  class Iterator {
   public:
    // Only meant for internal use (from begin() or end()).
    Iterator(const uint64_t* chunks, size_t chunk_index, size_t index)
        : chunks_(chunks),
          index_(index),
          chunk_index_(chunk_index),
          chunk_(chunks_[0]) {
      DCHECK(index == 0 || index == kBits);
      if (index < kBits) {
        ++*this;  // Go to the first set bit.
      }
    }

    inline void operator++() {
      // If there are no more bits set in this chunk,
      // skip to the next nonzero chunk (if any exists).
      while (!chunk_) {
        if (++chunk_index_ >= kChunks) {
          index_ = kBits;
          return;
        }
        chunk_ = UNSAFE_TODO(chunks_[chunk_index_]);
      }
      index_ = chunk_index_ * 64 + CountrZero(chunk_);
      chunk_ &= chunk_ - 1;  // Clear the lowest bit.
    }

    inline CSSPropertyID operator*() const {
      return static_cast<CSSPropertyID>(index_);
    }

    inline bool operator==(const Iterator& o) const {
      return index_ == o.index_;
    }
    inline bool operator!=(const Iterator& o) const {
      return index_ != o.index_;
    }

   private:
    const uint64_t* chunks_;
    // The current bit index this Iterator is pointing to. Note that this is
    // the "global" index, i.e. it has the range [0, kBits]. (It is not a local
    // index with range [0, 64]).
    //
    // Never exceeds kBits.
    size_t index_ = 0;
    // The current chunk index this Iterator is pointing to.
    // Points to kChunks if we are done.
    size_t chunk_index_ = 0;
    // The iterator works by "pre-fetching" the current chunk (corresponding
    // (to the current index), and removing its bits one by one.
    // This is not used (contains junk) for the end() iterator.
    uint64_t chunk_ = 0;
  };

  Iterator begin() const { return Iterator(chunks_.data(), 0, 0); }
  Iterator end() const { return Iterator(chunks_.data(), kChunks, kBits); }

 private:
  std::array<uint64_t, kChunks> chunks_;

  template <int N>
  static constexpr std::array<uint64_t, kChunks> CreateChunks(
      const CSSPropertyID (&list)[N]) {
    std::array<uint64_t, kChunks> chunks{};
    for (CSSPropertyID id : list) {
      unsigned bit = static_cast<unsigned>(id);
      chunks[bit / 64] |= uint64_t{1} << (bit % 64);
    }
    return chunks;
  }
};

template <size_t kBits>
inline CSSBitsetBase<kBits> operator&(CSSBitsetBase<kBits> lhs,
                                      const CSSBitsetBase<kBits>& rhs) {
  lhs &= rhs;
  return lhs;
}

template <size_t kBits>
inline CSSBitsetBase<kBits> operator^(CSSBitsetBase<kBits> lhs,
                                      const CSSBitsetBase<kBits>& rhs) {
  lhs ^= rhs;
  return lhs;
}

template <size_t kBits>
inline CSSBitsetBase<kBits> operator|(CSSBitsetBase<kBits> lhs,
                                      const CSSBitsetBase<kBits>& rhs) {
  lhs |= rhs;
  return lhs;
}

using CSSIDBitset = CSSBitsetBase<kPropertyEnd>;
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_CSS_CSS_PROPERTY_BITSET_H_
