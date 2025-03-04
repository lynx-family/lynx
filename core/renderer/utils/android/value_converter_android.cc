// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/android/value_converter_android.h"

#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "core/base/android/jni_helper.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {
namespace android {

void ValueConverterAndroid::PushKeyAndValueToJavaOnlyMap(
    base::android::JavaOnlyMap& jni_map, const char* key,
    const lepus::Value& value) {
  if (value.IsNil()) {
    jni_map.PushNull(key);
  } else if (value.IsBool()) {
    jni_map.PushBoolean(key, value.Bool());
  } else if (value.IsNumber()) {
    jni_map.PushDouble(key, value.Number());
  } else if (value.IsString()) {
    jni_map.PushString(key, value.StdString());
  } else if (value.IsArrayOrJSArray()) {
    auto array = ConvertLepusToJavaOnlyArray(value);
    jni_map.PushArray(key, &array);
  } else if (value.IsObject()) {
    auto map = ConvertLepusToJavaOnlyMap(value);
    jni_map.PushMap(key, &map);
  }
}

void ValueConverterAndroid::PushValueToJavaOnlyArray(
    base::android::JavaOnlyArray& jni_ary, const lepus::Value& value) {
  if (value.IsNil()) {
    jni_ary.PushNull();
  } else if (value.IsBool()) {
    jni_ary.PushBoolean(value.Bool());
  } else if (value.IsNumber()) {
    jni_ary.PushDouble(value.Number());
  } else if (value.IsString()) {
    jni_ary.PushString(value.StdString());
  } else if (value.IsArrayOrJSArray()) {
    auto array = ConvertLepusToJavaOnlyArray(value);
    jni_ary.PushArray(&array);
  } else if (value.IsObject()) {
    auto map = ConvertLepusToJavaOnlyMap(value);
    jni_ary.PushMap(&map);
  }
}

base::android::JavaOnlyMap ValueConverterAndroid::ConvertLepusToJavaOnlyMap(
    const lepus::Value& value) {
  base::android::JavaOnlyMap jni_map;
  if (!value.IsObject()) {
    LOGE(
        "ConvertLepusToJavaOnlyMap failed since the lepus value is not a "
        "table.");
    return jni_map;
  }

  tasm::ForEachLepusValue(
      value, [&jni_map](const lepus::Value key, const lepus::Value& value) {
        PushKeyAndValueToJavaOnlyMap(jni_map, key.CString(), value);
      });

  return jni_map;
}

void ValueConverterAndroid::PushKeyAndValueToJavaOnlyMapForTiming(
    base::android::JavaOnlyMap& jni_map, const char* key,
    const lepus::Value& value) {
  if (value.IsNil()) {
    jni_map.PushNull(key);
  } else if (value.IsBool()) {
    jni_map.PushBoolean(key, value.Bool());
  } else if (value.IsInt32()) {
    jni_map.PushInt(key, value.Int32());
  } else if (value.IsUInt32()) {
    jni_map.PushInt(key, value.UInt32());
  } else if (value.IsInt64()) {
    jni_map.PushInt64(key, value.Int64());
  } else if (value.IsUInt64()) {
    jni_map.PushInt64(key, value.UInt64());
  } else if (value.IsNumber()) {
    jni_map.PushDouble(key, value.Number());
  } else if (value.IsString()) {
    jni_map.PushString(key, value.StdString());
  } else if (value.IsObject()) {
    auto map = ConvertLepusToJavaOnlyMapForTiming(value);
    jni_map.PushMap(key, &map);
  }
}

base::android::JavaOnlyMap
ValueConverterAndroid::ConvertLepusToJavaOnlyMapForTiming(
    const lepus::Value& value) {
  base::android::JavaOnlyMap jni_map;
  if (!value.IsObject()) {
    LOGE(
        "ConvertLepusToJavaOnlyMap failed since the lepus value is not a "
        "table.");
    return jni_map;
  }

  tasm::ForEachLepusValue(
      value, [&jni_map](const lepus::Value key, const lepus::Value& value) {
        PushKeyAndValueToJavaOnlyMapForTiming(jni_map, key.CString(), value);
      });

  return jni_map;
}

base::android::JavaOnlyArray ValueConverterAndroid::ConvertLepusToJavaOnlyArray(
    const lepus::Value& value) {
  base::android::JavaOnlyArray jni_ary;
  if (!value.IsArrayOrJSArray()) {
    LOGE(
        "ConvertLepusToJavaOnlyMap failed since the lepus value is not a "
        "table.");
    return jni_ary;
  }

  tasm::ForEachLepusValue(
      value, [&jni_ary](const lepus::Value key, const lepus::Value& value) {
        PushValueToJavaOnlyArray(jni_ary, value);
      });

  return jni_ary;
}

lepus::Value ValueConverterAndroid::ConvertJavaOnlyArrayToLepus(JNIEnv* env,
                                                                jobject array) {
  size_t size = base::android::JavaOnlyArray::JavaOnlyArrayGetSize(env, array);
  auto ary = lepus::CArray::Create();
  for (size_t i = 0; i < size; i++) {
    lynx::base::android::ReadableType type =
        static_cast<lynx::base::android::ReadableType>(
            base::android::JavaOnlyArray::JavaOnlyArrayGetTypeAtIndex(
                env, array, i));
    switch (type) {
      case lynx::base::android::ReadableType::Null:
        ary->emplace_back();
        break;
      case lynx::base::android::ReadableType::String:
        ary->emplace_back(
            base::android::JavaOnlyArray::JavaOnlyArrayGetStringAtIndex(
                env, array, i)
                .c_str());
        break;
      case lynx::base::android::ReadableType::Boolean: {
        ary->emplace_back(
            base::android::JavaOnlyArray::JavaOnlyArrayGetBooleanAtIndex(
                env, array, i));
        break;
      }
      case lynx::base::android::ReadableType::Int:
        ary->emplace_back(
            base::android::JavaOnlyArray::JavaOnlyArrayGetIntAtIndex(env, array,
                                                                     i));
        break;
      case lynx::base::android::ReadableType::Long:
        ary->emplace_back(
            base::android::JavaOnlyArray::JavaOnlyArrayGetLongAtIndex(
                env, array, i));
        break;
      case lynx::base::android::ReadableType::Number:
        ary->emplace_back(
            base::android::JavaOnlyArray::JavaOnlyArrayGetDoubleAtIndex(
                env, array, i));
        break;
      case lynx::base::android::ReadableType::Array:
        ary->emplace_back(ConvertJavaOnlyArrayToLepus(
            env, base::android::JavaOnlyArray::JavaOnlyArrayGetArrayAtIndex(
                     env, array, i)
                     .Get()));
        break;
      case lynx::base::android::ReadableType::Map:
        ary->emplace_back(ConvertJavaOnlyMapToLepus(
            env, base::android::JavaOnlyArray::JavaOnlyArrayGetMapAtIndex(
                     env, array, i)
                     .Get()));
        break;
      default:
        break;
    }
  }
  return lepus::Value(ary);
}

lepus::Value ValueConverterAndroid::ConvertJavaOnlyMapToLepus(JNIEnv* env,
                                                              jobject map) {
  auto table = lepus::Dictionary::Create();
  base::android::JavaOnlyMap::ForEachClosure closure =
      [&table](JNIEnv* env, jobject map, jstring j_key,
               const std::string& key) {
        base::android::ReadableType type =
            base::android::JavaOnlyMap::JavaOnlyMapGetTypeAtIndex(env, map,
                                                                  j_key);
        switch (type) {
          case lynx::base::android::ReadableType::Null:
            table->SetValue(key);
            break;
          case lynx::base::android::ReadableType::String:
            table->SetValue(
                key, base::android::JavaOnlyMap::JavaOnlyMapGetStringAtIndex(
                         env, map, j_key)
                         .c_str());
            break;
          case lynx::base::android::ReadableType::Boolean:
            table->SetValue(
                key, base::android::JavaOnlyMap::JavaOnlyMapGetBooleanAtIndex(
                         env, map, j_key));
            break;
          case lynx::base::android::ReadableType::Int:
            table->SetValue(
                key, base::android::JavaOnlyMap::JavaOnlyMapGetIntAtIndex(
                         env, map, j_key));
            break;
          case lynx::base::android::ReadableType::Long:
            table->SetValue(
                key, base::android::JavaOnlyMap::JavaOnlyMapGetLongAtIndex(
                         env, map, j_key));
            break;
          case lynx::base::android::ReadableType::Number:
            table->SetValue(
                key, base::android::JavaOnlyMap::JavaOnlyMapGetDoubleAtIndex(
                         env, map, j_key));
            break;

          case lynx::base::android::ReadableType::Array:
            table->SetValue(
                key,
                ConvertJavaOnlyArrayToLepus(
                    env, base::android::JavaOnlyMap::JavaOnlyMapGetArrayAtIndex(
                             env, map, j_key)
                             .Get()));
            break;
          case lynx::base::android::ReadableType::Map:
            table->SetValue(
                key,
                ConvertJavaOnlyMapToLepus(
                    env, base::android::JavaOnlyMap::JavaOnlyMapGetMapAtIndex(
                             env, map, j_key)
                             .Get()));
            break;
          default:
            break;
        }
      };
  base::android::JavaOnlyMap::ForEach(env, map, std::move(closure));

  return lepus::Value(table);
}
}  // namespace android
}  // namespace tasm
}  // namespace lynx
