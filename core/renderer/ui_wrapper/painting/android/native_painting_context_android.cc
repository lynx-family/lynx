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
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"
#include "core/renderer/ui_wrapper/layout/android/text_layout_android.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"
#include "core/renderer/ui_wrapper/painting/android/native_painting_context_platform_android_ref.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/shell/lynx_shell.h"
#include "core/value_wrapper/value_wrapper_utils.h"
#include "platform/android/lynx_android/src/main/jni/gen/NativePaintingContext_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/NativePaintingContext_register_jni.h"

// TODO: implement necessary functions for native ui renderer.
jlong CreatePaintingContext(JNIEnv *env, jobject jcaller, jobject jThis,
                            jlong platformRendererContextPtr,
                            jobject textLayout, jlong textra) {
  // This native object will be managed by NativePaintingContextAndroid with
  // unique_ptr.
  return reinterpret_cast<jlong>(new lynx::tasm::NativePaintingCtxAndroid(
      env, textLayout, textra,
      reinterpret_cast<lynx::tasm::PlatformRendererContext *>(
          platformRendererContextPtr)));
}

void SetLynxEngineActorForPlatformContextRef(JNIEnv *env, jobject /*jcaller*/,
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
  platform_ref->SetLynxEngineActorForPlatformContextRef(
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

  jsize i_event_data_size = env->GetArrayLength(iEventData);
  jint *i_event_data = env->GetIntArrayElements(iEventData, JNI_FALSE);
  if (i_event_data == nullptr) {
    return JNI_FALSE;
  }
  jfloat *f_event_data = env->GetFloatArrayElements(fEventData, JNI_FALSE);
  if (f_event_data == nullptr) {
    env->ReleaseIntArrayElements(iEventData, i_event_data, 0);
    return JNI_FALSE;
  }
  auto platform_ref =
      std::static_pointer_cast<lynx::tasm::NativePaintingCtxAndroidRef>(
          context->GetPlatformRef());
  if (platform_ref == nullptr) {
    env->ReleaseIntArrayElements(iEventData, i_event_data, 0);
    env->ReleaseFloatArrayElements(fEventData, f_event_data, 0);
    return JNI_FALSE;
  }

  int32_t event_target_root_id = kRootId;
  if (i_event_data_size > 4) {
    event_target_root_id = i_event_data[4];
  }
  auto res = platform_ref->DispatchPlatformInputEvent(
      i_event_data, f_event_data, event_target_root_id);
  env->ReleaseIntArrayElements(iEventData, i_event_data, 0);
  env->ReleaseFloatArrayElements(fEventData, f_event_data, 0);
  return res;
}

void SetPlatformEventRootActive(JNIEnv *env, jobject /*jcaller*/,
                                jlong nativePtr, jint rootSign,
                                jboolean active) {
  if (nativePtr == 0) {
    return;
  }
  auto *context =
      reinterpret_cast<lynx::tasm::NativePaintingCtxAndroid *>(nativePtr);
  auto platform_ref =
      std::static_pointer_cast<lynx::tasm::NativePaintingCtxAndroidRef>(
          context->GetPlatformRef());
  if (platform_ref) {
    platform_ref->SetPlatformEventRootActive(rootSign, active == JNI_TRUE);
  }
}

void SetPlatformEventRootOffset(JNIEnv *env, jobject /*jcaller*/,
                                jlong nativePtr, jint rootSign, jfloat offsetX,
                                jfloat offsetY) {
  if (nativePtr == 0) {
    return;
  }
  auto *context =
      reinterpret_cast<lynx::tasm::NativePaintingCtxAndroid *>(nativePtr);
  auto platform_ref =
      std::static_pointer_cast<lynx::tasm::NativePaintingCtxAndroidRef>(
          context->GetPlatformRef());
  if (platform_ref) {
    platform_ref->SetPlatformEventRootOffset(rootSign, offsetX, offsetY);
  }
}

jintArray GetMeaningfulPaintingAreaRecords(JNIEnv *env, jobject /*jcaller*/,
                                           jlong nativePtr) {
  if (nativePtr == 0) {
    return nullptr;
  }

  auto *context =
      reinterpret_cast<lynx::tasm::NativePaintingCtxAndroid *>(nativePtr);
  auto platform_ref =
      std::static_pointer_cast<lynx::tasm::NativePaintingCtxAndroidRef>(
          context->GetPlatformRef());
  if (platform_ref == nullptr) {
    return nullptr;
  }

  auto records = platform_ref->CollectMeaningfulPaintingAreaRecords();
  auto result = env->NewIntArray(static_cast<jsize>(records.size()));
  if (result == nullptr || records.empty()) {
    return result;
  }
  env->SetIntArrayRegion(result, 0, static_cast<jsize>(records.size()),
                         records.data());
  return result;
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

void Destroy(JNIEnv *env, jobject /*jcaller*/, jlong nativePtr) {
  if (nativePtr == 0) {
    return;
  }
  auto *context =
      reinterpret_cast<lynx::tasm::NativePaintingCtxAndroid *>(nativePtr);
  auto platform_ref =
      std::static_pointer_cast<lynx::tasm::NativePaintingCtxAndroidRef>(
          context->GetPlatformRef());
  if (platform_ref) {
    platform_ref->Destroy();
  }
}

namespace lynx {
namespace jni {
bool RegisterJNIForNativePaintingContext(JNIEnv *env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni

namespace tasm {

NativePaintingCtxAndroid::NativePaintingCtxAndroid(
    JNIEnv *env, jobject text_layout, jlong textra,
    PlatformRendererContext *view_manager)
    : view_manager_(std::unique_ptr<PlatformRendererContext>(view_manager)) {
  platform_ref_ = std::make_shared<NativePaintingCtxAndroidRef>(
      std::make_unique<PlatformRendererAndroidFactory>(view_manager_.get()));
  if (textra != 0) {
    text_layout_impl_ =
        std::make_unique<TextLayoutTextra>(static_cast<intptr_t>(textra));
  } else {
    text_layout_impl_ = std::make_unique<TextLayoutAndroid>(env, text_layout);
  }
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
    const fml::RefPtr<PropBundle> &painting_data) {
  if (!painting_data) {
    return;
  }

  auto platform_ref = platform_ref_;
  Enqueue([platform_ref, id, tend_to_flatten, painting_data]() mutable {
    auto android_ref =
        std::static_pointer_cast<NativePaintingCtxAndroidRef>(platform_ref);
    if (!android_ref) {
      return;
    }

    android_ref->UpdateAttributes(id, painting_data, tend_to_flatten);
  });
}

std::unique_ptr<pub::Value> NativePaintingCtxAndroid::GetTextInfo(
    const std::string &content, const pub::Value &info) {
  // TODO: impl this function later.
  return std::unique_ptr<pub::Value>();
}

void NativePaintingCtxAndroid::StopExposure(const pub::Value &options) {
  auto runner = base::UIThread::GetRunner();
  if (!runner) {
    return;
  }

  auto lepus_options = pub::ValueUtils::ConvertValueToLepusValue(options);
  runner->PostTask(
      [ref = platform_ref_, options = std::move(lepus_options)]() mutable {
        auto android_ref =
            std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref);
        if (android_ref) {
          android_ref->StopExposure(options);
        }
      });
}

void NativePaintingCtxAndroid::ResumeExposure() {
  auto runner = base::UIThread::GetRunner();
  if (!runner) {
    return;
  }
  runner->PostTask([ref = platform_ref_]() mutable {
    auto android_ref =
        std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref);
    if (android_ref) {
      android_ref->ResumeExposure();
    }
  });
}

void NativePaintingCtxAndroid::CreatePlatformExtendedRenderer(
    int id, const base::String &tag_name,
    const fml::RefPtr<PropBundle> &init_data) {
  Enqueue([ref = platform_ref_, id, tag_name, data_ref = init_data]() {
    std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref)
        ->CreatePlatformExtendedRenderer(id, tag_name, data_ref);
  });
}

void NativePaintingCtxAndroid::UpdateLayout(
    int tag, float x, float y, float width, float height, const float *paddings,
    const float *margins, const float *borders, const float *bounds,
    const float *sticky, float max_height, uint32_t node_index) {}

void NativePaintingCtxAndroid::UpdatePlatformExtraBundle(
    int32_t id, PlatformExtraBundle *bundle) {
  if (!bundle) {
    return;
  }
  JNIEnv *env = base::android::AttachCurrentThread();
  base::android::ScopedGlobalJavaRef<jobject> java_bundle_ref(
      env, static_cast<PlatformExtraBundleAndroid *>(bundle)
               ->GetPlatformBundle()
               .Get());
  Enqueue([view_manager = view_manager_.get(), id,
           java_bundle_ref = std::move(java_bundle_ref)]() {
    view_manager->UpdatePlatformRendererExtraData(id, java_bundle_ref.Get());
  });
}

void NativePaintingCtxAndroid::SetKeyframes(
    fml::RefPtr<PropBundle> keyframes_data) {}

void NativePaintingCtxAndroid::Flush() { queue_->Flush(); }

void NativePaintingCtxAndroid::HandleValidate(int tag) {}

void NativePaintingCtxAndroid::OnFirstScreen() { has_first_screen_ = true; }

void NativePaintingCtxAndroid::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions> &options) {
  if (view_manager_) {
    Enqueue([view_manager = view_manager_.get(), options]() {
      view_manager->FinishTasmOperation(options->operation_id);
    });
  }
  if (options->native_update_data_order_ ==
      queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
  }
}

void NativePaintingCtxAndroid::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions> &options) {
  if (!has_first_screen_) {
    return;
  }

  if (view_manager_) {
    Enqueue([view_manager = view_manager_.get(), options]() {
      view_manager->FinishLayoutOperation(options->list_comp_id_,
                                          options->operation_id,
                                          options->is_first_screen);
    });
  }

  Enqueue([weak_queue = std::weak_ptr<shell::DynamicUIOperationQueue>(queue_),
           native_update_data_order = options->native_update_data_order_]() {
    if (auto queue = weak_queue.lock()) {
      if (native_update_data_order == queue->GetNativeUpdateDataOrder()) {
        queue->UpdateStatus(shell::UIOperationStatus::ALL_FINISH);
      }
    }
  });

  if (options->native_update_data_order_ ==
      queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
  }
}

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

// For BTS
void NativePaintingCtxAndroid::Invoke(
    int64_t id, const std::string &method, const pub::Value &params,
    const std::function<void(int32_t, const pub::Value &)> &callback) {
  auto runner = base::UIThread::GetRunner();
  if (!runner) {
    return;
  }
  const auto &lepus_params = pub::ValueUtils::ConvertValueToLepusValue(params);
  base::MoveOnlyClosure<void, int32_t, const pub::Value &> cb =
      [callback](int32_t code, const pub::Value &data) {
        callback(code, data);
      };
  // Since invokeUIMethod may not necessarily trigger the pipeline, we will
  // temporarily use TaskRunner to throw the UI thread for execution instead of
  // Enqueue.
  runner->PostTask([ref = platform_ref_, id, method,
                    params = lepus_params.ToLepusValue(),
                    cb = std::move(cb)]() mutable {
    auto android_ref =
        std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref);
    if (android_ref) {
      android_ref->InvokeUIMethod(id, method, params, std::move(cb));
    }
  });
}

