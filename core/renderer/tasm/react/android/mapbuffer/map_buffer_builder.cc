// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include "core/renderer/tasm/react/android/mapbuffer/map_buffer_builder.h"

#include <algorithm>
#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace android {

MapBuffer MapBufferBuilder::EMPTY() { return MapBufferBuilder(0).build(); }

void MapBufferBuilder::storeKeyValue(MapBuffer::Key key,
                                     MapBuffer::DataType type,
                                     const uint8_t* value, uint32_t valueSize) {
  if (valueSize > MAX_BUCKET_VALUE_SIZE) {
    LOGE("Error: size of value must be <= MAX_VALUE_SIZE. ValueSize: "
         << valueSize);
    return;
  }

  uint64_t data = 0;
  auto* dataPtr = reinterpret_cast<uint8_t*>(&data);
  memcpy(dataPtr, value, valueSize);

  buckets_.emplace_back(key, static_cast<uint16_t>(type), data);

  header_.count++;

  if (lastKey_ > key) {
    needsSort_ = true;
  }
  lastKey_ = key;
}

void MapBufferBuilder::putBool(MapBuffer::Key key, bool value) {
  int intValue = static_cast<int>(value);
  storeKeyValue(key, MapBuffer::DataType::Boolean,
                reinterpret_cast<const uint8_t*>(&intValue), INT_SIZE);
}

void MapBufferBuilder::putDouble(MapBuffer::Key key, double value) {
  storeKeyValue(key, MapBuffer::DataType::Double,
                reinterpret_cast<const uint8_t*>(&value), DOUBLE_SIZE);
}

void MapBufferBuilder::putNull(MapBuffer::Key key) {
  // todo(nihao.royal) support putNull later~
}

void MapBufferBuilder::putInt(MapBuffer::Key key, int32_t value) {
  storeKeyValue(key, MapBuffer::DataType::Int,
                reinterpret_cast<const uint8_t*>(&value), INT_SIZE);
}

void MapBufferBuilder::putLong(MapBuffer::Key key, int64_t value) {
  storeKeyValue(key, MapBuffer::DataType::Long,
                reinterpret_cast<uint8_t const*>(&value), LONG_SIZE);
}

void MapBufferBuilder::putString(MapBuffer::Key key, const char* value) {
  auto offset = WriteString(value);
  // Store Key and pointer to the string
  storeKeyValue(key, MapBuffer::DataType::String,
                reinterpret_cast<const uint8_t*>(&offset), INT_SIZE);
}

void MapBufferBuilder::putMapBuffer(MapBuffer::Key key, const MapBuffer& map) {
  auto mapBufferSize = map.size();

  auto offset = dynamicData_.size();

  // format [length of buffer (int)] + [bytes of MapBuffer]
  VectorResizeNotInitialize(dynamicData_, offset + INT_SIZE + mapBufferSize);
  memcpy(dynamicData_.data() + offset, &mapBufferSize, INT_SIZE);
  // Copy the content of the map into dynamicData_
  memcpy(dynamicData_.data() + offset + INT_SIZE, map.data(), mapBufferSize);

  // Store Key and pointer to the string
  storeKeyValue(key, MapBuffer::DataType::Array,
                reinterpret_cast<const uint8_t*>(&offset), INT_SIZE);
}

void MapBufferBuilder::putMapBufferList(
    MapBuffer::Key key, const std::vector<MapBuffer>& mapBufferList) {
  int32_t offset = dynamicData_.size();
  int32_t dataSize = 0;
  for (const MapBuffer& mapBuffer : mapBufferList) {
    dataSize = dataSize + INT_SIZE + mapBuffer.size();
  }

  VectorResizeNotInitialize(dynamicData_, offset + INT_SIZE);
  memcpy(dynamicData_.data() + offset, &dataSize, INT_SIZE);

  for (const MapBuffer& mapBuffer : mapBufferList) {
    int32_t mapBufferSize = mapBuffer.size();
    int32_t dynamicDataSize = dynamicData_.size();
    VectorResizeNotInitialize(dynamicData_,
                              dynamicDataSize + INT_SIZE + mapBufferSize);
    // format [length of buffer (int)] + [bytes of MapBuffer]
    memcpy(dynamicData_.data() + dynamicDataSize, &mapBufferSize, INT_SIZE);
    // Copy the content of the map into dynamicData_
    memcpy(dynamicData_.data() + dynamicDataSize + INT_SIZE, mapBuffer.data(),
           mapBufferSize);
  }

  // Store Key and pointer to the string
  storeKeyValue(key, MapBuffer::DataType::Array,
                reinterpret_cast<const uint8_t*>(&offset), INT_SIZE);
}

static inline bool compareBuckets(const MapBuffer::Bucket& a,
                                  const MapBuffer::Bucket& b) {
  return a.key < b.key;
}

void MapBufferBuilder::beforeBuild() {
  if (needsSort_) {
    std::sort(buckets_.begin(), buckets_.end(), compareBuckets);
  }
}

}  // namespace android
}  // namespace base
}  // namespace lynx
