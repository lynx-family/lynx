// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/ui_wrapper/painting/android/native_painting_context_platform_android_ref.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"

namespace {

namespace base = lynx::base;
namespace shell = lynx::shell;
namespace tasm = lynx::tasm;

constexpr int kRendererId = 101;
using Result = lynx::base::android::ScopedGlobalJavaRef<jobject>;
using Scheduler = lynx::tasm::PlatformRendererContext::PreparationScheduler;

// No worker is scheduled: consuming the renderer must claim this unstarted
// task.
class DeferredRendererState {
 public:
  DeferredRendererState(JNIEnv* env, jobject java_context, jobject completion)
      : context(env, java_context,
                std::make_shared<Scheduler>([](lynx::base::closure) {})) {
    auto preparation = context.GetPreparationScheduler()->Schedule(
        [this, completion = Result(env, completion)]() mutable {
          ++preparation_runs;
          return std::move(completion);
        });
    renderer = fml::MakeRefCounted<lynx::tasm::PlatformRendererAndroid>(
        &context, kRendererId, PlatformRendererType::kUnknown,
        lynx::base::String("deferred-test"), nullptr,
        lynx::tasm::PlatformRendererInitConfig(), std::move(preparation));
  }

  // The context and counter outlive the renderer and its task.
  lynx::tasm::PlatformRendererContext context;
  int preparation_runs = 0;
  fml::RefPtr<lynx::tasm::PlatformRendererAndroid> renderer;
  fml::RefPtr<lynx::tasm::PlatformRendererAndroid> other_renderer;
};

DeferredRendererState* State(jlong ptr) {
  return reinterpret_cast<DeferredRendererState*>(ptr);
}

}  // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeCreate(
    JNIEnv* env, jclass, jobject context, jobject completion) {
  return reinterpret_cast<jlong>(
      new DeferredRendererState(env, context, completion));
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeCacheLayout(
    JNIEnv*, jclass, jlong ptr) {
  auto* state = State(ptr);
  const float paddings[] = {1, 2, 3, 4};
  const float margins[] = {5, 6, 7, 8};
  const float borders[] = {9, 10, 11, 12};
  state->renderer->UpdateLayoutMetrics(20, 30, 100, 200, paddings, margins,
                                       borders);
  return state->context.GetPlatformRenderer(kRendererId) ==
             state->renderer.get() &&
         state->renderer->HasLayoutMetrics() &&
         state->renderer->GetLayoutPaddings()[0] == 1 &&
         state->renderer->GetLayoutMargins()[0] == 5 &&
         state->renderer->GetLayoutBorders()[0] == 9;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativePreparationRuns(
    JNIEnv*, jclass, jlong ptr) {
  return State(ptr)->preparation_runs;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeHasPendingPreparations(
    JNIEnv*, jclass, jlong ptr) {
  return State(ptr)->context.HasPendingPreparations();
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeAddAbortedPreparation(
    JNIEnv*, jclass, jlong ptr) {
  auto* state = State(ptr);
  auto preparation = state->context.GetPreparationScheduler()->Schedule(
      []() { return Result(); });
  state->other_renderer = fml::MakeRefCounted<tasm::PlatformRendererAndroid>(
      &state->context, kRendererId + 1, PlatformRendererType::kUnknown,
      base::String("deferred-test"), nullptr,
      tasm::PlatformRendererInitConfig(), std::move(preparation));
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeConsumeAbortedPreparation(
    JNIEnv*, jclass, jlong ptr) {
  State(ptr)->other_renderer->EnsureAndroidViewCreated();
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeUpdateDisplayList(
    JNIEnv*, jclass, jlong ptr) {
  lynx::tasm::DisplayListBuilder builder;
  builder.Begin(kRendererId, PlatformRendererType::kUnknown, 20, 30, 100, 200)
      .End();
  auto renderer = State(ptr)->renderer;
  renderer->UpdateDisplayList(builder.Build());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeTearDownRenderer(
    JNIEnv*, jclass, jlong ptr, jboolean destroy_context) {
  auto* state = State(ptr);
  if (destroy_context) {
    state->context.Destroy();
    // A late host request must not finalize into a destroyed owner.
    state->renderer->EnsureAndroidViewCreated();
  }
  state->renderer = nullptr;
  return state->context.GetPlatformRenderer(kRendererId) == nullptr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeDestroy(
    JNIEnv*, jclass, jlong ptr) {
  delete State(ptr);
}
