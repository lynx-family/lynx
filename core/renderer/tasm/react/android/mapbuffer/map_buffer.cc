// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "core/renderer/tasm/react/android/mapbuffer/map_buffer.h"

#include <utility>

namespace lynx {
namespace base {
namespace android {

int32_t MapBuffer::getKeyBucket(Key key) const {
  int32_t lo = 0;
  int32_t hi = count_ - 1;
  while (lo <= hi) {
    int32_t mid = (lo + hi) >> 1;

    Key midVal =
        *reinterpret_cast<const Key*>(bytes_.data() + bucketOffset(mid));

    if (midVal < key) {
      lo = mid + 1;
    } else if (midVal > key) {
      hi = mid - 1;
    } else {
      return mid;
    }
  }

  return -1;
}

int32_t MapBuffer::getInt(Key key) const {
  auto bucketIndex = getKeyBucket(key);
  return ReadInt(valueOffset(bucketIndex));
}

bool MapBuffer::getBool(Key key) const { return getInt(key) != 0; }

double MapBuffer::getDouble(Key key) const {
  auto bucketIndex = getKeyBucket(key);

  return *reinterpret_cast<const double*>(bytes_.data() +
                                          valueOffset(bucketIndex));
}

std::string MapBuffer::getString(Key key) const {
  int32_t dynamicDataOffset = getDynamicDataOffset();
  int32_t offset = getInt(key);
  return ReadString(dynamicDataOffset + offset);
}

MapBuffer MapBuffer::getMapBuffer(Key key) const {
  int32_t dynamicDataOffset = getDynamicDataOffset();

  int32_t offset = getInt(key);
  int32_t mapBufferLength = *reinterpret_cast<const int32_t*>(
      bytes_.data() + dynamicDataOffset + offset);

  std::vector<uint8_t> value(mapBufferLength);

  memcpy(value.data(),
         bytes_.data() + dynamicDataOffset + offset + sizeof(int32_t),
         mapBufferLength);

  return MapBuffer(std::move(value));
}

std::vector<MapBuffer> MapBuffer::getMapBufferList(MapBuffer::Key key) const {
  std::vector<MapBuffer> mapBufferList;

  int32_t dynamicDataOffset = getDynamicDataOffset();
  int32_t offset = getInt(key);
  int32_t mapBufferListLength = *reinterpret_cast<const int32_t*>(
      bytes_.data() + dynamicDataOffset + offset);
  offset = offset + sizeof(uint32_t);

  int32_t curLen = 0;
  while (curLen < mapBufferListLength) {
    int32_t mapBufferLength = *reinterpret_cast<const int32_t*>(
        bytes_.data() + dynamicDataOffset + offset + curLen);
    curLen = curLen + sizeof(uint32_t);
    auto begin = bytes_.data() + dynamicDataOffset + offset + curLen;
    std::vector<uint8_t> value(begin, begin + mapBufferLength);
    mapBufferList.emplace_back(MapBuffer(std::move(value)));
    curLen = curLen + mapBufferLength;
  }
  return mapBufferList;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
