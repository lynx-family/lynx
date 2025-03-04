// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/painting_context_android.h"

#include <future>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "core/base/lynx_trace_categories.h"
#include "core/base/thread/once_task.h"
#include "core/build/gen/PaintingContext_jni.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "core/renderer/tasm/react/android/mapbuffer/readable_compact_array_buffer.h"
#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"
#include "core/renderer/utils/android/text_utils_android.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/runtime/bindings/jsi/modules/android/method_invoker.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/shell/lynx_ui_operation_async_queue.h"
#include "core/value_wrapper/value_impl_lepus.h"

void InvokeCallback(JNIEnv* env, jobject jcaller, jlong context, jint callback,
                    jobject array) {
  if (!context) {
    return;
  }
  const auto& para =
      lynx::tasm::android::ValueConverterAndroid::ConvertJavaOnlyArrayToLepus(
          env, array);
  constexpr const static int32_t sErrCodeIndex = 0;
  constexpr const static int32_t sDataIndex = 1;
  reinterpret_cast<lynx::tasm::PaintingContextAndroid*>(context)
      ->InvokeUIMethodCallback(callback,
                               para.Array()->get(sErrCodeIndex).Int32(),
                               para.Array()->get(sDataIndex));
}

jlong CreatePaintingContext(JNIEnv* env, jobject jcaller,
                            jobject painting_context, jint thread_strategy,
                            jboolean enable_context_free) {
  return reinterpret_cast<jlong>(new lynx::tasm::PaintingContextAndroid(
      env, painting_context, thread_strategy, enable_context_free));
}

namespace lynx {
namespace tasm {

bool PaintingContextAndroidRef::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

PaintingContextAndroidRef::PaintingContextAndroidRef(JNIEnv* env, jobject impl)
    : java_ref_(base::android::ScopedWeakGlobalJavaRef<jobject>(env, impl)) {}

void PaintingContextAndroidRef::OnCollectExtraUpdates(int32_t id) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_onCollectExtraUpdates(env, local_ref.Get(), id);
}

void PaintingContextAndroidRef::InsertPaintingNode(int parent, int child,
                                                   int index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::InsertPaintingNodeTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_insertNode(env, local_ref.Get(), parent, child, index);
}

void PaintingContextAndroidRef::RemovePaintingNode(int parent, int child,
                                                   int index, bool is_move) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::RemovePaintingNodeTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_removeNode(env, local_ref.Get(), parent, child);
}

void PaintingContextAndroidRef::DestroyPaintingNode(int parent, int child,
                                                    int index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::DestroyPaintingNodeTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_destroyNode(env, local_ref.Get(), parent, child);
}

void PaintingContextAndroidRef::UpdateScrollInfo(int32_t container_id,
                                                 bool smooth,
                                                 float estimated_offset,
                                                 bool scrolling) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::UpdateScrollInfoTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_updateScrollInfo(env, local_ref.Get(), container_id,
                                        smooth, estimated_offset, scrolling);
}

// state: 1 - Acive 2 - Fail 3 - End
void PaintingContextAndroidRef::SetGestureDetectorState(int64_t idx,
                                                        int32_t gesture_id,
                                                        int32_t state) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "UIOperationQueue::SetGestureDetectorStateTask");

  // Check if the Java reference is null before proceeding.
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();

  // Call the Java method to set gesture detector state.
  Java_PaintingContext_SetGestureDetectorState(env, local_ref.Get(), idx,
                                               gesture_id, state);
}

void PaintingContextAndroidRef::UpdateNodeReadyPatching(
    std::vector<int32_t> ready_ids, std::vector<int32_t> remove_ids) {
  if (ready_ids.empty() && remove_ids.empty()) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "UIOperationQueue::UpdateNodeReadyPatchingTask");
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jintArray> node_ready_ids(
      env, env->NewIntArray(ready_ids.size()));
  env->SetIntArrayRegion(node_ready_ids.Get(), 0, ready_ids.size(),
                         &ready_ids[0]);
  base::android::ScopedLocalJavaRef<jintArray> node_remove_ids(
      env, env->NewIntArray(remove_ids.size()));
  env->SetIntArrayRegion(node_remove_ids.Get(), 0, remove_ids.size(),
                         &remove_ids[0]);

  Java_PaintingContext_updateNodeReadyPatching(
      env, local_ref.Get(), node_ready_ids.Get(), node_remove_ids.Get());
}

void PaintingContextAndroidRef::UpdateNodeReloadPatching(
    std::vector<int32_t> reload_ids) {
  if (reload_ids.empty()) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "UIOperationQueue::UpdateNodeReloadPatchingTask");
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  size_t size = reload_ids.size();
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jintArray> node_reload_ids(
      env, env->NewIntArray(size));
  env->SetIntArrayRegion(node_reload_ids.Get(), 0, size, &reload_ids[0]);
  Java_PaintingContext_updateNodeReloadPatching(env, local_ref.Get(),
                                                node_reload_ids.Get());
}

