// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_HOST_SCRIPT_ANDROID_SESSION_H_
#define CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_HOST_SCRIPT_ANDROID_SESSION_H_

#include <memory>
#include <string>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/string/string_utils.h"
#include "core/shell/host_script/runtime/host_script_session.h"

namespace lynx {
namespace shell {

// Java owns a handle; the N-API module independently retains the session until
// JS-thread detach. Releasing a Java handle must not destroy N-API references.
using HostScriptSessionHandle = std::shared_ptr<HostScriptSession>;

inline HostScriptSession& HostScriptSessionFromHandle(jlong ptr) {
  return **reinterpret_cast<HostScriptSessionHandle*>(ptr);
}

inline base::android::ScopedLocalJavaRef<jstring> HostScriptJavaString(
    JNIEnv* env, const std::string& value) {
  auto utf16 = base::U8StringToU16(value);
  return base::android::JNIConvertHelper::ConvertToJNIString(
      env, reinterpret_cast<const jchar*>(utf16.data()),
      static_cast<jsize>(utf16.size()));
}

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_HOST_SCRIPT_ANDROID_SESSION_H_
