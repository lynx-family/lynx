// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/binary_writer.h"

#include <assert.h>

#include <cstddef>
#include <cstring>

#include "core/runtime/vm/lepus/output_stream.h"

namespace lynx {
namespace lepus {

void BinaryWriter::WriteU8(uint8_t value) {
  stream_->WriteData(&value, sizeof(value));
}

void BinaryWriter::WriteByte(uint8_t value) { WriteU8(value); }

void BinaryWriter::WriteU32(uint32_t value) {
  stream_->WriteData(reinterpret_cast<uint8_t*>(&value), sizeof(uint32_t));
}

void BinaryWriter::WriteCompactU32(uint32_t value) {
  stream_->WriteCompactU32(value);
}

void BinaryWriter::WriteCompactS32(int32_t value) {
  stream_->WriteCompactS32(value);
}

void BinaryWriter::WriteCompactU64(uint64_t value) {
  stream_->WriteCompactU64(value);
}

void BinaryWriter::WriteCompactD64(double value) {
  stream_->WriteCompactD64(value);
}

void BinaryWriter::WriteStringDirectly(const char* str) {
  WriteStringDirectly(str, strlen(str));
}

void BinaryWriter::WriteStringDirectly(const char* str, size_t length) {
  WriteCompactU32(static_cast<uint32_t>(length));

  if (length != 0) {
    stream_->WriteData(reinterpret_cast<const uint8_t*>(str), length);
  }
}

void BinaryWriter::Move(uint32_t insert_pos, uint32_t start, uint32_t size) {
  stream_->Move(insert_pos, start, size);
}

size_t BinaryWriter::Offset() { return stream_->Offset(); }

}  // namespace lepus
}  // namespace lynx