void PaintingContextAndroidRef::UpdateEventInfo(bool has_touch_pseudo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::UpdateEventInfoTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_updateEventInfo(env, local_ref.Get(),
                                       static_cast<jboolean>(has_touch_pseudo));
}

void PaintingContextAndroidRef::UpdateFlattenStatus(int id, bool flatten) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::UpdateFlattenStatusTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_updateFlattenStatus(env, local_ref.Get(), id, flatten);
}

void PaintingContextAndroidRef::ListReusePaintingNode(
    int sign, const std::string& item_key) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  const auto key =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, item_key);
  Java_PaintingContext_reuseListNode(env, local_ref.Get(), sign, key.Get());
}

void PaintingContextAndroidRef::ListCellWillAppear(
    int sign, const std::string& item_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::ListCellWillAppearTask");
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }
  const auto key =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, item_key);

  Java_PaintingContext_listCellAppear(env, local_ref.Get(), sign, key.Get());
}

void PaintingContextAndroidRef::ListCellDisappear(int sign, bool isExist,
                                                  const std::string& item_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::ListCellDisappearTask");
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  const auto key =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, item_key);
  Java_PaintingContext_listCellDisappear(env, local_ref.Get(), sign, isExist,
                                         key.Get());
}

void PaintingContextAndroidRef::InsertListItemPaintingNode(int list_sign,
                                                           int child_sign) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "UIOperationQueue::InsertListItemPaintingNodeTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_insertListItemNode(env, local_ref.Get(), list_sign,
                                          child_sign);
}

void PaintingContextAndroidRef::RemoveListItemPaintingNode(int list_sign,
                                                           int child_sign) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "UIOperationQueue::RemoveListItemPaintingNodeTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_removeListItemNode(env, local_ref.Get(), list_sign,
                                          child_sign);
}

void PaintingContextAndroidRef::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float target_content_offset_x,
    float target_content_offset_y, bool is_init_scroll_offset) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "UIOperationQueue::UpdateContentOffsetForListContainerTask");

  base::android::ScopedLocalJavaRef<jobject> local_ref(java_ref_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_updateContentSizeAndOffset(
      env, local_ref.Get(), container_id, content_size, target_content_offset_x,
      target_content_offset_y);
}

void PaintingContextAndroidRef::SetNeedMarkDrawEndTiming(
    std::weak_ptr<shell::TimingCollectorPlatform> weak_timing_collector,
    const tasm::PipelineID& pipeline_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "UIOperationQueue::SetNeedMarkDrawEndTimingTask");
  if (auto timing_collector_platform = weak_timing_collector.lock()) {
    timing_collector_platform->SetNeedMarkDrawEndTiming(pipeline_id);
  }
}

void PaintingContextAndroid::SetKeyframes(
    std::unique_ptr<PropBundle> keyframes_data) {
  PropBundleAndroid* pda =
      static_cast<PropBundleAndroid*>(keyframes_data.get());
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedGlobalJavaRef<jobject> props_ref{
      env, pda->jni_map()->jni_object()};
  Enqueue([impl = impl_, props_ref = std::move(props_ref)]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::SetKeyframesTask");

    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }

    JNIEnv* env = base::android::AttachCurrentThread();
    Java_PaintingContext_setKeyframes(env, local_ref.Get(), props_ref.Get());
  });
}

bool PaintingContextAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void PaintingContextAndroid::ConsumeGesture(int64_t idx, int32_t gesture_id,
                                            const pub::Value& params) {
  auto lepus_map = pub::ValueUtils::ConvertValueToLepusValue(params);

  Enqueue([impl = impl_, idx, gesture_id, map = std::move(lepus_map)]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "UIOperationQueue::ConsumeGesture");

    // Check if the Java reference is null before proceeding.
    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }

    JNIEnv* env = base::android::AttachCurrentThread();

    auto j_map =
        tasm::android::ValueConverterAndroid::ConvertLepusToJavaOnlyMap(map);
    // Call the Java method to consume gesture or not
    Java_PaintingContext_consumeGesture(env, local_ref.Get(), idx, gesture_id,
                                        j_map.jni_object());
  });
}

PaintingContextAndroid::PaintingContextAndroid(JNIEnv* env, jobject impl,
                                               jint thread_strategy,
                                               bool enable_context_free)
    : impl_(std::make_shared<base::android::ScopedWeakGlobalJavaRef<jobject>>(
          env, impl)),
      thread_strategy_(thread_strategy),
      enable_context_free_(enable_context_free) {
  platform_ref_ = std::make_shared<PaintingContextAndroidRef>(env, impl);
}

