// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_SHELL_PLATFORM_EMBEDDER_RENDERING_BACKEND_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_SHELL_PLATFORM_EMBEDDER_RENDERING_BACKEND_H_

#ifndef ENABLE_SKITY
#include "clay/shell/gpu/gpu_surface_gl_skia.h"
#else
#include "clay/shell/gpu/gpu_surface_gl_skity.h"
#endif  // ENABLE_SKITY

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_SHELL_PLATFORM_EMBEDDER_RENDERING_BACKEND_H_
