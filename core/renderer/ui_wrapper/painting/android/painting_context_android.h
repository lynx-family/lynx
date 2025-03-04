// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PAINTING_CONTEXT_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PAINTING_CONTEXT_ANDROID_H_
#include <memory>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "base/include/concurrent_queue.h"
#include "core/base/android/java_only_map.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/tasm/react/android/mapbuffer/compact_array_buffer_builder.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/modules/android/callback_impl.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/shell/dynamic_ui_operation_queue.h"

namespace lynx {
namespace tasm {

class PaintingContextAndroidRef : public PaintingCtxPlatformRef {
 public:
  static bool RegisterJNI(JNIEnv* env);
  PaintingContextAndroidRef(JNIEnv* env, jobject impl);
  ~PaintingContextAndroidRef() override = default;

  void InsertPaintingNode(int parent, int child, int index) override;
  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;

  void OnCollectExtraUpdates(int32_t id) override;
  void UpdateScrollInfo(int32_t container_id, bool smooth,
                        float estimated_offset, bool scrolling) override;

  void SetGestureDetectorState(int64_t idx, int32_t gesture_id,
                               int32_t state) override;

  void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                               std::vector<int32_t> remove_ids) override;
  void UpdateNodeReloadPatching(std::vector<int32_t> reload_ids) override;

  void UpdateEventInfo(bool has_touch_pseudo) override;
  void UpdateFlattenStatus(int id, bool flatten) override;

  void ListReusePaintingNode(int sign, const std::string& item_key) override;
  void ListCellWillAppear(int sign, const std::string& item_key) override;
  void ListCellDisappear(int sign, bool isExist,
                         const std::string& item_key) override;
  void InsertListItemPaintingNode(int list_sign, int child_sign) override;
  void RemoveListItemPaintingNode(int list_sign, int child_sign) override;
  void UpdateContentOffsetForListContainer(int32_t container_id,
                                           float content_size, float delta_x,
                                           float delta_y,
                                           bool is_init_scroll_offset) override;
  void SetNeedMarkDrawEndTiming(
      std::weak_ptr<shell::TimingCollectorPlatform> weak_timing_collector,
      const tasm::PipelineID& pipeline_id) override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> java_ref_;
};

class PaintingContextAndroid : public PaintingCtxPlatformImpl {
 public:
  static bool RegisterJNI(JNIEnv* env);
  PaintingContextAndroid(JNIEnv* env, jobject impl, jint thread_strategy,
                         bool enable_context_free);
  ~PaintingContextAndroid() override = default;
  virtual void SetUIOperationQueue(
      const std::shared_ptr<shell::DynamicUIOperationQueue>& queue) override;
  void SetInstanceId(const int32_t instance_id) override {
    instance_id_ = instance_id;
  };
  void CreatePaintingNode(int id, const std::string& tag,
                          const std::shared_ptr<PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override;
  void InsertPaintingNode(int parent, int child, int index) override;
  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;
  void SetKeyframes(std::unique_ptr<PropBundle> keyframes_data) override;
  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info) override;
  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const std::shared_ptr<PropBundle>& painting_data) override;
  void UpdateLayout(int tag, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height,
                    uint32_t node_index = 0) override;
  void UpdatePlatformExtraBundle(int32_t id,
                                 PlatformExtraBundle* bundle) override;
  void Flush() override;
  void FlushImmediately() override;
  void HandleValidate(int tag) override;

  void FinishTasmOperation(const PipelineOptions& options) override;
  void FinishLayoutOperation(const PipelineOptions& options) override;

  void ConsumeGesture(int64_t id, int32_t gesture_id,
                      const pub::Value& params) override;

  std::vector<float> getBoundingClientOrigin(int id) override;
  std::vector<float> getWindowSize(int id) override;
  std::vector<float> GetRectToWindow(int id) override;
  std::vector<float> GetRectToLynxView(int64_t id) override;
  std::vector<float> ScrollBy(int64_t id, float width, float height) override;
  void Invoke(int64_t id, const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback) override;
  void InvokeUIMethodCallback(int32_t id, int32_t code,
                              const lepus::Value params);

  int32_t GetTagInfo(const std::string& tag_name) override;
  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;

  void UpdateLayoutPatching() override;