void PaintingContextAndroid::SetUIOperationQueue(
    const std::shared_ptr<shell::DynamicUIOperationQueue>& queue) {
  queue_ = queue;
}

void PaintingContextAndroid::InvokeNativeRunnable(
    const base::android::ScopedGlobalJavaRef<jobject>& runnable, JNIEnv* env) {
  static const base::android::ScopedGlobalJavaRef<jclass> kRunnableClassRef =
      base::android::GetGlobalClass(env, "java/lang/Runnable");
  static jmethodID kMethodRunID = nullptr;
  if (!kMethodRunID) {
    kMethodRunID = base::android::GetMethod(
        env, kRunnableClassRef.Get(), lynx::base::android::INSTANCE_METHOD,
        "run", "()V");
  }
  env->CallVoidMethod(runnable.Get(), kMethodRunID);
  lynx::base::android::CheckException(env);
}

std::tuple<PropBundleAndroid*, jobject, jobject, jobject>
PaintingContextAndroid::GetArgsForCreatePaintingNode(
    const std::shared_ptr<PropBundle>& painting_data) {
  PropBundleAndroid* pda = static_cast<PropBundleAndroid*>(painting_data.get());
  const auto props_object =
      pda->jni_map() ? pda->jni_map()->jni_object() : nullptr;
  const auto listeners_object = pda->jni_event_handler_map()
                                    ? pda->jni_event_handler_map()->jni_object()
                                    : nullptr;
  const auto gestures_object =
      pda->jni_gesture_detector_map()
          ? pda->jni_gesture_detector_map()->jni_object()
          : nullptr;
  return std::make_tuple(pda, props_object, listeners_object, gestures_object);
}

void PaintingContextAndroid::CreatePaintingNode(
    int id, const std::string& tag,
    const std::shared_ptr<PropBundle>& painting_data, bool flatten,
    bool create_node_async, uint32_t node_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "PaintingContextAndroid::CreatePaintingNode");
  if (!LynxEnv::GetInstance().EnableNativeCreateViewAsync()) {
    JNIEnv* env = base::android::AttachCurrentThread();
    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
    if (local_ref.IsNull()) {
      return;
    }
    PropBundleAndroid* pda;
    jobject props_object;
    jobject listeners_object;
    jobject gestures_object;
    std::tie(pda, props_object, listeners_object, gestures_object) =
        GetArgsForCreatePaintingNode(painting_data);
    const auto tag_ref = base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, tag.c_str());
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "UIOperationQueue::createNode.enqueueCreateView");
    // we now split props into initialProps & initialStyles.
    // if using mapBuffer, css styles parts in stored in initialStyles,
    // otherwise initialStyles is null and css styles info & attribute info is
    // stored in initialProps.
    // TODO(@nihao.royal): after attribute is converted into integer format, we
    // can merge initialStyles and initialProps.
    base::android::ScopedGlobalJavaRef<jobject> task_ref =
        Java_PaintingContext_createNode(
            env, local_ref.Get(), id, tag_ref.Get(), props_object,
            pda->GetStyleMapBuffer().Get(), listeners_object, flatten,
            node_index, gestures_object);
    if (lynx::base::android::HasJNIException()) {
      base::ErrorStorage::GetInstance().AddCustomInfoToError(
          {{"node_index", std::to_string(node_index)}});
    }
    Enqueue([this, task_ref = std::move(task_ref)]() {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "UIOperationQueue::CreatePaintingNodeTask");
      if (task_ref.IsNull()) {
        return;
      }
      JNIEnv* env = base::android::AttachCurrentThread();
      InvokeNativeRunnable(task_ref, env);
    });
    return;
  }

  // Sync create
  if (!create_node_async) {
    Enqueue([this, impl = impl_, id, tag, painting_data, flatten,
             node_index]() mutable {
      JNIEnv* env = base::android::AttachCurrentThread();
      base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
      if (local_ref.IsNull()) {
        return;
      }
      PropBundleAndroid* pda;
      jobject props_object;
      jobject listeners_object;
      jobject gestures_object;
      std::tie(pda, props_object, listeners_object, gestures_object) =
          GetArgsForCreatePaintingNode(painting_data);
      const auto tag_ref =
          base::android::JNIConvertHelper::ConvertToJNIStringUTF(env,
                                                                 tag.c_str());
      Java_PaintingContext_createPaintingNodeSync(
          env, local_ref.Get(), id, tag_ref.Get(), props_object,
          pda->GetStyleMapBuffer().Get(), listeners_object, flatten, node_index,
          gestures_object);
      if (lynx::base::android::HasJNIException()) {
        base::ErrorStorage::GetInstance().AddCustomInfoToError(
            {{"node_index", std::to_string(node_index)}});
      }
    });
    return;
  }

  // Async Create
  std::promise<base::android::ScopedGlobalJavaRef<jobject>> promise;
  std::future<base::android::ScopedGlobalJavaRef<jobject>> future =
      promise.get_future();

  auto create_node_async_task = fml::MakeRefCounted<
      base::OnceTask<base::android::ScopedGlobalJavaRef<jobject>>>(
      [this, impl = impl_, id, tag, painting_data, flatten, node_index,
       promise = std::move(promise)]() mutable {
        base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
        if (local_ref.IsNull()) {
          return;
        }
        JNIEnv* env = base::android::AttachCurrentThread();
        PropBundleAndroid* pda;
        jobject props_object;
        jobject listeners_object;
        jobject gestures_object;
        std::tie(pda, props_object, listeners_object, gestures_object) =
            GetArgsForCreatePaintingNode(painting_data);
        const auto tag_ref =
            base::android::JNIConvertHelper::ConvertToJNIStringUTF(env,
                                                                   tag.c_str());
        promise.set_value(base::android::ScopedGlobalJavaRef<jobject>(
            env, Java_PaintingContext_createPaintingNodeAsync(
                     env, local_ref.Get(), id, tag_ref.Get(), props_object,
                     pda->GetStyleMapBuffer().Get(), listeners_object, flatten,
                     node_index, gestures_object)
                     .Get()));

        if (lynx::base::android::HasJNIException()) {
          base::ErrorStorage::GetInstance().AddCustomInfoToError(
              {{"node_index", std::to_string(node_index)}});
        }
        return;
      },
      std::move(future));

  if (enable_context_free_) {
    context_free_create_node_async_task_queue_.Push(create_node_async_task);
  } else {
    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [create_node_async_task]() { create_node_async_task->Run(); },
        base::ConcurrentTaskType::HIGH_PRIORITY);
    scheduled_create_node_async_task_queue_.Push(create_node_async_task);
  }
  Enqueue([this, task = std::move(create_node_async_task)]() {
    JNIEnv* env = base::android::AttachCurrentThread();
    task->Run();
    // Taking tasks from the LIFO queue iterable container to execute while
    // waiting for the current task to finish.
    while (!(task->GetFuture().valid() &&
             task.get()->GetFuture().wait_for(std::chrono::seconds(0)) ==
                 std::future_status::ready) &&
           (!backward_create_node_async_task_iterable_container_.empty() &&
            backward_create_node_async_task_iterator_ !=
                backward_create_node_async_task_iterable_container_.end())) {
      auto back_task = *backward_create_node_async_task_iterator_;
      back_task->Run();
      backward_create_node_async_task_iterator_++;
    }

    if (task->GetFuture().valid()) {
      auto runnable = task->GetFuture().get();
      if (!runnable.IsNull()) {
        InvokeNativeRunnable(runnable, env);
      }
    }
  });
}

