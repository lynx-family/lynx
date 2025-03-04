// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/android/jni_utils.h"

#include <android/log.h>
#include <sys/prctl.h>
#include <sys/system_properties.h>

#include <string>
#include <utility>

#include "base/include/fml/macros.h"
#include "base/include/platform/android/jni_convert_helper.h"

namespace {
JavaVM *g_jvm = nullptr;
}

namespace lynx {
namespace base {
namespace android {

void InitVM(JavaVM *vm) { g_jvm = vm; }

struct JNIDetach {
  ~JNIDetach() { DetachFromVM(); }
};

// Thread-local object that will detach from JNI during thread shutdown;
static thread_local std::unique_ptr<JNIDetach> tls_jni_detach;

JNIEnv *AttachCurrentThread() {
  LYNX_BASE_DCHECK(g_jvm != nullptr);

  JNIEnv *env = nullptr;
  jint ret = g_jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_2);
  if (ret == JNI_OK && env) {
    return env;
  }

  if (ret == JNI_EDETACHED || !env) {
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_2;
    args.group = nullptr;

    // 16 is the maximum size for thread names on Android.
    char thread_name[16];
    int err = prctl(PR_GET_NAME, thread_name);
    if (err < 0) {
      args.name = nullptr;
    } else {
      args.name = thread_name;
    }

    ret = g_jvm->AttachCurrentThread(&env, &args);
  }

  if (tls_jni_detach.get() == nullptr) {
    tls_jni_detach.reset(new JNIDetach());
  }

  return env;
}

void DetachFromVM() {
  if (g_jvm) {
    g_jvm->DetachCurrentThread();
  }
}

ScopedLocalJavaRef<jclass> GetClass(JNIEnv *env, const char *class_name) {
  jclass clazz = env->FindClass(class_name);
  if (ClearException(env) || !clazz) {
    std::string msg = "Failed to find class " + std::string(class_name);
    __android_log_write(ANDROID_LOG_FATAL, "lynx", msg.c_str());
  }
  return ScopedLocalJavaRef<jclass>(env, clazz);
}

ScopedGlobalJavaRef<jclass> GetGlobalClass(JNIEnv *env,
                                           const char *class_name) {
  jclass clazz = env->FindClass(class_name);
  if (ClearException(env) || !clazz) {
    std::string msg = "Failed to find class " + std::string(class_name ?: "");
    __android_log_write(ANDROID_LOG_FATAL, "lynx", msg.c_str());
  }
  return ScopedGlobalJavaRef<jclass>(env, clazz);
}

jmethodID GetMethod(JNIEnv *env, jclass clazz, MethodType type,
                    const char *method_name, const char *jni_signature) {
  jmethodID id = nullptr;
  if (clazz) {
    if (type == STATIC_METHOD) {
      id = env->GetStaticMethodID(clazz, method_name, jni_signature);
    } else if (type == INSTANCE_METHOD) {
      id = env->GetMethodID(clazz, method_name, jni_signature);
    }
    if (ClearException(env) || !id) {
      std::string msg = "Failed to find " +
                        std::string((type == STATIC_METHOD) ? "static" : "") +
                        std::string(method_name ?: "") +
                        std::string(jni_signature ?: "");
      __android_log_write(ANDROID_LOG_FATAL, "lynx", msg.c_str());
    }
  }
  return id;
}

jmethodID GetMethod(JNIEnv *env, jclass clazz, MethodType type,
                    const char *method_name, const char *jni_signature,
                    intptr_t *method_id) {
  if (*method_id) {
    return reinterpret_cast<jmethodID>(*method_id);
  }
  *method_id = reinterpret_cast<intptr_t>(
      GetMethod(env, clazz, type, method_name, jni_signature));
  return reinterpret_cast<jmethodID>(*method_id);
}

bool HasException(JNIEnv *env) { return env->ExceptionCheck() != JNI_FALSE; }

bool ClearException(JNIEnv *env) {
  if (!HasException(env)) return false;
  env->ExceptionDescribe();
  env->ExceptionClear();
  return true;
}

bool CheckException(JNIEnv *env, std::string &exception_msg) {
  if (!HasException(env)) return true;

  // Exception has been found, might as well tell BreakPad about it.
  lynx::base::android::ScopedLocalJavaRef<jthrowable> throwable(
      env, env->ExceptionOccurred());
  if (throwable.Get()) {
    // Clear the pending exception, since a local reference is now held.
    env->ExceptionDescribe();
    env->ExceptionClear();
    exception_msg = GetJavaExceptionInfo(env, throwable.Get());
  }

  return false;
}

bool CheckAndPrintException(JNIEnv *env) {
  std::string error_msg;
  bool has_no_exception = CheckException(env, error_msg);
  if (!has_no_exception) {
    std::string exception_msg = "JNI exception found: " + error_msg;
    __android_log_write(ANDROID_LOG_FATAL, "lynx", exception_msg.c_str());
    return false;
  }
  return true;
}

std::string GetJavaExceptionInfo(JNIEnv *env, jthrowable java_throwable) {
  ScopedLocalJavaRef<jclass> throwable_clazz(
      env, env->FindClass("java/lang/Throwable"));
  jmethodID throwable_printstacktrace = env->GetMethodID(
      throwable_clazz.Get(), "printStackTrace", "(Ljava/io/PrintStream;)V");

  // Create an instance of ByteArrayOutputStream.
  ScopedLocalJavaRef<jclass> bytearray_output_stream_clazz(
      env, env->FindClass("java/io/ByteArrayOutputStream"));
  jmethodID bytearray_output_stream_constructor =
      env->GetMethodID(bytearray_output_stream_clazz.Get(), "<init>", "()V");
  jmethodID bytearray_output_stream_tostring = env->GetMethodID(
      bytearray_output_stream_clazz.Get(), "toString", "()Ljava/lang/String;");
  ScopedLocalJavaRef<jobject> bytearray_output_stream(
      env, env->NewObject(bytearray_output_stream_clazz.Get(),
                          bytearray_output_stream_constructor));

  // Create an instance of PrintStream.
  ScopedLocalJavaRef<jclass> printstream_clazz(
      env, env->FindClass("java/io/PrintStream"));
  jmethodID printstream_constructor = env->GetMethodID(
      printstream_clazz.Get(), "<init>", "(Ljava/io/OutputStream;)V");
  ScopedLocalJavaRef<jobject> printstream(
      env, env->NewObject(printstream_clazz.Get(), printstream_constructor,
                          bytearray_output_stream.Get()));

  // Call Throwable.printStackTrace(PrintStream)
  env->CallVoidMethod(java_throwable, throwable_printstacktrace,
                      printstream.Get());

  // Call ByteArrayOutputStream.toString()
  ScopedLocalJavaRef<jstring> exception_string(
      env,
      static_cast<jstring>(env->CallObjectMethod(
          bytearray_output_stream.Get(), bytearray_output_stream_tostring)));
  if (ClearException(env)) {
    return "Java OOM'd in exception handling, check logcat";
  }

  return JNIConvertHelper::ConvertToString(env, exception_string.Get());
}

int GetAPILevel() {
  char sdk_version_string[PROP_VALUE_MAX];
  if (__system_property_get("ro.build.version.sdk", sdk_version_string)) {
    return atoi(sdk_version_string);
  } else {
    return -1;
  }
}

}  // namespace android
}  // namespace base
}  // namespace lynx
