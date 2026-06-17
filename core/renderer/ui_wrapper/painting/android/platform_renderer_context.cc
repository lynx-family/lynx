// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/event/event.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/shell/lynx_engine.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/android/lynx_android/src/main/jni/gen/PlatformRendererContext_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/PlatformRendererContext_register_jni.h"

jlong CreateEmbeddedViewContext(JNIEnv* env, jobject jcaller, jobject jThis) {
  return reinterpret_cast<jlong>(
      new lynx::tasm::PlatformRendererContext(env, jThis));
}

void InvokeUIMethodCallback(JNIEnv* env, jobject /*jcaller*/, jlong nativePtr,
                            jint callback, jint code, jobject params) {
  if (nativePtr == 0) {
    return;
  }
  lynx::lepus::Value data;
  if (params != nullptr) {
    auto params_array =
        lynx::tasm::android::ValueConverterAndroid::ConvertJavaOnlyArrayToLepus(
            env, params);
    if (params_array.IsArrayOrJSArray() && params_array.Array()->size() > 0) {
      data = params_array.Array()->get(0);
    }
  }
  reinterpret_cast<lynx::tasm::PlatformRendererContext*>(nativePtr)
      ->InvokeUIMethodCallback(callback, code, data);
}

namespace lynx {
namespace jni {
bool RegisterJNIForPlatformRendererContext(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
namespace tasm {
void PlatformRendererContext::CreatePlatformRenderer(
    int32_t id, PlatformRendererType type) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_createPlatformRenderer(
      env, local_ref.Get(), id, static_cast<int32_t>(type));
}

void PlatformRendererContext::CreatePlatformExtendedRenderer(
    int32_t id, const base::String& tag_name, jobject init_data) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_tag_name = base::android::JNIConvertHelper::ConvertToJNIStringUTF(
      env, tag_name.c_str());
  Java_PlatformRendererContext_createPlatformExtendedRenderer(
      env, local_ref.Get(), id, j_tag_name.Get(), init_data);
}

void PlatformRendererContext::InsertPlatformRenderer(int32_t parent,
                                                     int32_t child,
                                                     int32_t index) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_insertPlatformRenderer(env, local_ref.Get(),
                                                      parent, child, index);
}

void PlatformRendererContext::RemovePlatformRenderer(int32_t target) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_removePlatformRendererFromParent(
      env, local_ref.Get(), target);
}

void PlatformRendererContext::DestroyPlatformRenderer(int32_t target) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_destroyPlatformRenderer(env, local_ref.Get(),
                                                       target);

  // Unregister the renderer
  UnregisterPlatformRenderer(target);
}

void PlatformRendererContext::CreateImage(int32_t id, base::String src,
                                          float width, float height,
                                          int32_t event_mask) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_src =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, src.c_str());
  Java_PlatformRendererContext_createImage(
      env, local_ref.Get(), id, j_src.Get(), static_cast<int>(width),
      static_cast<int>(height), static_cast<int>(event_mask));
}

void PlatformRendererContext::DestroyImage(int32_t id) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_destroyImage(env, local_ref.Get(), id);
}

void PlatformRendererContext::UpdateTextBundle(int32_t id,
                                               intptr_t text_bundle) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_updateTextBundle(
      env, local_ref.Get(), id, static_cast<jlong>(text_bundle));
}

void PlatformRendererContext::DestroyTextBundle(int32_t id) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_destroyTextBundle(env, local_ref.Get(), id);
}

void PlatformRendererContext::InsertListItemPaintingNode(int32_t list_sign,
                                                         int32_t child_sign) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_insertListItemPaintingNode(
      env, local_ref.Get(), list_sign, child_sign);
}

void PlatformRendererContext::RemoveListItemPaintingNode(int32_t list_sign,
                                                         int32_t child_sign) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_removeListItemPaintingNode(
      env, local_ref.Get(), list_sign, child_sign);
}

