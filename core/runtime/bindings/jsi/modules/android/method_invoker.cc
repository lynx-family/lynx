// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/method_invoker.h"

#include <optional>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/include/expected.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/java_value.h"
#include "core/base/android/jni_helper.h"
#include "core/base/android/piper_data.h"
#include "core/base/js_constants.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/bindings/jsi/modules/android/platform_jsi/lynx_platform_jsi_object_android.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/value_wrapper/android/value_impl_android.h"

namespace lynx {
namespace piper {
namespace {

std::string PubValueTypeInJS(const base::android::JavaValue& value) {
  if (value.IsNull()) {
    return "Undefined Or Null";
  } else if (value.IsNumber()) {
    return "Number";
  } else if (value.IsString()) {
    return "String";
  } else if (value.IsMap() || value.IsArray()) {
    return "Object";
  } else {
    return "Unknown";
  }
}

std::string JavaTypeCharToString(char type) {
  switch (type) {
    case 'b':
      return "byte";
      break;
    case 'B':
      return "Byte";
      break;
    case 's':
      return "short";
      break;
    case 'S':
      return "Short";
      break;
    case 'l':
      return "long";
      break;
    case 'L':
      return "Long";
      break;
    case 'c':
      return "char";
      break;
    case 'C':
      return "Character";
      break;
    case 'z':
      return "boolean";
      break;
    case 'Z':
      return "Boolean";
      break;
    case 'i':
      return "int";
      break;
    case 'I':
      return "Int";
      break;
    case 'd':
      return "double";
      break;
    case 'D':
      return "Double";
      break;
    case 'f':
      return "float";
      break;
    case 'F':
      return "Float";
      break;
    case 'T':
      return "String";
      break;
    case 'X':
      return "Callback";
      break;
    case 'P':
      return "Promise";
      break;
    case 'M':
      return "ReadableMap";
      break;
    case 'A':
      return "ReadableArray";
      break;
    case 'Y':
      return "Dynamic";
      break;
    case 'a':
      return "byteArray";
      break;
    default:
      return "unknown type";
      break;
  }
}

bool isNullable(char type) {
  switch (type) {
    case 'B':
    case 'L':
    case 'C':
    case 'P':
    case 'Y':
    case 'D':
    case 'T':
    case 'Z':
    case 'I':
    case 'F':
    case 'S':
    case 'A':
    case 'M':
    case 'X':
    case 'a':
    case 0:
      return true;
    default:
      return false;
      ;
  }
}

// Target Type Checkers
bool IsNumber(char type) {
  switch (type) {
    // byte & Byte
    case 'b':
    case 'B':
    // short & Short
    case 's':
    case 'S':
    // long & Long
    case 'l':
    case 'L':
    // int & Int
    case 'i':
    case 'I':
    // double & Double
    case 'd':
    case 'D':
    // float & Float
    case 'f':
    case 'F':
      return true;
    default:
      return false;
  }
}

bool IsBool(char type) {
  switch (type) {
    case 'Z':
    case 'z':
    case '0':
      return true;
    default:
      return false;
  }
}

bool IsString(char type) {
  switch (type) {
    // String
    case 'T':
    // char & Character
    case 'c':
    case 'C':
      return true;
    default:
      return false;
  }
}

bool IsArray(char type) {
  switch (type) {
    case 'A':
      return true;
    default:
      return false;
  }
}

bool IsMap(char type) {
  switch (type) {
    case 'M':
      return true;
    default:
      return false;
  }
}

bool IsArrayBuffer(char type) {
  switch (type) {
    case 'a':
      return true;
    default:
      return false;
  }
}

bool IsCallback(char type) {
  switch (type) {
    case 'X':
      return true;
    default:
      return false;
  }
}

}  // namespace

std::optional<base::LynxError> MethodInvoker::ReportPendingJniException() {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (env->ExceptionCheck() == JNI_FALSE) {
    return std::nullopt;
  }

  lynx::base::android::ScopedLocalJavaRef<jthrowable> throwable(
      env, env->ExceptionOccurred());

  if (!throwable.Get()) {
    return std::make_optional<base::LynxError>(
        error::E_NATIVE_MODULES_EXCEPTION,
        LynxModuleUtils::GenerateErrorMessage(
            module_name_, method_name_, "Unable to get pending JNI exception."),
        "This error is caught by native, please ask RD of Lynx or client for "
        "help.",
        base::LynxErrorLevel::Error);
  }
  env->ExceptionDescribe();
  env->ExceptionClear();

  std::string error_stack;
  std::string error_msg;
  base::android::GetExceptionInfo(env, throwable, error_msg, error_stack);
  std::optional<base::LynxError> error = std::make_optional<base::LynxError>(
      error::E_NATIVE_MODULES_EXCEPTION,
      LynxModuleUtils::GenerateErrorMessage(module_name_, method_name_,
                                            error_msg),
      "This error is caught by native, please ask RD of Lynx "
      "or client for help.",
      base::LynxErrorLevel::Error);
  error->AddCallStack(error_stack);
  return error;
}

namespace {
template <typename T>
static double doubleValue(JNIEnv* env, jclass c, const T& value) {
  static jmethodID doubleValueMethod =
      env->GetMethodID(c, "doubleValue", "()D");
  return env->CallDoubleMethod(value, doubleValueMethod);
}
}  // namespace

namespace {
std::size_t CountJsArgs(const std::string& signature) {
  std::size_t count = 0;
  for (char c : signature) {
    switch (c) {
      case 'P':
        break;
      default:
        count += 1;
        break;
    }
  }
  return count;
}

}  // namespace

MethodInvoker::MethodInvoker(jobject method, std::string signature,
                             const std::string& module_name,
                             const std::string& method_name)
    : signature_(signature),
      args_count_(CountJsArgs(signature) - 2),
      module_name_(module_name),
      method_name_(method_name) {
  JNIEnv* env = base::android::AttachCurrentThread();
  method_ = env->FromReflectedMethod(method);
}

base::expected<jvalue, std::string> MethodInvoker::ExtractPubValue(
    const base::android::JavaValue& transfer_method_params, int arg_index,
    char type) {
  jvalue convert_result;
  const std::string java_type_str = JavaTypeCharToString(type);
  const std::string arg_type_str = PubValueTypeInJS(transfer_method_params);
  auto on_error_occurred = [&arg_type_str, &java_type_str, arg_index,
                            this]() -> std::string {
    auto error_message = LynxModuleUtils::GenerateErrorMessage(
        module_name_, method_name_,
        LynxModuleUtils::ExpectedButGotAtIndexError(java_type_str, arg_type_str,
                                                    arg_index));
    return error_message;
  };

  // null & undefine in JS method params will convert to nullPtr java value
  // but in JS Object, null & undefine will drop in piperValue convert to
  // pubValue
  if (transfer_method_params.IsNull()) {
    if (isNullable(type)) {
      convert_result = transfer_method_params.JNull();
      return convert_result;
    } else {
      return base::unexpected(on_error_occurred());
    }
  }

  if (transfer_method_params.IsBool()) {
    if (type == 'z' || type == '0') {
      convert_result = transfer_method_params.JBoolean();
    } else if (type == 'Z') {
      convert_result = transfer_method_params.WrapperJBoolean();
    } else {
      return base::unexpected(on_error_occurred());
    }
    return convert_result;
  }

  if (transfer_method_params.IsNumber()) {
    switch (type) {
      case 'b':
        convert_result = transfer_method_params.JByte();
        break;
      case 'B':
        convert_result = transfer_method_params.WrapperJByte();
        break;
      case 's':
        convert_result = transfer_method_params.JShort();
        break;
      case 'S':
        convert_result = transfer_method_params.WrapperJShort();
        break;
      case 'l':
        convert_result = transfer_method_params.JLong();
        break;
      case 'L':
        convert_result = transfer_method_params.WrapperJLong();
        break;
      case 'i':
        convert_result = transfer_method_params.JInt();
        break;
      case 'I':
        convert_result = transfer_method_params.WrapperJInt();
        break;
      case 'd':
        convert_result = transfer_method_params.JDouble();
        break;
      case 'D':
        convert_result = transfer_method_params.WrapperJDouble();
        break;
      case 'f':
        convert_result = transfer_method_params.JFloat();
        break;
      case 'F':
        convert_result = transfer_method_params.WrapperJFloat();
        break;
      default:
        return base::unexpected(on_error_occurred());
    }
    return convert_result;
  }

  if (transfer_method_params.IsString()) {
    if (type == 'c') {
      convert_result = transfer_method_params.JChar();
    } else if (type == 'C') {
      convert_result = transfer_method_params.WrapperJChar();
    } else if (type == 'T') {
      convert_result.l = transfer_method_params.JString();
    } else {
      return base::unexpected(on_error_occurred());
    }
    return convert_result;
  }

  if (transfer_method_params.IsMap()) {
    if (type == 'M') {
      convert_result.l = transfer_method_params.Map()->jni_object();
      return convert_result;
    } else {
      return base::unexpected(on_error_occurred());
    }
  }

  if (transfer_method_params.IsArray()) {
    if (type == 'A') {
      convert_result.l = transfer_method_params.Array()->jni_object();
      return convert_result;
    } else {
      return base::unexpected(on_error_occurred());
    }
  }

  if (transfer_method_params.IsArrayBuffer()) {
    if (type == 'a') {
      convert_result.l = transfer_method_params.JByteArray();
      return convert_result;
    } else {
      return base::unexpected(on_error_occurred());
    }
  }

  jvalue value;
  value.l = nullptr;
  return value;
}
base::expected<std::unique_ptr<pub::Value>, ErrorPair> MethodInvoker::Invoke(
    jobject module, const pub::Value* args, size_t args_count,
    base::MoveOnlyClosure<
        base::expected<base::android::ScopedGlobalJavaRef<jobject>,
                       std::string>,
        int>
        function_creator,
    jobject nativePromise) {
  // parser first arg
  std::string first_arg_str;
  if (args_count > 0 && args && args->IsArray() &&
      args->GetValueAtIndex(0)->IsString()) {
    first_arg_str = args->GetValueAtIndex(0)->str();
  }

  auto required_arg_count = ContainsPromise() ? args_count_ + 1 : args_count_;
  // Check Args Count
  if (args_count != args_count_) {
    // create JS error msg
    auto error_message = LynxModuleUtils::GenerateErrorMessage(
        module_name_, method_name_,
        first_arg_str +
            LynxModuleUtils::ExpectedButGotError(args_count_, args_count));
    // create LynxError for delegate
    auto error = base::LynxError{error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_NUM,
                                 std::string{error_message},
                                 "Please check the arguments.",
                                 base::LynxErrorLevel::Error};
    return base::unexpected(
        std::make_pair(std::move(error_message), std::move(error)));
  }

  LOGI("NativeModule: LynxModuleAndroid MethodInvoker::InvokeMethod, method: ("
       << module_name_ << "." << method_name_ << "." << first_arg_str
       << ") will fire " << this);

  // Convert Pub::Value To Android Value
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::JniLocalScope scope(env);
  base::android::JavaValue transfer_method_params[args_count_];
  jvalue java_arguments[required_arg_count];
  // Since the cached ScopedGlobalJavaRef is an lvalue, the copy constructor of
  // ScopedGlobalJavaRef will be called when Vector is expanded, causing the
  // jobject from ScopedGlobalJavaRef to be destroyed and JNI to call Crash.
  // Therefore, list is used for caching here to prevent crash when Vector is
  // expanded.
  std::list<base::android::ScopedGlobalJavaRef<jobject>> callback_container;
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, PUB_VALUE_TO_JNI_VALUE);
  for (size_t i = 0; i < args_count_; i++) {
    char type = signature_[i + 2];
    base::expected<jvalue, std::string> ret;
    transfer_method_params[i] = pub::ValueUtilsAndroid::ConvertValueToJavaValue(
        *(args->GetValueAtIndex(i)));
    if (type == 'X') {
      base::expected<base::android::ScopedGlobalJavaRef<jobject>, std::string>
          expected_callback = function_creator(i);
      if (expected_callback.has_value()) {
        callback_container.push_back(std::move(expected_callback.value()));
        ret = {.l = callback_container.back().Get()};
      } else {
        // If the Callback parameter in JS is undefined, the Callback in
        // LynxModule will be converted to null.
        LOGE(expected_callback.error().c_str());
        ret = {.l = nullptr};
      }
    } else {
      ret = ExtractPubValue(transfer_method_params[i], i, type);
    }
    if (ret.has_value()) {
      java_arguments[i] = std::move(ret.value());
    } else {
      std::string error_message = std::move(ret.error());
      auto error = base::LynxError{
          error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_TYPE,
          std::string{error_message}, "Please check the arguments.",
          base::LynxErrorLevel::Error};
      return base::unexpected(
          std::make_pair(std::move(error_message), std::move(error)));
    }
  }
  // Auth Verify
  if (auth_verify_) {
    std::shared_ptr<base::android::JavaOnlyArray> verify_params =
        std::make_shared<base::android::JavaOnlyArray>();
    for (size_t i = 0; i < args_count_; i++) {
      verify_params->PushJavaValue(transfer_method_params[i]);
    }
    bool verify_result = auth_verify_(method_name_, verify_params);
    if (!verify_result) {
      std::string error_message =
          std::string{" has been rejected by LynxMethodAuth!"};
      auto error = base::LynxError{
          error::E_NATIVE_MODULES_COMMON_AUTHORIZATION_ERROR, error_message,
          "Please check the arguments.", base::LynxErrorLevel::Error};
      return base::unexpected(
          std::make_pair(std::move(error_message), std::move(error)));
    }
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);
  // Append promise param
  if (ContainsPromise()) {
    java_arguments[required_arg_count - 1] = {.l = nativePromise};
  }
  // Real Call Module Method!
  base::expected<std::unique_ptr<pub::Value>, ErrorPair> ret =
      CallPlatformImplementation(env, module, java_arguments);
  if (!ret.has_value()) {
    return base::unexpected(std::move(ret.error()));
  }
  LOGI("NativeModules: LynxModuleAndroid MethodInvoker::InvokeMethod, method: ("
       << module_name_ << "." << method_name_ << "." << first_arg_str
       << ") call platform implementation" << this)
  return ret;
}

base::expected<std::unique_ptr<pub::Value>, ErrorPair>
MethodInvoker::CallPlatformImplementation(JNIEnv* env, jobject module,
                                          jvalue* java_arguments) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, CALL_PLATFORM_IMPLEMENTATION);
