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
#include "core/base/android/jni_helper.h"
#include "core/base/android/piper_data.h"
#include "core/base/js_constants.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/jsi/jsi.h"
#include "core/services/recorder/recorder_controller.h"

namespace lynx {
namespace piper {
namespace {

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

}  // namespace

bool MethodInvoker::reportPendingJniException() {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (env->ExceptionCheck() == JNI_FALSE) {
    return false;
  }

  lynx::base::android::ScopedLocalJavaRef<jthrowable> throwable(
      env, env->ExceptionOccurred());

  if (!throwable.Get()) {
    delegate_->OnMethodInvoked(module_name_, method_name_,
                               error::E_NATIVE_MODULES_EXCEPTION);
    delegate_->OnErrorOccurred(base::LynxError{
        error::E_NATIVE_MODULES_EXCEPTION,
        LynxModuleUtils::GenerateErrorMessage(
            module_name_, method_name_, "Unable to get pending JNI exception."),
        "This error is caught by native, please ask RD of Lynx or client for "
        "help.",
        base::LynxErrorLevel::Error});
    return true;
  }
  env->ExceptionDescribe();
  env->ExceptionClear();

  std::string error_stack;
  std::string error_msg;
  base::android::GetExceptionInfo(env, throwable, error_msg, error_stack);
  delegate_->OnMethodInvoked(module_name_, method_name_,
                             error::E_NATIVE_MODULES_EXCEPTION);
  base::LynxError error{error::E_NATIVE_MODULES_EXCEPTION,
                        LynxModuleUtils::GenerateErrorMessage(
                            module_name_, method_name_, error_msg),
                        "This error is caught by native, please ask RD of Lynx "
                        "or client for help.",
                        base::LynxErrorLevel::Error};
  error.AddCallStack(error_stack);
  delegate_->OnErrorOccurred(std::move(error));
  return true;
}

std::optional<JSINativeException> AssembleJSArray(
    lynx::base::android::JavaOnlyArray* array, Runtime* rt,
    const piper::Array* args, PtrContainerMap& ptr_container_map,
    JSValueCircularArray& pre_object_vector, int depth) {
  auto size_opt = args->size(*rt);
  if (!size_opt) {
    return std::optional(BUILD_JSI_NATIVE_EXCEPTION("Can't find args's size."));
  }

  Object obj_temp = piper::Value(*rt, *args).getObject(*rt);
  if (CheckIsCircularJSObjectIfNecessaryAndReportError(
          *rt, obj_temp, pre_object_vector, depth, "AssembleJSArray!")) {
    return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
        "Find circular js object in AssembleJSArray."));
  }
  // As Object is Movable, not copyable, do not push the Object you will use
  // later to vector! You need clone a new one.
  ScopedJSObjectPushPopHelper scoped_push_pop_helper(pre_object_vector,
                                                     std::move(obj_temp));

  for (size_t index = 0; index < *size_opt; index++) {
    auto value_opt = args->getValueAtIndex(*rt, index);
    if (!value_opt) {
      return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
          "args[" + std::to_string(index) + "] is null."));
    }
    Value& value = *value_opt;
    if (value.isBool()) {
      array->PushBoolean(value.getBool());
    } else if (value.isNumber()) {
      array->PushDouble(value.getNumber());
    } else if (value.isString()) {
      array->PushString(value.getString(*rt).utf8(*rt));
    } else if (value.isObject()) {
      Object obj = value.getObject(*rt);
      if (obj.isArray(*rt)) {
        Array arr = obj.getArray(*rt);
        auto jni_array = std::make_shared<base::android::JavaOnlyArray>();
        ptr_container_map.array_container_.emplace_back(jni_array);
        auto opt_exception =
            AssembleJSArray(jni_array.get(), rt, &arr, ptr_container_map,
                            pre_object_vector, depth + 1);
        if (opt_exception) {
          LOGE("Error happened in AssembleJSArray, array index:" << index);
          ADD_STACK(opt_exception.value());
          return opt_exception;
        }
        array->PushArray(jni_array.get());
      } else if (obj.isHostObject(*rt) || obj.isFunction(*rt)) {
        continue;
      } else if (obj.isArrayBuffer(*rt)) {
        ArrayBuffer array_buffer = obj.getArrayBuffer(*rt);
        PushByteArrayToJavaArray(rt, array_buffer, array);
      } else {
        auto jni_map = std::make_shared<base::android::JavaOnlyMap>();
        ptr_container_map.map_container_.emplace_back(jni_map);
        if (obj.hasProperty(*rt, BIG_INT_VAL)) {
          // some tricky logic for BigInt
          // such as {"id":8913891381287328398} will exist on js
          // { "id" : { "__lynx_val__" : "8913891381287328398" }}
          auto value_long_opt = obj.getProperty(*rt, BIG_INT_VAL);
          if (!value_long_opt) {
            return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
                "Can't find BigInt's " + std::string(BIG_INT_VAL) +
                " property."));
          }
          if (value_long_opt->isString()) {
            // remove {"__lynx_val__":} and convert to  {"id":
            // 8913891381287328398 }}
            auto val_str_opt = value_long_opt->toString(*rt);
            if (!val_str_opt) {
              return std::optional(
                  BUILD_JSI_NATIVE_EXCEPTION("BigInt to string failed."));
            }
            const std::string val_str = val_str_opt->utf8(*rt);
            array->PushInt64(static_cast<int64_t>(
                std::strtoll(val_str.c_str(), nullptr, 0)));
            continue;
          }
        }
        auto opt_exception =
            AssembleJSMap(jni_map.get(), rt, &obj, ptr_container_map,
                          pre_object_vector, depth + 1);
        if (opt_exception) {
          LOGE("Error happened in AssembleJSArray, array index:" << index);
          ADD_STACK(opt_exception.value());
          return opt_exception;
        }
        array->PushMap(jni_map.get());
      }
    }
  }
  return std::nullopt;
}

