// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_BASE_BUFFER_H_
#define CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_BASE_BUFFER_H_

#include <string>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace android {

// clang-format off

/**
 * BaseBuffer data is stored in a continuous chunk of memory (bytes_ field below) with the following layout:
 *
 * ┌─────────────────────Header──────────────────────┐
 * │                    10 bytes                     │
 * ├─Alignment─┬─Item count─┬──────Buffer size───────┤
 * │  2 bytes  │  2 bytes   │        4 bytes         │
 * └───────────┴────────────┴────────────────────────┘
 * ┌─────────────────────────────────────────────┐
 * │    Buckets (one per item in the map)        │
 * ├────Bucket─────┬────Bucket─────┬─────────────┤
 * │ several bytes │ several bytes │             │
 * └───────────────┴───────────────┴─────────────┘
 * ┌────────────────────────────────────────────────────────────────────────────────────────┐
 * │  Dynamic data                                                                          │
 * │                                                                                        │
 * │  Free-form data for complex objects (e.g. strings or nested MapBuffers).               │
 * │  When dynamic data is serialized with some object, bucket value contains an offset of  │
 * │  associated byte in the array. The format of the data is not restricted, but common    │
 * │  practice is to use [length | bytes].                                                  │
 * └────────────────────────────────────────────────────────────────────────────────────────┘
 */

// clang-format on

class BaseBuffer {
 public:
  // The first value in the buffer, used to check correct encoding/endianness on
  // JVM side.
  constexpr static uint16_t HEADER_ALIGNMENT = 0xFE;

  struct Header {
    uint16_t alignment = HEADER_ALIGNMENT;  // alignment of serialization
    uint16_t count;                         // amount of items in the map
    uint32_t bufferSize;  // Amount of bytes used to store the map in memory
  };

  /**
   * Data types available for serialization in MapBuffer
   * Keep in sync with `DataType` enum in `JReadableMapBuffer.java`, which
   * expects the same values after reading them through JNI.
   */
  enum class DataType : uint16_t {
    Null = 0,
    Boolean = 1,
    Int = 2,
    Long = 3,
    Double = 4,
    String = 5,
    Array = 6,
  };

  explicit BaseBuffer(std::vector<uint8_t> data) : bytes_(std::move(data)) {
    auto header = reinterpret_cast<const Header*>(bytes_.data());
    count_ = header->count;

    if (header->bufferSize != bytes_.size()) {
      LOGE("Error: Data size does not match, expected "
           << header->bufferSize << " found: " << bytes_.size());
      count_ = 0;
      bytes_.clear();
    }
  }

  virtual ~BaseBuffer() = default;
  BaseBuffer(const BaseBuffer& buffer) = delete;
  BaseBuffer& operator=(const BaseBuffer& other) = delete;
  BaseBuffer(BaseBuffer&& buffer) = default;
  BaseBuffer& operator=(BaseBuffer&& other) = default;

  size_t size() const { return bytes_.size(); }

  const uint8_t* data() const { return bytes_.data(); }

  uint16_t count() const { return count_; }

 protected:
  int32_t ReadInt(int32_t offset) const {
    return *reinterpret_cast<const int32_t*>(bytes_.data() + offset);
  }

  int64_t ReadLong(int32_t offset) const {
    return *reinterpret_cast<const int64_t*>(bytes_.data() + offset);
  }

  double ReadDouble(int32_t offset) const {
    return *reinterpret_cast<const double*>(bytes_.data() + offset);
  }

  std::string ReadString(int32_t offset) const {
    int32_t stringLength =
        *reinterpret_cast<const int32_t*>(bytes_.data() + offset);
    const uint8_t* stringPtr = bytes_.data() + offset + sizeof(int);

    return {stringPtr, stringPtr + stringLength};
  }

  // Buffer and its size
  std::vector<uint8_t> bytes_;

  // amount of items in the MapBuffer
  uint16_t count_ = 0;
};

template <typename T>
class BaseByteBuffer : public BaseBuffer {
 public:
  explicit BaseByteBuffer(std::vector<uint8_t> data)
      : BaseBuffer(std::move(data)) {}

  ~BaseByteBuffer() override = default;
  BaseByteBuffer(const BaseByteBuffer& buffer) = delete;
  BaseByteBuffer& operator=(const BaseByteBuffer& other) = delete;
  BaseByteBuffer(BaseByteBuffer&& buffer) = default;
  BaseByteBuffer& operator=(BaseByteBuffer&& other) = default;

 protected:
  int32_t bucketOffset(int32_t index) const {
    return sizeof(Header) + sizeof(T) * index;
  }

  int32_t valueOffset(int32_t bucketIndex) const {
    return bucketOffset(bucketIndex) + offsetof(T, data);
  }

  // returns the relative offset of the first byte of dynamic data
  int32_t getDynamicDataOffset() const {
    // The start of dynamic data can be calculated as the offset of the next
    // key in the map
    return bucketOffset(count_);
  }
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_BASE_BUFFER_H_
