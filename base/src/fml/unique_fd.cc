// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/unique_fd.h"

#include "base/include/fml/eintr_wrapper.h"

namespace lynx {
namespace fml {
namespace internal {

#if OS_WIN

namespace os_win {

std::mutex UniqueFDTraits::file_map_mutex;
std::map<HANDLE, DirCacheEntry> UniqueFDTraits::file_map;

void UniqueFDTraits::Free_Handle(HANDLE fd) { CloseHandle(fd); }

}  // namespace os_win

#else  // OS_WIN

namespace os_unix {

void UniqueFDTraits::Free(int fd) { close(fd); }

void UniqueDirTraits::Free(DIR* dir) { closedir(dir); }

}  // namespace os_unix

#endif  // OS_WIN

}  // namespace internal
}  // namespace fml
}  // namespace lynx
