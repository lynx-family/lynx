// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXPLORER_WINDOWS_LYNX_EXPLORER_DPI_UTILS_WIN32_H_
#define EXPLORER_WINDOWS_LYNX_EXPLORER_DPI_UTILS_WIN32_H_

#include <Windows.h>

/// Returns the DPI for |hwnd|. Supports all DPI awareness modes, and is
/// backward compatible down to Windows Vista. If |hwnd| is nullptr, returns the
/// DPI for the primary monitor. If Per-Monitor DPI awareness is not available,
/// returns the system's DPI.
UINT GetDpiForHWND(HWND hwnd);

/// Returns the DPI of a given monitor. Defaults to 96 if the API is not
/// available.
UINT GetDpiForMonitor(HMONITOR monitor);

#endif  // EXPLORER_WINDOWS_LYNX_EXPLORER_DPI_UTILS_WIN32_H_
