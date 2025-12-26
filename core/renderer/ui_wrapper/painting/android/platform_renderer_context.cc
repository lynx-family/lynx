// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "platform/android/lynx_android/src/main/jni/gen/PlatformRendererContext_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/PlatformRendererContext_register_jni.h"

jlong CreateEmbeddedViewContext(JNIEnv* env, jobject jcaller, jobject jThis) {
  return reinterpret_cast<jlong>(
      new lynx::tasm::PlatformRendererContext(env, jThis));
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
    int32_t id, const base::String& tag_name) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_tag_name = base::android::JNIConvertHelper::ConvertToJNIStringUTF(
      env, tag_name.c_str());
  Java_PlatformRendererContext_createPlatformExtendedRenderer(
      env, local_ref.Get(), id, j_tag_name.Get());
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
                                          float width, float height) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_src =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, src.c_str());
  Java_PlatformRendererContext_createImage(env, local_ref.Get(), id,
                                           j_src.Get(), static_cast<int>(width),
                                           static_cast<int>(height));
}

void PlatformRendererContext::DestroyImage(int32_t id) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformRendererContext_destroyImage(env, local_ref.Get(), id);
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

}  // namespace tasm
}  // namespace lynx

// JNI function implementations
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
