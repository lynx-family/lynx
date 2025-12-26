// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/native_painting_context_android.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/layout/android/text_layout_android.h"
#include "core/renderer/ui_wrapper/painting/android/native_painting_context_platform_android_ref.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"
#include "core/shell/lynx_shell.h"
#include "platform/android/lynx_android/src/main/jni/gen/NativePaintingContext_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/NativePaintingContext_register_jni.h"

// TODO: implement necessary functions for native ui renderer.
jlong CreatePaintingContext(JNIEnv *env, jobject jcaller, jobject jThis,
                            jlong platformRendererContextPtr,
                            jobject textLayout) {
  // This native object will be managed by NativePaintingContextAndroid with
  // unique_ptr.
  return reinterpret_cast<jlong>(new lynx::tasm::NativePaintingCtxAndroid(
      env, textLayout,
      reinterpret_cast<lynx::tasm::PlatformRendererContext *>(
          platformRendererContextPtr)));
}

void SetLynxEngineActorForPlatformRendererContext(JNIEnv *env,
                                                  jobject /*jcaller*/,
                                                  jlong nativePtr, jlong ptr) {
  // Get the NativePaintingCtxAndroid instance from the native pointer
  if (nativePtr == 0 || ptr == 0) {
    return;
  }

  lynx::tasm::NativePaintingCtxAndroid *context =
      reinterpret_cast<lynx::tasm::NativePaintingCtxAndroid *>(nativePtr);

  auto *shell = reinterpret_cast<lynx::shell::LynxShell *>(ptr);
  if (shell == nullptr) {
    return;
  }
  auto platform_ref =
      std::static_pointer_cast<lynx::tasm::NativePaintingCtxAndroidRef>(
          context->GetPlatformRef());
  if (platform_ref == nullptr) {
    return;
  }
  platform_ref->SetLynxEngineActorForPlatformRendererContext(
      shell->GetEngineActor());
}

jboolean DispatchPlatformInputEvent(JNIEnv *env, jobject /*jcaller*/,
                                    jlong nativePtr, jintArray iEventData,
                                    jfloatArray fEventData) {
  // Get the NativePaintingCtxAndroid instance from the native pointer
  if (nativePtr == 0 || iEventData == nullptr || fEventData == nullptr) {
    return JNI_FALSE;
  }

  lynx::tasm::NativePaintingCtxAndroid *context =
      reinterpret_cast<lynx::tasm::NativePaintingCtxAndroid *>(nativePtr);

  jint *i_event_data = env->GetIntArrayElements(iEventData, JNI_FALSE);
  if (i_event_data == nullptr) {
    env->ReleaseIntArrayElements(iEventData, i_event_data, 0);
    return JNI_FALSE;
  }
  jfloat *f_event_data = env->GetFloatArrayElements(fEventData, JNI_FALSE);
  if (f_event_data == nullptr) {
    env->ReleaseFloatArrayElements(fEventData, f_event_data, 0);
    return JNI_FALSE;
  }
  auto platform_ref =
      std::static_pointer_cast<lynx::tasm::NativePaintingCtxAndroidRef>(
          context->GetPlatformRef());
  if (platform_ref == nullptr) {
    return JNI_FALSE;
  }

  auto res =
      platform_ref->DispatchPlatformInputEvent(i_event_data, f_event_data);
  env->ReleaseIntArrayElements(iEventData, i_event_data, 0);
  env->ReleaseFloatArrayElements(fEventData, f_event_data, 0);
  return res;
}

jint GetPlatformEventHandlerState(JNIEnv *env, jobject /*jcaller*/,
                                  jlong nativePtr) {
  // Get the NativePaintingCtxAndroid instance from the native pointer
  if (nativePtr == 0) {
    return 0;
  }

  lynx::tasm::NativePaintingCtxAndroid *context =
      reinterpret_cast<lynx::tasm::NativePaintingCtxAndroid *>(nativePtr);

  auto platform_ref =
      std::static_pointer_cast<lynx::tasm::NativePaintingCtxAndroidRef>(
          context->GetPlatformRef());
  if (platform_ref == nullptr) {
    return 0;
  }
  return platform_ref->GetPlatformEventHandlerState();
}