  void OnFirstMeaningfulLayout() override;
  // TODO(liting.src): remove this method after ui operation queue refactor.
  void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                               std::vector<int32_t> remove_ids) override;
  void SetContextHasAttached() override;

  void SetEnableVsyncAlignedFlush(bool enabled) override {
    static bool exp_switch = tasm::Config::GetConfig(
        LynxEnv::Key::ENABLE_VSYNC_ALIGNED_FLUSH, CompileOptions{});
    enable_vsync_aligned_flush_ =
        enabled && exp_switch &&
        LynxEnv::GetInstance().GetVsyncAlignedFlushGlobalSwitch() &&
        (thread_strategy_ ==
             lynx::base::ThreadStrategyForRendering::ALL_ON_UI ||
         thread_strategy_ ==
             lynx::base::ThreadStrategyForRendering::PART_ON_LAYOUT);
  };

  bool GetEnableVsyncAlignedFlush() { return enable_vsync_aligned_flush_; }

  void RequestPlatformLayout();

  bool NeedAnimationProps() override { return false; }

  void EnableUIOperationBatching() override;

  bool EnableUIOperationQueue() override { return true; }

  bool HasEnableUIOperationBatching() override {
    return ui_operation_batch_builder_.has_value();
  }

 private:
  enum class IntValueIndex {
    LEFT,
    TOP,
    WIDTH,
    HEIGHT,
    PADDING_LEFT,
    PADDING_TOP,
    PADDING_RIGHT,
    PADDING_BOTTOM,
    MARGIN_LEFT,
    MARGIN_TOP,
    MARGIN_RIGHT,
    MARGIN_BOTTOM,
    BORDER_LEFT,
    BORDER_TOP,
    BORDER_RIGHT,
    BORDER_BOTTOM,
    HAS_BOUND,
    HAS_STICKY,
    MAX_HEIGHT,
    SIZE
  };

  enum class UIOperationType : int32_t {
    kInsert = 0,
    kRemove = 1,
    kDestroy = 2,
    kReadyBatching = 3,
    kRemoveBatching = 4,
    kUpdateLayoutPatching = 5,
    kTasmFinish = 6,
    kLayoutFinish = 7,
  };

  void Enqueue(shell::UIOperation op);
  void EnqueueHighPriorityUIOperation(shell::UIOperation op);
  void BeforeFlush();
  void InvokeNativeRunnable(
      const base::android::ScopedGlobalJavaRef<jobject>& runnable_ref,
      JNIEnv* env);
  std::tuple<PropBundleAndroid*, jobject, jobject, jobject>
  GetArgsForCreatePaintingNode(
      const std::shared_ptr<PropBundle>& painting_data);
  static_assert(static_cast<size_t>(IntValueIndex::SIZE) == 19,
                "size has changed, make sure stay in sync with platform");

  std::vector<int> patching_ids_;
  std::vector<int> patching_node_index_;
  std::vector<std::array<int, static_cast<size_t>(IntValueIndex::SIZE)>>
      patching_ints_;
  std::vector<std::array<float, 4>> patching_bounds_;
  std::vector<std::array<float, 4>> patching_stickies_;
  std::shared_ptr<base::android::ScopedWeakGlobalJavaRef<jobject>> impl_;
  PaintingContextAndroid(const PaintingContextAndroid&) = delete;
  PaintingContextAndroid& operator=(const PaintingContextAndroid&) = delete;
  std::unordered_map<int32_t,
                     std::function<void(int32_t code, const pub::Value& data)>>
      invoke_callback_maps_;
  std::shared_ptr<shell::DynamicUIOperationQueue> queue_;
  bool enable_vsync_aligned_flush_ = false;
  jint thread_strategy_;
  bool enable_context_free_;
  // A thread-safe queue used to store create_view_async tasks before context is
  // attached
  lynx::base::ConcurrentQueue<
      fml::RefPtr<base::OnceTask<base::android::ScopedGlobalJavaRef<jobject>>>>
      context_free_create_node_async_task_queue_;
  // A thread-safe queue used to store create_view_async tasks posted to
  // thread-pool
  lynx::base::ConcurrentQueue<
      fml::RefPtr<base::OnceTask<base::android::ScopedGlobalJavaRef<jobject>>>>
      scheduled_create_node_async_task_queue_;
  // A container used to iterate scheduled create_view_async tasks from the end
  // to the beginning
  lynx::base::ConcurrentQueue<fml::RefPtr<base::OnceTask<
      base::android::ScopedGlobalJavaRef<jobject>>>>::IterableContainer
      backward_create_node_async_task_iterable_container_;
  // An iterator used to track the create_view_async tasks from the end to the
  // beginning
  lynx::base::ConcurrentQueue<fml::RefPtr<
      base::OnceTask<base::android::ScopedGlobalJavaRef<jobject>>>>::Iterator
      backward_create_node_async_task_iterator_;

  int32_t instance_id_ = 0;

  std::optional<base::android::CompactArrayBufferBuilder>
      ui_operation_batch_builder_{std::nullopt};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PAINTING_CONTEXT_ANDROID_H_