std::optional<JSINativeException> AssembleJSMap(
    lynx::base::android::JavaOnlyMap* map, Runtime* rt,
    const piper::Object* js_map, PtrContainerMap& ptr_container_map,
    JSValueCircularArray& pre_object_vector, int depth) {
  std::optional<Array> array = js_map->getPropertyNames(*rt);
  if (!array) {
    return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
        "Error happened in AssembleJSMap, js_map's propertyNames is empty."));
  }
  auto size = array->size(*rt);
  if (!size) {
    return std::optional(
        BUILD_JSI_NATIVE_EXCEPTION("Error happened in AssembleJSMap, js_map's "
                                   "propertyNames's can't find size."));
  }

  if (CheckIsCircularJSObjectIfNecessaryAndReportError(
          *rt, *js_map, pre_object_vector, depth, "AssembleJSMap!")) {
    return std::optional(
        BUILD_JSI_NATIVE_EXCEPTION("AssembleJSMap has circular JSObject."));
  }
  // As Object is Movable, not copyable, do not push the Object you will use
  // later to vector! You need clone a new one.
  ScopedJSObjectPushPopHelper scoped_push_pop_helper(
      pre_object_vector, Value(*rt, *js_map).getObject(*rt));

  for (size_t index = 0; index < *size; index++) {
    auto item = array->getValueAtIndex(*rt, index);
    if (!item) {
      return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
          "AssembleJSMap value[" + std::to_string(index) + "] is null."));
    }
    String key = item->getString(*rt);
    std::string strKey = key.utf8(*rt);
    auto value = js_map->getProperty(*rt, key);
    if (!value) {
      return std::optional(BUILD_JSI_NATIVE_EXCEPTION("AssembleJSMap value[" +
                                                      strKey + "] is null."));
    }
    if (value->isBool()) {
      map->PushBoolean(strKey, value->getBool());
    } else if (value->isNumber()) {
      map->PushDouble(strKey, value->getNumber());
    } else if (value->isString()) {
      map->PushString(strKey, value->getString(*rt).utf8(*rt));
    } else if (value->isObject()) {
      Object obj = value->getObject(*rt);
      if (obj.isArray(*rt)) {
        Array arr = obj.getArray(*rt);
        auto jni_array = std::make_shared<base::android::JavaOnlyArray>();
        ptr_container_map.array_container_.emplace_back(jni_array);
        auto opt_exception =
            AssembleJSArray(jni_array.get(), rt, &arr, ptr_container_map,
                            pre_object_vector, depth + 1);
        if (opt_exception) {
          LOGE("Error happened in AssembleJSMap, key:" << strKey);
          ADD_STACK(opt_exception.value());
          return opt_exception;
        }
        map->PushArray(strKey, jni_array.get());
      } else if (obj.isHostObject(*rt) || obj.isFunction(*rt)) {
        continue;
      } else if (obj.isArrayBuffer(*rt)) {
        ArrayBuffer array_buffer = obj.getArrayBuffer(*rt);
        PushByteArrayToJavaMap(rt, strKey, array_buffer, map);
      } else {
        // some tricky logic for BigInt
        // such as {"id":8913891381287328398} will exist on js
        // { "id" : { "__lynx_val__" : "8913891381287328398" }}
        if (obj.hasProperty(*rt, BIG_INT_VAL)) {
          auto value_long_opt = obj.getProperty(*rt, BIG_INT_VAL);
          if (!value_long_opt) {
            return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
                "Can't find BigInt's " + std::string(BIG_INT_VAL) +
                " property."));
          }
          if (value_long_opt->isString()) {
            // remove {"__lynx_val__":} and convert to  {"id":
            // 8913891381287328398 }}
            auto val_str_opt = value_long_opt->toString(*rt);
            if (!val_str_opt) {
              return std::optional(
                  BUILD_JSI_NATIVE_EXCEPTION("BigInt to string failed."));
            }
            const std::string val_str = val_str_opt->utf8(*rt);
            map->PushInt64(strKey.c_str(), static_cast<int64_t>(std::strtoll(
                                               val_str.c_str(), nullptr, 0)));
            continue;
          }
        }

        auto jni_map = std::make_shared<base::android::JavaOnlyMap>();
        ptr_container_map.map_container_.emplace_back(jni_map);
        auto opt_exception =
            AssembleJSMap(jni_map.get(), rt, &obj, ptr_container_map,
                          pre_object_vector, depth + 1);
        if (opt_exception) {
          LOGE("Error happened in AssembleJSMap, key:" << strKey);
          ADD_STACK(opt_exception.value());
          return opt_exception;
        }
        map->PushMap(strKey, jni_map.get());
      }
    }
  }
  return std::nullopt;
}