// For MTS
void NativePaintingCtxAndroid::EnqueueInvoke(
    int64_t id, const std::string &method, const pub::Value &params,
    const std::function<void(int32_t, const pub::Value &)> &callback) {
  const auto &lepus_params = pub::ValueUtils::ConvertValueToLepusValue(params);
  base::MoveOnlyClosure<void, int32_t, const pub::Value &> cb =
      [callback](int32_t code, const pub::Value &data) {
        callback(code, data);
      };
  Enqueue([ref = platform_ref_, id, method,
           params = lepus_params.ToLepusValue(), cb = std::move(cb)]() mutable {
    auto android_ref =
        std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref);
    if (android_ref) {
      android_ref->InvokeUIMethod(id, method, params, std::move(cb));
    }
  });
}

int32_t NativePaintingCtxAndroid::GetTagInfo(const std::string &tag_name) {
  return view_manager_->GetTagInfo(tag_name);
}

bool NativePaintingCtxAndroid::IsFlatten(
    base::MoveOnlyClosure<bool, bool> func) {
  return false;
}

bool NativePaintingCtxAndroid::NeedAnimationProps() { return false; }

void NativePaintingCtxAndroid::CreatePlatformRenderer(
    int id, PlatformRendererType type,
    const fml::RefPtr<PropBundle> &init_data) {
  Enqueue([ref = platform_ref_, id, type, data_ref = init_data]() {
    std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref)
        ->CreatePlatformRenderer(id, type, data_ref);
  });
}

