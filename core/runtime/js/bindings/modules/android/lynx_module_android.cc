// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/android/lynx_module_android.h"

#include <jni.h>

#include <utility>

#include "base/include/expected.h"
#include "base/include/timer/time_utils.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/jni_helper.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/js/bindings/modules/android/java_method_descriptor.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/value_wrapper/value_impl_piper.h"
#include "core/value_wrapper/value_wrapper_utils.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxModuleWrapper_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxModuleWrapper_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxModuleWrapper(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"
namespace lynx {
namespace runtime {
namespace js {
template <typename Func>
base::expected<Value, JSINativeException> InvokeWithErrorReport(
    Func func, ModuleDelegate& delegate, const std::string& module_name,
    const std::string& method_name) {
  return func();
}

LynxModuleAndroid::LynxModuleAndroid(
    JNIEnv* env, jobject jni_object,
    std::shared_ptr<pub::PubValueFactory> value_factory)
    : LynxNativeModule(std::move(value_factory)), wrapper_(env, jni_object) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(wrapper_);
  if (local_ref.IsNull()) {
    return;
  }
  module_name_ = getName(local_ref.Get());
  // Build callback_invoker_ Map
  auto method_descriptions =
      Java_LynxModuleWrapper_getMethodDescriptors(env, local_ref.Get());
  has_auth_validator_ =
      Java_LynxModuleWrapper_hasAuthValidator(env, local_ref.Get()) == JNI_TRUE;
  buildMap(env, method_descriptions, methods_, method_invokers_);
  LOGI("NativeModule: LynxModuleAndroid Create: " << module_name_);
}

void LynxModuleAndroid::Destroy() {
  method_invokers_.clear();
  scope_rts_.clear();
  scope_timing_collectors_.clear();
  scope_native_promise_rets_.clear();
  LOGI("NativeModule: LynxModuleAndroid Destroy: " << module_name_);
}

std::string LynxModuleAndroid::getName(jobject jni_object) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> jstr =
      Java_LynxModuleWrapper_getName(env, jni_object);
  const char* str = env->GetStringUTFChars(jstr.Get(), JNI_FALSE);
  std::string name(str);
  env->ReleaseStringUTFChars(jstr.Get(), str);
  return name;
}

base::expected<std::unique_ptr<pub::Value>, std::string>
LynxModuleAndroid::InvokeMethod(const std::string& method_name,
                                std::unique_ptr<pub::Value> args, size_t count,
                                const CallbackMap& callbacks) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(wrapper_);
  if (local_ref.IsNull()) {
    LOGE(
        "NativeModule: LynxModuleAndroid InvokeMethod Failed: local_ref "
        "isNull");
    return base::unexpected(
        "NativeModule: LynxModuleAndroid::InvokeMethod Failed: local_ref "
        "isNull");
  }
  if (method_invokers_.find(method_name) == method_invokers_.end()) {
    LOGE("NativeModule: LynxModuleAndroid InvokeMethod. Method not found. "
         << method_name);
    return base::unexpected(
        "NativeModule: LynxModuleAndroid InvokeMethod. Method not found. " +
        method_name);
  }

  std::string first_param_str;
  if (count > 0 && args && args->IsArray() &&
      args->GetValueAtIndex(0)->IsString()) {
    first_param_str = args->GetValueAtIndex(0)->str();
  }

  auto method_invoker = GetMethodInvoker(method_name, args.get(), count);
  if (method_invoker == nullptr) {
    LOGE("NativeModule: LynxModuleAndroid InvokeMethod. Method not found. "
         << method_name);
    return base::unexpected(
        "NativeModule: LynxModuleAndroid InvokeMethod. Method not found. " +
        method_name);
  }
  // NativePromise Invoke
  if (method_invoker->ContainsPromise()) {
    auto native_promise = CreateLynxNativePromise(
        method_invoker,
        Java_LynxModuleWrapper_getModule(env, local_ref.Get()).Get(),
        args.get(), count, callbacks);
    if (native_promise.has_value()) {
      scope_native_promise_rets_.emplace_back(
          std::optional<Value>(std::move(native_promise.value())));
      return base::unexpected("__IS_NATIVE_PROMISE__");
    }
    return base::unexpected(native_promise.error());
  }
  // Common Invoke
  auto function_creator = [callbacks, ref = shared_from_this()](int arg_index)
      -> base::expected<base::android::ScopedGlobalJavaRef<jobject>,
                        std::string> {
    if (callbacks.find(arg_index) != callbacks.end()) {
      const base::android::ScopedGlobalJavaRef<jobject> callback_global_ptr =
          ref->CreateLynxModuleCallback(callbacks.at(arg_index));
      return std::move(callback_global_ptr);
    }
    return base::unexpected(
        "NativeModule: LynxModuleAndroid Create Callback Failed: Callback not "
        "Found.");
  };
  base::expected<std::unique_ptr<pub::Value>, ErrorPair> invoke_result =
      method_invoker->Invoke(
          Java_LynxModuleWrapper_getModule(env, local_ref.Get()).Get(),
          args.get(), count, std::move(function_creator));

