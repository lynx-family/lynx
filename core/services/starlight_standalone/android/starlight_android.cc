// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/starlight_standalone/android/starlight_android.h"

#include <jni.h>

#include <string>

#include "core/base/android/jni_helper.h"
#include "core/services/starlight_standalone/core/include/starlight.h"
#include "core/services/starlight_standalone/jni_headers/build/gen/StarlightNode_jni.h"

const short int LAYOUT_WIDTH_INDEX = 0;
const short int LAYOUT_HEIGHT_INDEX = 1;
const short int LAYOUT_LEFT_INDEX = 2;
const short int LAYOUT_TOP_INDEX = 3;
const short int LAYOUT_MARGIN_START_INDEX = 4;
const short int LAYOUT_PADDING_START_INDEX = 8;
const short int LAYOUT_BORDER_START_INDEX = 12;

namespace starlight {
bool StarlightAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

SLMeasureDelegateAndroid::SLMeasureDelegateAndroid(JNIEnv* env, jobject obj)
    : jni_object_(env, obj) {}

SLMeasureDelegateAndroid::~SLMeasureDelegateAndroid() {}

SLSize SLMeasureDelegateAndroid::Measure(SLConstraints& constraint) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_java_ref(jni_object_);
  if (local_java_ref.IsNull()) {
    return SLSize{0, 0};
  }
  float width = 0.f, height = 0.f;

  width = constraint[kSLHorizontal].size_;
  height = constraint[kSLVertical].size_;

  StarlightMeasureMode width_mode = constraint[kSLHorizontal].mode_;
  StarlightMeasureMode height_mode = constraint[kSLVertical].mode_;

  jlong measureResult = Java_StarlightNode_measure(
      env, local_java_ref.Get(), width, width_mode, height, height_mode);
  int32_t wBits = 0xFFFFFFFF & (measureResult >> 32);
  int32_t hBits = 0xFFFFFFFF & measureResult;

  const float* measuredWidth = reinterpret_cast<float*>(&wBits);
  const float* measuredHeight = reinterpret_cast<float*>(&hBits);

  return SLSize{*measuredWidth, *measuredHeight};
}

void SLMeasureDelegateAndroid::Alignment() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_java_ref(jni_object_);
  if (local_java_ref.IsNull()) {
    return;
  }
  Java_StarlightNode_align(env, local_java_ref.Get());
}

float SLMeasureDelegateAndroid::Baseline(SLConstraints& constraint) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_java_ref(jni_object_);
  if (local_java_ref.IsNull()) {
    return 0.f;
  }
  float width = 0.f, height = 0.f;

  width = constraint[kSLHorizontal].size_;
  height = constraint[kSLVertical].size_;

  StarlightMeasureMode width_mode = constraint[kSLHorizontal].mode_;
  StarlightMeasureMode height_mode = constraint[kSLVertical].mode_;
  jfloat result = Java_StarlightNode_baseline(env, local_java_ref.Get(), width,
                                              width_mode, height, height_mode);

  return result;
}

}  // namespace starlight

jlong CreateLayoutObject(JNIEnv* env, jobject jcaller, jint screenWidth,
                         jint screenHeight, jfloat scale, jfloat density) {
  auto config =
      starlight::LayoutConfig(screenWidth, screenHeight, scale, density);
  return reinterpret_cast<jlong>(starlight::CreateWithConfig(config));
}

void UpdateViewport(JNIEnv* env, jobject jcaller, jlong ptr,
                    jfloat viewportWidth, jfloat viewportHeight,
                    jint viewportWidthMode, jint viewportHeightMode) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(ptr);
  starlight::UpdateViewport(
      node, viewportWidth,
      static_cast<starlight::StarlightMeasureMode>(viewportWidthMode),
      viewportHeight,
      static_cast<starlight::StarlightMeasureMode>(viewportHeightMode));
}

jlong CreateMeasureDelegate(JNIEnv* env, jobject jcaller, jlong ptr) {
  return reinterpret_cast<jlong>(
      new starlight::SLMeasureDelegateAndroid(env, jcaller));
}

void FreeMeasureDelegate(JNIEnv* env, jobject jcaller, jlong ptr,
                         jlong delegatePtr) {
  auto* delegate = reinterpret_cast<starlight::MeasureDelegate*>(delegatePtr);
  delete delegate;
}