namespace {
template <typename T>
static jobject valueOf(JNIEnv* env, jclass c, const char* signature,
                       const T& value) {
  static jmethodID valueOfMethod =
      env->GetStaticMethodID(c, "valueOf", signature);
  return env->CallStaticObjectMethod(c, valueOfMethod, value);
}

template <typename T>
static double doubleValue(JNIEnv* env, jclass c, const T& value) {
  static jmethodID doubleValueMethod =
      env->GetMethodID(c, "doubleValue", "()D");
  return env->CallDoubleMethod(value, doubleValueMethod);
}
jobject booleanValueOf(JNIEnv* env, jboolean value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Boolean");
  return valueOf(env, cls.Get(), "(Z)Ljava/lang/Boolean;", value);
}
jobject byteValueOf(JNIEnv* env, jbyte value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Byte");
  return valueOf(env, cls.Get(), "(B)Ljava/lang/Byte;", value);
}

jobject charValueOf(JNIEnv* env, jchar value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Character");
  return valueOf(env, cls.Get(), "(C)Ljava/lang/Character;", value);
}

jobject shortValueOf(JNIEnv* env, jshort value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Short");
  return valueOf(env, cls.Get(), "(S)Ljava/lang/Short;", value);
}
jobject integerValueOf(JNIEnv* env, jint value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Integer");
  return valueOf(env, cls.Get(), "(I)Ljava/lang/Integer;", value);
}
jobject longValueOf(JNIEnv* env, jlong value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Long");
  return valueOf(env, cls.Get(), "(J)Ljava/lang/Long;", value);
}
jobject floatValueOf(JNIEnv* env, jfloat value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Float");
  return valueOf(env, cls.Get(), "(F)Ljava/lang/Float;", value);
}

jobject doubleValueOf(JNIEnv* env, jdouble value) {
  static auto cls = base::android::GetGlobalClass(env, "java/lang/Double");
  return valueOf(env, cls.Get(), "(D)Ljava/lang/Double;", value);
}

}  // namespace

base::android::ScopedGlobalJavaRef<jobject>
MethodInvoker::ConvertJSIFunctionToCallbackObject(
    Function function, Runtime* rt, ModuleCallbackType type,
    const Value* first_arg, uint64_t start_time,
    const piper::NativeModuleInfoCollectorPtr& timing_collector) {
  int64_t callback_id =
      delegate_->RegisterJSCallbackFunction(std::move(function));
#if ENABLE_TESTBENCH_RECORDER
  if (!callbacks_stack_.empty()) {
    auto& callbacks_top_vector = callbacks_stack_.top();
    callbacks_top_vector.push_back(callback_id);
  }
#endif
  if (callback_id == ModuleCallback::kInvalidCallbackId) {
    LOGW(
        "LynxModule, extractJSValue::extractJSValue failed, "
        "LynxRuntime has destroyed");
    return base::android::ScopedGlobalJavaRef<jobject>();
  }
  LOGV("LynxModule, extractJSValue found |JS FUNCTION| id: " << callback_id);
  ModuleCallbackAndroid::CallbackPair callback_pair =
      ModuleCallbackAndroid::createCallbackImpl(callback_id, shared_from_this(),
                                                type);
  auto callback = callback_pair.first;
  // Some JSB implement will use first arg as JSB function
  // name, so we need first arg for tracing.
  callback->SetStartTimeMS(start_time);
  callback->SetModuleName(GetModuleName());
  callback->SetMethodName(GetMethodName());
  callback->timing_collector_ = timing_collector;
  if (first_arg && first_arg->isString()) {
    std::string first_arg_str = first_arg->getString(*rt).utf8(*rt);
    callback->SetFirstArg(first_arg_str);
  }
  uint64_t callback_flow_id = TRACE_FLOW_ID();
  callback->SetCallbackFlowId(callback_flow_id);
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_JSB, "CreateJSB Callback",
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_flow_ids(callback_flow_id);
                        ctx.event()->add_debug_annotations(
                            "startTimestamp", std::to_string(start_time));
                      });
  callbackWrappers_.insert(callback);
  return std::move(callback_pair.second);
}

/**
 * Convert a JS Value to a Jni Value.
 *
 * @param rt: JS runtime instance.
 * @param args: JS Value.
 * @param type: A char representing the Jni value type. 0 for no type
 * specified. See 'LynxJavaMethodWrapper.java' for type specification.
 *
 * @return converted jvalue or exception.
 *
 * @throws runtime_error if typ mismatch.
 *
 */
