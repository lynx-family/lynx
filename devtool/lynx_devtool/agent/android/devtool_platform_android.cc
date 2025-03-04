// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/android/devtool_platform_android.h"

#include <sys/system_properties.h>

#include "core/base/android/jni_helper.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"
#include "devtool/lynx_devtool/build/gen/DevToolPlatformAndroidDelegate_jni.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"

jlong CreateDevToolPlatformFacade(JNIEnv* env, jobject jcaller) {
  auto* ptr = new lynx::devtool::DevToolPlatformAndroid(env, jcaller);
  auto* sp_ptr =
      new std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>(ptr);
  return reinterpret_cast<jlong>(sp_ptr);
}

void DestroyDevToolPlatformFacade(JNIEnv* env, jobject jcaller,
                                  jlong facadePtr) {
  auto* sp_ptr =
      reinterpret_cast<std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(
          facadePtr);
  (*sp_ptr)->Destroy();
  delete sp_ptr;
}

void FlushConsoleMessages(JNIEnv* env, jobject jcaller, jlong facadePtr) {
  auto platform_facade = *reinterpret_cast<
      std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(facadePtr);
  auto debugger = platform_facade->GetJSDebugger().lock();
  CHECK_NULL_AND_LOG_RETURN(debugger, "js debug: debugger is null");
  debugger->FlushConsoleMessages();
}

void GetConsoleObject(JNIEnv* env, jobject jcaller, jlong facadePtr,
                      jstring objectId, jboolean needStringify,
                      jint callbackId) {
  auto platform_facade = *reinterpret_cast<
      std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(facadePtr);
  auto debugger = platform_facade->GetJSDebugger().lock();
  CHECK_NULL_AND_LOG_RETURN(debugger, "js debug: debugger is null");
  debugger->GetConsoleObject(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, objectId),
      needStringify, callbackId);
}

void SendPageScreencastFrameEvent(JNIEnv* env, jobject jcaller, jlong facadePtr,
                                  jstring data, jfloat offsetTop,
                                  jfloat pageScaleFactor, jfloat deviceWidth,
                                  jfloat deviceHeight, jfloat scrollOffsetX,
                                  jfloat scrollOffsetY, jfloat timestamp) {
  auto platform_facade = *reinterpret_cast<
      std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(facadePtr);
  std::shared_ptr<lynx::devtool::ScreenMetadata> metadata =
      std::make_shared<lynx::devtool::ScreenMetadata>();
  metadata->offset_top_ = offsetTop;
  metadata->page_scale_factor_ = pageScaleFactor;
  metadata->device_width_ = deviceWidth;
  metadata->device_height_ = deviceHeight;
  metadata->scroll_off_set_x_ = scrollOffsetX;
  metadata->scroll_off_set_y_ = scrollOffsetY;
  metadata->timestamp_ = timestamp;
  platform_facade->SendPageScreencastFrameEvent(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, data),
      metadata);
}

void SendPageFrameNavigatedEvent(JNIEnv* env, jobject jcaller, jlong facadePtr,
                                 jstring url) {
  auto platform_facade = *reinterpret_cast<
      std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(facadePtr);
  platform_facade->SendPageFrameNavigatedEvent(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, url));
}

void DispatchScreencastVisibilityChanged(JNIEnv* env, jobject jcaller,
                                         jlong facadePtr, jboolean status) {
  auto platform_facade = *reinterpret_cast<
      std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(facadePtr);
  platform_facade->SendPageScreencastVisibilityChangedEvent(status);
}

void SendLynxScreenshotCapturedEvent(JNIEnv* env, jobject jcaller,
                                     jlong facadePtr, jstring data) {
  auto platform_facade = *reinterpret_cast<
      std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(facadePtr);
  platform_facade->SendLynxScreenshotCapturedEvent(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, data));
}

void SendConsoleEvent(JNIEnv* env, jobject jcaller, jlong facadePtr,
                      jstring text, jint level, jlong timestamp) {
  auto platform_facade = *reinterpret_cast<
      std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(facadePtr);
  platform_facade->SendConsoleEvent(
      {lynx::base::android::JNIConvertHelper::ConvertToString(env, text), level,
       timestamp});
}

void SendLayerTreeDidChangeEvent(JNIEnv* env, jobject jcaller,
                                 jlong facadePtr) {
  auto platform_facade = *reinterpret_cast<
      std::shared_ptr<lynx::devtool::DevToolPlatformAndroid>*>(facadePtr);
  platform_facade->SendLayerTreeDidChangeEvent();
}

namespace lynx {
namespace devtool {

bool DevToolPlatformAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
DevToolPlatformAndroid::DevToolPlatformAndroid(JNIEnv* env, jobject owner)
    : weak_android_delegate_(env, owner) {}

int DevToolPlatformAndroid::FindNodeIdForLocation(
    float x, float y, std::string screen_shot_mode) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto jni_mode = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
      env, screen_shot_mode);
  return Java_DevToolPlatformAndroidDelegate_findNodeIdForLocationFromUI(
      env, weak_android_delegate_.Get(), x, y, jni_mode.Get());
}

