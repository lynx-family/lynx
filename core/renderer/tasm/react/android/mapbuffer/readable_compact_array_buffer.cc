// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/react/android/mapbuffer/readable_compact_array_buffer.h"

#include "core/build/gen/ReadableCompactArrayBuffer_jni.h"

namespace lynx {
namespace base {
namespace android {

bool JReadableCompactArrayBuffer::RegisterJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

jobject JReadableCompactArrayBuffer::CreateReadableCompactArrayBuffer(
    const CompactArrayBuffer& array) {
  JNIEnv* env = base::android::AttachCurrentThread();
  int count = array.count();
  if (count == 0) {
    return nullptr;
  }

  size_t length = array.bytes_.size();
  jbyteArray ret = env->NewByteArray(length);  // NOLINT
  env->SetByteArrayRegion(ret, 0, length,
                          reinterpret_cast<const jbyte*>(array.bytes_.data()));
  auto scoped_array_buffer =
      Java_ReadableCompactArrayBuffer_fromByteBufferWithCount(env, ret, count);

  return env->NewLocalRef(scoped_array_buffer.Get());  // NOLINT
}

}  // namespace android
}  // namespace base
}  // namespace lynx
