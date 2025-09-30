// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/starlight_standalone/android/starlight_android.h"

#include <jni.h>

#include <string>

#include "core/base/android/jni_helper.h"
#include "platform/android/lynx_starlight/src/main/jni/gen/StarlightNode_jni.h"
#include "platform/android/lynx_starlight/src/main/jni/gen/StarlightNode_register_jni.h"

static constexpr uint16_t kLayoutWidthIndex = 0;
static constexpr uint16_t kLayoutHeightIndex = 1;
static constexpr uint16_t kLayoutLeftIndex = 2;
static constexpr uint16_t kLayoutTopIndex = 3;
static constexpr uint16_t kLayoutMarginStartIndex = 4;
static constexpr uint16_t kLayoutPaddingStartIndex = 8;
static constexpr uint16_t kLayoutBorderStartIndex = 12;
static constexpr uint16_t kLayoutResultSize = 16;

static jclass g_starlightNodeClass = nullptr;
static jmethodID g_measureMethod = nullptr;
static jmethodID g_baselineMethod = nullptr;

namespace lynx_starlight {
namespace jni {
bool RegisterJNIForStarlightNode(JNIEnv* env) {
  g_starlightNodeClass =
      env->FindClass("com/lynx/starlight/nodes/StarlightNode");
  if (g_starlightNodeClass == NULL) {
    return false;
  }
  jobject res = env->NewGlobalRef(g_starlightNodeClass);  // NOLINT

  g_starlightNodeClass = static_cast<jclass>(res);
  if (!g_starlightNodeClass) {
    return false;
  }

  g_measureMethod =
      env->GetMethodID(g_starlightNodeClass, "measure", "(FIFI)J");

  g_baselineMethod =
      env->GetMethodID(g_starlightNodeClass, "baseline", "(FF)F");

  if (!g_measureMethod || !g_baselineMethod) {
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
    }
    return false;
  }

  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx_starlight

namespace starlight {
SLMeasureDelegateAndroid::SLMeasureDelegateAndroid(JNIEnv* env, jobject obj)
    : jni_object_(env, obj) {}

StarlightSize SLMeasureDelegateAndroid::Measure(float width,
                                                SLNodeMeasureMode width_mode,
                                                float height,
                                                SLNodeMeasureMode height_mode) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_java_ref(jni_object_);

  if (local_java_ref.IsNull()) {
    return StarlightSize{0.f, 0.f};
  }

  if (!g_measureMethod) {
    return StarlightSize{0.f, 0.f};
  }

  const jlong measureResult =
      env->CallLongMethod(local_java_ref.Get(), g_measureMethod, width,
                          width_mode, height, height_mode);

  int32_t wBits = 0xFFFFFFFF & (measureResult >> 32);
  int32_t hBits = 0xFFFFFFFF & measureResult;

  const float* measuredWidth = reinterpret_cast<float*>(&wBits);
  const float* measuredHeight = reinterpret_cast<float*>(&hBits);

  return StarlightSize{*measuredWidth, *measuredHeight};
}

float SLMeasureDelegateAndroid::Baseline(float width, float height) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> local_java_ref(jni_object_);
  if (local_java_ref.IsNull()) {
    return 0.f;
  }

  if (!g_baselineMethod) {
    return 0.f;
  }

  const jfloat result = env->CallFloatMethod(local_java_ref.Get(),
                                             g_baselineMethod, width, height);

  return result;
}

}  // namespace starlight

StarlightSize StarlightMeasureFuncForAndroid(void* manager_node, float width,
                                             SLNodeMeasureMode width_mode,
                                             float height,
                                             SLNodeMeasureMode height_mode) {
  starlight::SLMeasureDelegateAndroid* delegate =
      static_cast<starlight::SLMeasureDelegateAndroid*>(manager_node);
  return delegate->Measure(width, width_mode, height, height_mode);
}

float StarlightBaselineFuncForAndroid(void* manager_node, float width,
                                      float height) {
  starlight::SLMeasureDelegateAndroid* delegate =
      static_cast<starlight::SLMeasureDelegateAndroid*>(manager_node);
  return delegate->Baseline(width, height);
}

// transfer C++'s StarlightValue to java long, and will be transfered to Java's
// StarlightValue in java
struct JNIStarlightValue {
  static jlong asJavaLong(const StarlightValue& value) {
    uint32_t valueBytes = 0;
    memcpy(&valueBytes, &value.value_, sizeof valueBytes);
    return ((jlong)value.unit_) << 32 | valueBytes;
  }
};