void NativePaintingCtxAndroid::UpdateDisplayList(int id,
                                                 DisplayList display_list) {
  Enqueue([ref = platform_ref_, id, dl = std::move(display_list)]() mutable {
    std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref)
        ->UpdateDisplayList(id, std::move(dl));
  });
}

void NativePaintingCtxAndroid::UpdatePlatformEventBundle(
    int32_t id, PlatformEventBundle bundle) {
  Enqueue([ref = platform_ref_, id, bundle = std::move(bundle)]() mutable {
    auto android_ref =
        std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref);
    if (android_ref) {
      android_ref->UpdatePlatformEventBundle(id, std::move(bundle));
    }
  });
}

void NativePaintingCtxAndroid::ReconstructEventTargetTreeRecursively() {
  auto platform_ref =
      std::static_pointer_cast<NativePaintingCtxPlatformRef>(platform_ref_);
  if (platform_ref && platform_ref->HasScheduledEventTargetTreeUpdate()) {
    return;
  }
  bool expected = false;
  if (!event_target_tree_update_enqueued_->compare_exchange_strong(expected,
                                                                   true)) {
    return;
  }
  Enqueue([enqueued = event_target_tree_update_enqueued_,
           ref = std::move(platform_ref)]() mutable {
    auto android_ref =
        std::static_pointer_cast<NativePaintingCtxAndroidRef>(ref);
    if (android_ref) {
      android_ref->ScheduleEnsureEventTargetTree(kRootId);
    }
    enqueued->store(false);
  });
}

