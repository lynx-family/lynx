// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/common/utils.h"

#include <memory>
#include <utility>

#if defined(OS_ANDROID)
#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "core/base/android/piper_data.h"
#include "core/build/gen/JavaOnlyArray_jni.h"
#include "core/runtime/bindings/jsi/modules/android/platform_jsi/lynx_platform_jsi_object_android.h"
#endif  // OS_ANDROID

#include "core/base/js_constants.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/bindings/jsi/console.h"
#include "core/runtime/common/jsi_object_wrapper.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/byte_array.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace piper {

std::optional<Value> valueFromLepus(
    Runtime& runtime, const lepus::Value& data,
    JSIObjectWrapperManager* jsi_object_wrapper_manager) {
  piper::Scope scope(runtime);
  if (data.IsJSValue()) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        std::string("Find JSValue in valueFromLepus!!")));
    LOGE("!!! value form lepus: is JSValue");
    return Value::null();
  }
  GCPauseSuppressionMode mode(&runtime);
  switch (data.Type()) {
    case lepus::ValueType::Value_Nil:
      return Value::null();
    case lepus::ValueType::Value_Undefined:
      return Value::undefined();
    case lepus::ValueType::Value_Double:
    case lepus::ValueType::Value_UInt32:
    case lepus::ValueType::Value_Int32:
      return Value(data.Number());
    case lepus::ValueType::Value_Int64: {
      int64_t value = data.Int64();
      // In JavaScript,  the max safe integer is 9007199254740991 and the min
      // safe integer is -9007199254740991, so when integer beyond limit, use
      // BigInt Object to define it. More information from
      // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number
      if (value < kMinJavaScriptNumber || value > kMaxJavaScriptNumber) {
        auto bigint = BigInt::createWithString(runtime, std::to_string(value));
        return bigint ? std::optional<Value>(Value(*bigint))
                      : std::optional<Value>();
      } else {
        return Value(data.Number());
      }
    }
    case lepus::ValueType::Value_UInt64: {
      if (data.UInt64() > kMaxJavaScriptNumber) {
        auto bigint =
            BigInt::createWithString(runtime, std::to_string(data.UInt64()));
        return bigint ? std::optional<Value>(Value(*bigint))
                      : std::optional<Value>();
      } else {
        return Value(data.Number());
      }
    }
    case lepus::ValueType::Value_String: {
      const std::string& origin = data.StdString();
      piper::String result = String::createFromUtf8(runtime, origin);
      return Value(result);
    }
    case lepus::ValueType::Value_Array: {
      lepus::CArray* array = data.Array().get();
      auto ret = Array::createWithLength(runtime, array->size());
      if (!ret) {
        return std::optional<Value>();
      }
      for (size_t i = 0; i < array->size(); ++i) {
        auto element =
            valueFromLepus(runtime, array->get(i), jsi_object_wrapper_manager);
        if (!element) {
          return std::optional<Value>();
        }
        if (!((*ret).setValueAtIndex(runtime, i, std::move(*element)))) {
          return std::optional<Value>();
        }
      }
      piper::Value jsArray(std::move(*ret));
      return jsArray;
    }
    case lepus::Value_Bool:
      return Value(data.Bool());
    case lepus::Value_Table: {
      lepus::Dictionary* dict = data.Table().get();
      Object ret(runtime);
      for (auto& iter : *dict) {
        auto element =
            valueFromLepus(runtime, iter.second, jsi_object_wrapper_manager);
        if (!element) {
          return std::optional<Value>();
        }
        if (runtime.getGCFlag()) {
          if (!runtime.setPropertyValueGC(ret, iter.first.c_str(),
                                          detail::toValue(runtime, *element)))
            return std::optional<Value>();
        } else if (!ret.setProperty(runtime, iter.first.c_str(),
                                    std::move(*element))) {
          return std::optional<Value>();
        }
      }
      return Value(std::move(ret));
    }
    case lepus::Value_JSObject: {
      if (jsi_object_wrapper_manager) {
        piper::Value func =
            jsi_object_wrapper_manager->GetJSIObjectByIDOnJSThread(
                runtime, data.LEPUSObject()->JSIObjectID());
        return func;
      }

      return Value::null();
    }
    case lepus::ValueType::Value_ByteArray: {
      auto byte_array = data.ByteArray();
      size_t length = byte_array->GetLength();
      std::unique_ptr<const uint8_t[]> buffer = byte_array->MovePtr();
      return Value(piper::ArrayBuffer(runtime, std::move(buffer), length));
    }
    case lepus::ValueType::Value_Closure:
    case lepus::ValueType::Value_CFunction:
    case lepus::ValueType::Value_CPointer:
    case lepus::ValueType::Value_RefCounted:
    case lepus::ValueType::Value_NaN:
    case lepus::ValueType::Value_CDate:
    case lepus::ValueType::Value_RegExp:
    case lepus::ValueType::Value_PrimJsValue:
    case lepus::ValueType::Value_TypeCount:
      break;
  }
  return Value::null();
}

