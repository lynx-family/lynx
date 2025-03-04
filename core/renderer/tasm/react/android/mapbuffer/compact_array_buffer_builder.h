// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_COMPACT_ARRAY_BUFFER_BUILDER_H_
#define CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_COMPACT_ARRAY_BUFFER_BUILDER_H_

#include <vector>

#include "core/renderer/tasm/react/android/mapbuffer/base_buffer_builder.h"
#include "core/renderer/tasm/react/android/mapbuffer/compact_array_buffer.h"

namespace lynx {
namespace base {
namespace android {

class CompactArrayBufferBuilder
    : public BaseByteBufferBuilder<CompactArrayBuffer> {
 public:
  explicit CompactArrayBufferBuilder(
      uint32_t initialSize = BaseBufferBuilder::INITIAL_BUCKETS_SIZE)
      : BaseByteBufferBuilder(initialSize) {}

  void putInt(int32_t value);

  void putDouble(double value);

  void putLong(int64_t value);

  void putIntArray(std::vector<int32_t> intAry);

  void putString(const char* value);

 private:
  void storeKeyValue(const uint8_t* value, uint32_t valueSize);
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_COMPACT_ARRAY_BUFFER_BUILDER_H_
