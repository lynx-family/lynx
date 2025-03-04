// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_MODULE_ANDROID_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_MODULE_ANDROID_H_

#include <jni.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/expected.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "core/runtime/bindings/jsi/modules/android/java_attribute_descriptor.h"
#include "core/runtime/bindings/jsi/modules/android/method_invoker.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"

#if ENABLE_TESTBENCH_RECORDER
#include <vector>
#endif

namespace lynx {

namespace piper {

class LynxModuleAndroid : public LynxModule {
 public:
  static bool RegisterJNI(JNIEnv *env);
  LynxModuleAndroid(JNIEnv *env, jobject jni_object,
                    const std::shared_ptr<ModuleDelegate> &delegate);
  void Destroy() override;

  static std::string getName(jobject jni_object);

 public:
  void buildMap(
      JNIEnv *env, const std::shared_ptr<ModuleDelegate> &delegate,
      lynx::base::android::ScopedLocalJavaRef<jobject> &descriptions,
      std::unordered_map<std::string, std::shared_ptr<MethodMetadata>> &map,
      std::unordered_map<std::string, std::shared_ptr<MethodInvoker>>
          &invokeMapByName_);
  void buildAttributeMap(
      JNIEnv *env,
      lynx::base::android::ScopedLocalJavaRef<jobject> &descriptions,
      std::unordered_map<std::string, JavaAttributeDescriptor> &map);

  piper::Value Fire(const MethodMetadata &method, Runtime *rt,
                    jvalue *javaArguments);
  base::android::ScopedGlobalJavaRef<jobject>
  ConvertJSIFunctionToCallbackObject(
      const MethodMetadata &method, Function &&function, Runtime *rt,
      ModuleCallbackType type, const Value *first_arg, uint64_t start_time,
      const piper::NativeModuleInfoCollectorPtr &timingCollector);
#if ENABLE_TESTBENCH_RECORDER
  void SetRecordID(int64_t record_id) override;
  void StartRecordFunction(const std::string &method_name = "") override;
  void EndRecordFunction(const std::string &method_name, size_t count,
                         const piper::Value *js_args, Runtime *rt,
                         piper::Value &res) override;
#endif

 protected:
  base::expected<piper::Value, piper::JSINativeException> invokeMethod(
      const MethodMetadata &method, Runtime *rt, const piper::Value *args,
      size_t count) override;
  piper::Value getAttributeValue(Runtime *rt, std::string propName) override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> wrapper_;
  std::unordered_map<std::string, std::shared_ptr<MethodInvoker>>
      methodsByName_;
  std::unordered_map<std::string, JavaAttributeDescriptor> attributeByName_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_MODULE_ANDROID_H_