jlong CreateSLNode(JNIEnv* env, jobject jcaller) {
  return reinterpret_cast<jlong>(SLNodeNew());
}

jlong CreateMeasureDelegate(JNIEnv* env, jobject jcaller, jlong ptr) {
  starlight::SLMeasureDelegateAndroid* const delegate_android =
      new starlight::SLMeasureDelegateAndroid(env, jcaller);
  StarlightMeasureDelegate* const delegate = new StarlightMeasureDelegate();
  delegate->manager_node_ = delegate_android;
  const SLNodeRef node = reinterpret_cast<SLNodeRef>(ptr);
  SLNodeSetMeasureDelegate(node, delegate);
  return reinterpret_cast<jlong>(delegate);
}

jlong CreateMeasureDelegateAndSetMeasureFunc(JNIEnv* env, jobject jcaller,
                                             jlong ptr) {
  starlight::SLMeasureDelegateAndroid* const delegate_android =
      new starlight::SLMeasureDelegateAndroid(env, jcaller);
  StarlightMeasureDelegate* const delegate = new StarlightMeasureDelegate();
  delegate->measure_func_ = StarlightMeasureFuncForAndroid;
  delegate->manager_node_ = delegate_android;
  const SLNodeRef node = reinterpret_cast<SLNodeRef>(ptr);
  SLNodeSetMeasureDelegate(node, delegate);
  return reinterpret_cast<jlong>(delegate);
}

void SetMeasureFunction(JNIEnv* env, jobject jcaller, jlong delegatePtr) {
  StarlightMeasureDelegate* const delegate =
      reinterpret_cast<StarlightMeasureDelegate*>(delegatePtr);
  delegate->measure_func_ = StarlightMeasureFuncForAndroid;
}

jlong CreateMeasureDelegateAndSetBaselineFunc(JNIEnv* env, jobject jcaller,
                                              jlong ptr) {
  starlight::SLMeasureDelegateAndroid* const delegate_android =
      new starlight::SLMeasureDelegateAndroid(env, jcaller);
  StarlightMeasureDelegate* const delegate = new StarlightMeasureDelegate();
  delegate->baseline_func_ = StarlightBaselineFuncForAndroid;
  delegate->manager_node_ = delegate_android;
  const SLNodeRef node = reinterpret_cast<SLNodeRef>(ptr);
  SLNodeSetMeasureDelegate(node, delegate);
  return reinterpret_cast<jlong>(delegate);
}

void SetBaselineFunction(JNIEnv* env, jobject jcaller, jlong delegatePtr) {
  StarlightMeasureDelegate* const delegate =
      reinterpret_cast<StarlightMeasureDelegate*>(delegatePtr);
  delegate->baseline_func_ = StarlightBaselineFuncForAndroid;
}

void Reset(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  SLNodeReset(node);
}

void MarkDirty(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  SLNodeMarkDirty(node);
}

jboolean IsNodeDirty(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  return SLNodeIsDirty(node);
}

// jboolean IsNodeHasNewLayout(JNIEnv* env, jobject jcaller, jlong nativePtr) {
//   // SLNodeRef node =
//   // reinterpret_cast<SLNodeRef>(nativePtr); return
//   // SLNodeGetHasNewLayout(node);
//   return true;
// }

// void MarkLayoutSeen(JNIEnv* env, jobject jcaller, jlong nativePtr) {
//   // SLNodeRef node =
//   // reinterpret_cast<SLNodeRef>(nativePtr); return
//   // SLNodeSetHasNewLayout(node, false);
//   // return true;
// }

void InsertChild(JNIEnv* env, jobject jcaller, jlong parentPtr, jlong childPtr,
                 jint index) {
  SLNodeRef parent = reinterpret_cast<SLNodeRef>(parentPtr);
  SLNodeRef child = reinterpret_cast<SLNodeRef>(childPtr);
  SLNodeInsertChild(parent, child, index);
}

void RemoveChild(JNIEnv* env, jobject jcaller, jlong parentPtr,
                 jlong childPtr) {
  SLNodeRef parent = reinterpret_cast<SLNodeRef>(parentPtr);
  SLNodeRef child = reinterpret_cast<SLNodeRef>(childPtr);
  SLNodeRemoveChild(parent, child);
}

void RemoveAllChildren(JNIEnv* env, jobject jcaller, jlong parentPtr) {
  SLNodeRef parent = reinterpret_cast<SLNodeRef>(parentPtr);
  SLNodeRemoveAllChildren(parent);
}