  if (tasm::LynxEnv::GetInstance().IsPiperMonitorEnabled()) {
    tasm::LynxEnv::onPiperInvoked(module_name_, method_name, first_param_str,
                                  "", "");
  }

  if (!invoke_result.has_value()) {
    auto lock_delegate = delegate_.lock();
    if (lock_delegate && invoke_result.error().second.has_value()) {
      lock_delegate->OnErrorOccurred(
          module_name_, method_name,
          std::move(invoke_result.error().second.value()));
    }
    LOGE("NativeModule: Exception Happen In LynxModuleAndroid InvokeMethod: " +
             module_name_
         << "." << method_name << " , args: " << first_param_str);
    return base::unexpected(std::move(invoke_result.error().first));
  }
  return std::move(invoke_result.value());
}
void LynxModuleAndroid::buildMap(
    JNIEnv* env, lynx::base::android::ScopedLocalJavaRef<jobject>& descriptions,
    NativeModuleMethods& methods,
    std::unordered_map<std::string,
                       std::vector<std::shared_ptr<MethodInvoker>>>&
        method_invoker_maps) {
  // Get method descriptions , use MethodDescriptor
  jclass cls_array_list = env->GetObjectClass(descriptions.Get());
  jmethodID array_list_get_method =
      env->GetMethodID(cls_array_list, "get", "(I)Ljava/lang/Object;");
  jmethodID array_list_size_method =
      env->GetMethodID(cls_array_list, "size", "()I");
  env->DeleteLocalRef(cls_array_list);
  jint total_methods_num =
      env->CallIntMethod(descriptions.Get(), array_list_size_method);
  // Build the method invoker corresponding to each method
  for (int i = 0; i < total_methods_num; i++) {
    jobject description_wrapper =
        env->CallObjectMethod(descriptions.Get(), array_list_get_method, i);
    if (description_wrapper == nullptr) {
      LOGE("NativeModule: Module Description Is Null. ModuleName: "
           << module_name_);
      continue;
    }
    JavaMethodDescriptor descriptor(env, description_wrapper);
    const std::string method_name = descriptor.getName();
    const NativeModuleMethod metadata(method_name, 0);
    methods.emplace(method_name, metadata);
    // Add method invoker to the list
    if (method_invoker_maps.find(method_name) == method_invoker_maps.end()) {
      method_invoker_maps[method_name] =
          std::vector<std::shared_ptr<MethodInvoker>>();
    }
    auto method_invoker = std::make_shared<MethodInvoker>(
        descriptor.getMethod().Get(), descriptor.getSignature(), module_name_,
        method_name);
    method_invoker_maps[method_name].push_back(method_invoker);

    // Verify that the method is allowed to be called
    if (has_auth_validator_) {
      method_invoker->SetAuthValidator(
          [this](const std::string& method_name,
                 const std::shared_ptr<base::android::JavaOnlyArray>& validator)
              -> bool { return Verify(method_name, validator); });
    }

    method_invoker->SetContextFinder(
        [this]() -> base::android::ScopedLocalJavaRef<jobject> {
          return GetLynxContext();
        });
    env->DeleteLocalRef(description_wrapper);
  }
}

void LynxModuleAndroid::EnterInvokeScope(
    Runtime* rt, std::shared_ptr<ModuleDelegate> module_delegate) {
  if (!legacy_module_delegate_) {
    legacy_module_delegate_ = module_delegate;
  }
  scope_rts_.push_back(rt);
}

void LynxModuleAndroid::ExitInvokeScope() {
  if (!scope_rts_.empty()) {
    scope_rts_.pop_back();
  }
}
std::optional<Value> LynxModuleAndroid::TryGetPromiseRet() {
  if (!scope_native_promise_rets_.empty()) {
    auto ret = std::move(scope_native_promise_rets_.back());
    scope_native_promise_rets_.pop_back();
    return ret;
  }
  return std::nullopt;
}

