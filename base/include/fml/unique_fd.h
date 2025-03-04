// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_UNIQUE_FD_H_
#define BASE_INCLUDE_FML_UNIQUE_FD_H_

#include "base/include/fml/unique_object.h"
#include "build/build_config.h"

#if OS_WIN
#include <windows.h>

#include <map>
#include <mutex>
#include <optional>
#else  // OS_WIN
#include <dirent.h>
#include <unistd.h>

#include <map>
#endif  // OS_WIN

namespace lynx {
namespace fml {
namespace internal {

#if OS_WIN

namespace os_win {

struct DirCacheEntry {
  std::wstring filename;
  FILE_ID_128 id;
};

// The order of these is important.  Must come before UniqueFDTraits struct
// else linker error.  Embedding in struct also causes linker error.

struct UniqueFDTraits {
  static std::mutex file_map_mutex;
  static std::map<HANDLE, DirCacheEntry> file_map;

  static HANDLE InvalidValue() { return INVALID_HANDLE_VALUE; }
  static bool IsValid(HANDLE value) { return value != InvalidValue(); }
  static void Free_Handle(HANDLE fd);

  static void Free(HANDLE fd) {
    RemoveCacheEntry(fd);

    UniqueFDTraits::Free_Handle(fd);
  }

  static void RemoveCacheEntry(HANDLE fd) {
    const std::lock_guard<std::mutex> lock(file_map_mutex);

    file_map.erase(fd);
  }

  static void StoreCacheEntry(HANDLE fd, DirCacheEntry state) {
    const std::lock_guard<std::mutex> lock(file_map_mutex);
    file_map[fd] = state;
  }

  static std::optional<DirCacheEntry> GetCacheEntry(HANDLE fd) {
    const std::lock_guard<std::mutex> lock(file_map_mutex);
    auto found = file_map.find(fd);
    return found == file_map.end()
               ? std::nullopt
               : std::optional<DirCacheEntry>{found->second};
  }
};

}  // namespace os_win

#else  // OS_WIN

namespace os_unix {

struct UniqueFDTraits {
  static int InvalidValue() { return -1; }
  static bool IsValid(int value) { return value >= 0; }
  static void Free(int fd);
};

struct UniqueDirTraits {
  static DIR* InvalidValue() { return nullptr; }
  static bool IsValid(DIR* value) { return value != nullptr; }
  static void Free(DIR* dir);
};

}  // namespace os_unix

#endif  // OS_WIN

}  // namespace internal

#if OS_WIN

using UniqueFD = UniqueObject<HANDLE, internal::os_win::UniqueFDTraits>;

#else  // OS_WIN

using UniqueFD = UniqueObject<int, internal::os_unix::UniqueFDTraits>;
using UniqueDir = UniqueObject<DIR*, internal::os_unix::UniqueDirTraits>;

#endif  // OS_WIN

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::UniqueFD;
#if !OS_WIN
using lynx::fml::UniqueDir;
#endif
}  // namespace fml

#endif  // BASE_INCLUDE_FML_UNIQUE_FD_H_
