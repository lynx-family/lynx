// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_LYNX_PLATFORM_JSI_OBJECT_ANDROID_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_LYNX_PLATFORM_JSI_OBJECT_ANDROID_H_

#include <jni.h>

#include <memory>
#include <string>
#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/runtime/bindings/jsi/modules/android/platform_jsi/jsi_object_utils.h"
#include "core/runtime/bindings/jsi/modules/android/platform_jsi/lynx_jsi_object_descriptor.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

/**
 * `LynxPlatformJSIObject` is a method designed to enhance the efficiency of
 * cross-platform data transfer. JavaScript can directly manipulate
 * platform-level objects through this class without going through the Bridge,
 * thereby avoiding extensive data conversion. For the Android implementation,
 * please refer to `ILynxJSIObject.java`.
 */
class LynxPlatformJSIObjectAndroid : public HostObject {
 public:
  static bool RegisterJNI(JNIEnv* env);

  static std::shared_ptr<LynxPlatformJSIObjectAndroid> Create(
      JNIEnv* env, jobject i_jsi_object);

  ~LynxPlatformJSIObjectAndroid() override = default;
  LynxPlatformJSIObjectAndroid(const LynxPlatformJSIObjectAndroid&) = delete;
  LynxPlatformJSIObjectAndroid& operator=(const LynxPlatformJSIObjectAndroid&) =
      delete;

  Value get(lynx::piper::Runtime*,
            const lynx::piper::PropNameID& name) override;

  std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

  /**
   * Type of JSIObject field
   * assign with com.lynx.jsbridge.jsi.LynxJSIObjectHub.JObjectType
   */
  enum class JObjectType : int8_t {
    kUnknownType = 0,
    kLynxJSIObjectType,
    kStringType,
    kObjectArrayType,
    kBoolArrayType,
    kIntArrayType,
    kLongArrayType,
    kFloatArrayType,
    kDoubleArrayType,
    kBoolType,
    kIntType,
    kLongType,
    kFloatType,
    kDoubleType,
  };

 private:
  LynxPlatformJSIObjectAndroid(JNIEnv* env, jobject i_jsi_object);

  std::unique_ptr<LynxJSIObjectDescriptor>& GetJSIObjectDescriptor(JNIEnv* env);

  // convert jni object to jsi object
  std::unique_ptr<platform_jsi::JSIObject> ConvertJSIObjectField(
      JNIEnv* env, jobject root_obj, const std::string& field_type,
      jfieldID field_id);

  // convert jni object whose type is ObjectArray
  std::unique_ptr<platform_jsi::JSIObject> ConvertJSIObjectArray(
      JNIEnv* env, jobject root_obj);

  // convert jni object whose type is primitive
  std::unique_ptr<platform_jsi::JSIObject> ConvertJSIObjectFieldPrimitive(
      JNIEnv* env, jobject root_object,
      LynxPlatformJSIObjectAndroid::JObjectType type, jfieldID field_id);

  // convert jni object whose type is String
  std::unique_ptr<platform_jsi::JSIObject> ConvertJSIObjectFieldString(
      JNIEnv* env, jobject field_obj);

  // convert jni object whose type is PrimitiveArray
  std::unique_ptr<platform_jsi::JSIObject> ConvertJSIObjectFieldPrimitiveArray(
      JNIEnv* env, jobject field_obj,
      LynxPlatformJSIObjectAndroid::JObjectType type);

  // convert jni object whose type is Object
  std::unique_ptr<platform_jsi::JSIObject> ConvertJSIObjectFieldObject(
      JNIEnv* env, jobject field_obj,
      LynxPlatformJSIObjectAndroid::JObjectType type);

  base::android::ScopedGlobalJavaRef<jobject> jsi_object_;
  base::android::ScopedLocalJavaRef<jclass> jsi_object_class_;
  std::unique_ptr<LynxJSIObjectDescriptor> jsi_object_descriptor_ = nullptr;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_LYNX_PLATFORM_JSI_OBJECT_ANDROID_H_