base::expected<jvalue, JSINativeException> MethodInvoker::extractJSValue(
    Runtime* rt, const piper::Value* args, char type,
    PtrContainerMap& ptr_container_map, int arg_index, uint64_t start_time,
    const piper::NativeModuleInfoCollectorPtr& timing_collector) {
  piper::Scope scope(*rt);
  jvalue value;
  value.l = nullptr;
  JNIEnv* env = base::android::AttachCurrentThread();
  const auto java_type_str = JavaTypeCharToString(type);
  const auto arg_type_str = LynxModuleUtils::JSTypeToString(args);
  // #issue: #1510, Android, 4
  // report_error
  auto on_error_occurred = [&arg_type_str, &java_type_str, arg_index,
                            this]() -> JSINativeException {
    auto error_message = LynxModuleUtils::GenerateErrorMessage(
        module_name_, method_name_,
        LynxModuleUtils::ExpectedButGotAtIndexError(java_type_str, arg_type_str,
                                                    arg_index));
    if (delegate_) {
      delegate_->OnMethodInvoked(
          module_name_, method_name_,
          error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_TYPE);
      delegate_->OnErrorOccurred(base::LynxError{
          error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_TYPE, error_message,
          "Please check the arguments.", base::LynxErrorLevel::Error});
    }
    return BUILD_JSI_NATIVE_EXCEPTION(error_message);
  };
  if (args->isNull() || args->isUndefined()) {
    if (isNullable(type)) {
      value.l = nullptr;
      return value;
    } else {
      return base::unexpected(on_error_occurred());
    }
  } else if (args->isNumber() || args->isBool()) {
    if (args->isBool()) {
      jboolean b = static_cast<jboolean>(args->getBool());
      if (type == 'z' || type == '0') {
        value.z = b;
        return value;
      } else if (type == 'Z') {
        value.l = booleanValueOf(env, b);
        return value;
      } else {
        return base::unexpected(on_error_occurred());
      }
    }
    switch (type) {
      case 'b':
        value.b = static_cast<jbyte>(args->getNumber());
        return value;
      case 'B':
        value.l = byteValueOf(env, static_cast<jbyte>(args->getNumber()));
        return value;
      case 's':
        value.s = static_cast<jshort>(args->getNumber());
        return value;
      case 'S':
        value.l = shortValueOf(env, static_cast<jshort>(args->getNumber()));
        return value;
      case 'l':
        value.j = static_cast<jlong>(args->getNumber());
        return value;
      case 'L':
        value.l = longValueOf(env, static_cast<jlong>(args->getNumber()));
        return value;
      case 'i':
        value.i = static_cast<jint>(args->getNumber());
        return value;
      case 'I':
        value.l = integerValueOf(env, static_cast<jint>(args->getNumber()));
        return value;
      case 'd':
        value.d = static_cast<jdouble>(args->getNumber());
        return value;
      case 'D':
        value.l = doubleValueOf(env, static_cast<jdouble>(args->getNumber()));
        return value;
      case 'f':
        value.f = static_cast<jfloat>(args->getNumber());
        return value;
      case 'F':
        value.l = floatValueOf(env, static_cast<jfloat>(args->getNumber()));
        return value;
      default:
        return base::unexpected(on_error_occurred());
    }
  } else if (args->isString()) {
    switch (type) {
      case 'c': {
        std::string result = args->getString(*rt).utf8(*rt);
        char c_result = '\0';
        if (!result.empty()) {
          c_result = result.c_str()[0];
        }
        value.c = static_cast<jchar>(c_result);
        return value;
      } break;
      case 'C': {
        std::string result = args->getString(*rt).utf8(*rt);
        char c_result = '\0';
        if (!result.empty()) {
          c_result = result.c_str()[0];
        }
        value.l = charValueOf(env, static_cast<jchar>(c_result));
        return value;
      } break;
      case 'T': {
        base::android::ScopedGlobalJavaRef<jstring> j_string(
            base::android::JNIConvertHelper::ConvertToJNIStringUTF(
                env, args->getString(*rt).utf8(*rt)));
        value.l = j_string.Get();
        ptr_container_map.string_container_.emplace_back(std::move(j_string));
        return value;
      }
      default:
        return base::unexpected(on_error_occurred());
    }
  } else if (args->isObject()) {
    JSValueCircularArray pre_object_vector;

    Object obj = args->getObject(*rt);
    if (obj.isArray(*rt)) {
      if (type != 'A') {
        return base::unexpected(on_error_occurred());
      }
      Array arr = obj.getArray(*rt);
      auto jni_array = std::make_shared<base::android::JavaOnlyArray>();
      ptr_container_map.array_container_.emplace_back(jni_array);
      auto opt_exception = AssembleJSArray(
          jni_array.get(), rt, &arr, ptr_container_map, pre_object_vector);
      if (opt_exception) {
        ADD_STACK(opt_exception.value());
        return base::unexpected(std::move(opt_exception.value()));
      }
      value.l = jni_array->jni_object();
    } else if (obj.isFunction(*rt)) {
      // TODO(liujilong.me): handle callback.
      if (type != 'X') {
        return base::unexpected(on_error_occurred());
      }
      Function function = obj.getFunction(*rt);
      auto ref = ConvertJSIFunctionToCallbackObject(
          std::move(function), rt, ModuleCallbackType::Base, args - arg_index,
          start_time, timing_collector);
      ptr_container_map.jobject_container_.push_back(std::move(ref));
      value.l = ptr_container_map.jobject_container_.back().Get();
      return value;
    } else if (obj.hasProperty(*rt, BIG_INT_VAL)) {
      // if post BigInt object directly, then return value which type is "long"
      auto value_long_opt = obj.getProperty(*rt, BIG_INT_VAL);
      if (!value_long_opt) {
        return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
            "Can't find BigInt's " + std::string(BIG_INT_VAL) + " property."));
      }
      if (value_long_opt->isString()) {
        auto val_str_opt = value_long_opt->toString(*rt);
        if (!val_str_opt) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION("BigInt to string failed."));
        }
        const std::string val_str = val_str_opt->utf8(*rt);
        value.j = static_cast<jlong>(std::strtoll(val_str.c_str(), nullptr, 0));
      }
    } else if (obj.isArrayBuffer(*rt)) {
      if (type != 'a') {
        return base::unexpected(on_error_occurred());
      }
      ArrayBuffer array_buffer = obj.getArrayBuffer(*rt);
      base::android::ScopedGlobalJavaRef<jbyteArray> jni_byte_array(
          base::android::JNIHelper::ConvertToJNIByteArray(env, rt,
                                                          array_buffer));
      value.l = jni_byte_array.Get();
      ptr_container_map.byte_array_container_.emplace_back(
          std::move(jni_byte_array));
      return value;
    } else {
      // issue: #1510
      if (type != 'M') {
        return base::unexpected(on_error_occurred());
      }
      auto jni_map = std::make_shared<base::android::JavaOnlyMap>();
      ptr_container_map.map_container_.emplace_back(jni_map);
      auto opt_exception = AssembleJSMap(jni_map.get(), rt, &obj,
                                         ptr_container_map, pre_object_vector);
      if (opt_exception) {
        ADD_STACK(opt_exception.value());
        return base::unexpected(std::move(*opt_exception));
      }
      value.l = jni_map->jni_object();
    }
  }
  return value;
}

