// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/react/android/mapbuffer/compact_array_buffer.h"

#include <utility>

namespace lynx {
namespace base {
namespace android {

int32_t CompactArrayBuffer::getInt(uint32_t index) const {
  return ReadInt(valueOffset(index));
}

std::string CompactArrayBuffer::getString(uint32_t index) const {
  int32_t dynamicDataOffset = getDynamicDataOffset();
  int32_t offset = getInt(index);
  return ReadString(dynamicDataOffset + offset);
}

int64_t CompactArrayBuffer::getLong(uint32_t index) const {
  return ReadLong(valueOffset(index));
}

double CompactArrayBuffer::getDouble(uint32_t index) const {
  return ReadDouble(valueOffset(index));
}

}  // namespace android
}  // namespace base
}  // namespace lynx
