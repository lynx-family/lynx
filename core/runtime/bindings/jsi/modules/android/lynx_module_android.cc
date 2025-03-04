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
#include "lynx/core/value_wrapper/value_impl_piper.h"

namespace lynx {
namespace piper {

bool LynxModuleAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

LynxModuleAndroid::LynxModuleAndroid(
    JNIEnv* env, jobject jni_object,
    const std::shared_ptr<ModuleDelegate>& delegate)
    : LynxModule(getName(jni_object), delegate), wrapper_(env, jni_object) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(wrapper_);
  if (local_ref.IsNull()) {
    return;
  }
  auto method_descriptions =
      Java_LynxModuleWrapper_getMethodDescriptors(env, local_ref.Get());
  buildMap(env, delegate, method_descriptions, methodMap_, methodsByName_);
  auto attribute_descriptions =
      Java_LynxModuleWrapper_getAttributeDescriptor(env, local_ref.Get());
  buildAttributeMap(env, attribute_descriptions, attributeByName_);
}

void LynxModuleAndroid::Destroy() {
  LOGI("lynx LynxModule Destroy " << name_);
  methodsByName_.clear();
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

piper::Value LynxModuleAndroid::Fire(const MethodMetadata& method, Runtime* rt,
                                     jvalue* javaArguments) {
  const std::string& method_name = method.name;
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(wrapper_);
  if (local_ref.IsNull()) {
    LOGE("LynxModuleAndroid::invokeMethod failed: local_ref isNull()");
    return Value();
  }
  if (methodsByName_.find(method_name) == methodsByName_.end()) {
    LOGE("LynxModuleAndroid::Fire. Method not found. " << method_name);
    return Value();
  }
  auto method_invoker = methodsByName_[method_name];
  auto value_expected = method_invoker->Fire(
      env, Java_LynxModuleWrapper_getModule(env, local_ref.Get()).Get(),
      javaArguments, rt);
  if (value_expected.has_value()) {
    return std::move(value_expected.value());
  } else {
    return Value();
  }
}

base::android::ScopedGlobalJavaRef<jobject>
LynxModuleAndroid::ConvertJSIFunctionToCallbackObject(
    const MethodMetadata& method, Function&& function, Runtime* rt,
    ModuleCallbackType type, const Value* first_arg, uint64_t start_time,
    const piper::NativeModuleInfoCollectorPtr& timing_collector) {
  const std::string& method_name = method.name;
  if (methodsByName_.find(method_name) == methodsByName_.end()) {
    LOGE("LynxModuleAndroid::ConvertJSIFunction. Method not found: "
         << method_name);
    return base::android::ScopedGlobalJavaRef<jobject>();
  }
  auto& method_invoker = methodsByName_[method_name];
  return method_invoker->ConvertJSIFunctionToCallbackObject(
      std::move(function), rt, type, first_arg, start_time, timing_collector);
}

base::expected<piper::Value, piper::JSINativeException>
LynxModuleAndroid::invokeMethod(const MethodMetadata& method, Runtime* rt,
                                const piper::Value* args, size_t count) {
  // TODO(zhangqun): delete those call, use lynx_module_impl directly.
  if (group_interceptor_) {
    auto interceptor_result = group_interceptor_->InterceptModuleMethod(
        shared_from_this(), method, rt, delegate_, args, count);
    if (interceptor_result.handled) {
      return std::move(interceptor_result.result);
    }
  }
  uint64_t start_time = base::CurrentSystemTimeMilliseconds();
  const std::string& method_name = method.name;
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(wrapper_);
  if (local_ref.IsNull()) {
    LOGE("LynxModuleAndroid::invokeMethod failed: local_ref isNull()");
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "LynxModuleAndroid::invokeMethod failed: local_ref isNull()"));
  }
  if (methodsByName_.find(method_name) == methodsByName_.end()) {
    LOGE("LynxModuleAndroid::invokeMethod. Method not found. " << method_name);
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "LynxModuleAndroid::invokeMethod. Method not found. " + method_name));
  }
  std::string first_param_str;
  if (count > 0 && args && args[0].isString()) {
    first_param_str = args[0].getString(*rt).utf8(*rt);
  }
  piper::NativeModuleInfoCollectorPtr timing_collector =
      std::make_shared<piper::NativeModuleInfoCollector>(
          delegate_, name_, method_name, first_param_str);
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_func_call_start",
      [&first_param_str, start_time](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg", first_param_str);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(start_time));
      });
  // We need these information to monitor network request information,
  // the rate of success and the proportion of requests accomplished by
  // Lynx. After fully switch to Lynx Network, we can remove these logics.
  // TODO(liyanbo.monster): when this is inherit from NativeModule, delete this.
  //
  {
    auto value_factory = std::make_shared<pub::PubValueFactoryDefault>();
    auto args_array = value_factory->CreateArray();
    // args is a pointer array. can not use getArray to read value.
    for (size_t i = 0; i < count; i++) {
      const lynx::piper::Value* arg = &args[i];
      if (arg->isBool()) {
        args_array->PushBoolToArray(arg->getBool());
      } else if (arg->isNumber()) {
        args_array->PushDoubleToArray(arg->getNumber());
      } else if (arg->isNull()) {
        args_array->PushNullToArray();
      } else if (arg->isString()) {
        args_array->PushStringToArray(arg->getString(*rt).utf8(*rt));
      } else if (arg->isObject()) {
        lynx::piper::Object o = arg->getObject(*rt);
        if (o.isArray(*rt)) {
          auto sub_arr = o.getArray(*rt);
          auto sub_arr_result = pub::ValueUtils::ConvertPiperArrayToPubValue(
              *rt, sub_arr, value_factory);
          args_array->PushValueToArray(std::move(sub_arr_result));
        } else if (o.isArrayBuffer(*rt)) {
          size_t length;
          args_array->PushArrayBufferToArray(
              pub::ValueUtils::ConvertPiperToArrayBuffer(*rt, o, length),
              length);
        } else if (o.isFunction(*rt)) {
          // ignore
        } else {
          std::string r;
          auto ret =
              pub::ValueUtils::ConvertBigIntToStringIfNecessary(*rt, o, r);
          if (ret) {
            args_array->PushBigIntToArray(r);
            continue;
          }
          auto dict = pub::ValueUtils::ConvertPiperObjectToPubValue(
              *rt, o, value_factory);
          args_array->PushValueToArray(std::move(dict));
        }
      }
    }
    network::SetNetworkCallbackInfo(method.name, args_array, count,
                                    timing_collector);
  }
  auto method_invoker = methodsByName_[method_name];
  auto value = method_invoker->Invoke(
      Java_LynxModuleWrapper_getModule(env, local_ref.Get()).Get(), rt, args,
      count, timing_collector);

  if (tasm::LynxEnv::GetInstance().IsPiperMonitorEnabled()) {
    tasm::LynxEnv::onPiperInvoked(name_, method_name, first_param_str, "", "");
  }
  timing_collector->EndCallFunc(start_time);
  if (!value.has_value()) {
    LOGE("Exception happen in LynxModuleAndroid invokeMethod: " + name_
         << "." << method_name << " , args: " << first_param_str);
  }
  return value;
}