void DevToolPlatformAndroid::ScrollIntoView(int node_id) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  return Java_DevToolPlatformAndroidDelegate_scrollIntoViewFromUI(
      env, weak_android_delegate_.Get(), node_id);
}

std::vector<float> DevToolPlatformAndroid::GetRectToWindow() const {
  std::vector<float> res;
  base::android::ScopedLocalJavaRef<jobject> local_ref(weak_android_delegate_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jfloatArray> result =
      Java_DevToolPlatformAndroidDelegate_getRectToWindow(
          env, weak_android_delegate_.Get());
  jsize size = env->GetArrayLength(result.Get());
  jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
  for (auto i = 0; i < size; i++) {
    res.push_back(arr[i]);
  }
  env->ReleaseFloatArrayElements(result.Get(), arr, 0);
  return res;
}

std::string DevToolPlatformAndroid::GetLynxVersion() const {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto lynx_version = Java_DevToolPlatformAndroidDelegate_getLynxVersion(env);
  return lynx::base::android::JNIConvertHelper::ConvertToString(
      env, lynx_version.Get());
}

void DevToolPlatformAndroid::SetDevToolSwitch(const std::string& key,
                                              bool value) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto jniKey =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  Java_DevToolPlatformAndroidDelegate_setDevToolSwitch(env, jniKey.Get(),
                                                       value);
}

void DevToolPlatformAndroid::EmulateTouch(
    std::shared_ptr<lynx::devtool::MouseEvent> input) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  int x = input->x_;
  int y = input->y_;
  lynx::base::android::ScopedLocalJavaRef<jstring> type =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
          env, input->type_);
  float deltaX = input->delta_x_;
  float deltaY = input->delta_y_;
  lynx::base::android::ScopedLocalJavaRef<jstring> button =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
          env, input->button_);

  Java_DevToolPlatformAndroidDelegate_emulateTouch(
      env, ref.Get(), type.Get(), x, y, deltaX, deltaY, button.Get());
}

void DevToolPlatformAndroid::OnConsoleMessage(const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  if (!env->IsSameObject(ref.Get(), nullptr)) {
    auto jni_msg = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, message);
    Java_DevToolPlatformAndroidDelegate_onConsoleMessage(env, ref.Get(),
                                                         jni_msg.Get());
  }
}

void DevToolPlatformAndroid::OnConsoleObject(const std::string& detail,
                                             int callback_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  if (!env->IsSameObject(ref.Get(), nullptr)) {
    auto jni_detail =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env,
                                                                     detail);
    Java_DevToolPlatformAndroidDelegate_onConsoleObject(
        env, ref.Get(), jni_detail.Get(), callback_id);
  }
}

std::string DevToolPlatformAndroid::GetLepusDebugInfo(const std::string& url) {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  if (!env->IsSameObject(ref.Get(), nullptr)) {
    auto jni_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
    auto jni_debug_info = Java_DevToolPlatformAndroidDelegate_getLepusDebugInfo(
        env, ref.Get(), jni_url.Get());
    return lynx::base::android::JNIConvertHelper::ConvertToString(
        env, jni_debug_info.Get());
  }
  return "";
}

void DevToolPlatformAndroid::SetLepusDebugInfoUrl(const std::string& url) {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  if (!env->IsSameObject(ref.Get(), nullptr)) {
    auto jni_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
    Java_DevToolPlatformAndroidDelegate_setLepusDebugInfoUrl(env, ref.Get(),
                                                             jni_url.Get());
  }
}

// OnConsoleObject and OnConsoleMessage are called from the JS thread, and
// Destroy is called from the UI thread, so we need to lock in these functions
// and reset weak_android_delegate_ when destroying.
void DevToolPlatformAndroid::Destroy() {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  weak_android_delegate_.Reset(env, nullptr);
}

void DevToolPlatformAndroid::StartScreenCast(ScreenshotRequest request) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  std::string mode = lynx::devtool::DevToolStatus::GetInstance().GetStatus(
      lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      lynx::devtool::DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);
  auto jni_mode =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, mode);
  Java_DevToolPlatformAndroidDelegate_startCasting(
      env, weak_android_delegate_.Get(), request.quality_, request.max_width_,
      request.max_height_, jni_mode.Get());
}

void DevToolPlatformAndroid::StopScreenCast() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_DevToolPlatformAndroidDelegate_stopCasting(env,
                                                  weak_android_delegate_.Get());
}

void DevToolPlatformAndroid::GetLynxScreenShot() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_DevToolPlatformAndroidDelegate_sendCardPreview(
      env, weak_android_delegate_.Get());
}

void DevToolPlatformAndroid::OnAckReceived() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_DevToolPlatformAndroidDelegate_onAckReceived(
      env, weak_android_delegate_.Get());
}

void DevToolPlatformAndroid::OnReceiveTemplateFragment(const std::string& data,
                                                       bool eof) {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  if (!env->IsSameObject(ref.Get(), nullptr)) {
    auto jni_data =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, data);
    Java_DevToolPlatformAndroidDelegate_onReceiveTemplateFragment(
        env, ref.Get(), jni_data.Get(), eof);
  }
}

