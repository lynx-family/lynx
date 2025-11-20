// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_V8_V8_PLATFORM_DATA_H_
#define CORE_RUNTIME_JSI_V8_V8_PLATFORM_DATA_H_

namespace lynx {
namespace piper {

class V8PlatformData {
 public:
  V8PlatformData() {}
  virtual ~V8PlatformData() {}

  static void SetV8Platform(void* platform);
  static void* GetV8Platform();
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_V8_V8_PLATFORM_DATA_H_
