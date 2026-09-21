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

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeCheckFlushBatches(
    JNIEnv* env, jclass, jobject java_context, jobject completion) {
  std::vector<base::closure> workers;
  auto scheduler = std::make_shared<Scheduler>(
      [&](base::closure task) { workers.push_back(std::move(task)); });
  auto* context =
      new tasm::PlatformRendererContext(env, java_context, scheduler);
  tasm::NativePaintingCtxAndroid painting(env, nullptr, 0, context);
  auto ref = std::static_pointer_cast<tasm::NativePaintingCtxAndroidRef>(
      painting.GetPlatformRef());
  auto queue = std::make_shared<shell::DynamicUIOperationQueue>(
      base::ThreadStrategyForRendering::ALL_ON_UI, nullptr);
  painting.SetUIOperationQueue(queue);
  queue->SetEnableFlush(false);
  auto register_renderer = [&](int id, Scheduler::TaskRef task) {
    ref->CreatePreparedRenderer(id, PlatformRendererType::kUnknown,
                                base::String("deferred-test"), nullptr,
                                tasm::PlatformRendererInitConfig(), task);
  };
  auto enqueue_host = [&](int id) {
    tasm::DisplayListBuilder builder;
    builder.Begin(id, PlatformRendererType::kUnknown, 0, 0, 10, 10).End();
    painting.EnqueueDisplayList(id, builder.Build());
  };

  std::promise<void> started, release;
  auto gate = release.get_future();
  bool assisted = false;
  register_renderer(
      101, scheduler->Schedule([&, result = Result(env, completion)]() mutable {
        started.set_value();
        assisted =
            gate.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
        return std::move(result);
      }));
  std::thread worker([task = std::move(workers[0])]() mutable { task(); });
  started.get_future().wait();
  std::vector<int> order;
  register_renderer(
      102, scheduler->Schedule([&, result = Result(env, completion)]() mutable {
        order.push_back(102);
        release.set_value();
        return std::move(result);
      }));
  register_renderer(
      103, scheduler->Schedule([&, result = Result(env, completion)]() mutable {
        order.push_back(103);
        context->GetPlatformRenderer(103)->EnsureAndroidViewCreated();
        return std::move(result);
      }));
  enqueue_host(101);
  painting.Flush();

  // Coalesced high-priority operations must not replace the first flush's work.
  register_renderer(
      201, scheduler->Schedule([&, result = Result(env, completion)]() mutable {
        order.push_back(201);
        return std::move(result);
      }));
  enqueue_host(201);
  painting.Flush();
  queue->SetEnableFlush(true);
  queue->ForceFlush();
  worker.join();
  const bool correct_order = order == std::vector<int>({103, 102, 201});
  // The reentrant rejection in preparation must leave 103 available to
  // finalize.
  context->GetPlatformRenderer(103)->EnsureAndroidViewCreated();
  context->GetPlatformRenderer(103)->EnsureAndroidViewCreated();
  const bool drained = !scheduler->TakeBatch();
  ref->Destroy();
  return assisted && correct_order && drained;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_tasm_behavior_render_DeferredRendererPreparationTest_nativeCheckFlushReset(
    JNIEnv* env, jclass, jobject java_context) {
  std::vector<base::closure> workers;
  auto scheduler = std::make_shared<Scheduler>(
      [&](base::closure task) { workers.push_back(std::move(task)); });
  auto* context =
      new tasm::PlatformRendererContext(env, java_context, scheduler);
  tasm::NativePaintingCtxAndroid painting(env, nullptr, 0, context);
  auto queue = std::make_shared<shell::DynamicUIOperationQueue>(
      base::ThreadStrategyForRendering::ALL_ON_UI, nullptr);
  painting.SetUIOperationQueue(queue);
  int preparations = 0;
  scheduler->Schedule([&] {
    ++preparations;
    return Result();
  });
  // Layout-only flushing must release assistance even without
  // FinishTasmOperation.
  painting.UpdateLayout(999, 0, 0, 10, 10, nullptr, nullptr, nullptr, nullptr,
                        nullptr, 0, 0, false);
  painting.Flush();
  std::promise<void> started, release;
  auto gate = release.get_future();
  auto task = scheduler->Schedule([&] {
    started.set_value();
    gate.wait_for(std::chrono::milliseconds(100));
    return Result();
  });
  std::thread worker([work = std::move(workers.back())]() mutable { work(); });
  started.get_future().wait();
  auto renderer = lynx::fml::MakeRefCounted<tasm::PlatformRendererAndroid>(
      context, 101, PlatformRendererType::kUnknown,
      base::String("deferred-test"), nullptr,
      tasm::PlatformRendererInitConfig(), task);
  renderer->EnsureAndroidViewCreated();
  worker.join();
  return preparations == 0;
}
