// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/value_wrapper/android/value_impl_android.h"

#include <memory>

#include "core/base/android/java_value.h"
#include "core/base/js_constants.h"
#include "core/runtime/common/utils.h"

namespace lynx {
namespace pub {

std::unique_ptr<pub::Value> ValueImplAndroidFactory::CreateBool(bool value) {
  return std::make_unique<ValueImplAndroid>(base::android::JavaValue(value));
}

std::unique_ptr<pub::Value> ValueImplAndroidFactory::CreateNumber(
    double value) {
  return std::make_unique<ValueImplAndroid>(base::android::JavaValue(value));
}

std::unique_ptr<pub::Value> ValueImplAndroidFactory::CreateString(
    const std::string& value) {
  return std::make_unique<ValueImplAndroid>(base::android::JavaValue(value));
}

std::unique_ptr<pub::Value> ValueImplAndroidFactory::CreateArrayBuffer(
    std::unique_ptr<uint8_t[]> value, size_t length) {
  return std::make_unique<ValueImplAndroid>(base::android::JavaValue(
      static_cast<const uint8_t*>(value.get()), length));
}

std::unique_ptr<pub::Value> ValueImplAndroidFactory::CreateArray() {
  return std::make_unique<ValueImplAndroid>(base::android::JavaValue(
      std::make_shared<base::android::JavaOnlyArray>()));
}

std::unique_ptr<pub::Value> ValueImplAndroidFactory::CreateMap() {
  return std::make_unique<ValueImplAndroid>(
      base::android::JavaValue(std::make_shared<base::android::JavaOnlyMap>()));
}

// Iterator
void ValueImplAndroid::ForeachArray(pub::ForeachArrayFunc func) const {
  if (!backend_value_.IsArray()) {
    return;
  }
  const std::shared_ptr<base::android::JavaOnlyArray>& array =
      backend_value_.Array();
  JNIEnv* env = base::android::AttachCurrentThread();
  int32_t size = base::android::JavaOnlyArray::JavaOnlyArrayGetSize(
      env, array->jni_object());
  for (size_t i = 0; i < size; ++i) {
    base::android::JavaValue val =
        base::android::JavaOnlyArray::JavaOnlyArrayGetJavaValueAtIndex(
            env, array->jni_object(), i);
    func(i, ValueImplAndroid(std::move(val)));
  }
}

void ValueImplAndroid::ForeachMap(pub::ForeachMapFunc func) const {
  if (!backend_value_.IsMap()) {
    return;
  }

  base::android::JavaOnlyMap::ForEachClosure closure =
      [func = std::move(func)](JNIEnv* env, jobject map, jstring j_key,
                               const std::string& key) {
        base::android::JavaValue val =
            base::android::JavaOnlyMap::JavaOnlyMapGetJavaValueAtIndex(env, map,
                                                                       j_key);
        func(ValueImplAndroid(base::android::JavaValue(j_key)),
             ValueImplAndroid(std::move(val)));
      };

  JNIEnv* env = base::android::AttachCurrentThread();
  const std::shared_ptr<base::android::JavaOnlyMap>& java_only_map =
      backend_value_.Map();
  base::android::JavaOnlyMap::ForEach(env, java_only_map->jni_object(),
                                      std::move(closure));
}

base::android::JavaValue ValueUtilsAndroid::ConvertValueToJavaValue(
    const Value& value,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  if (value.backend_type() == pub::ValueBackendType::ValueBackendTypeJava) {
    return (reinterpret_cast<const ValueImplAndroid*>(&value))->backend_value();
  } else {
    base::android::JavaValue res;
    if (value.IsString()) {
      res = base::android::JavaValue(value.str());
    } else if (value.IsBool()) {
      res = base::android::JavaValue(value.Bool());
    } else if (value.IsInt32()) {
      res = base::android::JavaValue(value.Int32());
    } else if (value.IsUInt32()) {
      // Since Java value only has two integer types, int32 and int64, we
      // convert uint32 to int64 for storage to prevent range overflow.
      res = base::android::JavaValue(static_cast<int64_t>(value.UInt32()));
    } else if (value.IsInt64()) {
      res = base::android::JavaValue(value.Int64());
    } else if (value.IsUInt64()) {
      // Note: Since Java value only has two integer types, int32 and int64,
      // using uint64 may exceed the range and cause overflow.
      res = base::android::JavaValue(static_cast<int64_t>(value.UInt64()));
    } else if (value.IsNumber()) {
      res = base::android::JavaValue(value.Number());
    } else if (value.IsArrayBuffer()) {
      int length = value.Length();
      res = base::android::JavaValue(value.ArrayBuffer(), length);
    } else if (value.IsArray()) {
      ScopedCircleChecker scoped_circle_checker;
      if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector,
                                                         value, depth)) {
        res = ConvertValueToJavaArray(value, prev_value_vector, depth + 1);
      }
    } else if (value.IsMap()) {
      ScopedCircleChecker scoped_circle_checker;
      if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector,
                                                         value, depth)) {
        res = ConvertValueToJavaMap(value, prev_value_vector, depth + 1);
      }
    }
    return res;
  }
}

base::android::JavaValue ValueUtilsAndroid::ConvertValueToJavaArray(
    const Value& value,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  if (value.backend_type() == pub::ValueBackendType::ValueBackendTypeJava) {
    return (reinterpret_cast<const ValueImplAndroid*>(&value))->backend_value();
  } else {
    std::shared_ptr<base::android::JavaOnlyArray> array =
        std::make_shared<base::android::JavaOnlyArray>();
    value.ForeachArray([&array, &prev_value_vector, depth](
                           int64_t index, const pub::Value& val) {
      array->PushJavaValue(ValueUtilsAndroid::ConvertValueToJavaValue(
          val, prev_value_vector, depth + 1));
    });
    return base::android::JavaValue(std::move(array));
  }
}

base::android::JavaValue ValueUtilsAndroid::ConvertValueToJavaMap(
    const Value& value,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  if (value.backend_type() == pub::ValueBackendType::ValueBackendTypeJava) {
    return (reinterpret_cast<const ValueImplAndroid*>(&value))->backend_value();
  } else {
    std::shared_ptr<base::android::JavaOnlyMap> map =
        std::make_shared<base::android::JavaOnlyMap>();
    value.ForeachMap([&map, &prev_value_vector, depth](const pub::Value& key,
                                                       const pub::Value& val) {
      map->PushJavaValue(key.str(), ConvertValueToJavaValue(
                                        val, prev_value_vector, depth + 1));
    });
    return base::android::JavaValue(std::move(map));
  }
}

}  // namespace pub
}  // namespace lynx