void PaintingContextAndroid::SetContextHasAttached() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "PaintingContextAndroid::SetContextHasAttached");
  if (enable_context_free_) {
    enable_context_free_ = false;
    auto tasks = context_free_create_node_async_task_queue_.PopAll();
    for (auto& task : tasks) {
      base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
          [task]() { task->Run(); }, base::ConcurrentTaskType::HIGH_PRIORITY);
      scheduled_create_node_async_task_queue_.Push(task);
    }
  }
}

void PaintingContextAndroid::InsertPaintingNode(int parent, int child,
                                                int index) {
  if (ui_operation_batch_builder_) {
    ui_operation_batch_builder_->putInt(
        static_cast<int32_t>(UIOperationType::kInsert));
    ui_operation_batch_builder_->putInt(parent);
    ui_operation_batch_builder_->putInt(child);
    ui_operation_batch_builder_->putInt(index);
    return;
  }
  Enqueue([impl = impl_, parent, child, index]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "UIOperationQueue::InsertPaintingNodeTask");

    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }

    JNIEnv* env = base::android::AttachCurrentThread();
    Java_PaintingContext_insertNode(env, local_ref.Get(), parent, child, index);
  });
}

void PaintingContextAndroid::RemovePaintingNode(int parent, int child,
                                                int index, bool is_move) {
  if (ui_operation_batch_builder_) {
    ui_operation_batch_builder_->putInt(
        static_cast<int32_t>(UIOperationType::kRemove));
    ui_operation_batch_builder_->putInt(parent);
    ui_operation_batch_builder_->putInt(child);
    return;
  }

  Enqueue([impl = impl_, parent, child]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "UIOperationQueue::RemovePaintingNodeTask");

    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }

    JNIEnv* env = base::android::AttachCurrentThread();
    Java_PaintingContext_removeNode(env, local_ref.Get(), parent, child);
  });
}

