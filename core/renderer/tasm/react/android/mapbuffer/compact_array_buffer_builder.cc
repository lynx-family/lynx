// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/react/android/mapbuffer/compact_array_buffer_builder.h"

#include <utility>

namespace lynx {
namespace base {
namespace android {

void CompactArrayBufferBuilder::storeKeyValue(const uint8_t* value,
                                              uint32_t valueSize) {
  uint64_t data = 0;
  auto* dataPtr = reinterpret_cast<uint8_t*>(&data);
  memcpy(dataPtr, value, valueSize);

  buckets_.emplace_back(data);

  header_.count++;
}

void CompactArrayBufferBuilder::putInt(int32_t value) {
  storeKeyValue(reinterpret_cast<const uint8_t*>(&value), INT_SIZE);
}

void CompactArrayBufferBuilder::putDouble(double value) {
  storeKeyValue(reinterpret_cast<const uint8_t*>(&value), DOUBLE_SIZE);
}

void CompactArrayBufferBuilder::putLong(int64_t value) {
  storeKeyValue(reinterpret_cast<const uint8_t*>(&value), LONG_SIZE);
}

void CompactArrayBufferBuilder::putIntArray(std::vector<int32_t> intAry) {
  putInt(intAry.size());
  for (int32_t e : intAry) {
    putInt(e);
  }
}

void CompactArrayBufferBuilder::putString(const char* value) {
  auto offset = WriteString(value);
  // Store Key and pointer to the string
  storeKeyValue(reinterpret_cast<const uint8_t*>(&offset), INT_SIZE);
}

}  // namespace android
}  // namespace base
}  // namespace lynx
