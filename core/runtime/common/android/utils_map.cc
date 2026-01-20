// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>
#include <utility>

#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "core/base/android/piper_data.h"
#include "core/base/js_constants.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/js/bindings/modules/android/platform_jsi/lynx_platform_jsi_object_android.h"
#include "platform/android/lynx_android/src/main/jni/gen/JavaOnlyMap_jni.h"

namespace lynx {
namespace runtime {
namespace js {
bool JSBUtilsMapRegisterJNI(JNIEnv* env) { return RegisterNativesImpl(env); }

void PushByteArrayToJavaMap(Runtime* rt, const std::string& key,
                            const ArrayBuffer& array_buffer,
                            base::android::JavaOnlyMap* jmap) {
  // TODO(qiuxian):Add unit test when framework is ready.
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  base::android::ScopedLocalJavaRef<jbyteArray> jni_byte_array =
      base::android::JNIHelper::ConvertToJNIByteArray(env, rt, array_buffer);
  Java_JavaOnlyMap_putByteArray(env, jmap->jni_object(), jni_key.Get(),
                                jni_byte_array.Get());
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
