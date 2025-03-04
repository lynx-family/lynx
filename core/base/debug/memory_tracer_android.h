// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_DEBUG_MEMORY_TRACER_ANDROID_H_
#define CORE_BASE_DEBUG_MEMORY_TRACER_ANDROID_H_

#include <jni.h>

namespace lynx {
namespace base {

class MemoryTracerAndroid {
 public:
  static bool RegisterJNIUtils(JNIEnv *env);
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_DEBUG_MEMORY_TRACER_ANDROID_H_
