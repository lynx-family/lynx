// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_BASE_BUFFER_BUILDER_H_
#define CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_BASE_BUFFER_BUILDER_H_

#include <utility>
#include <vector>

#include "core/renderer/tasm/react/android/mapbuffer/base_buffer.h"

namespace lynx {
namespace base {
namespace android {

template <typename T, typename = std::enable_if<std::is_integral_v<T>>>
inline void VectorResizeNotInitialize(std::vector<T>& input,
                                      typename std::vector<T>::size_type size) {
  struct UninitializedT {
    UninitializedT() = default;
    UninitializedT(T value) : value(value){};
    T value;
  };

  reinterpret_cast<std::vector<UninitializedT>*>(&input)->resize(size);
}

class BaseBufferBuilder {
 public:
  using Header = BaseBuffer::Header;

  BaseBufferBuilder() {
    header_.count = 0;
    header_.bufferSize = 0;
  }

  virtual ~BaseBufferBuilder() = default;

  bool empty() const { return header_.count == 0; }

 protected:
  size_t WriteString(const char* str) {
    auto strSize = strlen(str);
    // format [length of string (int)] + [Array of Characters in the string]
    auto offset = dynamicData_.size();
    VectorResizeNotInitialize(dynamicData_, offset + INT_SIZE + strSize);
    memcpy(dynamicData_.data() + offset, &strSize, INT_SIZE);
    memcpy(dynamicData_.data() + offset + INT_SIZE, str, strSize);
    return offset;
  }

  static constexpr uint32_t INT_SIZE = sizeof(uint32_t);
  static constexpr uint32_t LONG_SIZE = sizeof(uint64_t);
  static constexpr uint32_t DOUBLE_SIZE = sizeof(double);
  static constexpr uint32_t MAX_BUCKET_VALUE_SIZE = sizeof(uint64_t);
  // Default reserved size for buckets_ vector
  static constexpr uint32_t INITIAL_BUCKETS_SIZE = 10;

  Header header_;
  std::vector<uint8_t> dynamicData_{};
};

template <typename T>
class BaseByteBufferBuilder : public BaseBufferBuilder {
 public:
  using Bucket = typename T::Bucket;

  explicit BaseByteBufferBuilder(uint32_t initialSize) : BaseBufferBuilder() {
    header_.count = 0;
    header_.bufferSize = 0;
    buckets_.reserve(initialSize);
  }

  virtual void beforeBuild(){};

  T build() {
    beforeBuild();
    // Create buffer: [header] + [buckets] + [dynamic data]
    auto bucketSize = buckets_.size() * sizeof(Bucket);
    auto headerSize = sizeof(Header);
    auto bufferSize = headerSize + bucketSize + dynamicData_.size();

    header_.bufferSize = static_cast<uint32_t>(bufferSize);

    std::vector<uint8_t> buffer;
    VectorResizeNotInitialize(buffer, bufferSize);
    memcpy(buffer.data(), &header_, headerSize);
    memcpy(buffer.data() + headerSize, buckets_.data(), bucketSize);
    memcpy(buffer.data() + headerSize + bucketSize, dynamicData_.data(),
           dynamicData_.size());

    return T(std::move(buffer));
  }

 protected:
  std::vector<Bucket> buckets_{};
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_BASE_BUFFER_BUILDER_H_
