// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_MAP_BUFFER_BUILDER_H_
#define CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_MAP_BUFFER_BUILDER_H_

#include <vector>

#include "core/renderer/tasm/react/android/mapbuffer/base_buffer_builder.h"
#include "core/renderer/tasm/react/android/mapbuffer/map_buffer.h"

namespace lynx {
namespace base {
namespace android {

/**
 * MapBufferBuilder is a builder class for MapBuffer
 */
class MapBufferBuilder : public BaseByteBufferBuilder<MapBuffer> {
 public:
  explicit MapBufferBuilder(
      uint32_t initialSize = BaseBufferBuilder::INITIAL_BUCKETS_SIZE)
      : BaseByteBufferBuilder(initialSize) {}

  static MapBuffer EMPTY();

  void putNull(MapBuffer::Key key);

  void putInt(MapBuffer::Key key, int32_t value);

  void putLong(MapBuffer::Key key, int64_t value);

  void putBool(MapBuffer::Key key, bool value);

  void putDouble(MapBuffer::Key key, double value);

  void putString(MapBuffer::Key key, const char* value);

  void putMapBuffer(MapBuffer::Key key, const MapBuffer& map);

  void putMapBufferList(MapBuffer::Key key,
                        const std::vector<MapBuffer>& mapBufferList);

 protected:
  void beforeBuild() override;

 private:
  uint16_t lastKey_{0};

  bool needsSort_{false};

  void storeKeyValue(MapBuffer::Key key, MapBuffer::DataType type,
                     const uint8_t* value, uint32_t valueSize);
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_MAP_BUFFER_BUILDER_H_