#define PRIMITIVE_CASE(METHOD)                                                 \
  {                                                                            \
    auto result = env->Call##METHOD##MethodA(module, method_, java_arguments); \
    auto jni_error = ReportPendingJniException();                              \
    if (jni_error.has_value()) {                                               \
      return base::unexpected(                                                 \
          std::make_pair(std::move(jni_error_hit), std::move(jni_error)));     \
    }                                                                          \
    return std::make_unique<lynx::pub::ValueImplAndroid>(                      \
        base::android::JavaValue(result));                                     \
  }
#define PRIMITIVE_CASE_CASTING(METHOD, RESULT_TYPE)                            \
  {                                                                            \
    auto result = env->Call##METHOD##MethodA(module, method_, java_arguments); \
    auto jni_error = ReportPendingJniException();                              \
    if (jni_error.has_value()) {                                               \
      return base::unexpected(                                                 \
          std::make_pair(std::move(jni_error_hit), std::move(jni_error)));     \
    }                                                                          \
    return std::make_unique<lynx::pub::ValueImplAndroid>(                      \
        base::android::JavaValue(static_cast<RESULT_TYPE>(result)));           \
  }

#define OBJECT_CASE(CLASS)                                                     \
  {                                                                            \
    lynx::base::android::ScopedLocalJavaRef<jobject> obj(                      \
        env, env->CallObjectMethodA(module, method_, java_arguments));         \
    auto jni_error = ReportPendingJniException();                              \
    if (jni_error.has_value()) {                                               \
      return base::unexpected(                                                 \
          std::make_pair(std::move(jni_error_hit), std::move(jni_error)));     \
    }                                                                          \
    static auto cls = base::android::GetGlobalClass(env, "java/lang/" #CLASS); \
    return std::make_unique<lynx::pub::ValueImplAndroid>(                      \
        base::android::JavaValue(doubleValue(env, cls.Get(), obj.Get())));     \
  }

  char return_type = signature_.at(0);
  const std::string jni_error_hit = LynxModuleUtils::GenerateErrorMessage(
      module_name_, method_name_, "Unable to get pending JNI exception.");
  switch (return_type) {
    // Void Return
    case 'v': {
      env->CallVoidMethodA(module, method_, java_arguments);
      auto error_ptr = ReportPendingJniException();
      if (error_ptr.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error_ptr)));
      }
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue::Undefined());
    }
    // Int & Double correspond to int_32 & int_64 respectively, no need to
    // convert data
    case 'i':
      PRIMITIVE_CASE(Int)
    case 'I':
      OBJECT_CASE(Integer)
    case 'd':
      PRIMITIVE_CASE(Double)
    case 'D':
      OBJECT_CASE(Double)
    // Notice: Convert other data types to double
    // byte return
    case 'b':
      PRIMITIVE_CASE_CASTING(Byte, double)
    // Wrapper byte return
    case 'B':
      OBJECT_CASE(Byte)
    case 's':
      PRIMITIVE_CASE_CASTING(Short, double)
    case 'S':
      OBJECT_CASE(Short)
    case 'f':
      PRIMITIVE_CASE_CASTING(Float, double)
    case 'F':
      OBJECT_CASE(Float)
    case 'l':
      PRIMITIVE_CASE_CASTING(Long, int64_t)
    case 'L': {
      lynx::base::android::ScopedLocalJavaRef<jobject> obj(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      static auto cls = base::android::GetGlobalClass(env, "java/lang/Long");
      // The function signature of Long type is "J"
      static jmethodID long_value_method =
          env->GetMethodID(cls.Get(), "longValue", "()J");
      jlong value = env->CallLongMethod(obj.Get(), long_value_method);
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue(static_cast<int64_t>(value)));
    }
    case 'c': {
      jchar result = env->CallCharMethodA(module, method_, java_arguments);
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      lynx::base::android::ScopedLocalJavaRef<jstring> str =
          base::android::JNIConvertHelper::ConvertToJNIString(env, &result, 1);
      // const char* c_value = env->GetStringUTFChars(str.Get(), JNI_FALSE);
      auto char_str = std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue((str.Get())));
      // env->ReleaseStringUTFChars(str.Get(), c_value);
      return char_str;
    }
    case 'C': {
      lynx::base::android::ScopedLocalJavaRef<jobject> obj(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      static auto cls =
          base::android::GetGlobalClass(env, "java/lang/Character");
      static jmethodID charValueMethod =
          env->GetMethodID(cls.Get(), "charValue", "()C");
      jchar jc_value = env->CallCharMethod(obj.Get(), charValueMethod);
      lynx::base::android::ScopedLocalJavaRef<jstring> str =
          base::android::JNIConvertHelper::ConvertToJNIString(env, &jc_value,
                                                              1);
      // const char* c_value = env->GetStringUTFChars(str.Get(), JNI_FALSE);
      auto char_str = std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue((str.Get())));
      // env->ReleaseStringUTFChars(str.Get(), c_value);
      return char_str;
    }
    // String
    case 'T': {
      lynx::base::android::ScopedLocalJavaRef<jstring> str(
          env, static_cast<jstring>(
                   env->CallObjectMethodA(module, method_, java_arguments)));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue(str.Get()));
    }
    // Boolean
    case 'z':
      PRIMITIVE_CASE_CASTING(Boolean, bool)
    case 'Z': {
      lynx::base::android::ScopedLocalJavaRef<jobject> obj(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      static auto cls = base::android::GetGlobalClass(env, "java/lang/Boolean");
      static jmethodID methodID =
          env->GetMethodID(cls.Get(), "booleanValue", "()Z");
      jboolean v = env->CallBooleanMethod(obj.Get(), methodID);
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue((static_cast<bool>(v))));
    }
    // Map
    case 'M': {
      lynx::base::android::ScopedLocalJavaRef<jobject> obj(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      auto pub_java_value_map =
          std::make_shared<base::android::JavaOnlyMap>(env, obj);
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue(std::move(pub_java_value_map)));
    }
    // Aarry
    case 'A': {
      lynx::base::android::ScopedLocalJavaRef<jobject> arr(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      auto pub_java_value_array =
          std::make_shared<base::android::JavaOnlyArray>(env, arr);
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue(std::move(pub_java_value_array)));
    }
    case 'a': {
      lynx::base::android::ScopedGlobalJavaRef<jobject> byte_array(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue(
              std::move(byte_array),
              base::android::JavaValue::JavaValueType::ByteArray));
    }
    case 'J': {
      lynx::base::android::ScopedLocalJavaRef<jobject> json_data(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue(
              json_data, base::android::JavaValue::JavaValueType::Transfer));
    }
    case 'O': {
      lynx::base::android::ScopedLocalJavaRef<jobject> lynx_object(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      // create a lynx object module
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue(
              lynx_object,
              base::android::JavaValue::JavaValueType::LynxObject));
    }
    case 'E': {  // means templateData;
      lynx::base::android::ScopedLocalJavaRef<jobject> data(
          env, env->CallObjectMethodA(module, method_, java_arguments));
      auto error = ReportPendingJniException();
      if (error.has_value()) {
        return base::unexpected(
            std::make_pair(std::move(jni_error_hit), std::move(error)));
      }
      return std::make_unique<lynx::pub::ValueImplAndroid>(
          base::android::JavaValue(
              data, base::android::JavaValue::JavaValueType::TemplateData));
    }
    default:
      LOGF("NativeModule: FireMethod Unknown Return Type: " << return_type);
      std::string error_message =
          "NativeModule: FireMethod Unknown Return Type: " +
          std::to_string(return_type);
      return base::unexpected(
          std::make_pair(std::move(error_message), std::nullopt));
  }
}