piper::Value LynxModuleAndroid::getAttributeValue(Runtime* rt,
                                                  std::string propName) {
  piper::Scope scope(*rt);
  auto p = attributeByName_.find(propName);
  if (p != attributeByName_.end()) {
    auto descriptor = p->second;
    JNIEnv* env = base::android::AttachCurrentThread();
    auto value_arr =
        jsArrayFromJavaOnlyArray(env, descriptor.getValue().Get(), rt);
    if (value_arr) {
      auto ret = (*value_arr).getValueAtIndex(*rt, 0);
      if (ret) {
        return std::move(*ret);
      }
    }
  }
  return piper::Value::undefined();
}

void LynxModuleAndroid::buildMap(
    JNIEnv* env, const std::shared_ptr<ModuleDelegate>& delegate,
    lynx::base::android::ScopedLocalJavaRef<jobject>& descriptions,
    std::unordered_map<std::string, std::shared_ptr<MethodMetadata>>& map,
    std::unordered_map<std::string, std::shared_ptr<MethodInvoker>>&
        invokeMapByName_) {
  jclass clsArrayList = env->GetObjectClass(descriptions.Get());
  jmethodID arrayList_Get =
      env->GetMethodID(clsArrayList, "get", "(I)Ljava/lang/Object;");
  jmethodID arrayList_Size = env->GetMethodID(clsArrayList, "size", "()I");
  env->DeleteLocalRef(clsArrayList);
  jint moduleLen = env->CallIntMethod(descriptions.Get(), arrayList_Size);
  for (int i = 0; i < moduleLen; i++) {
    jobject description_wrapper =
        env->CallObjectMethod(descriptions.Get(), arrayList_Get, i);
    if (description_wrapper == nullptr) {
      LOGE("Module Description is null. module name: " << name_);
      continue;
    }
    JavaMethodDescriptor descriptor(env, description_wrapper);
    const std::string methodName = descriptor.getName();
    std::shared_ptr<MethodMetadata> metadata =
        std::make_shared<MethodMetadata>(0, methodName);
    map[methodName] = metadata;
    invokeMapByName_[methodName] = std::make_shared<MethodInvoker>(
        descriptor.getMethod().Get(), descriptor.getSignature(), name_,
        methodName, delegate);
    env->DeleteLocalRef(description_wrapper);
  }
}