void PaintingContextAndroid::DestroyPaintingNode(int parent, int child,
                                                 int index) {
  if (ui_operation_batch_builder_) {
    ui_operation_batch_builder_->putInt(
        static_cast<int32_t>(UIOperationType::kDestroy));
    ui_operation_batch_builder_->putInt(parent);
    ui_operation_batch_builder_->putInt(child);
    return;
  }
  Enqueue([impl = impl_, parent, child]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "UIOperationQueue::DestroyPaintingNodeTask");

    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }

    JNIEnv* env = base::android::AttachCurrentThread();
    Java_PaintingContext_destroyNode(env, local_ref.Get(), parent, child);
  });
}

void PaintingContextAndroid::UpdateLayout(
    int id, float x, float y, float width, float height, const float* paddings,
    const float* margins, const float* borders, const float* bounds,
    const float* sticky, float max_height, uint32_t node_index) {
  patching_ids_.emplace_back(id);
  patching_node_index_.emplace_back(node_index);
  if (bounds != nullptr) {
    patching_bounds_.emplace_back(
        std::array<float, 4>{bounds[0], bounds[1], bounds[2], bounds[3]});
  }
  if (sticky != nullptr) {
    patching_stickies_.emplace_back(
        std::array<float, 4>{sticky[0], sticky[1], sticky[2], sticky[3]});
  }
  patching_ints_.emplace_back(
      std::array<int, static_cast<size_t>(IntValueIndex::SIZE)>{
          static_cast<int>(round(x)), static_cast<int>(round(y)),
          static_cast<int>(round(width)), static_cast<int>(round(height)),
          static_cast<int>(paddings[0]), static_cast<int>(paddings[1]),
          static_cast<int>(paddings[2]), static_cast<int>(paddings[3]),
          static_cast<int>(margins[0]), static_cast<int>(margins[1]),
          static_cast<int>(margins[2]), static_cast<int>(margins[3]),
          static_cast<int>(borders[0]), static_cast<int>(borders[1]),
          static_cast<int>(borders[2]), static_cast<int>(borders[3]),
          bounds != nullptr, sticky != nullptr, static_cast<int>(max_height)});
}

void PaintingContextAndroid::UpdatePaintingNode(
    int id, bool tend_to_flatten,
    const std::shared_ptr<PropBundle>& painting_data) {
  PropBundleAndroid* pda = static_cast<PropBundleAndroid*>(painting_data.get());
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedGlobalJavaRef<jobject> props_ref(
      env, pda->jni_map()->jni_object());
  base::android::ScopedGlobalJavaRef<jobject> listeners_ref(
      env, pda->jni_event_handler_map()
               ? pda->jni_event_handler_map()->jni_object()
               : nullptr);

  base::android::ScopedGlobalJavaRef<jobject> gestures_ref(
      env, pda->jni_gesture_detector_map()
               ? pda->jni_gesture_detector_map()->jni_object()
               : nullptr);

  // we now split props into attributes & styles.
  // if using mapBuffer, css styles parts in stored in styles, otherwise
  // styles is null and css styles info & attribute info is stored in
  // attributes.
  // TODO(@nihao.royal): after attribute is converted into integer format, we
  // can merge attributes and styles.
  base::android::ScopedGlobalJavaRef<jobject> styles_ref(
      env, pda->GetStyleMapBuffer().Get());

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Catalyzer.UpdatePaintingNode");
  Enqueue([impl = impl_, id, tend_to_flatten, props_ref = std::move(props_ref),
           listeners_ref = std::move(listeners_ref),
           gestures_ref = std::move(gestures_ref),
           styles_ref = std::move(styles_ref)]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "UIOperationQueue::UpdatePaintingNodeTask");

    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }

    JNIEnv* env = base::android::AttachCurrentThread();
    Java_PaintingContext_updateProps(env, local_ref.Get(), id, tend_to_flatten,
                                     props_ref.Get(), styles_ref.Get(),
                                     listeners_ref.Get(), gestures_ref.Get());
  });
}

void PaintingContextAndroid::UpdatePlatformExtraBundle(
    int32_t id, PlatformExtraBundle* bundle) {
  if (!bundle) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedGlobalJavaRef<jobject> java_bundle_ref(
      env, static_cast<PlatformExtraBundleAndroid*>(bundle)
               ->GetPlatformBundle()
               .Get());
  Enqueue([impl = impl_, id, java_bundle_ref = std::move(java_bundle_ref)]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "UIOperationQueue::UpdatePlatformExtraBundleTask");

    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }

    JNIEnv* env = base::android::AttachCurrentThread();
    Java_PaintingContext_updateExtraData(env, local_ref.Get(), id,
                                         java_bundle_ref.Get());
  });
}

void PaintingContextAndroid::Flush() {
  if (GetEnableVsyncAlignedFlush()) {
    return;
  }
  FlushImmediately();
}

void PaintingContextAndroid::FlushImmediately() {
  BeforeFlush();
  queue_->Flush();
}

