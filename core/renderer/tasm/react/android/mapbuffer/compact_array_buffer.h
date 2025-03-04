// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_COMPACT_ARRAY_BUFFER_H_
#define CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_COMPACT_ARRAY_BUFFER_H_

#include <string>
#include <utility>
#include <vector>

#include "core/renderer/tasm/react/android/mapbuffer/base_buffer.h"

namespace lynx {
namespace base {
namespace android {

// clang-format off

/**
 * CompactArrayBuffer is an optimized sparse array format for transferring array
 * objects between C++ and other VMs. The implementation of this array is optimized to:
 * - be compact to optimize space when sparse (sparse is the common case).
 * - be accessible through JNI with zero/minimal copying via ByteBuffer.
 *
 * CompactArrayBuffer buckets data is stored in a continuous chunk of memory (bytes_ field below) with the following layout:
 *
 * ┌────────────────────────────────────────────────────────────────────┐
 * │               Buckets (one per item in the map)                    │
 * │                                                                    │
 * ├────────────────Bucket──────────────────┬───Bucket────┬─────────────┤
 * ├───────Value (primitive or offset)──────┤     ...     │     ...     │
 * │                8 bytes                 │             │             │
 * └────────────────────────────────────────┴─────────────┴─────────────┘
 */

// clang-format on

#pragma pack(push, 1)
struct CompactArrayBufferBucket {
  uint64_t data;

  CompactArrayBufferBucket(uint64_t data) : data(data) {}
};
#pragma pack(pop)
class CompactArrayBuffer : public BaseByteBuffer<CompactArrayBufferBucket> {
 public:
  using Bucket = CompactArrayBufferBucket;

  explicit CompactArrayBuffer(std::vector<uint8_t> data)
      : BaseByteBuffer(std::move(data)){};
  ~CompactArrayBuffer() = default;
  CompactArrayBuffer(const CompactArrayBuffer& buffer) = delete;
  CompactArrayBuffer& operator=(const CompactArrayBuffer& other) = delete;
  CompactArrayBuffer(CompactArrayBuffer&& buffer) = default;
  CompactArrayBuffer& operator=(CompactArrayBuffer&& other) = default;

  int32_t getInt(uint32_t index) const;

  std::string getString(uint32_t index) const;

  int64_t getLong(uint32_t index) const;

  double getDouble(uint32_t index) const;

  friend class JReadableCompactArrayBuffer;
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_COMPACT_ARRAY_BUFFER_H_
