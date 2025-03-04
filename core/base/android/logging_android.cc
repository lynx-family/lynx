// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/logging_android.h"

#include <android/api-level.h>
#include <android/log.h>
#include <jni.h>

#include <memory>
#include <string>

#include "base/include/log/alog_wrapper.h"
#include "base/include/log/logging.h"
#include "core/base/android/jni_helper.h"
#include "core/build/gen/LLog_jni.h"
#include "core/renderer/utils/lynx_env.h"

static lynx::base::alog_write_func_ptr s_alog_write = nullptr;

namespace lynx {
namespace base {
namespace logging {
namespace {
base::alog_write_func_ptr InitAlog() { return s_alog_write; }

void PrintLogMessageByLogDelegate(LogMessage* msg, const char* tag) {
  JNIEnv* env = android::AttachCurrentThread();
  auto c_msg = msg->stream().str();
  static android::ScopedGlobalJavaRef<jstring> lynx_tag(
      android::JNIConvertHelper::ConvertToJNIStringUTF(env, tag));

  size_t msg_len = c_msg.size();
  if (msg_len == 0) {
    return;
  }

  static const int api_level = android_get_device_api_level();
  // Emoji will make App crash when use `NewStringUTF` API in Android 5.x
  // So we send byte[] to Java and construct a new String in Java.
  if (api_level < __ANDROID_API_M__) {  // Build.VERSION_CODES.M
    base::android::ScopedLocalJavaRef<jbyteArray> jni_byte_msg =
        android::JNIConvertHelper::ConvertToJNIByteArray(env, c_msg);
    Java_LLog_logByte(env, msg->severity(), lynx_tag.Get(), jni_byte_msg.Get(),
                      msg->source(), msg->runtimeId(), msg->ChannelType(),
                      msg->messageStart());
  } else {
    base::android::ScopedLocalJavaRef<jstring> jni_msg =
        android::JNIConvertHelper::ConvertToJNIStringUTF(env, c_msg);
    Java_LLog_log(env, msg->severity(), lynx_tag.Get(), jni_msg.Get(),
                  msg->source(), msg->runtimeId(), msg->ChannelType(),
                  msg->messageStart());
  }
}
}  // namespace

bool RegisterJNI(JNIEnv* env) { return RegisterNativesImpl(env); }

long GetALogPtr(JNIEnv* env) { return Java_LLog_getALogPtr(env); }

void InitLynxLog() {
  InitLynxLogging(InitAlog, PrintLogMessageByLogDelegate,
                  tasm::LynxEnv::GetInstance().IsDevToolEnabled());
}

}  // namespace logging
}  // namespace base
}  // namespace lynx

void InitLynxLoggingNative(JNIEnv* env, jclass jcaller) {
  lynx::base::logging::InitLynxLog();
}

void SetNativeMinLogLevel(JNIEnv* env, jclass jcaller, jint level) {
  lynx::base::logging::SetMinLogLevel(level);
}

void InitALogNative(JNIEnv* env, jclass jcaller, jlong addr) {
  s_alog_write = reinterpret_cast<lynx::base::alog_write_func_ptr>(addr);
}

void InternalLog(JNIEnv* env, jclass jcaller, jint level, jstring tag,
                 jstring msg) {
  const char* c_tag = env->GetStringUTFChars(tag, JNI_FALSE);
  const char* c_msg = env->GetStringUTFChars(msg, JNI_FALSE);
  lynx::base::logging::PrintLogToLynxLogging(static_cast<int>(level), c_tag,
                                             c_msg);
  env->ReleaseStringUTFChars(tag, c_tag);
  env->ReleaseStringUTFChars(msg, c_msg);
}

void SetLogOutputByPlatform(JNIEnv* env, jclass jcaller) {
  lynx::base::logging::EnableLogOutputByPlatform();
}