void NativePaintingCtxAndroid::CreateImage(int id, base::String src,
                                           float width, float height,
                                           int32_t event_mask) {
  if (view_manager_) {
    view_manager_->CreateImage(id, src, width, height, event_mask);
  }
}

void NativePaintingCtxAndroid::UpdateTextBundle(int id, intptr_t bundle) {
  if (view_manager_) {
    view_manager_->UpdateTextBundle(id, bundle);
  }
}

void NativePaintingCtxAndroid::DestroyTextBundle(int id) {
  if (view_manager_) {
    view_manager_->DestroyTextBundle(id);
  }
}

void NativePaintingCtxAndroid::InsertListItemPaintingNode(int32_t list_id,
                                                          int32_t child_id) {
  if (!view_manager_) {
    return;
  }
  Enqueue([view_manager = view_manager_.get(), list_id, child_id]() {
    view_manager->InsertListItemPaintingNode(list_id, child_id);
  });
}

void NativePaintingCtxAndroid::RemoveListItemPaintingNode(int32_t list_id,
                                                          int32_t child_id) {
  if (!view_manager_) {
    return;
  }
  Enqueue([view_manager = view_manager_.get(), list_id, child_id]() {
    view_manager->RemoveListItemPaintingNode(list_id, child_id);
  });
}

void NativePaintingCtxAndroid::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  if (!view_manager_) {
    return;
  }
  Enqueue([view_manager = view_manager_.get(), container_id, content_size,
           delta_x, delta_y, is_init_scroll_offset, from_layout]() {
    view_manager->UpdateContentOffsetForListContainer(
        container_id, content_size, delta_x, delta_y, is_init_scroll_offset,
        from_layout);
  });
}

}  // namespace tasm
}  // namespace lynx
