// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/lynx_module_android.h"

#include <jni.h>

#include <utility>

#include "base/include/expected.h"
#include "base/include/timer/time_utils.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_array.h"
#include "core/build/gen/LynxModuleWrapper_jni.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/interceptor/network_monitor.h"
#include "core/runtime/bindings/jsi/modules/android/java_method_descriptor.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/jsi/jsi.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/value_wrapper/value_impl_piper.h"
#include "core/value_wrapper/value_wrapper_utils.h"
namespace lynx {
namespace piper {

template <typename Func>
base::expected<piper::Value, JSINativeException> InvokeWithErrorReport(
    Func func, ModuleDelegate& delegate, const std::string& module_name,
    const std::string& method_name) {
  return func();
}

bool LynxModuleAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
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
  buildMap(env, method_descriptions, methods_, method_invokers_);
  LOGI("lynx LynxModule Create " << module_name_);
}

void LynxModuleAndroid::Destroy() {
  method_invokers_.clear();
  scope_rts_.clear();
  scope_timing_collectors_.clear();
  scope_native_promise_rets_.clear();
  LOGI("lynx LynxModule Destroy " << module_name_);
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
    LOGE("LynxModuleAndroid::invokeMethod failed: local_ref isNull()");
    return base::unexpected(
        "LynxModuleAndroid::invokeMethod failed: local_ref isNull()");
  }
  if (method_invokers_.find(method_name) == method_invokers_.end()) {
    LOGE("LynxModuleAndroid::invokeMethod. Method not found. " << method_name);
    return base::unexpected(
        "LynxModuleAndroid::invokeMethod. Method not found. " + method_name);
  }

  std::string first_param_str;
  if (count > 0 && args && args->IsArray() &&
      args->GetValueAtIndex(0)->IsString()) {
    first_param_str = args->GetValueAtIndex(0)->str();
  }

  auto method_invoker = method_invokers_[method_name];
  // NativePromise Invoke
  if (method_invoker->ContainsPromise()) {
    auto native_promise = CreateLynxNativePromise(
        method_invoker,
        Java_LynxModuleWrapper_getModule(env, local_ref.Get()).Get(),
        args.get(), count, callbacks);
    if (native_promise.has_value()) {
      scope_native_promise_rets_.push_back(
          std::optional<piper::Value>(std::move(native_promise.value())));
      return base::unexpected("__IS_NATIVE_PROMISE__");
    }
    return base::unexpected(native_promise.error());
  }
  // Common Invoke
  auto function_creator =
      [callbacks, ref = shared_from_this()](
          int arg_index) -> base::expected<jvalue, std::string> {
    if (callbacks.find(arg_index) != callbacks.end()) {
      const base::android::ScopedGlobalJavaRef<jobject>& callback_global_ptr =
          ref->CreateLynxModuleCallback(callbacks.at(arg_index));
      jvalue convert_result;
      convert_result.l = callback_global_ptr.Get();
      return convert_result;
    }
    return base::unexpected(
        "LynxModuleAndroid::invokeMethod. Callback not Found.");
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
    if (!lock_delegate && invoke_result.error().second.has_value()) {
      lock_delegate->OnErrorOccurred(
          module_name_, method_name,
          std::move(invoke_result.error().second.value()));
    }
    LOGE("Exception happen in LynxModuleAndroid invokeMethod: " + module_name_
         << "." << method_name << " , args: " << first_param_str);
    return base::unexpected(std::move(invoke_result.error().first));
  }
  return std::move(invoke_result.value());
}

void LynxModuleAndroid::buildMap(
    JNIEnv* env, lynx::base::android::ScopedLocalJavaRef<jobject>& descriptions,
    NativeModuleMethods& methods,
    std::unordered_map<std::string, std::shared_ptr<MethodInvoker>>&
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
      LOGE("Module Description is null. module name: " << module_name_);
      continue;
    }
    JavaMethodDescriptor descriptor(env, description_wrapper);
    const std::string method_name = descriptor.getName();
    const NativeModuleMethod metadata(method_name, 0);
    methods.emplace(method_name, metadata);

    method_invoker_maps[method_name] = std::make_shared<MethodInvoker>(
        descriptor.getMethod().Get(), descriptor.getSignature(), module_name_,
        method_name);
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
std::optional<piper::Value> LynxModuleAndroid::TryGetPromiseRet() {
  if (!scope_native_promise_rets_.empty()) {
    auto ret = std::move(scope_native_promise_rets_.back());
    scope_native_promise_rets_.pop_back();
    return ret;
  }
  return std::nullopt;
}