base::expected<Value, std::string> LynxModuleAndroid::CreateLynxNativePromise(
    const std::shared_ptr<MethodInvoker>& invoker, jobject module,
    const pub::Value* method_args, size_t args_count,
    const CallbackMap& callbacks) {
  const std::string method_name = invoker->GetMethodName();
  LOGI("NativeModule: LynxModuleAndroid CreatePromise, got a |PROMISE| : ("
       << module_name_ << " " << method_name << ") will fire " << this);
  // Got a Promise Class.
  Runtime* rt = GetScopeRuntime();
  auto promise = rt->global().getPropertyAsFunction(*rt, "Promise");
  if (!promise) {
    return base::unexpected("NativeModule: Can't Find Promise Construct.");
  }

  tasm::report::FeatureCounter::Instance()->Count(
      tasm::report::LynxFeature::CPP_USE_NATIVE_PROMISE);

  // Notice: use value & JSIException Directly!
  // TODO(zhangqun.29) delete this after delete LynxNativePromise
  auto executor_function_impl =
      [invoker, callbacks, this, module_delegate = legacy_module_delegate_,
       method_args, args_count,
       module](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
    const std::string& method_name = invoker->GetMethodName();
    const std::string& module_name = invoker->GetModuleName();

    Scope piper_scope(rt);
    // The following three exceptions should never be thrown unless JS wrong
    if (count != 2) {
      LOGE("NativeModule: CreatePromise Arg Count Must Be 2.");
      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
          "NativeModule: CreatePromise Arg Count Must Be 2."));
    }

    if (!(args)->isObject() || !(args)->getObject(rt).isFunction(rt)) {
      LOGE("NativeModule: CreatePromise Parameter Should Be Two JS Function.");
      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
          "NativeModule: CreatePromise Parameter Should Be Two JS Function."));
    }

    if (!(args + 1)->isObject() || !(args + 1)->getObject(rt).isFunction(rt)) {
      LOGE("NativeModule: CreatePromise Parameter Should Be Two JS Function.");
      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
          "NativeModule: CreatePromise Parameter Should Be Two JS Function."));
    }

    Function resolve = (args)->getObject(rt).getFunction(rt);
    Function reject = (args + 1)->getObject(rt).getFunction(rt);
    int64_t resolve_callback_id =
        module_delegate->RegisterJSCallbackFunction(std::move(resolve));
    int64_t reject_callback_id =
        module_delegate->RegisterJSCallbackFunction(std::move(reject));
    if (resolve_callback_id == ModuleCallback::kInvalidCallbackId ||
        reject_callback_id == ModuleCallback::kInvalidCallbackId) {
      LOGW("NativeModule: Create Promise Failed, LynxRuntime Has Destroyed");
      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
          "NativeModule: Create Promise Failed, LynxRuntime Has Destroyed"));
    }
    LOGV("NativeModule:  Create Promise Success : ("
         << module_name << " " << method_name << ")"
         << " resolve callback id: " << resolve_callback_id);

    ModuleCallbackAndroid::CallbackPair resolve_callback_pair =
        ModuleCallbackAndroid::CreateCallbackImpl(
            std::make_shared<ModuleCallback>(resolve_callback_id),
            shared_from_this());
    ModuleCallbackAndroid::CallbackPair reject_callback_pair =
        ModuleCallbackAndroid::CreateCallbackImpl(
            std::make_shared<ModuleCallback>(reject_callback_id),
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

    auto function_creator = [callbacks, this](int arg_index)
        -> base::expected<base::android::ScopedGlobalJavaRef<jobject>,
                          std::string> {
      if (callbacks.find(arg_index) != callbacks.end()) {
        auto callback_global_ptr =
            CreateLynxModuleCallback(callbacks.at(arg_index));
        return std::move(callback_global_ptr);
      }
      return base::unexpected(
          "NativeModule: LynxModuleAndroid Create Promise Callback Failed: "
          "Callback not Found.");
    };

    auto ret =
        invoker->Invoke(module, method_args, args_count,
                        std::move(function_creator), promise->GetJniObject());
    if (!ret.has_value()) {
      auto error_message = BUILD_JSI_NATIVE_EXCEPTION(ret.error().first);
      return base::unexpected(std::move(error_message));
    }
    LOGI("NativeModule:  |NATIVE_PROMISE| : ("
         << module_name << " " << method_name << ") did fire " << this);
    legacy_module_delegate_->OnMethodInvoked(module_name, method_name,
                                             error::E_SUCCESS);
    auto piper_value =
        pub::ValueUtils::ConvertValueToPiperValue(rt, *(ret.value()));
    return piper_value;
  };

  auto executor_function = [executor_function_impl =
                                std::move(executor_function_impl),
                            self = shared_from_this(), this,
                            method_name](auto&&... args) {
    return InvokeWithErrorReport(
        [executor_function_impl = std::move(executor_function_impl),
         &args...]() {
          return executor_function_impl(std::forward<decltype(args)>(args)...);
        },
        *legacy_module_delegate_, module_name_, method_name);
  };

  Function fn = Function::createFromHostFunction(
      *rt, PropNameID::forAscii(*rt, "fn"), 2, std::move(executor_function));

  auto ret = promise->callAsConstructor(*rt, fn);
  if (!ret) {
    return base::unexpected("NativeModule: Promise Call Constructor Failed.");
  }
  return std::move(*ret);
}
// handle promise  & callback
void LynxModuleAndroid::InvokeCallback(
    const std::shared_ptr<LynxModuleCallback>& callback,
    std::weak_ptr<LynxPromiseImpl> promise) {
  LOGV("NativeModule: InvokeCallback, put callback: "
       << " id: "
       << (callback ? std::to_string(callback->CallbackId())
                    : std::string{"(no id due to callback is nullptr)"})
       << " to JSThread");
  auto lock_delegate = delegate_.lock();
  if (!lock_delegate) {
    LOGR("NativeModule: LynxModuleCallback Has Been Destroyed. id:"
         << callback->CallbackId());
    return;
  }
  std::shared_ptr<LynxPromiseImpl> lock_promise = promise.lock();
  if (lock_promise) {
    auto promise_pre_func = [lock_promise, this]() -> bool {
      promises_.erase(lock_promise);
      LOGV("NativeModule: Promise Has Been Removed From Holder .");
      return true;
    };
    if (lock_promise->GetReject() == callback) {
      lock_delegate->InvokeCallback(lock_promise->GetReject(),
                                    std::move(promise_pre_func));
    } else {
      lock_delegate->InvokeCallback(lock_promise->GetResolve(),
                                    std::move(promise_pre_func));
    }
  }
  if (callback) {
    auto callback_pre_func = [this, callback]() -> bool {
      LOGV("NativeModule: Callback Has Been Removed From Holder , CallbackId."
           << callback->CallbackId());
      return callbackHolders_.erase(callback->CallbackId());
    };
    lock_delegate->InvokeCallback(callback, std::move(callback_pre_func));
  }
}