void PaintingContextAndroid::HandleValidate(int tag) {
  Enqueue([impl = impl_, tag]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "UIOperationQueue::UpdatePlatformExtraBundleTask");

    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }

    JNIEnv* env = base::android::AttachCurrentThread();
    Java_PaintingContext_validate(env, local_ref.Get(), tag);
  });
}

void PaintingContextAndroid::FinishLayoutOperation(
    const PipelineOptions& options) {
  if (ui_operation_batch_builder_) {
    ui_operation_batch_builder_->putInt(
        static_cast<int32_t>(UIOperationType::kLayoutFinish));
    ui_operation_batch_builder_->putInt(options.list_comp_id_);
    ui_operation_batch_builder_->putLong(options.operation_id);
  } else {
    Enqueue([impl = impl_, options = options]() {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "UIOperationQueue::FinishLayoutOperationTask");

      base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
      if (local_ref.IsNull()) {
        return;
      }
      JNIEnv* env = base::android::AttachCurrentThread();
      Java_PaintingContext_FinishLayoutOperation(
          env, local_ref.Get(), options.list_comp_id_, options.operation_id,
          options.is_first_screen);
    });
  }

  Enqueue([weak_queue = std::weak_ptr<shell::DynamicUIOperationQueue>(queue_),
           native_update_data_order = options.native_update_data_order_]() {
    if (auto queue = weak_queue.lock()) {
      if (native_update_data_order == queue->GetNativeUpdateDataOrder()) {
        queue->UpdateStatus(shell::UIOperationStatus::ALL_FINISH);
      }
    }
  });

  if (options.native_update_data_order_ == queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
  }
  if (GetEnableVsyncAlignedFlush()) {
    RequestPlatformLayout();
  }
}

void PaintingContextAndroid::RequestPlatformLayout() {
  base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PaintingContext_requestLayout(env, local_ref.Get());
}

void PaintingContextAndroid::FinishTasmOperation(
    const PipelineOptions& options) {
  // Reset iterable container and iterator
  {
    if (LynxEnv::GetInstance().EnableNativeCreateViewAsync()) {
      Enqueue([this]() mutable {
        TRACE_EVENT(
            LYNX_TRACE_CATEGORY,
            "PaintingContextAndroid::ResetCreatePaintingNodeIterableContainer");
        backward_create_node_async_task_iterable_container_.reset();
      });
    }
  }
  if (ui_operation_batch_builder_) {
    ui_operation_batch_builder_->putInt(
        static_cast<int32_t>(UIOperationType::kTasmFinish));
    ui_operation_batch_builder_->putLong(options.operation_id);
  } else {
    Enqueue([impl = impl_, options]() {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "UIOperationQueue::FinishTasmOperationTask");

      base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
      if (local_ref.IsNull()) {
        return;
      }

      JNIEnv* env = base::android::AttachCurrentThread();

      Java_PaintingContext_finishTasmOperation(env, local_ref.Get(),
                                               options.operation_id);
    });
  }
  if (options.native_update_data_order_ == queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
  }
}

std::vector<float> PaintingContextAndroid::getBoundingClientOrigin(int id) {
  std::vector<float> res;
  base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jfloatArray> result =
      Java_PaintingContext_getBoundingClientOrigin(env, local_ref.Get(), id);
  jsize size = env->GetArrayLength(result.Get());
  jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
  for (auto i = 0; i < size; i++) {
    res.push_back(arr[i]);
  }
  env->ReleaseFloatArrayElements(result.Get(), arr, 0);
  return res;
}

std::vector<float> PaintingContextAndroid::getWindowSize(int id) {
  std::vector<float> res;

  base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jfloatArray> result =
      Java_PaintingContext_getWindowSize(env, local_ref.Get(), id);
  jsize size = env->GetArrayLength(result.Get());
  jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
  for (auto i = 0; i < size; i++) {
    res.push_back(arr[i]);
  }
  env->ReleaseFloatArrayElements(result.Get(), arr, 0);
  return res;
}

std::vector<float> PaintingContextAndroid::GetRectToWindow(int id) {
  std::vector<float> res;

  base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jfloatArray> result =
      Java_PaintingContext_getRectToWindow(env, local_ref.Get(), id);
  jsize size = env->GetArrayLength(result.Get());
  jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
  for (auto i = 0; i < size; i++) {
    res.push_back(arr[i]);
  }
  env->ReleaseFloatArrayElements(result.Get(), arr, 0);
  return res;
}

std::vector<float> PaintingContextAndroid::GetRectToLynxView(int64_t id) {
  std::vector<float> res;

  base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jfloatArray> result =
      Java_PaintingContext_getRectToLynxView(env, local_ref.Get(), id);
  jsize size = env->GetArrayLength(result.Get());
  jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
  for (auto i = 0; i < size; i++) {
    res.push_back(arr[i]);
  }
  env->ReleaseFloatArrayElements(result.Get(), arr, 0);
  return res;
}