bool MethodInvoker::VerifySignature(const pub::Value* args, size_t args_count) {
  // Verify the argument count
  auto required_arg_count = ContainsPromise() ? args_count_ + 1 : args_count_;
  if (args_count != required_arg_count) {
    return false;
  }
  // Verify the argument types
  for (size_t i = 0; i < args_count; i++) {
    auto arg = args->GetValueAtIndex(i);
    bool verify_result = false;
    char target_type = signature_[i + 2];
    if (arg->IsUndefined() || arg->IsNil()) {
      verify_result = isNullable(target_type);
    } else if (arg->IsNumber() && target_type == 'X') {
      verify_result = true;
    } else if (arg->IsBool()) {
      verify_result = IsBool(target_type);
    } else if (arg->IsNumber()) {
      verify_result = IsNumber(target_type);
    } else if (arg->IsString()) {
      verify_result = IsString(target_type);
    } else if (arg->IsMap()) {
      verify_result = IsMap(target_type);
    } else if (arg->IsArray()) {
      verify_result = IsArray(target_type);
    } else if (arg->IsArrayBuffer()) {
      verify_result = IsArrayBuffer(target_type);
    }
    if (verify_result) {
      continue;
    }
    return false;
  }
  return true;
}

}  // namespace piper
}  // namespace lynx
