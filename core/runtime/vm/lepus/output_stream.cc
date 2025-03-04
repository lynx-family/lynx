// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/output_stream.h"

#include <cstring>
#include <string>

namespace lynx {
namespace lepus {

void OutputStream::WriteData(const uint8_t* src, size_t size,
                             const char* desc) {
  WriteImpl(src, offset_, size);
  offset_ += size;
}

void ByteArrayOutputStream::WriteImpl(const uint8_t* buffer, size_t offset,
                                      size_t length) {
  if (length == 0) {
    return;
  }
  size_t end = offset + length;
  if (end > buf_->data.size()) {
    buf_->data.resize(end);
  }
  uint8_t* dst = &buf_->data[offset];
  memcpy(dst, buffer, length);
}
const std::vector<uint8_t>& ByteArrayOutputStream::byte_array() {
  return buf_->data;
}

bool ByteArrayOutputStream::WriteToFile(const std::string& filename) {
  FILE* pf = fopen(filename.c_str(), "wb");
  if (pf == nullptr) {
    return false;
  }
  size_t size = buf_->size() + sizeof(uint32_t);
  (void)fwrite((uint8_t*)&size, sizeof(uint32_t), 1, pf);
  (void)fwrite(&buf_->data[0], buf_->size(), 1, pf);
  fclose(pf);

  return true;
}

void ByteArrayOutputStream::Move(uint32_t insert_pos, uint32_t start,
                                 uint32_t size) {
  OutputBuffer* new_buf = new OutputBuffer();
  new_buf->data.resize(buf_->size());

  uint8_t* dst_pos = &new_buf->data[0];
  const uint8_t* src_pos = &buf_->data[0];
  const size_t src_size = buf_->size();

  memcpy(dst_pos, src_pos, insert_pos);
  dst_pos += insert_pos;
  memcpy(dst_pos, src_pos + start, size);
  dst_pos += size;
  memcpy(dst_pos, src_pos + insert_pos, src_size - insert_pos - size);
  buf_.reset(new_buf);
}

void ByteArrayOutputStream::OverwriteData(const uint8_t* src, size_t size,
                                          size_t overwrite_offset) {
  // Backup the current offset
  size_t backup_offset = offset_;

  // Update the offset to the specified position
  offset_ = overwrite_offset;

  // Overwrite the data at the given position
  WriteImpl(src, offset_, size);

  // Restore the offset
  offset_ = backup_offset;
}

void OutputStream::WriteCompactU32(uint32_t value) {
  WriteData(reinterpret_cast<uint8_t*>(&value), sizeof(uint32_t));
}

void OutputStream::WriteCompactS32(int32_t value) {
  WriteData(reinterpret_cast<uint8_t*>(&value), sizeof(int32_t));
}

void OutputStream::WriteCompactU64(uint64_t value) {
  WriteData(reinterpret_cast<uint8_t*>(&value), sizeof(uint64_t));
}

void OutputStream::WriteCompactD64(double value) {
  WriteData(reinterpret_cast<uint8_t*>(&value), sizeof(double));
}

size_t OutputStream::Offset() { return offset_; }
}  // namespace lepus
}  // namespace lynx