namespace lynx {
namespace jni {
bool RegisterJNIForNativePaintingContext(JNIEnv *env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni

namespace tasm {

NativePaintingCtxAndroid::NativePaintingCtxAndroid(
    JNIEnv *env, jobject text_layout, PlatformRendererContext *view_manager)
    : view_manager_(std::unique_ptr<PlatformRendererContext>(view_manager)) {
  platform_ref_ = std::make_shared<NativePaintingCtxAndroidRef>(
      std::make_unique<PlatformRendererAndroidFactory>(view_manager_.get()));
  text_layout_impl_ = std::make_unique<TextLayoutAndroid>(env, text_layout);
}

void NativePaintingCtxAndroid::SetUIOperationQueue(
    const std::shared_ptr<shell::UIOperationQueueInterface> &queue) {
  queue_ = std::static_pointer_cast<shell::DynamicUIOperationQueue>(queue);
}

void NativePaintingCtxAndroid::CreatePaintingNode(
    int id, const std::string &tag,
    const fml::RefPtr<PropBundle> &painting_data, bool flatten,
    bool create_node_async, uint32_t node_index) {}

void NativePaintingCtxAndroid::UpdatePaintingNode(
    int id, bool tend_to_flatten,
    const fml::RefPtr<PropBundle> &painting_data) {}

std::unique_ptr<pub::Value> NativePaintingCtxAndroid::GetTextInfo(
    const std::string &content, const pub::Value &info) {
  // TODO: impl this function later.
  return std::unique_ptr<pub::Value>();
}

void NativePaintingCtxAndroid::StopExposure(const pub::Value &options) {
  // TODO: impl this function later.
}

void NativePaintingCtxAndroid::ResumeExposure() {
  // TODO: impl this function later.
}

void NativePaintingCtxAndroid::CreatePlatformExtendedRenderer(
    int id, const base::String &tag_name) {
  Enqueue([ref = platform_ref_, id, tag_name]() {
    std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref)
        ->CreatePlatformExtendedRenderer(id, tag_name);
  });
}

void NativePaintingCtxAndroid::UpdateLayout(
    int tag, float x, float y, float width, float height, const float *paddings,
    const float *margins, const float *borders, const float *bounds,
    const float *sticky, float max_height, uint32_t node_index) {}

void NativePaintingCtxAndroid::UpdatePlatformExtraBundle(
    int32_t id, PlatformExtraBundle *bundle) {
  // TODO: impl this function later.
}

void NativePaintingCtxAndroid::SetKeyframes(
    fml::RefPtr<PropBundle> keyframes_data) {}

void NativePaintingCtxAndroid::Flush() { queue_->Flush(); }

void NativePaintingCtxAndroid::HandleValidate(int tag) {}

void NativePaintingCtxAndroid::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions> &options) {}

void NativePaintingCtxAndroid::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions> &options) {}

std::vector<float> NativePaintingCtxAndroid::getBoundingClientOrigin(int id) {
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxAndroid::getWindowSize(int id) {
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxAndroid::GetRectToWindow(int id) {
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxAndroid::GetRectToLynxView(int64_t id) {
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxAndroid::ScrollBy(int64_t id, float width,
                                                      float height) {
  return std::vector<float>();
}

void NativePaintingCtxAndroid::Invoke(
    int64_t id, const std::string &method, const pub::Value &params,
    const std::function<void(int32_t, const pub::Value &)> &callback) {}

int32_t NativePaintingCtxAndroid::GetTagInfo(const std::string &tag_name) {
  return 0;
}

bool NativePaintingCtxAndroid::IsFlatten(
    base::MoveOnlyClosure<bool, bool> func) {
  return false;
}

bool NativePaintingCtxAndroid::NeedAnimationProps() { return false; }

void NativePaintingCtxAndroid::CreatePlatformRenderer(
    int id, PlatformRendererType type) {
  Enqueue([ref = platform_ref_, id, type]() {
    std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref)
        ->CreatePlatformRenderer(id, type);
  });
}

void NativePaintingCtxAndroid::UpdateDisplayList(int id,
                                                 DisplayList display_list) {
  Enqueue([ref = platform_ref_, id, dl = std::move(display_list)]() mutable {
    std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref)
        ->UpdateDisplayList(id, std::move(dl));
  });
}

void NativePaintingCtxAndroid::CreateImage(int id, base::String src,
                                           float width, float height) {
  view_manager_->CreateImage(id, src, width, height);
}

}  // namespace tasm
}  // namespace lynx
