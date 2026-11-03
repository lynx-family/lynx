// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/buffer.h"

#include <cstdio>

namespace lynx {
namespace base {

using FileUniquePtr = std::unique_ptr<FILE, decltype(&fclose)>;

bool Buffer::WriteToFile(const std::string& path) const {
  const size_t length = size();
  const uint8_t* bytes = data();
  if (bytes == nullptr && length != 0) {
    return false;
  }
  FILE* file = fopen(path.c_str(), "wb");
  if (file == nullptr) {
    return false;
  }
  const bool success = length == 0 || fwrite(bytes, 1, length, file) == length;
  const int close_result = fclose(file);
  return success && close_result == 0;
}

std::unique_ptr<Buffer> Buffer::ReadFromFile(const std::string& path,
                                             size_t max_size) {
  auto file = FileUniquePtr(fopen(path.c_str(), "rb"), &fclose);
  if (!file) {
    return nullptr;
  }

  if (fseek(file.get(), 0, SEEK_END) != 0) {
    return nullptr;
  }
  const long file_size = ftell(file.get());
  if (file_size < 0 || static_cast<size_t>(file_size) > max_size ||
      fseek(file.get(), 0, SEEK_SET) != 0) {
    return nullptr;
  }

  auto buffer = std::make_unique<StringBuffer>(std::string{});
  auto& content = buffer->GetString();
  const size_t size = static_cast<size_t>(file_size);
  if (size > content.max_size()) {
    return nullptr;
  }
  content.resize(size);
  if ((size != 0 && fread(content.data(), 1, size, file.get()) != size) ||
      ferror(file.get())) {
    return nullptr;
  }
  if (fclose(file.release()) != 0) {
    return nullptr;
  }
  return buffer;
}

}  // namespace base
}  // namespace lynx
