// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_BINARY_WRITER_H_
#define CORE_RUNTIME_VM_LEPUS_BINARY_WRITER_H_

#include <stdlib.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "core/runtime/vm/lepus/output_stream.h"

namespace lynx {
namespace lepus {

class BinaryWriter {
 public:
  BinaryWriter() { stream_.reset(new ByteArrayOutputStream()); }
  virtual ~BinaryWriter() {}

  void WriteU8(uint8_t value);
  void WriteByte(uint8_t value);
  void WriteU32(uint32_t value);
  void WriteCompactU32(uint32_t value);
  void WriteCompactS32(int32_t value);
  void WriteCompactU64(uint64_t value);
  void WriteCompactD64(double value);
  void WriteStringDirectly(const char* str);
  void WriteStringDirectly(const char* str, size_t length);

  void Move(uint32_t insert_pos, uint32_t start, uint32_t size);
  size_t Offset();

  const OutputStream* stream() const { return stream_.get(); }

  void WriteData(const uint8_t* src, size_t size, const char* desc) {
    stream_->WriteData(src, size, desc);
  }

  const std::vector<uint8_t>& byte_array() { return stream_->byte_array(); }

  // Overwrite the data at a specific offset
  void OverwriteData(const uint8_t* src, size_t size, size_t overwrite_offset) {
    stream_->OverwriteData(src, size, overwrite_offset);
  }

 protected:
  std::unique_ptr<OutputStream> stream_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_BINARY_WRITER_H_
