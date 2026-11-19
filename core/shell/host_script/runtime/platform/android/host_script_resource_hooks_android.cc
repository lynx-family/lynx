// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/base_string.h"
#include "base/include/value/byte_array.h"
#include "core/shell/host_script/runtime/interceptor.h"
#include "platform/android/lynx_host_script/src/main/jni/gen/LynxHostScriptResourceHooks_jni.h"
#include "platform/android/lynx_host_script/src/main/jni/gen/LynxHostScriptResourceHooks_register_jni.h"

namespace {
BASE_STATIC_STRING_DECL(kBytes, "bytes");
BASE_STATIC_STRING_DECL(kUrl, "url");
}  // namespace

namespace lynx::jni {
bool RegisterJNIForLynxHostScriptResourceHooks(JNIEnv* env) {
  // An older engine can use the other hooks without this Java entry point.
  base::android::ScopedLocalJavaRef<jclass> hooks(
      env, env->FindClass(kLynxHostScriptResourceHooksClassPath));
  if (hooks.IsNull()) {
    env->ExceptionClear();
    return true;
  }
  if (!RegisterNativesImpl(env)) return false;
  Java_LynxHostScriptResourceHooks_setAvailable(env);
  return true;
}
}  // namespace lynx::jni

static jboolean HasHandler(JNIEnv*, jclass, jlong view_id) {
  return lynx::shell::Interceptor::HasResourceFetchHandler(view_id);
}

static jobject Dispatch(JNIEnv* env, jclass, jlong view_id, jbyteArray url) {
  using lynx::base::android::JNIConvertHelper;
  using lynx::shell::Interceptor;
  auto decision = Interceptor::DispatchResourceFetch(
      view_id, JNIConvertHelper::ConvertToString(env, url));
  if (decision.mock.IsTable()) {
    auto bytes = decision.mock.GetProperty(kBytes).ByteArray();
    if (bytes->GetLength() > std::numeric_limits<jsize>::max()) {
      Interceptor::ReportCoverageGap(lynx::shell::InterceptKind::kResourceFetch,
                                     "Mock exceeds the Android array size");
      return nullptr;
    }
    return JNIConvertHelper::ConvertToJNIByteArray(
               env, bytes->GetPtr(), static_cast<jsize>(bytes->GetLength()))
        .Release();
  }
  if (auto url_patch = decision.patch.GetProperty(kUrl); url_patch.IsString()) {
    auto utf16 = lynx::base::U8StringToU16(url_patch.StdString());
    return JNIConvertHelper::ConvertToJNIString(
               env, reinterpret_cast<const jchar*>(utf16.data()), utf16.size())
        .Release();
  }
  return nullptr;
}
