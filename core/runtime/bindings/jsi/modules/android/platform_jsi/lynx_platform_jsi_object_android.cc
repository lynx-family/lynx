// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/platform_jsi/lynx_platform_jsi_object_android.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/include/platform/android/jni_convert_helper.h"
#include "build/gen/LynxJSIObjectHub_jni.h"

namespace lynx {
namespace piper {

namespace {
[[maybe_unused]] constexpr char kTag[] = "LynxPlatformJSIObjectAndroid: ";

/**
 * Get object type by FieldType
 * This method doesn't need to use JNI, should be called first
 */
LynxPlatformJSIObjectAndroid::JObjectType GetJObjectTypeByFieldType(
    const std::string &field_type) {
  static const base::NoDestructor<const std::unordered_map<
      std::string, LynxPlatformJSIObjectAndroid::JObjectType>>
      kJObjectTypeMap{{
          {"Z", LynxPlatformJSIObjectAndroid::JObjectType::kBoolType},
          {"I", LynxPlatformJSIObjectAndroid::JObjectType::kIntType},
          {"J", LynxPlatformJSIObjectAndroid::JObjectType::kLongType},
          {"F", LynxPlatformJSIObjectAndroid::JObjectType::kFloatType},
          {"D", LynxPlatformJSIObjectAndroid::JObjectType::kDoubleType},
          {"[Z", LynxPlatformJSIObjectAndroid::JObjectType::kBoolArrayType},
          {"[I", LynxPlatformJSIObjectAndroid::JObjectType::kIntArrayType},
          {"[J", LynxPlatformJSIObjectAndroid::JObjectType::kLongArrayType},
          {"[F", LynxPlatformJSIObjectAndroid::JObjectType::kFloatArrayType},
          {"[D", LynxPlatformJSIObjectAndroid::JObjectType::kDoubleArrayType},
          {"Ljava/lang/String;",
           LynxPlatformJSIObjectAndroid::JObjectType::kStringType},

      }};
  auto type = kJObjectTypeMap->find(field_type);
  return type != kJObjectTypeMap->end()
             ? type->second
             : LynxPlatformJSIObjectAndroid::JObjectType::kUnknownType;
}

/**
 * Get object type by LynxJSIObjectHub in java
 */
LynxPlatformJSIObjectAndroid::JObjectType GetJObjectTypeByJObject(
    JNIEnv *env, jobject j_object) {
  jint type = Java_LynxJSIObjectHub_getJSIObjectFieldType(env, j_object);
  switch (type) {
    case 1:
      return LynxPlatformJSIObjectAndroid::JObjectType::kLynxJSIObjectType;
    case 2:
      return LynxPlatformJSIObjectAndroid::JObjectType::kStringType;
    case 3:
      return LynxPlatformJSIObjectAndroid::JObjectType::kObjectArrayType;
    case 4:
      return LynxPlatformJSIObjectAndroid::JObjectType::kBoolArrayType;
    case 5:
      return LynxPlatformJSIObjectAndroid::JObjectType::kIntArrayType;
    case 6:
      return LynxPlatformJSIObjectAndroid::JObjectType::kLongArrayType;
    case 7:
      return LynxPlatformJSIObjectAndroid::JObjectType::kFloatArrayType;
    case 8:
      return LynxPlatformJSIObjectAndroid::JObjectType::kDoubleArrayType;
    default:
      return LynxPlatformJSIObjectAndroid::JObjectType::kUnknownType;
  }
}
}  // namespace

bool LynxPlatformJSIObjectAndroid::RegisterJNI(JNIEnv *env) {
  return RegisterNativesImpl(env);
}

std::shared_ptr<LynxPlatformJSIObjectAndroid>
LynxPlatformJSIObjectAndroid::Create(JNIEnv *env, jobject i_jsi_object) {
  return std::shared_ptr<LynxPlatformJSIObjectAndroid>(
      new LynxPlatformJSIObjectAndroid(env, i_jsi_object));
}

LynxPlatformJSIObjectAndroid::LynxPlatformJSIObjectAndroid(JNIEnv *env,
                                                           jobject i_jsi_object)
    : jsi_object_(env, i_jsi_object) {}

std::unique_ptr<LynxJSIObjectDescriptor> &
LynxPlatformJSIObjectAndroid::GetJSIObjectDescriptor(JNIEnv *env) {
  if (!jsi_object_descriptor_) {
    // lazy get JSIObjectDescriptor from java LynxJSIObjectHub
    auto jsi_object_descriptor_object =
        Java_LynxJSIObjectHub_getJSIObjectDescriptor(env, jsi_object_.Get());
    jsi_object_descriptor_ =
        std::make_unique<LynxJSIObjectDescriptor>(jsi_object_descriptor_object);
  }
  return jsi_object_descriptor_;
}

std::vector<PropNameID> LynxPlatformJSIObjectAndroid::getPropertyNames(
    Runtime &rt) {
  // TODO(zhoupeng.z): implement
  return {};
}

lynx::piper::Value LynxPlatformJSIObjectAndroid::get(
    lynx::piper::Runtime *rt, const lynx::piper::PropNameID &name) {
  if (jsi_object_.IsNull()) {
    return Value::undefined();
  }

  auto field_name = name.utf8(*rt);
  JNIEnv *env = lynx::base::android::AttachCurrentThread();

  // get j field info
  auto field_info =
      GetJSIObjectDescriptor(env)->GetJSPropertyDescriptorInfo(env, field_name);
  if (!field_info) {
    LOGE(kTag << "fail to get JSPropertyDescriptor, fieldName: " << field_name);
    return Value::undefined();
  }

  // get j field object
  if (jsi_object_class_.IsNull()) {
    jsi_object_class_ = {env, env->GetObjectClass(jsi_object_.Get())};
  }
  jfieldID j_js_property_field_id = env->GetFieldID(
      jsi_object_class_.Get(), field_name.c_str(), field_info->c_str());

  auto jsi_object_field = ConvertJSIObjectField(
      env, jsi_object_.Get(), *field_info, j_js_property_field_id);
  return jsi_object_field ? *jsi_object_field->ConvertToValue(rt)
                          : piper::Value::null();
}

std::unique_ptr<platform_jsi::JSIObject>
LynxPlatformJSIObjectAndroid::ConvertJSIObjectField(
    JNIEnv *env, jobject jsi_object, const std::string &field_type,
    jfieldID field_id) {
  // 1. check primitive type
  LynxPlatformJSIObjectAndroid::JObjectType type =
      GetJObjectTypeByFieldType(field_type);
  auto value =
      ConvertJSIObjectFieldPrimitive(env, jsi_object_.Get(), type, field_id);
  if (value) {
    return value;
  }

  // other types are object type, so get field object here
  base::android::ScopedLocalJavaRef<jobject> field_obj(
      env, env->GetObjectField(jsi_object_.Get(), field_id));
  if (field_obj.IsNull()) {
    return nullptr;
  }

  // 2. check string type
  if (type == JObjectType::kStringType) {
    return ConvertJSIObjectFieldString(env, field_obj.Get());
  }

  // 3. check primitive array type
  value = ConvertJSIObjectFieldPrimitiveArray(env, field_obj.Get(), type);
  if (value) {
    return value;
  }

  // 4. check object type
  type = GetJObjectTypeByJObject(env, field_obj.Get());
  return ConvertJSIObjectFieldObject(env, field_obj.Get(), type);
}

std::unique_ptr<platform_jsi::JSIObject>
LynxPlatformJSIObjectAndroid::ConvertJSIObjectFieldPrimitive(
    JNIEnv *env, jobject root_object,
    LynxPlatformJSIObjectAndroid::JObjectType type, jfieldID field_id) {
  switch (type) {
    case JObjectType::kBoolType:
      return platform_jsi::JSIObject::Create(
          static_cast<bool>(env->GetBooleanField(root_object, field_id)));
    case JObjectType::kIntType:
      return platform_jsi::JSIObject::Create(
          static_cast<double>(env->GetIntField(root_object, field_id)));
    case JObjectType::kLongType:
      return platform_jsi::JSIObject::Create(
          env->GetLongField(root_object, field_id));
    case JObjectType::kFloatType:
      return platform_jsi::JSIObject::Create(
          static_cast<double>(env->GetFloatField(root_object, field_id)));
    case JObjectType::kDoubleType:
      return platform_jsi::JSIObject::Create(
          static_cast<double>(env->GetDoubleField(root_object, field_id)));
    default:
      return nullptr;
  }
}

std::unique_ptr<platform_jsi::JSIObject>
LynxPlatformJSIObjectAndroid::ConvertJSIObjectFieldString(JNIEnv *env,
                                                          jobject field_obj) {
  std::string string_value = base::android::JNIConvertHelper::ConvertToString(
      env, static_cast<jstring>(field_obj));
  return platform_jsi::JSIObject::Create(std::move(string_value));
}

std::unique_ptr<platform_jsi::JSIObject>
LynxPlatformJSIObjectAndroid::ConvertJSIObjectFieldPrimitiveArray(
    JNIEnv *env, jobject field_obj,
    LynxPlatformJSIObjectAndroid::JObjectType type) {
#define PRIMITIVE_ARRAY_CASE_CASTING(JNI_FIELD, CAST_J_ARRAY_TYPE,      \
                                     CAST_JS_TYPE)                      \
  auto j_array = (j##CAST_J_ARRAY_TYPE##Array)field_obj;                \
  auto *elements = env->Get##JNI_FIELD##ArrayElements(j_array, NULL);   \
  std::vector<std::unique_ptr<platform_jsi::JSIObject>> fields{};       \
  jsize size = env->GetArrayLength(j_array);                            \
  for (int i = 0; i < size; ++i) {                                      \
    fields.emplace_back(platform_jsi::JSIObject::Create(                \
        static_cast<CAST_JS_TYPE>(elements[i])));                       \
  }                                                                     \
  env->Release##JNI_FIELD##ArrayElements(j_array, elements, JNI_ABORT); \
  return platform_jsi::JSIObject::Create(std::move(fields));

  switch (type) {
    case JObjectType::kBoolArrayType: {
      PRIMITIVE_ARRAY_CASE_CASTING(Boolean, boolean, bool);
    }
    case JObjectType::kIntArrayType: {
      PRIMITIVE_ARRAY_CASE_CASTING(Int, int, double);
    }
    case JObjectType::kLongArrayType: {
      PRIMITIVE_ARRAY_CASE_CASTING(Long, long, jlong);
    }
    case JObjectType::kFloatArrayType: {
      PRIMITIVE_ARRAY_CASE_CASTING(Float, float, double);
    }
    case JObjectType::kDoubleArrayType: {
      PRIMITIVE_ARRAY_CASE_CASTING(Double, double, double);
    }
    default:
      return nullptr;
  }

#undef PRIMITIVE_ARRAY_CASE_CASTING
}

std::unique_ptr<platform_jsi::JSIObject>
LynxPlatformJSIObjectAndroid::ConvertJSIObjectFieldObject(
    JNIEnv *env, jobject field_obj,
    LynxPlatformJSIObjectAndroid::JObjectType type) {
  switch (type) {
    case JObjectType::kLynxJSIObjectType: {
      auto child_jsi_object_module =
          lynx::piper::LynxPlatformJSIObjectAndroid::Create(env, field_obj);
      return platform_jsi::JSIObject::Create(
          std::move(child_jsi_object_module));
    }
    case JObjectType::kStringType: {
      return ConvertJSIObjectFieldString(env, field_obj);
    }
    case JObjectType::kObjectArrayType:
      return ConvertJSIObjectArray(env, field_obj);
    default:
      return ConvertJSIObjectFieldPrimitiveArray(env, field_obj, type);
  }
}

std::unique_ptr<platform_jsi::JSIObject>
LynxPlatformJSIObjectAndroid::ConvertJSIObjectArray(JNIEnv *env,
                                                    jobject field_object) {
  jint length = env->GetArrayLength(static_cast<jarray>(field_object));
  std::vector<std::unique_ptr<platform_jsi::JSIObject>> raw_field_array;
  JObjectType element_type = JObjectType::kUnknownType;
  for (jint i = 0; i < length; ++i) {
    base::android::ScopedLocalJavaRef<jobject> element(
        env,
        env->GetObjectArrayElement(static_cast<jobjectArray>(field_object), i));
    if (element.IsNull()) {
      raw_field_array.emplace_back(nullptr);
      continue;
    }
    jobject element_obj = element.Get();
    if (element_type == JObjectType::kUnknownType) {
      element_type = GetJObjectTypeByJObject(env, element_obj);
    }
    raw_field_array.emplace_back(
        ConvertJSIObjectFieldObject(env, element_obj, element_type));
  }
  return platform_jsi::JSIObject::Create(std::move(raw_field_array));
}

}  // namespace piper
}  // namespace lynx