void FreeNode(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  SLNodeFree(node);
}

void FreeMeasureDelegateAndNode(JNIEnv* env, jobject jcaller, jlong ptr,
                                jlong delegatePtr) {
  if (!delegatePtr) {
    return;
  }
  auto* delegate = reinterpret_cast<StarlightMeasureDelegate*>(delegatePtr);
  auto* delegate_android =
      reinterpret_cast<starlight::SLMeasureDelegateAndroid*>(
          delegate->manager_node_);
  if (delegate_android) {
    delete delegate_android;
  }
  delete delegate;
  SLNodeRef node = reinterpret_cast<SLNodeRef>(ptr);
  SLNodeFree(node);
}

void FreeMeasureDelegate(JNIEnv* env, jobject jcaller, jlong delegatePtr) {
  if (!delegatePtr) {
    return;
  }
  auto* delegate = reinterpret_cast<StarlightMeasureDelegate*>(delegatePtr);
  auto* delegate_android =
      reinterpret_cast<starlight::SLMeasureDelegateAndroid*>(
          delegate->manager_node_);
  if (delegate_android) {
    delete delegate_android;
  }
  delete delegate;
}

void CalculateLayout(JNIEnv* env, jobject jcaller, jlong nativePtr,
                     jfloat width, jfloat height) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  const SLDirection direction =
      SLNodeIsRTL(node) ? SLDirectionRTL : SLDirectionLTR;
  SLNodeCalculateLayout(node, width, height, direction);
}

void SLNodeUpdateLayoutInfo(JNIEnv* env, jobject jcaller, jlong nativePtr,
                            jfloatArray array) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  auto array_len = env->GetArrayLength(array);
  jfloat* layout_results = env->GetFloatArrayElements(array, nullptr);
  if (static_cast<uint16_t>(array_len) != kLayoutResultSize ||
      layout_results == nullptr) {
    return;
  }
  float padding[] = {
      SLNodeLayoutGetPadding(node, SLEdgeLeft),
      SLNodeLayoutGetPadding(node, SLEdgeRight),
      SLNodeLayoutGetPadding(node, SLEdgeTop),
      SLNodeLayoutGetPadding(node, SLEdgeBottom),
  };

  float margin[] = {
      SLNodeLayoutGetMargin(node, SLEdgeLeft),
      SLNodeLayoutGetMargin(node, SLEdgeRight),
      SLNodeLayoutGetMargin(node, SLEdgeTop),
      SLNodeLayoutGetMargin(node, SLEdgeBottom),
  };

  float border[] = {
      SLNodeLayoutGetBorder(node, SLEdgeLeft),
      SLNodeLayoutGetBorder(node, SLEdgeRight),
      SLNodeLayoutGetBorder(node, SLEdgeTop),
      SLNodeLayoutGetBorder(node, SLEdgeBottom),
  };

  layout_results[kLayoutWidthIndex] = SLNodeLayoutGetWidth(node);
  layout_results[kLayoutHeightIndex] = SLNodeLayoutGetHeight(node);
  layout_results[kLayoutLeftIndex] = SLNodeLayoutGetLeft(node);
  layout_results[kLayoutTopIndex] = SLNodeLayoutGetTop(node);
  for (int i = 0; i < 4; ++i) {
    layout_results[kLayoutMarginStartIndex + i] = margin[i];
    layout_results[kLayoutPaddingStartIndex + i] = padding[i];
    layout_results[kLayoutBorderStartIndex + i] = border[i];
  }
  env->ReleaseFloatArrayElements(array, layout_results, 0);
}

// for gni: set basic type style. e.g. width, height, flex, flex-grow,
#define JNI_SET_BASIC_TYPE_STYLE(type_pre_fix, jni_basic_type)          \
  void Set##type_pre_fix(JNIEnv* env, jobject jcaller, jlong nativePtr, \
                         jni_basic_type value) {                        \
    SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);            \
    SLNodeStyleSet##type_pre_fix(node, value);                          \
  }

