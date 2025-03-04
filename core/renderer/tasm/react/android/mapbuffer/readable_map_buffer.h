// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_READABLE_MAP_BUFFER_H_
#define CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_READABLE_MAP_BUFFER_H_

#include <memory>
#include <vector>

#include "core/base/android/android_jni.h"
#include "core/renderer/tasm/react/android/mapbuffer/map_buffer.h"

namespace lynx {
namespace base {
namespace android {

/**
 * This class holds for a C++ buffer that consumed by JVM.
 * It will be freed by JVM.
 */
class JReadableMapBuffer {
 public:
  static bool RegisterJni(JNIEnv* env);

  static base::android::ScopedLocalJavaRef<jobject> CreateReadableMapBuffer(
      const MapBuffer& map);

  JReadableMapBuffer(MapBuffer&& map);

 private:
  std::vector<uint8_t> serialized_data_{};
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  //   CORE_RENDERER_TASM_REACT_ANDROID_MAPBUFFER_READABLE_MAP_BUFFER_H_