void SetMeasureFunc(JNIEnv* env, jobject jcaller, jlong ptr,
                    jlong delegatePtr) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(ptr);
  auto* delegate = reinterpret_cast<starlight::MeasureDelegate*>(delegatePtr);
  starlight::SetMeasureDelegate(node, delegate);
}

void SetStyleWithName(JNIEnv* env, jobject jcaller, jlong ptr, jstring key,
                      jstring value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(ptr);
  starlight::SetStyle(
      node, lynx::base::android::JNIConvertHelper::ConvertToString(env, key),
      lynx::base::android::JNIConvertHelper::ConvertToString(env, value));
}

void InsertNode(JNIEnv* env, jobject jcaller, jlong parentPtr, jlong childPtr,
                jint index) {
  starlight::SLNodeRef parent =
      reinterpret_cast<starlight::SLNodeRef>(parentPtr);
  starlight::SLNodeRef child = reinterpret_cast<starlight::SLNodeRef>(childPtr);
  starlight::InsertChild(parent, child, index);
}

void RemoveNode(JNIEnv* env, jobject jcaller, jlong parentPtr, jlong childPtr) {
  starlight::SLNodeRef parent =
      reinterpret_cast<starlight::SLNodeRef>(parentPtr);
  starlight::SLNodeRef child = reinterpret_cast<starlight::SLNodeRef>(childPtr);
  starlight::RemoveChild(parent, child);
}

void RemoveNodeAtIndex(JNIEnv* env, jobject jcaller, jlong parentPtr,
                       jint index) {
  starlight::SLNodeRef parent =
      reinterpret_cast<starlight::SLNodeRef>(parentPtr);
  starlight::RemoveChild(parent, index);
}

void RemoveAllNodes(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::RemoveAllChild(node);
}

void FreeNode(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::Free(node);
}

void MarkDirty(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::MarkDirty(node);
}

jboolean IsNodeDirty(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  return starlight::IsDirty(node);
}

jboolean IsRTL(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  return starlight::IsRTL(node);
}

void PerformLayout(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::CalculateLayout(node);
}

void UpdateLayout(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLSize size = starlight::GetLayoutSize(node);
  starlight::SLPoint offset = starlight::GetLayoutOffset(node);
  float padding[] = {
      starlight::GetLayoutPadding(node, starlight::kSLLeft),
      starlight::GetLayoutPadding(node, starlight::kSLRight),
      starlight::GetLayoutPadding(node, starlight::kSLTop),
      starlight::GetLayoutPadding(node, starlight::kSLBottom),
  };

  float margin[] = {
      starlight::GetLayoutMargin(node, starlight::kSLLeft),
      starlight::GetLayoutMargin(node, starlight::kSLRight),
      starlight::GetLayoutMargin(node, starlight::kSLTop),
      starlight::GetLayoutMargin(node, starlight::kSLBottom),
  };

  float border[] = {
      starlight::GetLayoutBorder(node, starlight::kSLLeft),
      starlight::GetLayoutBorder(node, starlight::kSLRight),
      starlight::GetLayoutBorder(node, starlight::kSLTop),
      starlight::GetLayoutBorder(node, starlight::kSLBottom),
  };

  lynx::base::android::ScopedLocalJavaRef<jfloatArray> jni_result(
      env, env->NewFloatArray(16));
  float layout_results[16];
  layout_results[LAYOUT_WIDTH_INDEX] = size.width_;
  layout_results[LAYOUT_HEIGHT_INDEX] = size.height_;
  layout_results[LAYOUT_LEFT_INDEX] = offset.x_;
  layout_results[LAYOUT_TOP_INDEX] = offset.y_;
  for (int i = 0; i < 4; ++i) {
    layout_results[LAYOUT_MARGIN_START_INDEX + i] = margin[i];
    layout_results[LAYOUT_PADDING_START_INDEX + i] = padding[i];
    layout_results[LAYOUT_BORDER_START_INDEX + i] = border[i];
  }
  env->SetFloatArrayRegion(jni_result.Get(), 0, 16, &layout_results[0]);
  lynx::base::android::ScopedLocalJavaRef<jobject> local_java_ref(env, jcaller);
  if (local_java_ref.IsNull()) {
    return;
  }
  Java_StarlightNode_updateLayoutResult(env, local_java_ref.Get(),
                                        jni_result.Get());
}

