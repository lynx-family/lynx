// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/react/android/mapbuffer/readable_map_buffer.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/ReadableMapBuffer_jni.h"

namespace lynx {
namespace base {
namespace android {

JReadableMapBuffer::JReadableMapBuffer(MapBuffer&& map)
    : serialized_data_(std::move(map.bytes_)) {}

bool JReadableMapBuffer::RegisterJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

base::android::ScopedLocalJavaRef<jobject>
JReadableMapBuffer::CreateReadableMapBuffer(const MapBuffer& map) {
  JNIEnv* env = base::android::AttachCurrentThread();
  int count = map.count();
  if (count == 0) {
    return base::android::ScopedLocalJavaRef<jobject>(env, nullptr);
  }

  size_t length = map.bytes_.size();
  jbyteArray ret = env->NewByteArray(length);  // NOLINT
  env->SetByteArrayRegion(ret, 0, length,
                          reinterpret_cast<const jbyte*>(map.bytes_.data()));
  return Java_ReadableMapBuffer_fromByteBufferWithCount(env, ret, count);
}

}  // namespace android
}  // namespace base
}  // namespace lynx
