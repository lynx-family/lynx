// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/lynx_node_info_result_android.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"

namespace lynx {
namespace shell {
namespace {

constexpr const char kNodeInfoItemClassName[] =
    "com/lynx/tasm/LynxNodeInfoItem";
constexpr const char kNodeInfoResultClassName[] =
    "com/lynx/tasm/LynxNodeInfoResult";
constexpr const char kNodeInfoItemCreateSignature[] =
    "(ILjava/lang/String;Ljava/lang/String;)Lcom/lynx/tasm/LynxNodeInfoItem;";
constexpr const char kNodeInfoResultCreateSignature[] =
    "([Lcom/lynx/tasm/LynxNodeInfoItem;)Lcom/lynx/tasm/LynxNodeInfoResult;";

lynx::base::android::ScopedLocalJavaRef<jobject> BuildNodeInfoItem(
    JNIEnv* env, const tasm::TemplateAssembler::NodeInfo& node_info) {
  auto item_class = base::android::GetClass(env, kNodeInfoItemClassName);
  auto create_method = env->GetStaticMethodID(item_class.Get(), "create",
                                              kNodeInfoItemCreateSignature);

  lynx::base::android::ScopedLocalJavaRef<jstring> entry_url;
  if (!node_info.entry_url.empty()) {
    entry_url = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, node_info.entry_url);
  }

  lynx::base::android::ScopedLocalJavaRef<jstring> debug_metadata_url;
  if (!node_info.debug_metadata_url.empty()) {
    debug_metadata_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
            env, node_info.debug_metadata_url);
  }

  return lynx::base::android::ScopedLocalJavaRef<jobject>(
      env, env->CallStaticObjectMethod(item_class.Get(), create_method,
                                       node_info.sign, entry_url.Get(),
                                       debug_metadata_url.Get()));
}

lynx::base::android::ScopedLocalJavaRef<jobjectArray> BuildNodeInfoArray(
    JNIEnv* env,
    const std::vector<tasm::TemplateAssembler::NodeInfo>& node_infos) {
  auto item_class = base::android::GetClass(env, kNodeInfoItemClassName);
  auto node_info_array = lynx::base::android::ScopedLocalJavaRef<jobjectArray>(
      env, env->NewObjectArray(static_cast<jsize>(node_infos.size()),
                               item_class.Get(), nullptr));
  for (size_t index = 0; index < node_infos.size(); ++index) {
    auto item = BuildNodeInfoItem(env, node_infos[index]);
    env->SetObjectArrayElement(node_info_array.Get(), static_cast<jsize>(index),
                               item.Get());
  }
  return node_info_array;
}

}  // namespace

LynxNodeInfoResultAndroid::LynxNodeInfoResultAndroid(
    const std::vector<tasm::TemplateAssembler::NodeInfo>& node_infos) {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto result_class = base::android::GetClass(env, kNodeInfoResultClassName);
  auto create_method = env->GetStaticMethodID(result_class.Get(), "create",
                                              kNodeInfoResultCreateSignature);
  auto node_info_array = BuildNodeInfoArray(env, node_infos);
  jni_object_.Reset(
      env, env->CallStaticObjectMethod(result_class.Get(), create_method,
                                       node_info_array.Get()));
}

}  // namespace shell
}  // namespace lynx