#define SUPPORTED_BASIC_TYPE_STYLE_SETTER(V) \
  V(Width, jfloat)                           \
  V(WidthPercent, jfloat)                    \
  V(Height, jfloat)                          \
  V(HeightPercent, jfloat)                   \
  V(MinWidth, jfloat)                        \
  V(MinWidthPercent, jfloat)                 \
  V(MinHeight, jfloat)                       \
  V(MinHeightPercent, jfloat)                \
  V(MaxWidth, jfloat)                        \
  V(MaxWidthPercent, jfloat)                 \
  V(MaxHeight, jfloat)                       \
  V(MaxHeightPercent, jfloat)                \
  V(FlexBasis, jfloat)                       \
  V(FlexBasisPercent, jfloat)                \
  V(AspectRatio, jfloat)                     \
  V(Order, jint)                             \
  V(Flex, jfloat)                            \
  V(FlexGrow, jfloat)                        \
  V(FlexShrink, jfloat)

SUPPORTED_BASIC_TYPE_STYLE_SETTER(JNI_SET_BASIC_TYPE_STYLE)

#undef SUPPORTED_BASIC_TYPE_STYLE_SETTER
#undef JNI_SET_BASIC_TYPE_STYLE

// for gni: set style with no param. e.g. widthAuto, heightAuto, flexBasisAuto
#define JNI_SET_STYLE_WITH_NO_PARAM(type_pre_fix)                         \
  void Set##type_pre_fix(JNIEnv* env, jobject jcaller, jlong nativePtr) { \
    SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);              \
    SLNodeStyleSet##type_pre_fix(node);                                   \
  }

#define SUPPORTED_STYLE_WITH_NO_PARAM(V) \
  V(WidthAuto)                           \
  V(WidthMaxContent)                     \
  V(WidthFitContent)                     \
  V(HeightAuto)                          \
  V(HeightMaxContent)                    \
  V(HeightFitContent)                    \
  V(FlexBasisAuto)

SUPPORTED_STYLE_WITH_NO_PARAM(JNI_SET_STYLE_WITH_NO_PARAM)

#undef SUPPORTED_STYLE_WITH_NO_PARAM
#undef JNI_SET_STYLE_WITH_NO_PARAM

// for gni: set enum style. e.g. flex-direction, justify-content, align-content
#define JNI_SET_ENUM_STYLE(type_name, enum_type)                     \
  void Set##type_name(JNIEnv* env, jobject jcaller, jlong nativePtr, \
                      jint type) {                                   \
    SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);         \
    SLNodeStyleSet##type_name(node, static_cast<enum_type>(type));   \
  }

#define SUPPORTED_ENUM_STYLE_GETTER(V) \
  V(FlexDirection, SLFlexDirection)    \
  V(JustifyContent, SLJustifyContent)  \
  V(AlignContent, SLAlignContent)      \
  V(AlignItems, SLFlexAlign)           \
  V(AlignSelf, SLFlexAlign)            \
  V(PositionType, SLPositionType)      \
  V(FlexWrap, SLFlexWrap)              \
  V(Display, SLDisplay)                \
  V(BoxSizing, SLBoxSizing)            \
  V(Direction, SLDirection)

SUPPORTED_ENUM_STYLE_GETTER(JNI_SET_ENUM_STYLE)

#undef SUPPORTED_ENUM_STYLE_GETTER
#undef JNI_SET_ENUM_STYLE

// for gni: set style with edge type. e.g. margin, padding
#define JNI_SET_STYLE_WITH_EDGE(type_pre_fix)                             \
  void Set##type_pre_fix(JNIEnv* env, jobject jcaller, jlong nativePtr,   \
                         jint type, jfloat value) {                       \
    SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);              \
    SLNodeStyleSet##type_pre_fix(node, static_cast<SLEdge>(type), value); \
  }

#define SUPPORTED_STYLE_WITH_EDGE(V) \
  V(Margin)                          \
  V(MarginPercent)                   \
  V(Padding)                         \
  V(PaddingPercent)                  \
  V(Border)                          \
  V(Position)                        \
  V(PositionPercent)

SUPPORTED_STYLE_WITH_EDGE(JNI_SET_STYLE_WITH_EDGE)

#undef SUPPORTED_STYLE_WITH_EDGE
#undef JNI_SET_STYLE_WITH_EDGE

void SetGap(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
            jfloat value) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  SLNodeStyleSetGap(node, static_cast<SLGap>(type), value);
}

void SetGapPercent(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type,
                   jfloat value) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  SLNodeStyleSetGapPercent(node, static_cast<SLGap>(type), value);
}

jlong GetGap(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  return JNIStarlightValue::asJavaLong(
      SLNodeStyleGetGap(node, static_cast<SLGap>(type)));
}

void SetMarginAuto(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  SLNodeStyleSetMarginAuto(node, static_cast<SLEdge>(type));
}

