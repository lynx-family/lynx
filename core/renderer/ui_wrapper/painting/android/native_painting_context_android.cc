// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/native_painting_context_android.h"

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "platform/android/lynx_android/src/main/jni/gen/NativePaintingContext_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/NativePaintingContext_register_jni.h"

// TODO: implement necessary functions for native ui renderer.
jlong CreatePaintingContext(JNIEnv *env, jobject jcaller, jobject jThis,
                            jlong platformRendererContextPtr) {
  return reinterpret_cast<jlong>(new lynx::tasm::NativePaintingCtxAndroid());
}
namespace lynx {
namespace jni {
bool RegisterJNIForNativePaintingContext(JNIEnv *env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni

namespace tasm {
void NativePaintingCtxAndroid::SetUIOperationQueue(
    const std::shared_ptr<shell::DynamicUIOperationQueue> &queue) {
  PaintingCtxPlatformImpl::SetUIOperationQueue(queue);
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
  return std::unique_ptr<pub::Value>();
}

void NativePaintingCtxAndroid::UpdateLayout(
    int tag, float x, float y, float width, float height, const float *paddings,
    const float *margins, const float *borders, const float *bounds,
    const float *sticky, float max_height, uint32_t node_index) {}

void NativePaintingCtxAndroid::UpdatePlatformExtraBundle(
    int32_t id, PlatformExtraBundle *bundle) {
  PaintingCtxPlatformImpl::UpdatePlatformExtraBundle(id, bundle);
}

void NativePaintingCtxAndroid::SetKeyframes(
    fml::RefPtr<PropBundle> keyframes_data) {}

void NativePaintingCtxAndroid::Flush() {}

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
}  // namespace tasm
}  // namespace lynx
