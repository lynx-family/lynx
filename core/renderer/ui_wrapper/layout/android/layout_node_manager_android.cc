// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/ui_wrapper/layout/android/layout_node_manager_android.h"

#include <memory>

#include "core/build/gen/LayoutNodeManager_jni.h"
#include "core/public/layout_node_manager.h"
#include "core/public/layout_node_value.h"
#include "core/renderer/ui_wrapper/layout/android/layout_node_android.h"

void SetMeasureFunc(JNIEnv* env, jobject jcaller, jlong nativePtr, jint id,
                    jobject shadowNode) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  layout_node_manager->SetMeasureFunc(
      id, std::make_unique<lynx::tasm::MeasureFuncAndroid>(env, shadowNode));
}

void MarkDirty(JNIEnv* env, jobject jcaller, jlong nativePtr, jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  layout_node_manager->MarkDirtyAndRequestLayout(id);
}

jboolean IsDirty(JNIEnv* env, jobject jcaller, jlong nativePtr, jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  return layout_node_manager->IsDirty(id);
}

jlong MeasureNativeNode(JNIEnv* env, jobject jcaller, jlong nativePtr, jint id,
                        jfloat width, jint width_mode, jfloat height,
                        jint height_mode, jboolean finalMeasure) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  lynx::tasm::LayoutResult float_size =
      layout_node_manager->UpdateMeasureByPlatform(
          id, width, width_mode, height, height_mode, finalMeasure);
  const int* raw_bit_width = reinterpret_cast<int*>(&float_size.width_);
  const int* raw_bit_height = reinterpret_cast<int*>(&float_size.height_);
  jlong size = (static_cast<jlong>(*raw_bit_width) << 32) | (*raw_bit_height);
  return size;
}

jintArray MeasureNativeNodeReturnWithBaseline(JNIEnv* env, jobject jcaller,
                                              jlong nativePtr, jint id,
                                              jfloat width, jint width_mode,
                                              jfloat height, jint height_mode,
                                              jboolean finalMeasure) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  lynx::tasm::LayoutResult layout_result =
      layout_node_manager->UpdateMeasureByPlatform(
          id, width, width_mode, height, height_mode, finalMeasure);
  jintArray jni_size = env->NewIntArray(3);
  static int size[3];
  // same as process on android platform
  size[0] = static_cast<int>(ceilf(layout_result.width_));
  size[1] = static_cast<int>(ceilf(layout_result.height_));
  size[2] = static_cast<int>(ceilf(layout_result.baseline_));

  env->SetIntArrayRegion(jni_size, 0, 3, &size[0]);
  return jni_size;
}

void AlignNativeNode(JNIEnv* env, jobject jcaller, jlong nativePtr, jint id,
                     jfloat offset_top, jfloat offset_left) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  layout_node_manager->AlignmentByPlatform(id, offset_top, offset_left);
}

jint GetFlexDirection(JNIEnv* env, jobject jcaller, jlong nativePtr, jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  return static_cast<int>(layout_node_manager->GetFlexDirection(id));
}

jfloat GetWidth(JNIEnv* env, jobject jcaller, jlong nativePtr, jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  return layout_node_manager->GetWidth(id);
}

static jfloat GetHeight(JNIEnv* env, jobject jcaller, jlong nativePtr,
                        jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  return layout_node_manager->GetHeight(id);
}

static jfloat GetMinWidth(JNIEnv* env, jobject jcaller, jlong nativePtr,
                          jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  return layout_node_manager->GetMinWidth(id);
}

static jfloat GetMaxWidth(JNIEnv* env, jobject jcaller, jlong nativePtr,
                          jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  return layout_node_manager->GetMaxWidth(id);
}

static jfloat GetMinHeight(JNIEnv* env, jobject jcaller, jlong nativePtr,
                           jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  return layout_node_manager->GetMinHeight(id);
}

static jfloat GetMaxHeight(JNIEnv* env, jobject jcaller, jlong nativePtr,
                           jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  return layout_node_manager->GetMaxHeight(id);
}

static jintArray GetPadding(JNIEnv* env, jobject jcaller, jlong nativePtr,
                            jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  jintArray jni_paddings = env->NewIntArray(4);
  int paddings[4];
  paddings[0] = layout_node_manager->GetPaddingLeft(id);
  paddings[1] = layout_node_manager->GetPaddingTop(id);
  paddings[2] = layout_node_manager->GetPaddingRight(id);
  paddings[3] = layout_node_manager->GetPaddingBottom(id);
  env->SetIntArrayRegion(jni_paddings, 0, 4, &paddings[0]);
  return jni_paddings;
}

static jintArray GetMargin(JNIEnv* env, jobject jcaller, jlong nativePtr,
                           jint id) {
  auto layout_node_manager =
      reinterpret_cast<lynx::tasm::LayoutNodeManager*>(nativePtr);
  jintArray jni_margins = env->NewIntArray(4);
  int margins[4];
  margins[0] = layout_node_manager->GetMarginLeft(id);
  margins[1] = layout_node_manager->GetMarginTop(id);
  margins[2] = layout_node_manager->GetMarginRight(id);
  margins[3] = layout_node_manager->GetMarginBottom(id);
  env->SetIntArrayRegion(jni_margins, 0, 4, &margins[0]);
  return jni_margins;
}

namespace lynx {
namespace tasm {

bool LayoutNodeManagerAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

MeasureFuncAndroid::MeasureFuncAndroid(JNIEnv* env, jobject obj)
    : jni_object_(env, obj) {}

LayoutResult MeasureFuncAndroid::Measure(float width, int32_t width_mode,
                                         float height, int32_t height_mode,
                                         bool final_measure) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_java_ref(jni_object_);
  if (local_java_ref.IsNull()) {
    return LayoutResult{0, 0};
  }

  float baseline = 0.f;
  float measuredWidth = 0.f;
  float measuredHeight = 0.f;

  const base::android::ScopedLocalJavaRef<jfloatArray> jArray =
      lynx::tasm::LayoutNodeAndroid::Measure(env, local_java_ref.Get(), width,
                                             width_mode, height, height_mode,
                                             final_measure);

  if (jArray.IsNull()) {
    return LayoutResult{0, 0, 0};
  }

  jfloat* jMeasureResult = env->GetFloatArrayElements(jArray.Get(), JNI_FALSE);
  measuredWidth = jMeasureResult[0];
  measuredHeight = jMeasureResult[1];
  baseline = jMeasureResult[2];

  env->ReleaseFloatArrayElements(jArray.Get(), jMeasureResult, JNI_ABORT);

  return LayoutResult{measuredWidth, measuredHeight, baseline};
}

void MeasureFuncAndroid::Alignment() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_java_ref(jni_object_);
  if (local_java_ref.IsNull()) {
    return;
  }
  lynx::tasm::LayoutNodeAndroid::Align(env, local_java_ref.Get());
}

}  // namespace tasm
}  // namespace lynx