std::optional<Array> arrayFromLepus(Runtime& runtime,
                                    const lepus::CArray& array) {
  piper::Scope scope(runtime);
  auto ret = Array::createWithLength(runtime, array.size());
  if (!ret) {
    return std::optional<Array>();
  }
  for (size_t i = 0; i < array.size(); ++i) {
    auto element = valueFromLepus(runtime, array.get(i), nullptr);
    if (!element) {
      return std::optional<Array>();
    }
    if (!((*ret).setValueAtIndex(runtime, i, std::move(*element)))) {
      return std::optional<Array>();
    }
  }
  return ret;
}

// if 'jsi_object_wrapper_manager' is null, don't parse js function
std::optional<lepus_value> ParseJSValue(
    piper::Runtime& runtime, const piper::Value& value,
    JSIObjectWrapperManager* jsi_object_wrapper_manager,
    const std::string& jsi_object_group_id, const std::string& targetSDKVersion,
    JSValueCircularArray& pre_object_vector, int depth) {
  piper::Scope scope(runtime);
  if (value.isNull()) {
    return lepus::Value();
  } else if (value.isUndefined()) {
    lepus::Value result;
    result.SetUndefined();
    return result;
  } else if (value.isBool()) {
    return lepus::Value(value.getBool());
  } else if (value.isNumber()) {
    return lepus::Value(value.getNumber());
  } else if (value.isString()) {
    return lepus::Value(value.getString(runtime).utf8(runtime));
  } else if (value.isSymbol()) {
    // TODO(liyanbo): support symbol type.
    return std::nullopt;
  } else {
    piper::Object obj = value.getObject(runtime);
    if (CheckIsCircularJSObjectIfNecessaryAndReportError(
            runtime, obj, pre_object_vector, depth, "ParseJSValue!")) {
      return std::optional<lepus_value>();
    }
    // As Object is Movable, not copyable, do not push the Object you will use
    // later to vector! You need clone a new one.
    ScopedJSObjectPushPopHelper scoped_push_pop_helper(
        pre_object_vector, value.getObject(runtime));
    if (obj.isArray(runtime)) {
      piper::Array array = obj.getArray(runtime);
      auto size_opt = array.size(runtime);
      if (!size_opt) {
        return std::optional<lepus_value>();
      }
      auto lepus_array = lepus::CArray::Create();
      lepus_array->reserve(*size_opt);
      for (size_t i = 0; i < *size_opt; ++i) {
        auto item_opt = array.getValueAtIndex(runtime, i);
        if (!item_opt) {
          return std::optional<lepus_value>();
        }
        auto value_opt = ParseJSValue(
            runtime, *item_opt, jsi_object_wrapper_manager, jsi_object_group_id,
            targetSDKVersion, pre_object_vector, depth + 1);
        if (!value_opt) {
          LOGE("Error happened in ParseJSValue, array index: " << i);
          return std::optional<lepus_value>();
        }
        lepus_array.get()->emplace_back(std::move(*value_opt));
      }
      return lepus::Value(std::move(lepus_array));
    } else if (obj.isFunction(runtime)) {
      if (jsi_object_wrapper_manager) {
        return lepus_value(lepus::LEPUSObject::Create(
            jsi_object_wrapper_manager->CreateJSIObjectWrapperOnJSThread(
                runtime, std::move(obj), jsi_object_group_id)));
      } else {
        // do nothing
      }
    } else {
      if (obj.hasProperty(runtime, BIG_INT_VAL)) {
        auto value_long_opt = obj.getProperty(runtime, BIG_INT_VAL);
        if (!value_long_opt) {
          return std::optional<lepus_value>();
        }
        if (value_long_opt->isString()) {
          auto str = value_long_opt->toString(runtime);
          if (!str) {
            return std::optional<lepus_value>();
          }
          const std::string val_str = str->utf8(runtime);
          // TODO(liyabo): Fix the problem of loss of accuracy due to forced
          // transfer
          return lepus::Value(
              static_cast<int64_t>(std::strtoll(val_str.c_str(), nullptr, 0)));
        }
      }
      auto lepus_map = lepus::Dictionary::Create();
      auto names = obj.getPropertyNames(runtime);
      if (!names) {
        return std::optional<lepus_value>();
      }
      auto size = (*names).size(runtime);
      if (!size) {
        return std::optional<lepus_value>();
      }
      for (size_t i = 0; i < *size; ++i) {
        auto item = (*names).getValueAtIndex(runtime, i);
        if (!item) {
          return std::optional<lepus_value>();
        }
        // TODO(liyanbo): support Symbol type when getProperty.
        if (!item->isString()) {
          continue;
        }
        piper::String name = item->getString(runtime);
        auto prop = obj.getProperty(runtime, name);
        if (!prop) {
          return std::optional<lepus_value>();
        }
        auto key = base::String(name.utf8(runtime));
        // lynx sdk < 2.3, ignore undefined, compatible with old lynx project
        if (prop->isUndefined() && !lynx::tasm::Config::IsHigherOrEqual(
                                       targetSDKVersion, LYNX_VERSION_2_3)) {
          continue;
        }
        auto value_opt = ParseJSValue(
            runtime, *prop, jsi_object_wrapper_manager, jsi_object_group_id,
            targetSDKVersion, pre_object_vector, depth + 1);
        if (!value_opt) {
          LOGE("Error happened in ParseJSValue, key: " << key.str());
          return std::optional<lepus_value>();
        }
        lepus_map.get()->SetValue(key, std::move(*value_opt));
      }
      return lepus::Value(std::move(lepus_map));
    }
  }
  return lepus_value();
}