base::expected<piper::Value, std::string>
LynxModuleAndroid::CreateLynxNativePromise(
    const std::shared_ptr<MethodInvoker>& invoker, jobject module,
    const pub::Value* method_args, size_t args_count,
    const CallbackMap& callbacks) {
  const std::string method_name = invoker->GetMethodName();
  LOGI("LynxModule, MethodInvoker::InvokeImpl, got a |PROMISE| : ("
       << module_name_ << " " << method_name << ") will fire " << this);
  // Got a Promise Class.
  Runtime* rt = GetScopeRuntime();
  auto promise = rt->global().getPropertyAsFunction(*rt, "Promise");
  if (!promise) {
    return base::unexpected("Can't find Promise.");
  }

  tasm::report::FeatureCounter::Instance()->Count(
      tasm::report::LynxFeature::CPP_USE_NATIVE_PROMISE);

  // Notice: use piper::value & JSIException Directly!
  // TODO(zhangqun.29) delete this after delete LynxNativePromise
  auto executor_function_impl =
      [invoker, callbacks, this, module_delegate = legacy_module_delegate_,
       method_args, args_count, module](
          Runtime& rt, const Value& thisVal, const Value* args,
          size_t count) -> base::expected<piper::Value, JSINativeException> {
    const std::string& method_name = invoker->GetMethodName();
    const std::string& module_name = invoker->GetModuleName();

    piper::Scope piper_scope(rt);
    // The following three exceptions should never be thrown unless JS wrong
    if (count != 2) {
      LOGE("Promise arg count must be 2.");
      return base::unexpected(
          BUILD_JSI_NATIVE_EXCEPTION("Promise arg count must be 2."));
    }

    if (!(args)->isObject() || !(args)->getObject(rt).isFunction(rt)) {
      LOGE("Promise parameter should be two JS function.");
      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
          "Promise parameter should be two JS function."));
    }

    if (!(args + 1)->isObject() || !(args + 1)->getObject(rt).isFunction(rt)) {
      LOGE("Promise parameter should be two JS function.");
      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
          "Promise parameter should be two JS function."));
    }

    Function resolve = (args)->getObject(rt).getFunction(rt);
    Function reject = (args + 1)->getObject(rt).getFunction(rt);
    int64_t resolve_callback_id =
        module_delegate->RegisterJSCallbackFunction(std::move(resolve));
    int64_t reject_callback_id =
        module_delegate->RegisterJSCallbackFunction(std::move(reject));
    if (resolve_callback_id == ModuleCallback::kInvalidCallbackId ||
        reject_callback_id == ModuleCallback::kInvalidCallbackId) {
      LOGW(
          "LynxModule, MethodInvoker::InvokeImpl, create "
          "promise failed, LynxRuntime has destroyed");
      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
          "LynxModule, MethodInvoker::InvokeImpl, create "
          "promise failed, LynxRuntime has destroyed"));
    }
    LOGV("LynxModule, MethodInvoker::InvokeImpl, |PROMISE| : ("
         << module_name << " " << method_name << ")"
         << " resolve callback id: " << resolve_callback_id);

    ModuleCallbackAndroid::CallbackPair resolve_callback_pair =
        ModuleCallbackAndroid::createCallbackImpl(resolve_callback_id,
                                                  shared_from_this());
    ModuleCallbackAndroid::CallbackPair reject_callback_pair =
        ModuleCallbackAndroid::createCallbackImpl(reject_callback_id,
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

    auto function_creator =
        [callbacks,
         this](int arg_index) -> base::expected<jvalue, std::string> {
      if (callbacks.find(arg_index) != callbacks.end()) {
        auto callback_global_ptr =
            CreateLynxModuleCallback(callbacks.at(arg_index));
        jvalue convert_result;
        convert_result.l = callback_global_ptr.Get();
        return convert_result;
      }
      return base::unexpected(
          "LynxModuleAndroid::invokeMethod. Callback not Found.");
    };

    auto ret =
        invoker->Invoke(module, method_args, args_count,
                        std::move(function_creator), promise->GetJniObject());
    if (!ret.has_value()) {
      auto error_message = BUILD_JSI_NATIVE_EXCEPTION(ret.error().first);
      return base::unexpected(std::move(error_message));
    }
    LOGI("LynxModule, MethodInvoker::InvokeImpl, |PROMISE| : ("
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

  piper::Function fn = piper::Function::createFromHostFunction(
      *rt, piper::PropNameID::forAscii(*rt, "fn"), 2,
      std::move(executor_function));

  auto ret = promise->callAsConstructor(*rt, fn);
  if (!ret) {
    return base::unexpected("Promise call constructor failed.");
  }
  return std::move(*ret);
}
void LynxModuleAndroid::InvokeCallback(
    const std::shared_ptr<ModuleCallbackAndroid>& callback) {
  LOGV("LynxModule, MethodInvoker::InvokeCallback, put callback: "
       << " id: "
       << (callback ? std::to_string(callback->callback_id())
                    : std::string{"(no id due to callback is nullptr)"})
       << " to JSThread");
  auto lock_delegate = delegate_.lock();
  if (!lock_delegate) {
    LOGR("LynxModuleCallback has been destroyed. id:"
         << callback->callback_id());
    return;
  }
  lock_delegate->RunOnJSThread([callback, ref = shared_from_this()]() {
    ref->InvokeCallbackInternal(callback);
  });
}

// You must ensure that the call is made in the JS thread
void LynxModuleAndroid::InvokeCallbackInternal(
    const std::shared_ptr<ModuleCallbackAndroid>& callback) {
  auto lock_delegate = delegate_.lock();
  if (!lock_delegate) {
    LOGR("LynxModuleCallback has been destroyed. id:"
         << callback->callback_id());
    return;
  }
  std::shared_ptr<LynxPromiseImpl> promise = callback->promise.lock();
  if (promise) {
    if (promise->GetReject() == callback) {
      lock_delegate->InvokeCallback(promise->GetReject());
    } else {
      lock_delegate->InvokeCallback(promise->GetResolve());
    }
    promises_.erase(promise);
  } else if (callbackHolders_.erase(callback->callback_id())) {
    lock_delegate->InvokeCallback(callback);
  }
}

const base::android::ScopedGlobalJavaRef<jobject>&
LynxModuleAndroid::CreateLynxModuleCallback(
    const std::shared_ptr<LynxModuleCallback>& base_callback,
    ModuleCallbackType type) {
  uint64_t callback_id = base_callback->CallbackId();
  ModuleCallbackAndroid::CallbackPair callback_pair =
      ModuleCallbackAndroid::createCallbackImpl(callback_id, shared_from_this(),
                                                type);
  auto callback_android = callback_pair.first;
  // Notice: static pointer cast , in LynxModuleImpl, LynxModuleCallback is
  // ModuleCallback Copy callback info
  auto static_base_callback =
      std::static_pointer_cast<ModuleCallback>(base_callback);
  callback_android->SetStartTimeMS(static_base_callback->StartTimeMS());
  callback_android->SetModuleName(static_base_callback->module_name_);
  callback_android->SetModuleName(static_base_callback->method_name_);
  callback_android->SetCallbackFlowId(static_base_callback->CallbackFlowId());
  callback_android->timing_collector_ = static_base_callback->timing_collector_;
  callback_android->SetFirstArg(static_base_callback->first_arg_);
  // cache callback
  callbackHolders_[callback_id] = std::make_pair(
      std::move(callback_android), std::move(callback_pair.second));
  return callbackHolders_[callback_id].second;
}

}  // namespace piper
}  // namespace lynx
