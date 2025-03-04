// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/binary_input_stream.h"

namespace lynx {
namespace lepus {

bool ByteArrayInputStream::ReadFromFile(const char* file) {
  FILE* pf = fopen(file, "r");
  if (pf == nullptr) {
    return false;
  }

  fseek(pf, 0, SEEK_END);
  long size = ftell(pf);
  // FIXME, unnecessary resize value initialization.
  buf_->data.resize(size);
  uint8_t* text = &buf_->data[0];
  if (text != nullptr) {
    rewind(pf);
    fread(text, sizeof(char), size, pf);
    return true;
  }

  return false;
}

size_t InputStream::ReadCompactU32(uint32_t* out_value) {
  if (!CheckSize(1)) {
    return 0;
  }
  ReadUx<uint32_t>(out_value);
  return 1;
}

size_t InputStream::ReadCompactS32(int32_t* out_value) {
  if (!CheckSize(1)) {
    return 0;
  }
  ReadUx<int32_t>(out_value);
  return 1;
}

size_t InputStream::ReadCompactU64(uint64_t* out_value) {
  if (!CheckSize(1)) {
    return 0;
  }
  ReadUx<uint64_t>(out_value);
  return 1;
}

}  // namespace lepus
}  // namespace lynx