namespace {
std::size_t countJsArgs(const std::string& signature) {
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

template <typename Func>
base::expected<piper::Value, JSINativeException> InvokeWithErrorReport(
    Func func, ModuleDelegate& delegate, const std::string& module_name,
    const std::string& method_name) {
  return func();
}

}  // namespace

MethodInvoker::MethodInvoker(jobject method, std::string signature,
                             const std::string& module_name,
                             const std::string& method_name,
                             const std::shared_ptr<ModuleDelegate>& delegate)
    : signature_(signature),
      jsArgCount_(countJsArgs(signature) - 2),
      module_name_(module_name),
      method_name_(method_name),
      delegate_(delegate) {
  JNIEnv* env = base::android::AttachCurrentThread();
  method_ = env->FromReflectedMethod(method);
}

void MethodInvoker::Destroy() {
  callbackWrappers_.clear();
  promises_.clear();
}

base::expected<piper::Value, JSINativeException> MethodInvoker::Fire(
    JNIEnv* env, jobject module, jvalue* javaArguments, Runtime* rt) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "Fire");
#define PRIMITIVE_CASE(METHOD)                                                \
  {                                                                           \
    auto result = env->Call##METHOD##MethodA(module, method_, javaArguments); \
    if (reportPendingJniException()) {                                        \
      return base::unexpected(                                                \
          BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));             \
    }                                                                         \
    return piper::Value(result);                                              \
  }

#define PRIMITIVE_CASE_CASTING(METHOD, RESULT_TYPE)                           \
  {                                                                           \
    auto result = env->Call##METHOD##MethodA(module, method_, javaArguments); \
    if (reportPendingJniException()) {                                        \
      return base::unexpected(                                                \
          BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));             \
    }                                                                         \
    return piper::Value(static_cast<RESULT_TYPE>(result));                    \
  }

#define OBJECT_CASE(CLASS)                                                     \
  {                                                                            \
    lynx::base::android::ScopedLocalJavaRef<jobject> obj(                      \
        env, env->CallObjectMethodA(module, method_, javaArguments));          \
    if (reportPendingJniException()) {                                         \
      return base::unexpected(                                                 \
          BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));              \
    }                                                                          \
    static auto cls = base::android::GetGlobalClass(env, "java/lang/" #CLASS); \
    return piper::Value(doubleValue(env, cls.Get(), obj.Get()));               \
  }

  piper::Scope scope(*rt);
  /// See LynxJavaMethodWrapper.java to check types' meaning.
  char returnType = signature_.at(0);
  switch (returnType) {
    case 'v':
      env->CallVoidMethodA(module, method_, javaArguments);
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      return piper::Value::undefined();
    case 'b':
      PRIMITIVE_CASE_CASTING(Byte, double)
    case 'B':
      OBJECT_CASE(Byte)
    case 's':
      PRIMITIVE_CASE_CASTING(Short, double)
    case 'S':
      OBJECT_CASE(Short)
    case 'l': {
      // In JavaScript,  the max safe integer is 9007199254740991 and the min
      // safe integer is -9007199254740991, so when integer beyond limit, use
      // BigInt Object to define it. More information from
      // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number
      auto valueOrigin = env->CallLongMethodA(module, method_, javaArguments);
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      jlong value = static_cast<jlong>(valueOrigin);
      if (value < piper::kMinJavaScriptNumber ||
          value > piper::kMaxJavaScriptNumber) {
        auto bigint =
            piper::BigInt::createWithString(*rt, std::to_string(value));
        if (!bigint) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION("BigInt create failed."));
        }
        return Value(std::move(*bigint));
      } else {
        // cast to double
        return piper::Value(static_cast<double>(value));
      }
    }
    case 'L': {
      lynx::base::android::ScopedLocalJavaRef<jobject> obj(
          env, env->CallObjectMethodA(module, method_, javaArguments));
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      static auto cls = base::android::GetGlobalClass(env, "java/lang/Long");
      // The function signature of Long type is "J"
      static jmethodID longValueMethod =
          env->GetMethodID(cls.Get(), "longValue", "()J");
      jlong value = env->CallLongMethod(obj.Get(), longValueMethod);

      if (value < piper::kMinJavaScriptNumber ||
          value > piper::kMaxJavaScriptNumber) {
        auto bigint =
            piper::BigInt::createWithString(*rt, std::to_string(value));
        if (!bigint) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION("BigInt create failed."));
        }
        return Value(std::move(*bigint));
      } else {
        return piper::Value(static_cast<double>(value));
      }
    }
    case 'c': {
      jchar result = env->CallCharMethodA(module, method_, javaArguments);
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      lynx::base::android::ScopedLocalJavaRef<jstring> str =
          base::android::JNIConvertHelper::ConvertToJNIString(env, &result, 1);
      const char* c_value = env->GetStringUTFChars(str.Get(), JNI_FALSE);
      auto v8_value = piper::String::createFromUtf8(*rt, c_value);
      env->ReleaseStringUTFChars(str.Get(), c_value);
      return Value(v8_value);
    }
    case 'C': {
      lynx::base::android::ScopedLocalJavaRef<jobject> obj(
          env, env->CallObjectMethodA(module, method_, javaArguments));
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      static auto cls =
          base::android::GetGlobalClass(env, "java/lang/Character");
      static jmethodID charValueMethod =
          env->GetMethodID(cls.Get(), "charValue", "()C");
      jchar jc_value = env->CallCharMethod(obj.Get(), charValueMethod);
      lynx::base::android::ScopedLocalJavaRef<jstring> str =
          base::android::JNIConvertHelper::ConvertToJNIString(env, &jc_value,
                                                              1);
      const char* c_value = env->GetStringUTFChars(str.Get(), JNI_FALSE);
      auto v8_value = piper::String::createFromUtf8(*rt, c_value);
      env->ReleaseStringUTFChars(str.Get(), c_value);
      return Value(v8_value);
    }
    case 'i':
      PRIMITIVE_CASE(Int)
    case 'I':
      OBJECT_CASE(Integer)
    case 'd':
      PRIMITIVE_CASE(Double)
    case 'D':
      OBJECT_CASE(Double)
    case 'f':
      PRIMITIVE_CASE_CASTING(Float, double)
    case 'F':
      OBJECT_CASE(Float)
    case 'z':
      PRIMITIVE_CASE_CASTING(Boolean, bool)
    case 'Z': {
      lynx::base::android::ScopedLocalJavaRef<jobject> obj(
          env, env->CallObjectMethodA(module, method_, javaArguments));
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      static auto cls = base::android::GetGlobalClass(env, "java/lang/Boolean");
      static jmethodID methodID =
          env->GetMethodID(cls.Get(), "booleanValue", "()Z");
      jboolean v = env->CallBooleanMethod(obj.Get(), methodID);
      return piper::Value(static_cast<bool>(v));
    }
    case 'T': {
      lynx::base::android::ScopedLocalJavaRef<jstring> str(
          env, static_cast<jstring>(
                   env->CallObjectMethodA(module, method_, javaArguments)));
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      return Value(piper::String::createFromUtf8(
          *rt, lynx::base::android::JNIConvertHelper::ConvertToString(
                   env, str.Get())));
    }
    case 'M': {
      lynx::base::android::ScopedLocalJavaRef<jobject> obj(
          env, env->CallObjectMethodA(module, method_, javaArguments));
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      auto map_opt = jsObjectFromJavaOnlyMap(env, obj.Get(), rt);
      if (!map_opt) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("jsObjectFromJavaOnlyMap failed."));
      }
      return Value(std::move(*map_opt));
    }
    case 'A': {
      lynx::base::android::ScopedLocalJavaRef<jobject> arr(
          env, env->CallObjectMethodA(module, method_, javaArguments));
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      auto array_opt = jsArrayFromJavaOnlyArray(env, arr.Get(), rt);
      if (!array_opt) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("jsArrayFromJavaOnlyArray failed."));
      }
      return Value(std::move(*array_opt));
    }
    case 'a': {
      lynx::base::android::ScopedLocalJavaRef<jbyteArray> byte_array(
          env, static_cast<jbyteArray>(
                   env->CallObjectMethodA(module, method_, javaArguments)));
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      return Value(lynx::base::android::JNIHelper::ConvertToJSIArrayBuffer(
          env, rt, byte_array.Get()));
    }
    case 'J': {
      lynx::base::android::ScopedLocalJavaRef<jobject> json_data(
          env, env->CallObjectMethodA(module, method_, javaArguments));
      if (reportPendingJniException()) {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("JNI Exception occurred."));
      }
      // TODO(liyanbo): change return type to base::expected.
      auto opt_value = base::android::PiperData::jsObjectFromPiperData(
          env, rt, std::move(json_data));
      if (opt_value) {
        return base::ok(std::move(*opt_value));
      } else {
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("jsObjectFromPiperData failed."));
      }
    }
    default:
      LOGF("Unknown return type: " << returnType);
      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
          "Unknown return type: " + std::to_string(returnType)));
  }
}