void PlatformRendererContext::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_updateContentOffsetForListContainer(
      env, local_ref.Get(), container_id, content_size, delta_x, delta_y,
      is_init_scroll_offset, from_layout);
}

void PlatformRendererContext::FinishTasmOperation(int64_t operation_id) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_finishTasmOperation(
      env, local_ref.Get(), static_cast<jlong>(operation_id));
}

void PlatformRendererContext::FinishLayoutOperation(int32_t component_id,
                                                    int64_t operation_id,
                                                    bool is_first_screen) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_finishLayoutOperation(
      env, local_ref.Get(), component_id, static_cast<jlong>(operation_id),
      is_first_screen);
}

PlatformRendererAndroid* PlatformRendererContext::GetPlatformRenderer(
    int32_t id) {
  auto it = renderer_registry_.find(id);
  return (it != renderer_registry_.end()) ? it->second : nullptr;
}

void PlatformRendererContext::RegisterPlatformRenderer(
    int32_t id, PlatformRendererAndroid* renderer) {
  renderer_registry_[id] = renderer;
}

void PlatformRendererContext::UnregisterPlatformRenderer(int32_t id) {
  renderer_registry_.erase(id);
}

void PlatformRendererContext::UpdatePlatformRendererFrame(
    int32_t target, bool need_clip, const float* frame,
    const float* render_offset) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_updatePlatformRendererFrame(
      env, local_ref.Get(), target, need_clip, static_cast<jint>(frame[0]),
      static_cast<jint>(frame[1]), static_cast<jint>(frame[2]),
      static_cast<jint>(frame[3]), static_cast<jint>(render_offset[0]),
      static_cast<jint>(render_offset[1]));
}

void PlatformRendererContext::UpdatePlatformRendererAttributes(
    int32_t id, jobject prop_bundle) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull() || !prop_bundle) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_updatePlatformRendererAttributes(
      env, local_ref.Get(), id, prop_bundle);
}

int32_t PlatformRendererContext::GetTagInfo(const std::string& tag_name) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return 0;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> tag_ref =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, tag_name);
  return Java_PlatformRendererContext_getTagInfo(env, local_ref.Get(),
                                                 tag_ref.Get());
}

std::vector<float> PlatformRendererContext::GetRootViewLocationOnScreen() {
  std::vector<float> res;
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  auto arr = Java_PlatformRendererContext_getRootViewLocationOnScreen(
      env, local_ref.Get());
  if (arr.IsNull()) {
    return res;
  }

  const jsize size = env->GetArrayLength(arr.Get());
  jfloat* data = env->GetFloatArrayElements(arr.Get(), nullptr);
  if (data != nullptr && size > 0) {
    res.assign(data, data + size);
  }
  if (data != nullptr) {
    env->ReleaseFloatArrayElements(arr.Get(), data, 0);
  }
  return res;
}

std::vector<float> PlatformRendererContext::GetScreenSize() {
  std::vector<float> res;
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  auto arr = Java_PlatformRendererContext_getScreenSize(env, local_ref.Get());
  if (arr.IsNull()) {
    return res;
  }

  const jsize size = env->GetArrayLength(arr.Get());
  jfloat* data = env->GetFloatArrayElements(arr.Get(), nullptr);
  if (data != nullptr && size > 0) {
    res.assign(data, data + size);
  }
  if (data != nullptr) {
    env->ReleaseFloatArrayElements(arr.Get(), data, 0);
  }
  return res;
}

std::vector<float> PlatformRendererContext::GetRendererHostScrollOffset(
    int32_t sign) {
  std::vector<float> res;
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  auto arr = Java_PlatformRendererContext_getRendererHostScrollOffset(
      env, local_ref.Get(), sign);
  if (arr.IsNull()) {
    return res;
  }

  const jsize size = env->GetArrayLength(arr.Get());
  jfloat* data = env->GetFloatArrayElements(arr.Get(), nullptr);
  if (data != nullptr && size > 0) {
    res.assign(data, data + size);
  }
  if (data != nullptr) {
    env->ReleaseFloatArrayElements(arr.Get(), data, 0);
  }
  return res;
}

