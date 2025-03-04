// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/ui_wrapper/layout/android/layout_context_android.h"

#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/base/android/android_jni.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/LayoutContext_jni.h"
#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/value_wrapper/value_impl_lepus.h"

void TriggerLayout(JNIEnv* env, jobject jcaller, jlong ptr) {
  reinterpret_cast<lynx::tasm::LayoutContextAndroid*>(ptr)->TriggerLayout();
}

jlong CreateLayoutContext(JNIEnv* env, jobject jcaller,
                          jobject layout_context) {
  return reinterpret_cast<jlong>(
      new lynx::tasm::LayoutContextAndroid(env, layout_context));
}

namespace lynx {
namespace tasm {

static inline lepus::Value ResolveFontFaceToken(
    const std::vector<std::shared_ptr<FontFaceToken>>& tokenList) {
  lepus::Value result;
  auto dict = lepus::Dictionary::Create();
  if (tokenList.size() == 0) {
    return result;
  }
  auto token = tokenList[0];
  const FontFaceAttrsMap& map = token->second;
  for (const auto& iter : map) {
    dict->SetValue(iter.first, iter.second);
  }
  result.SetTable(dict);
  return result;
}

bool LayoutContextAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

LayoutContextAndroid::LayoutContextAndroid(JNIEnv* env, jobject impl)
    : impl_(env, impl), bundle_holder_() {
  Java_LayoutContext_attachNativePtr(env, impl, reinterpret_cast<jlong>(this));
}

LayoutContextAndroid::~LayoutContextAndroid() = default;

int LayoutContextAndroid::CreateLayoutNode(int id, const std::string& tag,
                                           PropBundle* painting_data,
                                           bool is_parent_inline_container) {
  PropBundleAndroid* pda = static_cast<PropBundleAndroid*>(painting_data);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return COMMON;
  }
  base::android::ScopedLocalJavaRef<jstring> tag_ref =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, tag);
  jint result;
  if (pda) {
    // we now split props into attributes & styles.
    // if using mapBuffer, css styles parts in stored in styles, otherwise
    // styles is null and css styles info & attribute info is stored in
    // attributes.
    // TODO(@nihao.royal): after attribute is converted into integer format, we
    // can merge attributes and styles.
    result = Java_LayoutContext_createNode(
        env, local_ref.Get(), id, tag_ref.Get(), pda->jni_map()->jni_object(),
        pda->GetStyleMapBuffer().Get(),
        pda->jni_event_handler_map()
            ? pda->jni_event_handler_map()->jni_object()
            : nullptr,
        is_parent_inline_container);
  } else {
    result = Java_LayoutContext_createNode(env, local_ref.Get(), id,
                                           tag_ref.Get(), nullptr, nullptr,
                                           nullptr, is_parent_inline_container);
  }
  if (base::android::HasJNIException()) {
    lynx::base::ErrorStorage::GetInstance().AddCustomInfoToError(
        {{"tag", tag}});
  }

  return result;
}

void LayoutContextAndroid::InsertLayoutNode(int parent, int child, int index) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LayoutContext_insertNode(env, local_ref.Get(), parent, child, index);
}

void LayoutContextAndroid::RemoveLayoutNode(int parent, int child, int index) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LayoutContext_removeNode(env, local_ref.Get(), parent, child, index);
}

void LayoutContextAndroid::MoveLayoutNode(int parent, int child, int from_index,
                                          int to_index) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LayoutContext_moveNode(env, local_ref.Get(), parent, child, from_index,
                              to_index);
}

void LayoutContextAndroid::UpdateLayoutNode(int id, PropBundle* painting_data) {
  PropBundleAndroid* pda = static_cast<PropBundleAndroid*>(painting_data);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LayoutContext.UpdateLayoutNode");
  // we now split props into attributes & styles.
  // if using mapBuffer, css styles parts in stored in styles, otherwise
  // styles is null and css styles info & attribute info is stored in
  // attributes.
  // TODO(@nihao.royal): after attribute is converted into integer format, we
  // can merge attributes and styles.
  Java_LayoutContext_updateProps(
      env, local_ref.Get(), id, pda->jni_map()->jni_object(),
      pda->GetStyleMapBuffer().Get(),
      pda->jni_event_handler_map() ? pda->jni_event_handler_map()->jni_object()
                                   : nullptr);
}

void LayoutContextAndroid::OnLayoutBefore(int id) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LayoutContext_dispatchOnLayoutBefore(env, local_ref.Get(), id);
}

void LayoutContextAndroid::OnLayout(int id, float left, float top, float width,
                                    float height,
                                    const std::array<float, 4>& paddings,
                                    const std::array<float, 4>& borders) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LayoutContext_dispatchOnLayout(env, local_ref.Get(), id, left, top,
                                      width, height);
}

void LayoutContextAndroid::ScheduleLayout(base::closure callback) {
  trigger_layout_callback_ = std::move(callback);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LayoutContext_scheduleLayout(env, local_ref.Get());
}

void LayoutContextAndroid::DestroyLayoutNodes(
    const std::unordered_set<int>& ids) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  std::vector<int> ids_array(ids.begin(), ids.end());
  base::android::ScopedLocalJavaRef<jintArray> jni_array(
      env, env->NewIntArray(ids_array.size()));
  env->SetIntArrayRegion(jni_array.Get(), 0, ids_array.size(), &ids_array[0]);
  Java_LayoutContext_destroyNodes(env, local_ref.Get(), jni_array.Get());
}

void LayoutContextAndroid::Destroy() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LayoutContext_detachNativePtr(env, local_ref.Get());
}

void LayoutContextAndroid::SetFontFaces(const FontFacesMap& fontfaces) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }

  auto props = std::make_unique<base::android::JavaOnlyMap>();

  lepus::Value lepus_font_faces;
  auto dict = lepus::Dictionary::Create();
  for (const auto& it : fontfaces) {
    dict->SetValue(it.first, ResolveFontFaceToken(it.second));
  }
  lepus_font_faces.SetTable(dict);

  PropBundleAndroid::AssembleMap(props.get(), "fontfaces",
                                 pub::ValueImplLepus(lepus_font_faces));

  jobject font_faces_jni = props->jni_object();

  Java_LayoutContext_setFontFaces(env, local_ref.Get(), font_faces_jni);
}

std::unique_ptr<PlatformExtraBundle>
LayoutContextAndroid::GetPlatformExtraBundle(int32_t id) {
  JNIEnv* env = base::android::AttachCurrentThread();

  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return std::unique_ptr<PlatformExtraBundle>();
  }

  auto java_bundle =
      Java_LayoutContext_getExtraBundle(env, local_ref.Get(), id);
  if (java_bundle.IsNull()) {
    return std::unique_ptr<PlatformExtraBundle>();
  }

  if (!bundle_holder_) {
    bundle_holder_ = std::make_unique<PlatformBundleHolderAndroid>(env);
  }

  return std::make_unique<PlatformExtraBundleAndroid>(id, bundle_holder_.get(),
                                                      java_bundle);
}

std::unique_ptr<PlatformExtraBundleHolder>
LayoutContextAndroid::ReleasePlatformBundleHolder() {
  return std::move(bundle_holder_);
}

void LayoutContextAndroid::TriggerLayout() { trigger_layout_callback_(); }

void LayoutContextAndroid::SetLayoutNodeManager(
    LayoutNodeManager* layout_node_manager) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(impl_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LayoutContext_attachLayoutNodeManager(
      env, local_ref.Get(), reinterpret_cast<long>(layout_node_manager));
}

}  // namespace tasm
}  // namespace lynx