base::expected<piper::Value, JSINativeException> MethodInvoker::Invoke(
    jobject module, Runtime* rt, const piper::Value* jsArgs, size_t count,
    const piper::NativeModuleInfoCollectorPtr& timing_collector) {
  return InvokeImpl(module, rt, jsArgs, count, timing_collector);
}

base::expected<piper::Value, JSINativeException> MethodInvoker::InvokeImpl(
    jobject module, Runtime* rt, const piper::Value* jsArgs, size_t count,
    const piper::NativeModuleInfoCollectorPtr& timing_collector) {
  piper::Scope piper_scope(*rt);
  std::string first_arg_str;
  if (count > 0 && jsArgs && jsArgs[0].isString()) {
    first_arg_str = jsArgs[0].getString(*rt).utf8(*rt);
  }
#if ENABLE_TESTBENCH_RECORDER
  StartRecordFunction();
#endif
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "CallJSB",
              [&, self = shared_from_this()](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("module_name", module_name_);
                ctx.event()->add_debug_annotations("method_name", method_name_);
                ctx.event()->add_debug_annotations("first_arg", first_arg_str);
                if (count != jsArgCount_) {
                  return;
                }
                for (size_t param_idx = 1; param_idx < count; param_idx++) {
                  if (!jsArgs[param_idx].isObject()) {
                    return;
                  }
                  std::optional<Value> json_arg =
                      jsArgs[param_idx].toJsonString(*rt);
                  if (!json_arg->isString()) {
                    return;
                  }
                  std::string str_arg = json_arg->asString(*rt)->utf8(*rt);
                  // avoid write too much content to trace
                  if (str_arg.length() < 4096) {
                    ctx.event()->add_debug_annotations(
                        "arg" + std::to_string(param_idx), str_arg);
                  }
                }
              });
  if (count != jsArgCount_) {
    delegate_->OnMethodInvoked(module_name_, method_name_,
                               error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_NUM);
    auto error_message = LynxModuleUtils::GenerateErrorMessage(
        module_name_, method_name_,
        first_arg_str +
            LynxModuleUtils::ExpectedButGotError(jsArgCount_, count));
    delegate_->OnErrorOccurred(base::LynxError{
        error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_NUM, error_message,
        "Please check the arguments.", base::LynxErrorLevel::Error});
    if (timing_collector != nullptr) {
      timing_collector->OnErrorOccurred(
          NativeModuleStatusCode::PARAMETER_ERROR);
    }
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(error_message));
  }

  if (signature_[signature_.length() - 1] == 'P') {
    LOGI("LynxModule, MethodInvoker::InvokeImpl, got a |PROMISE| : ("
         << module_name_ << " " << method_name_ << ") will fire " << this);
    // Got a promise.
    auto Promise = rt->global().getPropertyAsFunction(*rt, "Promise");
    if (!Promise) {
      return base::unexpected(
          BUILD_JSI_NATIVE_EXCEPTION("Can't find Promise."));
    }

    tasm::report::FeatureCounter::Instance()->Count(
        tasm::report::LynxFeature::CPP_USE_NATIVE_PROMISE);

    auto executor_function_impl =
        [self = shared_from_this(), this, jsArgs, module](
            Runtime& rt, const Value& thisVal, const Value* args,
            size_t count) -> base::expected<piper::Value, JSINativeException> {
      uint64_t start_time = lynx::base::CurrentTimeMilliseconds();
      piper::Scope piper_scope(rt);
      // The following three exceptions should never be thrown unless JS wrong
      if (count != 2) {
        LOGE("Promise arg count must be 2.");
        return base::unexpected(
            BUILD_JSI_NATIVE_EXCEPTION("Promise arg count must be 2."));
      }

      if (!(args)->isObject() || !(args)->getObject(rt).isFunction(rt)) {
        LOGE("Promise parameter should be two JS function.");
        return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
            "Promise parameter should be two JS function."));
      }

      if (!(args + 1)->isObject() ||
          !(args + 1)->getObject(rt).isFunction(rt)) {
        LOGE("Promise parameter should be two JS function.");
        return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
            "Promise parameter should be two JS function."));
      }

      Function resolve = (args)->getObject(rt).getFunction(rt);
      Function reject = (args + 1)->getObject(rt).getFunction(rt);
      int64_t resolve_callback_id =
          delegate_->RegisterJSCallbackFunction(std::move(resolve));
      int64_t reject_callback_id =
          delegate_->RegisterJSCallbackFunction(std::move(reject));
      if (resolve_callback_id == ModuleCallback::kInvalidCallbackId ||
          reject_callback_id == ModuleCallback::kInvalidCallbackId) {
        LOGW(
            "LynxModule, MethodInvoker::InvokeImpl, create "
            "promise failed, LynxRuntime has destroyed");
        return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
            "LynxModule, MethodInvoker::InvokeImpl, create "
            "promise failed, LynxRuntime has destroyed"));
      }
      LOGV("LynxModule, MethodInvoker::InvokeImpl, |PROMISE| : ("
           << module_name_ << " " << method_name_ << ")"
           << " resolve callback id: " << resolve_callback_id);

      ModuleCallbackAndroid::CallbackPair resolve_callback_pair =
          ModuleCallbackAndroid::createCallbackImpl(resolve_callback_id,
                                                    shared_from_this());
      ModuleCallbackAndroid::CallbackPair reject_callback_pair =
          ModuleCallbackAndroid::createCallbackImpl(reject_callback_id,
                                                    shared_from_this());
      std::shared_ptr<LynxPromiseImpl> promise =
          std::make_shared<LynxPromiseImpl>(resolve_callback_pair,
                                            reject_callback_pair);
      PtrContainerMap ptr_container_map;
      ptr_container_map.jobject_container_.push_back(
          std::move(resolve_callback_pair.second));
      ptr_container_map.jobject_container_.push_back(
          std::move(reject_callback_pair.second));
      resolve_callback_pair.first->promise = promise;
      reject_callback_pair.first->promise = promise;
      promises_.insert(promise);

      JNIEnv* env = base::android::AttachCurrentThread();
      auto argCount = signature_.size() - 2;

      base::android::JniLocalScope scope(env);
      jvalue java_arguments[argCount];

      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, "JSValueToJNIValue");
      for (size_t i = 0; i < argCount - 1; i++) {
        char type = signature_[i + 2];
        auto ret = extractJSValue(&rt, jsArgs + i, type, ptr_container_map, i,
                                  start_time, nullptr);
        if (ret.has_value()) {
          java_arguments[i] = std::move(ret.value());
        } else {
          ADD_STACK(ret.error());
          return base::unexpected(std::move(ret.error()));
        }
      }
      TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);
      jvalue v;
      v.l = promise->GetJniObject();
      java_arguments[argCount - 1] = v;
      auto ret = Fire(env, module, java_arguments, &rt);
      if (!ret.has_value()) {
        return ret;
      }
      LOGI("LynxModule, MethodInvoker::InvokeImpl, |PROMISE| : ("
           << module_name_ << " " << method_name_ << ") did fire " << this);
      delegate_->OnMethodInvoked(module_name_, method_name_, error::E_SUCCESS);
      return ret;
    };

    auto executor_function = [executor_function_impl =
                                  std::move(executor_function_impl),
                              self = shared_from_this(), this](auto&&... args) {
      return InvokeWithErrorReport(
          [executor_function_impl = std::move(executor_function_impl),
           &args...]() {
            return executor_function_impl(
                std::forward<decltype(args)>(args)...);
          },
          *delegate_, module_name_, method_name_);
    };

    piper::Function fn = piper::Function::createFromHostFunction(
        *rt, piper::PropNameID::forAscii(*rt, "fn"), 2,
        std::move(executor_function));

    auto ret = Promise->callAsConstructor(*rt, fn);
    if (!ret) {
      if (timing_collector != nullptr) {
        timing_collector->OnErrorOccurred(NativeModuleStatusCode::FAILURE);
      }
      return base::unexpected(
          BUILD_JSI_NATIVE_EXCEPTION("Promise call constructor failed."));
    }
    return std::move(*ret);
  }

  LOGI("LynxModule, MethodInvoker::InvokeImpl, method: ("
       << module_name_ << "." << method_name_ << "." << first_arg_str
       << ") will fire " << this);
  uint64_t start_time = lynx::base::CurrentTimeMilliseconds();
  JNIEnv* env = base::android::AttachCurrentThread();
  auto argCount = signature_.size() - 2;

  uint64_t convert_params_start = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_func_convert_params_start",
      [&first_arg_str, start_time](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg", first_arg_str);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(start_time));
      });
  base::android::JniLocalScope scope(env);
  jvalue java_arguments[argCount];
  PtrContainerMap ptr_container_map;

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, "JSValueToJNIValue");
  for (size_t i = 0; i < count; i++) {
    char type = signature_[i + 2];
    auto ret = extractJSValue(rt, jsArgs + i, type, ptr_container_map, i,
                              start_time, timing_collector);
    if (ret.has_value()) {
      java_arguments[i] = std::move(ret.value());
    } else {
      ADD_STACK(ret.error());
      return base::unexpected(std::move(ret.error()));
    }
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);
  if (timing_collector != nullptr) {
    timing_collector->EndFuncParamsConvert(convert_params_start);
  }
  uint64_t invoke_facade_method_start = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_func_platform_method_start",
      [&first_arg_str,
       invoke_facade_method_start](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg", first_arg_str);
        ctx.event()->add_debug_annotations(
            "timestamp", std::to_string(invoke_facade_method_start));
      });
  auto ret = Fire(env, module, java_arguments, rt);
  if (timing_collector != nullptr) {
    timing_collector->EndPlatformMethodInvoke(invoke_facade_method_start);
  }
  if (!ret.has_value()) {
    return ret;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "OnMethodInvoked");
  LOGI("LynxModule, MethodInvoker::InvokeImpl, method: ("
       << module_name_ << "." << method_name_ << "." << first_arg_str
       << ") did fire " << this);
  delegate_->OnMethodInvoked(module_name_, method_name_, error::E_SUCCESS);
