// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_METHOD_INVOKER_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_METHOD_INVOKER_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_map.h"
#include "core/runtime/bindings/jsi/modules/android/callback_impl.h"
#include "core/runtime/bindings/jsi/modules/android/lynx_promise_impl.h"
#include "core/runtime/piper/js/js_executor.h"

#if ENABLE_TESTBENCH_RECORDER
#include <stack>
#include <vector>
#endif

namespace lynx {
namespace piper {

using CallbackSet = std::unordered_set<std::shared_ptr<ModuleCallbackAndroid>>;

class PtrContainerMap {
 public:
  std::list<base::android::ScopedGlobalJavaRef<jobject>> jobject_container_;
  std::vector<std::shared_ptr<base::android::JavaOnlyArray>> array_container_;
  std::vector<std::shared_ptr<base::android::JavaOnlyMap>> map_container_;
  std::list<base::android::ScopedGlobalJavaRef<jbyteArray>>
      byte_array_container_;
  std::list<base::android::ScopedGlobalJavaRef<jstring>> string_container_;
};

std::optional<JSINativeException> AssembleJSMap(
    lynx::base::android::JavaOnlyMap* map, Runtime* rt,
    const piper::Object* obj, PtrContainerMap& ptr_container_map,
    JSValueCircularArray& pre_object_vector, int depth = 0);
std::optional<JSINativeException> AssembleJSArray(
    lynx::base::android::JavaOnlyArray* array, Runtime* rt,
    const piper::Array* args, PtrContainerMap& ptr_container_map,
    JSValueCircularArray& pre_object_vector, int depth = 0);

class ModuleDelegate;

class MethodInvoker : public std::enable_shared_from_this<MethodInvoker> {
 public:
  MethodInvoker(jobject method, std::string signature,
                const std::string& module_name, const std::string& method_name,
                const std::shared_ptr<ModuleDelegate>& delegate);
  base::expected<piper::Value, JSINativeException> Invoke(
      jobject module, Runtime* rt, const piper::Value* args, size_t count,
      const piper::NativeModuleInfoCollectorPtr& timing_collector);

  base::expected<piper::Value, JSINativeException> Fire(JNIEnv* env,
                                                        jobject module,
                                                        jvalue* javaArguments,
                                                        Runtime* rt);

  base::android::ScopedGlobalJavaRef<jobject>
  ConvertJSIFunctionToCallbackObject(
      Function function, Runtime* rt, ModuleCallbackType type,
      const Value* first_arg, uint64_t start_time,
      const piper::NativeModuleInfoCollectorPtr& timing_collector);
  void InvokeCallback(const std::shared_ptr<ModuleCallbackAndroid>& callback);
  void Destroy();
  std::string& GetModuleName() { return module_name_; }
  std::string& GetMethodName() { return method_name_; }

#if ENABLE_TESTBENCH_RECORDER
  void SetRecordID(int64_t record_id) { record_id_ = record_id; }
  void StartRecordFunction();
  void EndRecordFunction(size_t count, const piper::Value* js_args, Runtime* rt,
                         piper::Value& res);
#endif

 private:
  jmethodID method_;
  std::string signature_;
  std::size_t jsArgCount_;
  std::string module_name_;
  std::string method_name_;
  const std::shared_ptr<ModuleDelegate> delegate_;
#if ENABLE_TESTBENCH_RECORDER
  int64_t record_id_ = 0;
  std::stack<std::vector<int64_t>> callbacks_stack_;
#endif

  base::expected<piper::Value, JSINativeException> InvokeImpl(
      jobject module, Runtime* rt, const piper::Value* args, size_t count,
      const piper::NativeModuleInfoCollectorPtr& timing_collector);
  void invokeCallbackInJSThread(
      const std::shared_ptr<ModuleCallbackAndroid>& callback);
  base::expected<jvalue, JSINativeException> extractJSValue(
      Runtime* rt, const piper::Value* args, char type,
      PtrContainerMap& ptr_container_map, int argIndex, uint64_t start_time,
      const piper::NativeModuleInfoCollectorPtr& timing_collector);
  bool reportPendingJniException();

  CallbackSet callbackWrappers_;
  std::unordered_set<std::shared_ptr<LynxPromiseImpl>> promises_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_METHOD_INVOKER_H_
