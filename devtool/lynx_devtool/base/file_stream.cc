// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/base/file_stream.h"

namespace lynx {
namespace devtool {

std::map<int, std::unique_ptr<std::fstream>> FileStream::streams_;

int FileStream::Open(const std::string& file, std::ios::openmode mode) {
  static int next_handle = 1;
  auto file_stream_ptr = std::make_unique<std::fstream>(file, mode);
  if (!file_stream_ptr->is_open()) {
    return -1;
  }
  streams_[next_handle] = std::move(file_stream_ptr);
  return next_handle++;
}

void FileStream::Close(int handle) {
  auto item = streams_.find(handle);
  if (item == streams_.end()) {
    return;
  }
  streams_.erase(item);
}

int FileStream::Read(int handle, char* buf, size_t size) {
  auto item = streams_.find(handle);
  if (item == streams_.end()) {
    return -1;
  }
  return static_cast<int>(item->second->read(buf, size).gcount());
}

int FileStream::Read(int handle, std::ostream& oss, size_t size) {
  // TODO: make this more efficient
  std::unique_ptr<char[]> buf = std::make_unique<char[]>(size);
  int count = Read(handle, buf.get(), size);
  if (count < 0) {
    return count;
  }
  oss.write(buf.get(), count);
  return count;
}

}  // namespace devtool
}  // namespace lynx