#if ENABLE_TESTBENCH_RECORDER
  EndRecordFunction(count, jsArgs, rt, ret.value());
#endif
  return ret;
}

void MethodInvoker::InvokeCallback(
    const std::shared_ptr<ModuleCallbackAndroid>& callback) {
  LOGV("LynxModule, MethodInvoker::InvokeCallback, put callback: "
       << " id: "
       << (callback ? std::to_string(callback->callback_id())
                    : std::string{"(no id due to callback is nullptr)"})
       << " to JSThread");
  if (callback != nullptr && callback->timing_collector_ != nullptr) {
    callback->timing_collector_->CallbackThreadSwitchStart();
  }
  delegate_->RunOnJSThread([callback, ref = shared_from_this()]() {
    ref->invokeCallbackInJSThread(callback);
  });
}

void MethodInvoker::invokeCallbackInJSThread(
    const std::shared_ptr<ModuleCallbackAndroid>& callback) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY_JSB, "InvokeCallback",
      [&](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_terminating_flow_ids(callback->CallbackFlowId());
        ctx.event()->add_debug_annotations("module_name", module_name_);
        ctx.event()->add_debug_annotations("method_name", method_name_);
        ctx.event()->add_debug_annotations("first_arg", callback->FirstArg());
        uint64_t cost = -1;
        uint64_t start = callback->StartTimeMS();
        if (start > 0) {
          cost = lynx::base::CurrentTimeMilliseconds() - start;
        }
        ctx.event()->add_debug_annotations("startTimestamp",
                                           std::to_string(start));
        ctx.event()->add_debug_annotations("timeCost", std::to_string(cost));
      });
  std::shared_ptr<LynxPromiseImpl> promise = callback->promise.lock();
  if (promise) {
    if (promise->GetReject() == callback) {
      delegate_->CallJSCallback(promise->GetReject(),
                                promise->GetResolve()->callback_id());
    } else {
      delegate_->CallJSCallback(promise->GetResolve(),
                                promise->GetReject()->callback_id());
    }
    promises_.erase(promise);
  } else if (callbackWrappers_.erase(callback)) {
#if ENABLE_TESTBENCH_RECORDER
    callback->SetRecordID(record_id_);
    callback->SetModuleName(module_name_);
    callback->SetMethodName(method_name_);
#endif
    delegate_->CallJSCallback(callback);
  }
}

#if ENABLE_TESTBENCH_RECORDER
void MethodInvoker::StartRecordFunction() {
  callbacks_stack_.push(std::vector<int64_t>());
}

void MethodInvoker::EndRecordFunction(size_t count, const piper::Value* js_args,
                                      Runtime* rt, piper::Value& res) {
  if (!callbacks_stack_.empty()) {
    auto top_vector = callbacks_stack_.top();
    uint32_t callbacks_count = static_cast<uint32_t>(top_vector.size());
    std::string module_name = module_name_;
    tasm::recorder::NativeModuleRecorder::GetInstance().RecordFunctionCall(
        module_name.c_str(), method_name_.c_str(), static_cast<uint32_t>(count),
        js_args, top_vector.data(), callbacks_count, res, rt, record_id_);
    callbacks_stack_.pop();
  }
}
#endif

}  // namespace piper
}  // namespace lynx
