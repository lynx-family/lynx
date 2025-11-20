// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/v8/v8_platform_data.h"

namespace lynx {
namespace piper {

static void* v8_platform_ = nullptr;

void V8PlatformData::SetV8Platform(void* platform) { v8_platform_ = platform; }

void* V8PlatformData::GetV8Platform() { return v8_platform_; }

}  // namespace piper
}  // namespace lynx
