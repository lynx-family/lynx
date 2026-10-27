// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_UTILS_ANGLE_GET_PROC_H_
#define CLAY_GFX_SHARED_IMAGE_UTILS_ANGLE_GET_PROC_H_

#include <EGL/egl.h>

namespace clay {

PFNEGLGETPROCADDRESSPROC GetAngleEglGetProcAddressProc();

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_UTILS_ANGLE_GET_PROC_H_