void SetFlexGrow(JNIEnv* env, jobject jcaller, jlong nativePtr, jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetFlexGrow(node, value);
}

void SetFlexShrink(JNIEnv* env, jobject jcaller, jlong nativePtr,
                   jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetFlexShrink(node, value);
}

void SetFlexBasis(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                  jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetFlexBasis(node, length);
}

void SetFlexWrap(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetFlexWrap(node, static_cast<starlight::SLFlexWrapType>(type));
}

void SetJustifyContent(JNIEnv* env, jobject jcaller, jlong nativePtr,
                       jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetJustifyContent(
      node, static_cast<starlight::SLJustifyContentType>(type));
}

void SetAlignContent(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetAlignContent(node,
                             static_cast<starlight::SLAlignContentType>(type));
}

void SetAlignSelf(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetAlignSelf(node, static_cast<starlight::SLFlexAlignType>(type));
}

void SetAlignItems(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetAlignItems(node, static_cast<starlight::SLFlexAlignType>(type));
}

void SetBorderLeft(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                   jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetBorderLeft(node, length);
}

void SetBorderRight(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                    jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetBorderRight(node, length);
}

void SetBorderTop(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                  jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetBorderTop(node, length);
}

void SetBorderBottom(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                     jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetBorderBottom(node, length);
}

void SetBorder(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
               jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetBorder(node, length);
}

void SetDirection(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetDirection(node, static_cast<starlight::SLDirectionType>(type));
}

void SetFlexDirection(JNIEnv* env, jobject jcaller, jlong nativePtr,
                      jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetFlexDirection(node,
                              static_cast<starlight::SLFlexDirection>(type));
}

void SetDisplay(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetDisplay(node, static_cast<starlight::SLDisplayType>(type));
}

void SetAspectRatio(JNIEnv* env, jobject jcaller, jlong nativePtr,
                    jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetAspectRatio(node, value);
}

void SetWidth(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
              jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetWidth(node, length);
}

void SetHeight(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
               jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetHeight(node, length);
}

void SetMinWidth(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                 jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMinWidth(node, length);
}

void SetMinHeight(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                  jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMinHeight(node, length);
}

void SetMaxWidth(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                 jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMaxWidth(node, length);
}

void SetMaxHeight(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                  jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMaxHeight(node, length);
}

void SetLeft(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
             jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetLeft(node, length);
}

void SetTop(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
            jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetTop(node, length);
}

void SetRight(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
              jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetRight(node, length);
}

void SetBottom(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
               jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetBottom(node, length);
}

void SetInlineStart(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                    jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetInlineStart(node, length);
}

void SetInlineEnd(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                  jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetInlineEnd(node, length);
}

void SetMarginLeft(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                   jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMarginLeft(node, length);
}

void SetMarginTop(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                  jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMarginTop(node, length);
}

void SetMarginRight(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                    jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMarginRight(node, length);
}

void SetMarginBottom(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                     jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMarginBottom(node, length);
}

void SetMarginInlineStart(JNIEnv* env, jobject jcaller, jlong nativePtr,
                          jint type, jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMarginInlineStart(node, length);
}

void SetMarginInlineEnd(JNIEnv* env, jobject jcaller, jlong nativePtr,
                        jint type, jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMarginInlineEnd(node, length);
}

void SetMargin(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
               jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetMargin(node, length);
}

void SetPaddingLeft(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                    jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetPaddingLeft(node, length);
}

void SetPaddingTop(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                   jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetPaddingTop(node, length);
}

void SetPaddingRight(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                     jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetPaddingRight(node, length);
}

void SetPaddingBottom(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                      jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetPaddingBottom(node, length);
}

void SetPaddingInlineStart(JNIEnv* env, jobject jcaller, jlong nativePtr,
                           jint type, jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetPaddingInlineStart(node, length);
}

void SetPaddingInlineEnd(JNIEnv* env, jobject jcaller, jlong nativePtr,
                         jint type, jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetPaddingInlineEnd(node, length);
}

void SetPadding(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                jfloat value) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SLLength length =
      starlight::SLLength(value, static_cast<starlight::SLLengthType>(type));
  starlight::SetPadding(node, length);
}

void SetPosition(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  starlight::SLNodeRef node = reinterpret_cast<starlight::SLNodeRef>(nativePtr);
  starlight::SetPosition(node, static_cast<starlight::SLPositionType>(type));
}
