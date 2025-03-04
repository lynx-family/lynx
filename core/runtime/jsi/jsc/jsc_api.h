// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSC_JSC_API_H_
#define CORE_RUNTIME_JSI_JSC_JSC_API_H_

#if OS_ANDROID
#include <jni.h>
#endif  // OS_ANDROID

#include <memory>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

std::unique_ptr<Runtime> makeJSCRuntime();

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_API_H_
