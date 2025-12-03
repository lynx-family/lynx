// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/ui_delegate_android.h"

#include <memory>

#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxUIRenderer_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxUIRenderer_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxUIRenderer(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

jlong CreateUIDelegate(JNIEnv* env, jclass jcaller, jlong painting_context_ptr,
                       jlong layout_context_ptr) {
  return reinterpret_cast<jlong>(new lynx::tasm::UIDelegateAndroid(
      painting_context_ptr, layout_context_ptr));
}

void DestroyUIDelegate(JNIEnv* env, jclass jcaller, jlong ui_delegate_ptr) {
  delete reinterpret_cast<lynx::tasm::UIDelegateAndroid*>(ui_delegate_ptr);
}

namespace lynx {
namespace tasm {

std::unique_ptr<PaintingCtxPlatformImpl>
UIDelegateAndroid::CreatePaintingContext() {
  return std::unique_ptr<PaintingCtxPlatformImpl>(painting_context_);
}

std::unique_ptr<LayoutCtxPlatformImpl>
UIDelegateAndroid::CreateLayoutContext() {
  return std::unique_ptr<LayoutCtxPlatformImpl>(layout_context_);
}

std::unique_ptr<PropBundleCreator>
UIDelegateAndroid::CreatePropBundleCreator() {
  return std::make_unique<PropBundleCreatorAndroid>();
}

void UIDelegateAndroid::OnLynxCreate(
    const std::shared_ptr<shell::ListEngineProxy>& list_engine_proxy,
    const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
    const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
    const std::shared_ptr<shell::LynxLayoutProxy>& layout_proxy,
    const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
    const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
    const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
    const fml::RefPtr<fml::TaskRunner>& layout_task_runner, int32_t instance_id,
    bool is_embedded_mode) {
  if (is_embedded_mode) {
    layout_context_->SetTriggerLayoutCallback(
        [engine_proxy]() { engine_proxy->TriggerLayout(); });
  } else {
    layout_context_->SetTriggerLayoutCallback(
        [layout_proxy]() { layout_proxy->TriggerLayout(); });
  }
}

}  // namespace tasm
}  // namespace lynx
