// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_EGL_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_EGL_H_

#include <string_view>

namespace clay {
namespace egl {

/// Log the last EGL error with an error message.
void LogEGLError(std::string_view message);

/// Log the last EGL error.
void LogEGLError(std::string_view file, int line);

#define WINDOWS_LOG_EGL_ERROR LogEGLError(__FILE__, __LINE__);

}  // namespace egl
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_EGL_H_