std::vector<float> PaintingContextAndroid::ScrollBy(int64_t id, float width,
                                                    float height) {
  std::vector<float> res;

  base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
  if (local_ref.IsNull()) {
    return res;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jfloatArray> result =
      Java_PaintingContext_scrollBy(env, local_ref.Get(), id, width, height);
  jsize size = env->GetArrayLength(result.Get());
  jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
  for (auto i = 0; i < size; i++) {
    res.push_back(arr[i]);
  }
  env->ReleaseFloatArrayElements(result.Get(), arr, 0);
  return res;
}

int32_t PaintingContextAndroid::GetTagInfo(const std::string& tag_name) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
  if (local_ref.IsNull()) {
    return false;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> tag_ref =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, tag_name);
  return Java_PaintingContext_getTagInfo(env, local_ref.Get(), tag_ref.Get());
}

bool PaintingContextAndroid::IsFlatten(base::MoveOnlyClosure<bool, bool> func) {
  if (func != nullptr) {
    return func(false);
  }
  return false;
}

void PaintingContextAndroid::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(*impl_);
  if (local_ref.IsNull()) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  const auto& j_method =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, method);
  auto j_map = tasm::android::ValueConverterAndroid::ConvertLepusToJavaOnlyMap(
      pub::ValueUtils::ConvertValueToLepusValue(params));

  static int32_t sInvokeCallBackIndex = 0;
  invoke_callback_maps_[++sInvokeCallBackIndex] = callback;
  Java_PaintingContext_invoke(env, local_ref.Get(), id, j_method.Get(),
                              j_map.jni_object(), reinterpret_cast<jlong>(this),
                              sInvokeCallBackIndex);
}

void PaintingContextAndroid::InvokeUIMethodCallback(int32_t id, int32_t code,
                                                    const lepus::Value params) {
  auto iter = invoke_callback_maps_.find(id);
  if (iter == invoke_callback_maps_.end()) {
    LOGE("InvokeUIMethodCallback failed since can't find the callback");
    return;
  }
  iter->second(code, PubLepusValue(params));
  invoke_callback_maps_.erase(iter);
}

void PaintingContextAndroid::UpdateLayoutPatching() {
  if (!patching_ids_.empty()) {
    if (ui_operation_batch_builder_) {
      ui_operation_batch_builder_->putInt(
          static_cast<int32_t>(UIOperationType::kUpdateLayoutPatching));

      size_t patching_count = patching_ids_.size();
      // patching id
      ui_operation_batch_builder_->putIntArray(std::move(patching_ids_));

      // patching ints
      ui_operation_batch_builder_->putInt(
          static_cast<int>(IntValueIndex::SIZE) * patching_count);
      for (auto& e : patching_ints_) {
        for (int i : e) {
          ui_operation_batch_builder_->putInt(i);
        }
      }
      patching_ints_.clear();

      // patching bounds
      ui_operation_batch_builder_->putInt(4 * patching_bounds_.size());
      for (auto& e : patching_bounds_) {
        for (float i : e) {
          ui_operation_batch_builder_->putDouble(i);
        }
      }
      patching_bounds_.clear();

      // patching stickies
      ui_operation_batch_builder_->putInt(4 * patching_stickies_.size());
      for (auto& e : patching_stickies_) {
        for (float i : e) {
          ui_operation_batch_builder_->putDouble(i);
        }
      }
      patching_stickies_.clear();

      // patching node index
      ui_operation_batch_builder_->putIntArray(std::move(patching_node_index_));
      return;
    }

    Enqueue([impl = impl_, patching_ids = std::move(patching_ids_),
             patching_ints = std::move(patching_ints_),
             patching_bounds = std::move(patching_bounds_),
             patching_stickies = std::move(patching_stickies_),
             patching_node_index = std::move(patching_node_index_)]() {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "UIOperationQueue::UpdateLayoutPatchingTask");

      base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
      if (local_ref.IsNull()) {
        return;
      }

      JNIEnv* env = base::android::AttachCurrentThread();
      size_t patching_count = patching_ids.size();
      base::android::ScopedLocalJavaRef<jintArray> j_ids(
          env, env->NewIntArray(patching_count));
      env->SetIntArrayRegion(j_ids.Get(), 0, patching_count, &patching_ids[0]);
      base::android::ScopedLocalJavaRef<jintArray> j_node_index(
          env, env->NewIntArray(patching_count));
      env->SetIntArrayRegion(j_node_index.Get(), 0, patching_count,
                             &patching_node_index[0]);

      // rect, paddings, margins, bounds, sticky, max_height
      base::android::ScopedLocalJavaRef<jintArray> j_ints(
          env, env->NewIntArray(static_cast<int>(IntValueIndex::SIZE) *
                                patching_count));
      env->SetIntArrayRegion(
          j_ints.Get(), 0,
          static_cast<int>(IntValueIndex::SIZE) * patching_count,
          &patching_ints[0][0]);

      jfloatArray jni_bounds = nullptr, jni_stickies = nullptr;
      base::android::ScopedLocalJavaRef<jfloatArray> j_bounds(
          env, env->NewFloatArray(4 * patching_bounds.size()));
      if (!patching_bounds.empty()) {
        env->SetFloatArrayRegion(j_bounds.Get(), 0, 4 * patching_bounds.size(),
                                 &patching_bounds[0][0]);
        jni_bounds = j_bounds.Get();
      }
      base::android::ScopedLocalJavaRef<jfloatArray> j_stickies(
          env, env->NewFloatArray(4 * patching_stickies.size()));
      if (!patching_stickies.empty()) {
        env->SetFloatArrayRegion(j_stickies.Get(), 0,
                                 4 * patching_stickies.size(),
                                 &patching_stickies[0][0]);
        jni_stickies = j_stickies.Get();
      }

      Java_PaintingContext_UpdateLayoutPatching(
          env, local_ref.Get(), j_ids.Get(), j_ints.Get(), jni_bounds,
          jni_stickies, j_node_index.Get());
    });
  }
}

