// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_BASE_FILE_STREAM_H_
#define DEVTOOL_LYNX_DEVTOOL_BASE_FILE_STREAM_H_

#include <fstream>
#include <map>
#include <memory>

namespace lynx {
namespace devtool {

class FileStream {
 public:
  FileStream() = delete;
  static int Open(const std::string& file,
                  std::ios::openmode mode = std::ios::in);
  static void Close(int handle);
  static int Read(int handle, char* buf, size_t size);
  static int Read(int handle, std::ostream& oss, size_t size);

 private:
  static std::map<int, std::unique_ptr<std::fstream>> streams_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_BASE_FILE_STREAM_H_
