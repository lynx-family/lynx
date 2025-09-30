// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <jni.h>

#include <memory>

#include "core/base/android/jni_helper.h"
#include "core/renderer/tasm/config.h"
#include "devtool/lynx_devtool/android/cdp_event_listener_sender_android.h"
#include "devtool/lynx_devtool/android/devtool_message_handler_android.h"
#include "devtool/lynx_devtool/android/invoke_cdp_from_sdk_sender_android.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/LynxDevToolNGDelegate_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/LynxDevToolNGDelegate_register_jni.h"

jlong CreateLynxDevToolNG(JNIEnv* env, jobject jcaller, jboolean debuggable) {
  auto* ptr = new lynx::devtool::LynxDevToolNG(static_cast<bool>(debuggable));
  auto sp_ptr = new std::shared_ptr<lynx::devtool::LynxDevToolNG>(ptr);
  return reinterpret_cast<jlong>(sp_ptr);
}

void SetTag(JNIEnv* env, jobject jcaller, jlong nativePtr, jstring debugTag) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  lynx_devtool->SetTag(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, debugTag));
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

void AddCDPEventListener(JNIEnv* env, jobject jcaller, jlong nativePtr,
                         jstring name, jobject listener) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  lynx_devtool->AddCDPEventListener(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name),
      std::make_shared<lynx::devtool::CDPEventListenerSenderAndroid>(env,
                                                                     listener));
}

void RemoveCDPEventListener(JNIEnv* env, jobject jcaller, jlong nativePtr,
                            jstring name) {
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          nativePtr);
  lynx_devtool->RemoveCDPEventListener(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name));
}

void UpdateDevice(JNIEnv* env, jobject jcaller, jint width, jint height,
                  jfloat density) {
  lynx::tasm::Config::InitializeVersion(
      std::to_string(android_get_device_api_level()));
  lynx::tasm::Config::InitPixelValues(width, height, density);
}

namespace lynx {
namespace devtool {
namespace jni {

bool RegisterJNIForLynxDevToolNGDelegate(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace devtool
}  // namespace lynx
