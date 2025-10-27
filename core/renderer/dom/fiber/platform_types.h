// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_PLATFORM_TYPES_H_
#define CORE_RENDERER_DOM_FIBER_PLATFORM_TYPES_H_

namespace lynx {
namespace tasm {

enum class OSType {
  kAndroid,
  kHarmony,
  kIOS,
  kMacOS,
  kWindows,
  kLinux,
  kUnknown
};

constexpr OSType GetOSType() {
#ifdef OS_ANDROID
  return OSType::kAndroid;
#elif defined(OS_HARMONY)
  return OSType::kHarmony;
#elif defined(OS_IOS)
  return OSType::kIOS;
#elif defined(OS_OSX)
  return OSType::kMacOS;
#elif defined(OS_WIN)
  return OSType::kWindows;
#elif defined(OS_LINUX)
  return OSType::kLinux;
#else
  return OSType::kUnknown;
#endif
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_PLATFORM_TYPES_H_
