// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// clang-format off
// include symbols `eglGetProcAddress` from ANGLE
#undef EGL_EGL_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES 1 // cspell:disable-line
#include <EGL/egl.h>
// clang-format on

#include "clay/gfx/shared_image/utils/angle_get_proc.h"

namespace clay {

PFNEGLGETPROCADDRESSPROC GetAngleEglGetProcAddressProc() {
  return eglGetProcAddress;
}

}  // namespace clay
