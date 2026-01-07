// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_DPI_UTILS_WIN32_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_DPI_UTILS_WIN32_H_

#include <Windows.h>

namespace lynx {
namespace embedder {
namespace plugin {

/// Returns the DPI for |hwnd|. Supports all DPI awareness modes, and is
/// backward compatible down to Windows Vista. If |hwnd| is nullptr, returns the
/// DPI for the primary monitor. If Per-Monitor DPI awareness is not available,
/// returns the system's DPI.
UINT GetDpiForHWND(HWND hwnd);

}  // namespace plugin
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_DPI_UTILS_WIN32_H_
