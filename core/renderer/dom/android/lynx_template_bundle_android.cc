// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/android/lynx_template_bundle_android.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/base/android/jni_helper.h"
#include "core/build/gen/TemplateBundle_jni.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "core/runtime/jscache/js_cache_manager_facade.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"

jlong ParseTemplate(JNIEnv* env, jclass jcaller, jbyteArray j_binary,
                    jobjectArray j_buffer) {
  auto binary =
      lynx::base::android::JNIConvertHelper::ConvertJavaBinary(env, j_binary);
  auto reader =
      lynx::tasm::LynxBinaryReader::CreateLynxBinaryReader(std::move(binary));
  if (reader.Decode()) {
    // decode success.
    lynx::tasm::LynxTemplateBundle* bundle =
        new lynx::tasm::LynxTemplateBundle(reader.GetTemplateBundle());
    return reinterpret_cast<int64_t>(bundle);
  } else {
    // decode failed.
    LOGE("ParseTemplate failed. error_msg is : " << reader.error_message_);
    auto j_err_str =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
            env, reader.error_message_);
    env->SetObjectArrayElement(j_buffer, 0, j_err_str.Get());
    return 0;
  }
}

jobject GetExtraInfo(JNIEnv* env, jclass jcaller, jlong ptr) {
  auto bundle = reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  if (bundle) {
    lynx::lepus::Value extra_info = bundle->GetExtraInfo();
    lynx::tasm::LepusEncoder encoder;
    std::vector<int8_t> encoded_data = encoder.EncodeMessage(extra_info);
    auto buffer =
        env->NewDirectByteBuffer(encoded_data.data(), encoded_data.size());
    auto result_from_java = Java_TemplateBundle_decodeByteBuffer(env, buffer);
    return env->NewLocalRef(result_from_java.Get());  // NOLINT
  }
  return nullptr;
}

jboolean GetContainsElementTree(JNIEnv* env, jclass jcaller, jlong ptr) {
  auto bundle = reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  if (bundle) {
    return bundle->GetContainsElementTree();
  }
  return false;
}

void ReleaseBundle(JNIEnv* env, jclass jcaller, jlong ptr) {
  auto bundle = reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  delete bundle;
}

void PostJsCacheGenerationTask(JNIEnv* env, jclass jcaller, jlong bundle,
                               jstring bytecodeSourceUrl, jboolean useV8) {
  std::string template_url =
      lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                             bytecodeSourceUrl);
  lynx::tasm::LynxTemplateBundle* template_bundle =
      reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(bundle);
  lynx::piper::cache::JsCacheManagerFacade::PostCacheGenerationTask(
      *template_bundle, template_url,
      useV8 ? lynx::piper::JSRuntimeType::v8
            : lynx::piper::JSRuntimeType::quickjs);
}

jboolean ConstructContext(JNIEnv* env, jclass jcaller, jlong ptr, jint count) {
  lynx::tasm::LynxTemplateBundle* bundle =
      reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  return bundle && bundle->PrepareLepusContext(count);
}

void InitWithOption(JNIEnv* env, jclass jcaller, jlong ptr,
                    jint context_pool_size,
                    jboolean enable_context_auto_generate) {
  lynx::tasm::LynxTemplateBundle* bundle =
      reinterpret_cast<lynx::tasm::LynxTemplateBundle*>(ptr);
  if (!bundle) {
    return;
  }
  bundle->PrepareLepusContext(context_pool_size);
  bundle->SetEnableVMAutoGenerate(enable_context_auto_generate);
}

namespace lynx {
namespace tasm {
bool LynxTemplateBundleAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

lynx::base::android::ScopedLocalJavaRef<jobject>
ConstructJTemplateBundleFromNative(LynxTemplateBundle bundle) {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto* native_bundle_ptr = new tasm::LynxTemplateBundle(std::move(bundle));
  return Java_TemplateBundle_fromNative(
      env, reinterpret_cast<int64_t>(native_bundle_ptr));
}
}  // namespace tasm
}  // namespace lynx
