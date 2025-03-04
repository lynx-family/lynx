// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/android/lynx_env_android.h"

#include <string>
#include <utility>

#include "core/base/android/jni_helper.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/build/gen/LynxEnv_jni.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/jscache/js_cache_manager_facade.h"
#include "core/services/ssr/ssr_type_info.h"
#include "core/services/timing_handler/timing.h"

namespace lynxenv {

jstring GetSSRApiVersion(JNIEnv* env, jobject jcaller) {
  return env->NewStringUTF(lynx::ssr::kSSRCurrentApiVersion);  // NOLINT
}

void PrepareLynxGlobalPool(JNIEnv* env, jclass jcaller) {
  lynx::tasm::LynxGlobalPool::GetInstance().PreparePool();
}

void SetLocalEnv(JNIEnv* env, jobject jcaller, jstring key, jstring value) {
  lynx::tasm::LynxEnv::GetInstance().SetLocalEnv(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key),
      lynx::base::android::JNIConvertHelper::ConvertToString(env, value));
}

void CleanExternalCache(JNIEnv* env, jobject jcaller) {
  lynx::tasm::LynxEnv::GetInstance().CleanExternalCache();
}

void SetGroupedEnv(JNIEnv* env, jobject jcaller, jstring key, jboolean value,
                   jstring group_key) {
  lynx::tasm::LynxEnv::GetInstance().SetGroupedEnv(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key),
      value == JNI_TRUE,
      lynx::base::android::JNIConvertHelper::ConvertToString(env, group_key));
}

void SetGroupedEnvWithGroupSet(JNIEnv* env, jobject jcaller, jstring group_key,
                               jobject set) {
  lynx::tasm::LynxEnv::GetInstance().SetGroupedEnv(
      lynx::base::android::JNIConvertHelper::ConvertJavaStringSetToSTLStringSet(
          env, set),
      lynx::base::android::JNIConvertHelper::ConvertToString(env, group_key));
}

jstring GetDebugEnvDescription(JNIEnv* env, jobject jcaller) {
  std::string envJsonString =
      lynx::tasm::LynxEnv::GetInstance().GetDebugDescription();
  return lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
             env, envJsonString)
      .Get();
}

void SetEnvMask(JNIEnv* env, jobject jcaller, jstring key, jboolean value) {
  lynx::tasm::LynxEnv::GetInstance().SetEnvMask(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key),
      value == JNI_TRUE);
}

void InitUIThread(JNIEnv* env, jclass jcaller) { lynx::base::UIThread::Init(); }

void RunJavaTaskOnConcurrentLoop(JNIEnv* env, jclass jcaller, jlong task_id,
                                 jint task_type) {
  lynx::base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [task_id, task_type]() {
        lynxenv::Java_LynxEnv_onJavaTaskOnConcurrentLoop(
            lynx::base::android::AttachCurrentThread(), task_id, task_type);
      },
      static_cast<lynx::base::ConcurrentTaskType>(task_type));
}

void ClearBytecode(JNIEnv* env, jclass jcaller, jstring bytecodeSourceUrl,
                   jboolean useV8) {
  std::string template_url =
      lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                             bytecodeSourceUrl);
  lynx::piper::cache::JsCacheManagerFacade::ClearBytecode(
      template_url, useV8 ? lynx::piper::JSRuntimeType::v8
                          : lynx::piper::JSRuntimeType::quickjs);
}

}  // namespace lynxenv

namespace lynx {
namespace tasm {

void LynxEnvAndroid::RegisterJNI(JNIEnv* env) {
  lynx::tasm::Config::InitializeVersion(
      std::to_string(android_get_device_api_level()));
  (void)lynxenv::RegisterNativesImpl(env);
}

void LynxEnvAndroid::onPiperInvoked(const std::string& module_name,
                                    const std::string& method_name,
                                    const std::string& param_str,
                                    const std::string& url) {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_module =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, module_name);
  auto j_method =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, method_name);
  auto j_param =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, param_str);
  auto j_url = base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  lynxenv::Java_LynxEnv_reportPiperInvoked(env, j_module.Get(), j_method.Get(),
                                           j_param.Get(), j_url.Get());
}

std::optional<std::string> LynxEnvAndroid::GetStringFromExternalEnv(
    const std::string& key) {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_key = base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  auto value = lynxenv::Java_LynxEnv_getStringFromExternalEnv(env, j_key.Get());
  if (value.Get() == nullptr) {
    return std::nullopt;
  } else {
    return base::android::JNIConvertHelper::ConvertToString(env, value.Get());
  }
}

}  // namespace tasm
}  // namespace lynx