bool PlatformRendererContext::IsRendererHostScrollable(int32_t sign) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return false;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_PlatformRendererContext_isRendererHostScrollable(
      env, local_ref.Get(), sign);
}

void PlatformRendererContext::InvokeUIMethod(
    int32_t id, const std::string& method, const lepus::Value& params,
    base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    if (callback) {
      callback(LynxGetUIResult::UNKNOWN,
               PubLepusValue(lepus::Value("PlatformRendererContext is null")));
    }
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  const auto j_method =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, method);
  auto j_params =
      tasm::android::ValueConverterAndroid::ConvertLepusToJavaOnlyMap(params);

  const int32_t callback_id = ++invoke_ui_method_callback_id_;
  invoke_ui_method_callbacks_.emplace(callback_id, std::move(callback));
  Java_PlatformRendererContext_invokeUIMethod(
      env, local_ref.Get(), id, j_method.Get(), j_params.jni_object(),
      reinterpret_cast<jlong>(this), callback_id);
}

void PlatformRendererContext::InvokeUIMethodCallback(int32_t callback_id,
                                                     int32_t code,
                                                     const lepus::Value& data) {
  auto iter = invoke_ui_method_callbacks_.find(callback_id);
  if (iter == invoke_ui_method_callbacks_.end()) {
    return;
  }
  auto callback = std::move(iter->second);
  invoke_ui_method_callbacks_.erase(iter);
  if (callback) {
    callback(code, PubLepusValue(data));
  }
}

void PlatformRendererContext::UpdatePlatformRendererSubtreeProperties(
    int32_t id, const SubtreeProperty* properties, size_t count) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  // Count total size
  const size_t total_bytes = count * sizeof(SubtreeProperty);

  // Create DirectByteBuffer (zero-copy)
  void* buffer_data = const_cast<void*>(static_cast<const void*>(properties));
  jobject direct_buffer = env->NewDirectByteBuffer(buffer_data, total_bytes);

  if (direct_buffer != nullptr) {
    // Call Java method
    Java_PlatformRendererContext_updatePlatformRendererSubtreeProperties(
        env, local_ref.Get(), id, direct_buffer, static_cast<jint>(count));

    // Release local reference (DirectByteBuffer is a local reference)
    env->DeleteLocalRef(direct_buffer);
  }
}

void PlatformRendererContext::UpdatePlatformRendererExtraData(
    int32_t id, jobject extra_bundle) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull() || !extra_bundle) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_PlatformRendererContext_updatePlatformExtraData(env, local_ref.Get(), id,
                                                       extra_bundle);
}

void PlatformRendererContext::Destroy() {
  java_ref_.Reset(nullptr, nullptr);
  renderer_registry_.clear();
  invoke_ui_method_callbacks_.clear();
}

}  // namespace tasm
}  // namespace lynx

void Destroy(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  if (nativePtr == 0) {
    return;
  }
  reinterpret_cast<lynx::tasm::PlatformRendererContext*>(nativePtr)->Destroy();
}
jintArray GetDisplayListLengths(JNIEnv* env, jobject /*jcaller*/,
                                jlong nativePtr, jint id) {
  // Get the PlatformRendererContext instance from the native pointer
  if (nativePtr == 0) {
    return nullptr;
  }

  lynx::tasm::PlatformRendererContext* context =
      reinterpret_cast<lynx::tasm::PlatformRendererContext*>(nativePtr);

  // Find the PlatformRendererAndroid instance by id
  lynx::tasm::PlatformRendererAndroid* renderer =
      context->GetPlatformRenderer(id);
  if (renderer == nullptr) {
    return nullptr;
  }

  // Get the display list from the renderer
  const lynx::tasm::DisplayList& display_list = renderer->GetDisplayList();

  jintArray result = env->NewIntArray(3);
  if (result == nullptr) {
    return nullptr;
  }

  jint lengths[3] = {static_cast<jint>(display_list.GetContentOpTypesSize()),
                     static_cast<jint>(display_list.GetContentIntDataSize()),
                     static_cast<jint>(display_list.GetContentFloatDataSize())};
  env->SetIntArrayRegion(result, 0, 3, lengths);

  return result;
}