bool IsCircularJSObject(Runtime& runtime, const Object& object,
                        const JSValueCircularArray& pre_object_vector) {
  for (auto& pre_object : pre_object_vector) {
    if (piper::Object::strictEquals(runtime, pre_object, object)) {
      return true;
    }
  }
  return false;
}

bool CheckIsCircularJSObjectIfNecessaryAndReportError(
    Runtime& runtime, const Object& object,
    const JSValueCircularArray& pre_object_vector, int depth,
    const char* message) {
  if ((runtime.IsEnableCircularDataCheck() ||
       runtime.IsCircularDataCheckUnset()) &&
      IsCircularJSObject(runtime, object, pre_object_vector)) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        std::string("Find circular JS data in ") + message));
    LOGE("Find circular JS data in " << message);
    return true;
  }
  return false;
}

bool ConvertPiperValueToStringVector(Runtime& rt, const piper::Value& input,
                                     std::vector<std::string>& result) {
  if (!input.isObject()) {
    return false;
  }

  auto input_obj = input.getObject(rt);
  if (!input_obj.isArray(rt)) {
    return false;
  }

  auto input_array = input_obj.getArray(rt);
  auto size = input_array.size(rt);
  if (!size) {
    return false;
  }

  for (size_t i = 0; i < size; ++i) {
    auto element = input_array.getValueAtIndex(rt, i);
    if (!element || !element->isString()) {
      return false;
    }
    result.emplace_back(element->getString(rt).utf8(rt));
  }

  return true;
}

#ifdef OS_ANDROID
bool JSBUtilsRegisterJNI(JNIEnv* env) { return RegisterNativesImpl(env); }

void PushByteArrayToJavaArray(piper::Runtime* rt,
                              const piper::ArrayBuffer& array_buffer,
                              base::android::JavaOnlyArray* jarray) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jbyteArray> jni_byte_array =
      base::android::JNIHelper::ConvertToJNIByteArray(env, rt, array_buffer);
  Java_JavaOnlyArray_pushByteArray(env, jarray->jni_object(),
                                   jni_byte_array.Get());
}

