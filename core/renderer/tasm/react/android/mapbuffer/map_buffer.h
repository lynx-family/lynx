// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_MAP_BUFFER_H_
#define CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_MAP_BUFFER_H_

#include <string>
#include <utility>
#include <vector>

#include "core/renderer/tasm/react/android/mapbuffer/base_buffer.h"

namespace lynx {
namespace base {
namespace android {

class JReadableMapBuffer;

// clang-format off

/**
 * MapBuffer is an optimized sparse array format for transferring props-like
 * objects between C++ and other VMs. The implementation of this map is optimized to:
 * - be compact to optimize space when sparse (sparse is the common case).
 * - be accessible through JNI with zero/minimal copying via ByteBuffer.
 * - have excellent C++ single-write and many-read performance by maximizing
 *   CPU cache performance through compactness, data locality, and fixed offsets
 *   where possible.
 * - be optimized for iteration and intersection against other maps, but with
 *   reasonably good random access as well.
 * - work recursively for nested maps/arrays.
 * - support dynamic types that map to JSON.
 * - don't require mutability/copy - single-write on creation and move semantics.
 * - have minimal APK size and build time impact.
 *
 * MapBuffer buckets data is stored in a continuous chunk of memory (bytes_ field below) with the following layout:
 * ┌────────────────────────────────────────────────────────────────────────────────────────┐
 * │                           Buckets (one per item in the map)                            │
 * │                                                                                        │
 * ├───────────────────────────Bucket───────────────────────────┬───Bucket────┬─────────────┤
 * │                          12 bytes                          │  12 bytes   │             │
 * ├───Key───┬──Type───┬──────Value (primitive or offset)───────┤     ...     │     ...     │
 * │ 2 bytes │ 2 bytes │                8 bytes                 │             │             │
 * └─────────┴─────────┴────────────────────────────────────────┴─────────────┴─────────────┘
 */

// clang-format on

#pragma pack(push, 1)
struct MapBufferBucket {
  uint16_t key;
  uint16_t type;
  uint64_t data;

  MapBufferBucket(uint16_t key, uint16_t type, uint64_t data)
      : key(key), type(type), data(data) {}
};
#pragma pack(pop)

class MapBuffer : public BaseByteBuffer<MapBufferBucket> {
 public:
  using Key = uint16_t;
  using Bucket = MapBufferBucket;

  explicit MapBuffer(std::vector<uint8_t> data)
      : BaseByteBuffer(std::move(data)) {}
  ~MapBuffer() override = default;
  MapBuffer(const MapBuffer& buffer) = delete;
  MapBuffer& operator=(const MapBuffer& other) = delete;
  MapBuffer(MapBuffer&& buffer) = default;
  MapBuffer& operator=(MapBuffer&& other) = default;

  int32_t getInt(MapBuffer::Key key) const;

  bool getBool(MapBuffer::Key key) const;

  double getDouble(MapBuffer::Key key) const;

  std::string getString(MapBuffer::Key key) const;

  MapBuffer getMapBuffer(MapBuffer::Key key) const;

  std::vector<MapBuffer> getMapBufferList(MapBuffer::Key key) const;

 private:
  int32_t getKeyBucket(MapBuffer::Key key) const;

  friend JReadableMapBuffer;
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_MAP_BUFFER_H_