void SetPositionAuto(JNIEnv* env, jobject jcaller, jlong nativePtr, jint type) {
  SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);
  SLNodeStyleSetPositionAuto(node, static_cast<SLEdge>(type));
}

// for jni: get styles with eunm type getter, e.g., flex-direction,
// justify-content.
#define JNI_GET_ENUM_STYLE(type_name, enum_type, suffix)               \
  jint Get##type_name(JNIEnv* env, jobject jcaller, jlong nativePtr) { \
    SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);           \
    return static_cast<jint>(SLNodeStyleGet##suffix(node));            \
  }

#define JNI_SUPPORTED_ENUM_STYLE_GETTER(V)            \
  V(Display, SLDisplay, Display)                      \
  V(FlexDirection, SLFlexDirection, FlexDirection)    \
  V(JustifyContent, SLJustifyContent, JustifyContent) \
  V(AlignContent, SLAlignContent, AlignContent)       \
  V(AlignItems, SLFlexAlign, AlignItems)              \
  V(AlignSelf, SLFlexAlign, AlignSelf)                \
  V(PositionType, SLPositionType, PositionType)       \
  V(FlexWrap, SLFlexWrap, FlexWrap)                   \
  V(BoxSizing, SLBoxSizing, BoxSizing)

JNI_SUPPORTED_ENUM_STYLE_GETTER(JNI_GET_ENUM_STYLE)

#undef JNI_SUPPORTED_ENUM_STYLE_GETTER
#undef JNI_GET_ENUM_STYLE

// for gni: get styles with basic type getter, e.g., aspect-ratio, order.
#define JNI_GET_BASIC_TYPE_STYLE(type_name, jni_return_type)              \
  jni_return_type Get##type_name(JNIEnv* env, jobject jcaller,            \
                                 jlong nativePtr) {                       \
    SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);              \
    return static_cast<jni_return_type>(SLNodeStyleGet##type_name(node)); \
  }

#define JNI_SUPPORTED_BASIC_TYPE_STYLE_GETTER(V) \
  V(AspectRatio, jfloat)                         \
  V(Order, jint)                                 \
  V(FlexGrow, jfloat)                            \
  V(FlexShrink, jfloat)

JNI_SUPPORTED_BASIC_TYPE_STYLE_GETTER(JNI_GET_BASIC_TYPE_STYLE)

#undef JNI_SUPPORTED_BASIC_TYPE_STYLE_GETTER
#undef JNI_GET_BASIC_TYPE_STYLE

// for gni: get styles with length type getter, e.g., width, height.
#define JNI_GET_LENGTH_STYLE(type_name)                                    \
  jlong Get##type_name(JNIEnv* env, jobject jcaller, jlong nativePtr) {    \
    SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);               \
    return JNIStarlightValue::asJavaLong(SLNodeStyleGet##type_name(node)); \
  }

#define JNI_SUPPORTED_LENGTH_STYLE_GETTER(V) \
  V(FlexBasis)                               \
  V(Width)                                   \
  V(Height)                                  \
  V(MinWidth)                                \
  V(MaxWidth)                                \
  V(MinHeight)                               \
  V(MaxHeight)

JNI_SUPPORTED_LENGTH_STYLE_GETTER(JNI_GET_LENGTH_STYLE)

#undef JNI_SUPPORTED_LENGTH_STYLE_GETTER
#undef JNI_GET_LENGTH_STYLE

// for gni: get margin, padding, position(top, right, bottom, left).
#define JNI_GET_LENGTH_WITH_EDGE_STYLE(type_name)                     \
  jlong Get##type_name(JNIEnv* env, jobject jcaller, jlong nativePtr, \
                       jint type) {                                   \
    SLNodeRef node = reinterpret_cast<SLNodeRef>(nativePtr);          \
    return JNIStarlightValue::asJavaLong(                             \
        SLNodeStyleGet##type_name(node, static_cast<SLEdge>(type)));  \
  }

#define JNI_SUPPORTED_LENGTH_WITH_EDGE_STYLE(V) \
  V(Margin)                                     \
  V(Padding)                                    \
  V(Position)

JNI_SUPPORTED_LENGTH_WITH_EDGE_STYLE(JNI_GET_LENGTH_WITH_EDGE_STYLE)

#undef JNI_SUPPORTED_LENGTH_WITH_EDGE_STYLE
#undef JNI_GET_LENGTH_WITH_EDGE_STYLE