std::optional<piper::Array> jsArrayFromJavaOnlyArray(JNIEnv* env, jobject array,
                                                     piper::Runtime* rt) {
  piper::Scope scope(*rt);
  int size = Java_JavaOnlyArray_size(env, array);
  auto result_opt =
      piper::Array::createWithLength(*rt, static_cast<size_t>(size));
  if (!result_opt) {
    return std::optional<piper::Array>();
  }
  piper::Array result(std::move(result_opt.value()));
  for (size_t i = 0; i < static_cast<size_t>(size); i++) {
    auto type = static_cast<lynx::base::android::ReadableType>(
        Java_JavaOnlyArray_getTypeIndex(env, array, i));

    bool is_successful = true;
    switch (type) {
      case lynx::base::android::ReadableType::Null:
        is_successful = result.setValueAtIndex(*rt, i, piper::Value::null());

        break;
      case lynx::base::android::ReadableType::String: {
        std::string stdString;
        {
          auto jStr = Java_JavaOnlyArray_getString(env, array, i);
          if (jStr.Get()) {
            const char* str = env->GetStringUTFChars(jStr.Get(), JNI_FALSE);
            stdString = str;
            env->ReleaseStringUTFChars(jStr.Get(), str);
          }
        }
        is_successful = result.setValueAtIndex(
            *rt, i, piper::String::createFromUtf8(*rt, stdString));

      } break;
      case lynx::base::android::ReadableType::Boolean: {
        jboolean b = Java_JavaOnlyArray_getBoolean(env, array, i);
        is_successful =
            result.setValueAtIndex(*rt, i, piper::Value(static_cast<bool>(b)));

        break;
      }
      case lynx::base::android::ReadableType::Int: {
        jint n = Java_JavaOnlyArray_getInt(env, array, i);
        is_successful = result.setValueAtIndex(
            *rt, i, piper::Value(static_cast<double>(n)));

        break;
      }
      case lynx::base::android::ReadableType::Long: {
        // In JavaScript,  the max safe integer is 9007199254740991 and the min
        // safe integer is -9007199254740991, so when integer beyond limit, use
        // BigInt Object to define it. More information from
        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number
        jlong n = Java_JavaOnlyArray_getLong(env, array, i);
        if (n < piper::kMinJavaScriptNumber ||
            n > piper::kMaxJavaScriptNumber) {
          auto bigint = piper::BigInt::createWithString(*rt, std::to_string(n));
          if (!bigint) {
            return std::optional<piper::Array>();
          }
          is_successful = result.setValueAtIndex(*rt, i, std::move(*bigint));
        } else {
          is_successful = result.setValueAtIndex(
              *rt, i, piper::Value(static_cast<double>(n)));
        }
        break;
      }
      case lynx::base::android::ReadableType::Number: {
        jdouble d = Java_JavaOnlyArray_getDouble(env, array, i);
        is_successful = result.setValueAtIndex(
            *rt, i, piper::Value(static_cast<double>(d)));
        break;
      }
      case lynx::base::android::ReadableType::Array: {
        auto arr = Java_JavaOnlyArray_getAt(env, array, i);
        base::android::ScopedGlobalJavaRef<jobject> arr_global(env, arr.Get());
        arr.Reset();
        auto array_opt = jsArrayFromJavaOnlyArray(env, arr_global.Get(), rt);
        if (!array_opt) {
          return std::optional<piper::Array>();
        }
        is_successful = result.setValueAtIndex(*rt, i, std::move(*array_opt));

      } break;
      case lynx::base::android::ReadableType::Map: {
        auto map = Java_JavaOnlyArray_getMap(env, array, i);
        base::android::ScopedGlobalJavaRef<jobject> arr_global(env, map.Get());
        map.Reset();
        auto map_opt = jsObjectFromJavaOnlyMap(env, arr_global.Get(), rt);
        if (!map_opt) {
          return std::optional<piper::Array>();
        }
        is_successful = result.setValueAtIndex(*rt, i, std::move(*map_opt));
      } break;
      case lynx::base::android::ReadableType::ByteArray: {
        base::android::ScopedLocalJavaRef<jbyteArray> byte_array =
            Java_JavaOnlyArray_getByteArray(env, array, i);
        is_successful = result.setValueAtIndex(
            *rt, i,
            base::android::JNIHelper::ConvertToJSIArrayBuffer(
                env, rt, byte_array.Get()));
        break;
      }
      case lynx::base::android::ReadableType::PiperData: {
        base::android::ScopedLocalJavaRef<jobject> piper_data =
            Java_JavaOnlyArray_getPiperData(env, array, i);
        auto js_object_opt = base::android::PiperData::jsObjectFromPiperData(
            env, rt, std::move(piper_data));
        if (!js_object_opt) {
          return std::optional<piper::Array>();
        }
        result.setValueAtIndex(*rt, i, std::move(*js_object_opt));
        break;
      }
      case lynx::base::android::ReadableType::LynxObject: {
        base::android::ScopedLocalJavaRef<jobject> lynx_object =
            Java_JavaOnlyArray_getObject(env, array, i);
        // create a lynx object module
        auto object_module = lynx::piper::LynxPlatformJSIObjectAndroid::Create(
            env, lynx_object.Get());
        auto host_object =
            piper::Object::createFromHostObject(*rt, std::move(object_module));
        result.setValueAtIndex(*rt, i, std::move(host_object));
        break;
      }
      default:
        break;
    }
    if (!is_successful) {
      return std::optional<piper::Array>();
    }
  }

  return result;
}
#endif  // OS_ANDROID

}  // namespace piper
}  // namespace lynx