const base::android::ScopedGlobalJavaRef<jobject>
LynxModuleAndroid::CreateLynxModuleCallback(
    const std::shared_ptr<LynxModuleCallback>& base_callback) {
  uint64_t callback_id = base_callback->CallbackId();
  ModuleCallbackAndroid::CallbackPair callback_pair =
      ModuleCallbackAndroid::CreateCallbackImpl(base_callback,
                                                shared_from_this());
  // cache callback
  callbackHolders_[callback_id] = std::move(callback_pair.first);
  return std::move(callback_pair.second);
}

ModuleCallbackAndroid* LynxModuleAndroid::GetModuleCallbackById(
    uint64_t callback_id) {
  auto it = callbackHolders_.find(callback_id);
  if (it == callbackHolders_.end()) {
    return nullptr;
  }
  return it->second.get();
}

bool LynxModuleAndroid::Verify(
    const std::string& method_name,
    const std::shared_ptr<base::android::JavaOnlyArray>& params) {
  if (has_auth_validator_) {
    JNIEnv* env = base::android::AttachCurrentThread();
    base::android::ScopedLocalJavaRef<jobject> local_ref(wrapper_);
    if (local_ref.IsNull()) {
      return false;
    }
    return Java_LynxModuleWrapper_verify(
               env, local_ref.Get(),
               base::android::JNIConvertHelper::ConvertToJNIStringUTF(
                   env, module_name_)
                   .Get(),
               base::android::JNIConvertHelper::ConvertToJNIStringUTF(
                   env, method_name)
                   .Get(),
               params->jni_object()) == JNI_TRUE;
  }
  // Calling is allowed by default
  return true;
}
const std::shared_ptr<MethodInvoker> LynxModuleAndroid::GetMethodInvoker(
    const std::string& method_name, const pub::Value* args, size_t args_count) {
  const std::vector<std::shared_ptr<MethodInvoker>>& invoker_vec =
      method_invokers_[method_name];

  // If no invoker, return nullptr.
  // This means that no target method are registered in the Lynx module.
  if (invoker_vec.empty()) {
    return nullptr;
  }

  // If only one invoker, return it.
  // To maintain compatibility with legacy logic while also prioritizing
  // performance, there is no signature verification here; parameter matching
  // and verification are performed during invokeMethod call. This means that no
  // methods with the same name are registered in the Lynx module.
  if (invoker_vec.size() == 1) {
    return invoker_vec.back();
  }

  // If multiple invokers, verify signature.
  // This means that multiple methods with the same name are registered in the
  // Lynx module.
  for (const std::shared_ptr<MethodInvoker>& invoker : invoker_vec) {
    if (invoker->VerifySignature(args, args_count)) {
      return invoker;
    }
  }
  return nullptr;
}
base::android::ScopedLocalJavaRef<jobject> LynxModuleAndroid::GetLynxContext() {
  base::android::ScopedLocalJavaRef<jobject> local_ref(wrapper_);
  if (local_ref.IsNull()) {
    return base::android::ScopedLocalJavaRef<jobject>();
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_LynxModuleWrapper_getLynxContext(
      env, local_ref.Get(),
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(
          env, std::to_string(context_id_))
          .Get());
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