// TODO(huzhanbo.luc): remove this later
void PaintingContextAndroid::OnFirstMeaningfulLayout() {}

void PaintingContextAndroid::UpdateNodeReadyPatching(
    std::vector<int32_t> ready_ids, std::vector<int32_t> remove_ids) {
  if (ready_ids.empty() && remove_ids.empty()) {
    return;
  }

  // only for EnableUIOperationBatching now!
  if (ui_operation_batch_builder_) {
    if (!ready_ids.empty()) {
      ui_operation_batch_builder_->putInt(
          static_cast<int32_t>(UIOperationType::kReadyBatching));
      ui_operation_batch_builder_->putIntArray(std::move(ready_ids));
    }

    if (!remove_ids.empty()) {
      ui_operation_batch_builder_->putInt(
          static_cast<int32_t>(UIOperationType::kRemoveBatching));
      ui_operation_batch_builder_->putIntArray(std::move(remove_ids));
    }
    return;
  }
}

void PaintingContextAndroid::Enqueue(shell::UIOperation op) {
  queue_->EnqueueUIOperation(std::move(op));
}

void PaintingContextAndroid::EnqueueHighPriorityUIOperation(
    shell::UIOperation op) {
  queue_->EnqueueHighPriorityUIOperation(std::move(op));
}

void PaintingContextAndroid::BeforeFlush() {
  // Pop all scheduled AsyncCreateUI tasks and initialize iterable container
  {
    if (LynxEnv::GetInstance().EnableNativeCreateViewAsync() &&
        !scheduled_create_node_async_task_queue_.Empty()) {
      EnqueueHighPriorityUIOperation(
          [this, iterable_container = scheduled_create_node_async_task_queue_
                                          .ReversePopAll()]() mutable {
            TRACE_EVENT(LYNX_TRACE_CATEGORY,
                        "PaintingContextAndroid::"
                        "ReinitializeCreatePaintingNodeIterableContainer");
            backward_create_node_async_task_iterable_container_ =
                std::move(iterable_container);
            backward_create_node_async_task_iterator_ =
                backward_create_node_async_task_iterable_container_.begin();
          });
    }
  }

  std::optional<base::android::CompactArrayBuffer> ui_operation_batch =
      std::nullopt;
  if (ui_operation_batch_builder_ && !ui_operation_batch_builder_->empty()) {
    ui_operation_batch = ui_operation_batch_builder_->build();
    ui_operation_batch_builder_ = base::android::CompactArrayBufferBuilder{};
  }

  if (!ui_operation_batch || ui_operation_batch->count() == 0) {
    return;
  }

  Enqueue([impl = impl_, ui_operation_batch = std::move(ui_operation_batch)]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FlushUIOperationBatch");
    base::android::ScopedLocalJavaRef<jobject> local_ref(*impl);
    if (local_ref.IsNull()) {
      return;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    // try to flush ui operation batch before finishing tasm operation
    auto j_ui_operation_batch =
        ui_operation_batch
            ? base::android::JReadableCompactArrayBuffer::
                  CreateReadableCompactArrayBuffer(*ui_operation_batch)
            : nullptr;
    Java_PaintingContext_flushUIOperationBatch(env, local_ref.Get(),
                                               j_ui_operation_batch);
  });
}

void PaintingContextAndroid::EnableUIOperationBatching() {
  ui_operation_batch_builder_ = base::android::CompactArrayBufferBuilder{};
}
std::unique_ptr<pub::Value> PaintingContextAndroid::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  return TextUtilsAndroidHelper::GetTextInfo(content, info);
}

}  // namespace tasm
}  // namespace lynx