void LynxModuleAndroid::buildAttributeMap(
    JNIEnv* env, lynx::base::android::ScopedLocalJavaRef<jobject>& descriptions,
    std::unordered_map<std::string, JavaAttributeDescriptor>& map) {
  jclass clsArrayList = env->GetObjectClass(descriptions.Get());
  jmethodID arrayList_Get =
      env->GetMethodID(clsArrayList, "get", "(I)Ljava/lang/Object;");
  jmethodID arrayList_Size = env->GetMethodID(clsArrayList, "size", "()I");
  env->DeleteLocalRef(clsArrayList);
  jint moduleLen = env->CallIntMethod(descriptions.Get(), arrayList_Size);
  for (int i = 0; i < moduleLen; i++) {
    jobject descriptionWrapper =
        env->CallObjectMethod(descriptions.Get(), arrayList_Get, i);
    JavaAttributeDescriptor descriptor(env, descriptionWrapper);
    auto name = descriptor.getName();
    map[name] = descriptor;
    env->DeleteLocalRef(descriptionWrapper);
  }
}

#if ENABLE_TESTBENCH_RECORDER
void LynxModuleAndroid::StartRecordFunction(const std::string& method_name) {
  auto method_invoke = methodsByName_.find(method_name);
  if (method_invoke != methodsByName_.end()) {
    method_invoke->second->StartRecordFunction();
  }
}

void LynxModuleAndroid::EndRecordFunction(const std::string& method_name,
                                          size_t count,
                                          const piper::Value* js_args,
                                          Runtime* rt, piper::Value& res) {
  auto method_invoke = methodsByName_.find(method_name);
  if (method_invoke != methodsByName_.end()) {
    method_invoke->second->EndRecordFunction(count, js_args, rt, res);
  }
}

void LynxModuleAndroid::SetRecordID(int64_t record_id) {
  LynxModule::SetRecordID(record_id);
  for (auto p : methodsByName_) {
    p.second->SetRecordID(record_id_);
  }
}
#endif

}  // namespace piper
}  // namespace lynx
