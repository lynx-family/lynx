// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_MODULE_FACTORY_ANDROID_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_MODULE_FACTORY_ANDROID_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "core/public/jsb/native_module_factory.h"
#include "core/runtime/bindings/jsi/modules/android/lynx_module_android.h"

namespace lynx {
namespace piper {

class ModuleFactoryAndroid : public NativeModuleFactory {
 public:
  ModuleFactoryAndroid(JNIEnv* env, jobject moduleFactory);
  ~ModuleFactoryAndroid() override;

  std::shared_ptr<LynxNativeModule> CreateModule(
      const std::string& name) override;

  bool RetainJniObject();

  static bool RegisterJNIUtils(JNIEnv* env);

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
  base::android::ScopedGlobalJavaRef<jobject> strong_jni_object_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_MODULE_FACTORY_ANDROID_H_
