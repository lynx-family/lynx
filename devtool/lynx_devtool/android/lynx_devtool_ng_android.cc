// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/lynx_devtool/android/lynx_devtool_ng_android.h"

#include <jni.h>

#include <memory>

#include "core/base/android/jni_helper.h"
#include "core/renderer/tasm/config.h"
#include "devtool/lynx_devtool/android/devtool_message_handler_android.h"
#include "devtool/lynx_devtool/android/invoke_cdp_from_sdk_sender_android.h"
#include "devtool/lynx_devtool/build/gen/LynxDevToolNGDelegate_jni.h"

jlong CreateLynxDevToolNG(JNIEnv* env, jobject jcaller) {
  auto* ptr = new lynx::devtool::LynxDevToolNGAndroid();
  auto sp_ptr = new std::shared_ptr<lynx::devtool::LynxDevToolNG>(ptr);
  return reinterpret_cast<jlong>(sp_ptr);
}

void SendMessageToDebugPlatform(JNIEnv* env, jobject jcaller, jlong nativePtr,
                                jstring type, jstring msg) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  lynx_devtool->SendMessageToDebugPlatform(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, type),
      lynx::base::android::JNIConvertHelper::ConvertToString(env, msg));
}

jint AttachToDebug(JNIEnv* env, jobject jcaller, jlong nativePtr, jstring url) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  return lynx_devtool->Attach(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, url));
}

void DetachToDebug(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  lynx_devtool->Detach();
}

jlong OnBackgroundRuntimeCreated(JNIEnv* env, jobject jcaller, jlong nativePtr,
                                 jstring groupName) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  auto observer = lynx_devtool->OnBackgroundRuntimeCreated(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, groupName));
  return reinterpret_cast<jlong>(
      new std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>(observer));
}

void OnTasmCreated(JNIEnv* env, jobject jcaller, jlong nativePtr,
                   jlong shellPtr) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  lynx_devtool->OnTasmCreated(shellPtr);
}

void Destroy(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  auto lynx_devtool_ptr =
      reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  delete lynx_devtool_ptr;
}

void SetDevToolPlatformAbility(JNIEnv* env, jobject jcaller, jlong nativePtr,
                               jlong platformNativePtr) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  auto platform_facade =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::DevToolPlatformFacade>*>(
          platformNativePtr);
  lynx_devtool->SetDevToolPlatformFacade(platform_facade);
}

void SubscribeMessage(JNIEnv* env, jobject jcaller, jlong nativePtr,
                      jstring type, jobject handler) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  lynx_devtool->SubscribeMessage(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, type),
      std::make_unique<lynx::devtool::DevToolMessageHandlerAndroid>(env,
                                                                    handler));
}

static void UnSubscribeMessage(JNIEnv* env, jobject jcaller, jlong nativePtr,
                               jstring type) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);

  lynx_devtool->UnSubscribeMessage(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, type));
}

void InvokeCDPFromSDK(JNIEnv* env, jobject jcaller, jlong nativePtr,
                      jstring cdpMsg, jobject callback) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  lynx_devtool->DispatchMessage(
      std::make_shared<lynx::devtool::InvokeCDPFromSDKSenderAndroid>(env,
                                                                     callback),
      "CDP",
      lynx::base::android::JNIConvertHelper::ConvertToString(env, cdpMsg));
}

void UpdateDevice(JNIEnv* env, jobject jcaller, jint width, jint height,
                  jfloat density) {
  lynx::tasm::Config::InitializeVersion(
      std::to_string(android_get_device_api_level()));
  lynx::tasm::Config::InitPixelValues(width, height, density);
}

namespace lynx {
namespace devtool {

bool LynxDevToolNGAndroid ::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace devtool

}  // namespace lynx