/**
 * Fills the provided arrays with display list data.
 * It is assumed that the arrays have been pre-allocated with sufficient lengths
 * obtained from
 * Java_com_lynx_tasm_behavior_render_PlatformRendererContext_nativeGetDisplayListLengths().
 * If the arrays are smaller than expected, the method will only copy up to the
 * array's length.
 */
void GetDisplayListData(JNIEnv* env, jobject /*jcaller*/, jlong nativePtr,
                        jint id, jintArray ops, jintArray iArgv,
                        jfloatArray fArgv) {
  if (ops == nullptr || iArgv == nullptr || fArgv == nullptr) {
    return;
  }

  // Get the PlatformRendererContext instance from the native pointer
  if (nativePtr == 0) {
    return;
  }

  lynx::tasm::PlatformRendererContext* context =
      reinterpret_cast<lynx::tasm::PlatformRendererContext*>(nativePtr);

  // Find the PlatformRendererAndroid instance by id
  lynx::tasm::PlatformRendererAndroid* renderer =
      context->GetPlatformRenderer(id);
  if (renderer == nullptr) {
    return;
  }

  // Get the display list from the renderer
  const lynx::tasm::DisplayList& display_list = renderer->GetDisplayList();

  // Get array lengths
  jsize opsLength = env->GetArrayLength(ops);
  jsize iArgvLength = env->GetArrayLength(iArgv);
  jsize fArgvLength = env->GetArrayLength(fArgv);

  // Get the display list data pointers
  const int32_t* opsDataSrc = display_list.GetContentOpTypesData();
  const int32_t* iArgvDataSrc = display_list.GetContentIntData();
  const float* fArgvDataSrc = display_list.GetContentFloatData();

  // Get Java array elements
  jint* opsData = env->GetIntArrayElements(ops, nullptr);
  jint* iArgvData = env->GetIntArrayElements(iArgv, nullptr);
  jfloat* fArgvData = env->GetFloatArrayElements(fArgv, nullptr);

  if (opsData != nullptr && iArgvData != nullptr && fArgvData != nullptr) {
    // Copy data using memcpy as requested
    if (opsDataSrc != nullptr && opsLength > 0) {
      memcpy(opsData, opsDataSrc,
             std::min(opsLength, static_cast<jsize>(
                                     display_list.GetContentOpTypesSize())) *
                 sizeof(jint));
    }
    if (iArgvDataSrc != nullptr && iArgvLength > 0) {
      memcpy(iArgvData, iArgvDataSrc,
             std::min(iArgvLength, static_cast<jsize>(
                                       display_list.GetContentIntDataSize())) *
                 sizeof(jint));
    }
    if (fArgvDataSrc != nullptr && fArgvLength > 0) {
      memcpy(
          fArgvData, fArgvDataSrc,
          std::min(fArgvLength,
                   static_cast<jsize>(display_list.GetContentFloatDataSize())) *
              sizeof(jfloat));
    }

    // Release arrays
    env->ReleaseIntArrayElements(ops, opsData, 0);
    env->ReleaseIntArrayElements(iArgv, iArgvData, 0);
    env->ReleaseFloatArrayElements(fArgv, fArgvData, 0);
  }
}

void Java_com_lynx_tasm_behavior_render_PlatformRendererContext_nativeDestroy(
    JNIEnv* env, jobject jcaller, jlong nativePtr) {
  if (nativePtr == 0) {
    return;
  }
  reinterpret_cast<lynx::tasm::PlatformRendererContext*>(nativePtr)->Destroy();
}
