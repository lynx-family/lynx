// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_READABLE_COMPACT_ARRAY_BUFFER_H_
#define CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_READABLE_COMPACT_ARRAY_BUFFER_H_

#include "core/base/android/android_jni.h"
#include "core/renderer/tasm/react/android/mapbuffer/compact_array_buffer.h"

namespace lynx {
namespace base {
namespace android {

class JReadableCompactArrayBuffer {
 public:
  static bool RegisterJni(JNIEnv* env);

  /**
   * Create a NewLocalRef of ReadableCompactArrayBuffer
   * @param array a C++ CompactArrayBuffer
   * @return a java object which should be consumed and freed by JVM.
   */
  static jobject CreateReadableCompactArrayBuffer(
      const CompactArrayBuffer& array);
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  //   CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_READABLE_COMPACT_ARRAY_BUFFER_H_
