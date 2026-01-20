// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_METHOD_INVOKER_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_METHOD_INVOKER_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/java_value.h"
#include "core/runtime/js/bindings/modules/android/callback_impl.h"
#include "core/runtime/js/bindings/modules/android/lynx_promise_impl.h"
#include "core/runtime/js/js_executor.h"

#if ENABLE_TESTBENCH_RECORDER
#include <stack>
#endif

namespace lynx {
namespace runtime {
namespace js {
using ErrorPair = std::pair<std::string, std::optional<base::LynxError>>;

class PtrContainerMap {
 public:
  std::list<base::android::ScopedGlobalJavaRef<jobject>> jobject_container_;
  std::vector<std::shared_ptr<base::android::JavaOnlyArray>> array_container_;
  std::vector<std::shared_ptr<base::android::JavaOnlyMap>> map_container_;
  std::list<base::android::ScopedGlobalJavaRef<jbyteArray>>
      byte_array_container_;
  std::list<base::android::ScopedGlobalJavaRef<jstring>> string_container_;
};

class MethodInvoker : public std::enable_shared_from_this<MethodInvoker> {
 public:
  MethodInvoker(jobject method, std::string signature,
                const std::string& module_name, const std::string& method_name);

  base::expected<std::unique_ptr<pub::Value>, ErrorPair> Invoke(
      jobject module, const pub::Value* args, size_t args_count,
      base::MoveOnlyClosure<
          base::expected<base::android::ScopedGlobalJavaRef<jobject>,
                         std::string>,
          int>
          function_creator,
      jobject nativePromise = nullptr);

  void SetAuthValidator(
      base::MoveOnlyClosure<bool, std::string,
                            const std::shared_ptr<base::android::JavaOnlyArray>>
          validator) {
    auth_verify_ = std::move(validator);
  }

  void SetContextFinder(
      base::MoveOnlyClosure<base::android::ScopedLocalJavaRef<jobject>>
          context_finder) {
    context_finder_ = std::move(context_finder);
  }

  inline std::string GetModuleName() const { return module_name_; }
  inline std::string GetMethodName() const { return method_name_; }
  inline bool ContainsPromise() const {
    return signature_[signature_.length() - 1] == 'P';
  }
  // Check if the method signature contains LynxContext parameter.
  // The LynxContext parameter is always the first parameter.
  inline bool ContainsLynxContext() const { return signature_[2] == 'K'; }

  bool VerifySignature(const pub::Value* args, size_t args_count);

 private:
  jmethodID method_;
  std::string signature_;
  std::size_t args_count_;
  std::string module_name_;
  std::string method_name_;

  base::expected<jvalue, std::string> ExtractPubValue(
      const base::android::JavaValue& args, int arg_index, char type);
  base::expected<std::unique_ptr<pub::Value>, ErrorPair>
  CallPlatformImplementation(JNIEnv* env, jobject module,
                             jvalue* java_arguments);
  std::optional<base::LynxError> ReportPendingJniException();
  base::MoveOnlyClosure<bool, std::string,
                        const std::shared_ptr<base::android::JavaOnlyArray>>
      auth_verify_;

  // Find LynxContext object from the Android platform.
  base::MoveOnlyClosure<base::android::ScopedLocalJavaRef<jobject>>
      context_finder_;
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_METHOD_INVOKER_H_
