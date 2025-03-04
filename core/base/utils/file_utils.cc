// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/utils/file_utils.h"

#include <cstdio>
#include <memory>

namespace lynx {
namespace base {

using FileUniquePtr = std::unique_ptr<FILE, decltype(&fclose)>;

bool FileUtils::ReadFileBinary(const std::string &path, size_t max_size,
                               std::string &file_content) {
  auto file = FileUniquePtr(fopen(path.c_str(), "rb"), &fclose);
  if (!file) {
    return false;
  }

  // obtain file size
  fseek(file.get(), 0, SEEK_END);
  long size = ftell(file.get());
  rewind(file.get());
  if (size < 0 || static_cast<size_t>(size) > max_size) {
    return false;
  }

  // read file
  file_content.resize(size);
  fread(file_content.data(), 1, static_cast<size_t>(size), file.get());

  // check result
  bool ret = !ferror(file.get());
  return ret;
}

bool FileUtils::WriteFileBinary(const std::string &path,
                                const unsigned char *file_content,
                                size_t size) {
  auto file = FileUniquePtr(fopen(path.c_str(), "wb"), &fclose);
  if (!file) {
    return false;
  }
  size_t bytes_wrote =
      fwrite(file_content, sizeof(unsigned char), size, file.get());
  return bytes_wrote == size;
}

}  // namespace base
}  // namespace lynx
