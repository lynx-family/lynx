// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// cspell:ignore wfopen wremove

#include "base/include/file_utils.h"

#include <fcntl.h>

#if defined(_WIN32)
#include <io.h>

#include "base/include/string/string_conversion_win.h"
#endif

namespace lynx {
namespace base {

std::FILE* OpenFile(const std::string& path, const char* mode) {
#if defined(_WIN32)
  return _wfopen(Utf16FromUtf8(path).c_str(),
                 Utf16FromUtf8(std::string_view(mode)).c_str());
#else
  return std::fopen(path.c_str(), mode);
#endif
}

int RemoveFile(const std::string& path) {
#if defined(_WIN32)
  return _wremove(Utf16FromUtf8(path).c_str());
#else
  return std::remove(path.c_str());
#endif
}

int OpenFileDescriptor(const std::string& path, int flags, int permissions) {
#if defined(_WIN32)
  return _wopen(Utf16FromUtf8(path).c_str(), flags | _O_BINARY, permissions);
#else
  return open(path.c_str(), flags, permissions);
#endif
}

}  // namespace base
}  // namespace lynx
