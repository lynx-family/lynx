// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/binary_input_stream.h"

namespace lynx {
namespace lepus {

bool ByteArrayInputStream::ReadFromFile(const char* file) {
  FILE* pf = fopen(file, "rb");
  if (pf == nullptr) {
    return false;
  }

  fseek(pf, 0, SEEK_END);
  long size = ftell(pf);
  if (size < 0) {
    fclose(pf);
    return false;
  }

  // FIXME, unnecessary resize value initialization.
  std::vector<uint8_t> data(static_cast<size_t>(size));
  rewind(pf);
  const size_t read_size =
      data.empty() ? 0 : fread(data.data(), sizeof(char), data.size(), pf);
  fclose(pf);
  if (read_size != data.size()) {
    return false;
  }

  buf_ = std::make_shared<InputBuffer>(std::move(data));
  Initialize();
  return true;
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
