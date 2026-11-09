// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// cspell:ignore SHGFP

#include <windows.h>

// Windows type declarations must precede the shell headers.
#include <shlobj.h>

#include "base/include/string/string_conversion_win.h"
#include "base/src/application_directory/application_directory_impl.h"

namespace lynx {
namespace base {

std::string GetDefaultApplicationDataDirectory() {
  wchar_t path[MAX_PATH];
  if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
                              nullptr, SHGFP_TYPE_CURRENT, path))) {
    return {};
  }
  std::wstring directory(path);
  if (directory.empty()) return {};
  if (directory.back() != L'\\' && directory.back() != L'/') {
    directory.push_back(L'\\');
  }
  directory += L"lynx";
  if (!CreateDirectoryW(directory.c_str(), nullptr)) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) return {};
    const auto attributes = GetFileAttributesW(directory.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES ||
        !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
      return {};
    }
  }
  return Utf8FromUtf16(directory);
}

}  // namespace base
}  // namespace lynx