std::vector<int32_t> DevToolPlatformAndroid::GetViewLocationOnScreen() const {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  std::vector<int32_t> res;
  lynx::base::android::ScopedLocalJavaRef<jintArray> result =
      Java_DevToolPlatformAndroidDelegate_getViewLocationOnScreen(env,
                                                                  ref.Get());
  jsize size = env->GetArrayLength(result.Get());
  jint* arr = env->GetIntArrayElements(result.Get(), JNI_FALSE);
  for (auto i = 0; i < size; i++) {
    res.push_back(arr[i]);
  }
  env->ReleaseIntArrayElements(result.Get(), arr, 0);
  return res;
}

void DevToolPlatformAndroid::SendEventToVM(const std::string& vm_type,
                                           const std::string& event_name,
                                           const std::string& data) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  auto j_vm_type = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
      env, vm_type);
  auto j_event_name =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env,
                                                                   event_name);
  auto j_data =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, data);
  Java_DevToolPlatformAndroidDelegate_sendEventToVM(
      env, ref.Get(), j_vm_type.Get(), j_event_name.Get(), j_data.Get());
}

lynx::lepus::Value* DevToolPlatformAndroid::GetLepusValueFromTemplateData() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  jlong template_data_ptr =
      Java_DevToolPlatformAndroidDelegate_getTemplateDataPtr(
          env, weak_android_delegate_.Get());
  if (template_data_ptr != 0) {
    return reinterpret_cast<lynx::lepus::Value*>(template_data_ptr);
  }
  return nullptr;
}

std::string DevToolPlatformAndroid::GetTemplateJsInfo(int32_t offset,
                                                      int32_t size) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto template_js = Java_DevToolPlatformAndroidDelegate_getTemplateJsInfo(
      env, weak_android_delegate_.Get(), offset, size);
  return lynx::base::android::JNIConvertHelper::ConvertToString(
      env, template_js.Get());
}

std::string DevToolPlatformAndroid::GetUINodeInfo(int id) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto ui_node_info = Java_DevToolPlatformAndroidDelegate_getUINodeInfo(
      env, weak_android_delegate_.Get(), id);
  return lynx::base::android::JNIConvertHelper::ConvertToString(
      env, ui_node_info.Get());
}

std::string DevToolPlatformAndroid::GetLynxUITree() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto lynx_ui_tree = Java_DevToolPlatformAndroidDelegate_getLynxUITree(
      env, weak_android_delegate_.Get());
  return lynx::base::android::JNIConvertHelper::ConvertToString(
      env, lynx_ui_tree.Get());
}

int DevToolPlatformAndroid::SetUIStyle(int id, std::string name,
                                       std::string content) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto jni_name =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, name);
  auto jni_content =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env,
                                                                   content);
  return Java_DevToolPlatformAndroidDelegate_setUIStyle(
      env, weak_android_delegate_.Get(), id, jni_name.Get(), jni_content.Get());
}

std::vector<double> DevToolPlatformAndroid::GetBoxModel(
    tasm::Element* element) {
  if (element->GetTag() == "x-overlay-ng") {
    return ElementInspector::GetOverlayNGBoxModel(element);
  }
  auto box_model = GetBoxModelInGeneralPlatform(element);
  return box_model;
}

std::vector<float> DevToolPlatformAndroid::GetTransformValue(
    int id, const std::vector<float>& pad_border_margin_layout) {
  std::vector<float> res;

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jfloatArray> j_pad_border_margin_layout(
      env, env->NewFloatArray(pad_border_margin_layout.size()));
  env->SetFloatArrayRegion(j_pad_border_margin_layout.Get(), 0,
                           pad_border_margin_layout.size(),
                           &pad_border_margin_layout[0]);
  base::android::ScopedLocalJavaRef<jfloatArray> result =
      Java_DevToolPlatformAndroidDelegate_getTransformValue(
          env, weak_android_delegate_.Get(), id,
          j_pad_border_margin_layout.Get());
  jsize size = env->GetArrayLength(result.Get());
  jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
  for (auto i = 0; i < size; i++) {
    res.push_back(arr[i]);
  }
  env->ReleaseFloatArrayElements(result.Get(), arr, 0);
  return res;
}

void DevToolPlatformAndroid::PageReload(bool ignore_cache,
                                        std::string template_bin,
                                        bool from_template_fragments,
                                        int32_t template_size) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto jni_data = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
      env, template_bin);
  Java_DevToolPlatformAndroidDelegate_pageReload(
      env, weak_android_delegate_.Get(), ignore_cache, jni_data.Get(),
      from_template_fragments, template_size);
}

void DevToolPlatformAndroid::Navigate(const std::string& url) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_ref(
      weak_android_delegate_);
  auto jni_url =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  Java_DevToolPlatformAndroidDelegate_navigate(env, local_ref.Get(),
                                               jni_url.Get());
}

}  // namespace devtool
}  // namespace lynx
