// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_TYPE_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_TYPE_H_
#include <cstdint>

enum class PlatformRendererType : uint8_t {
  kUnknown = 0,
  kView = 1,
  kPage = 2,
  kScroll = 3,
};

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_TYPE_H_
