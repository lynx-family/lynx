// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_JAVASCRIPT_EXECUTION_CALLBACK_ANDROID_H_
#define CORE_SHELL_ANDROID_JAVASCRIPT_EXECUTION_CALLBACK_ANDROID_H_

#include <jni.h>

#include <string>

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace shell {

void DispatchJavaScriptExecutionResult(
    base::android::ScopedGlobalJavaRef<jobject> callback, int status,
    std::string stack);

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_JAVASCRIPT_EXECUTION_CALLBACK_ANDROID_H_
