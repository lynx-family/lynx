// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_PAINTING_CONTEXT_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_PAINTING_CONTEXT_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/public/painting_ctx_platform_impl.h"
#include "core/public/pipeline_option.h"
#include "core/public/platform_extra_bundle.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/css/css_font_face_token.h"
#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/services/timing_handler/timing.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/shell/dynamic_ui_operation_queue.h"

namespace lynx {
namespace tasm {

class LayoutNode;

class PaintingContext {
 public:
  constexpr static size_t kPatchingNodeVectorReserveSizeBig = 128;
  constexpr static size_t kPatchingNodeVectorReserveSizeSmall = 32;

  PaintingContext(std::unique_ptr<PaintingCtxPlatformImpl> platform_impl)
      : platform_impl_(std::move(platform_impl)) {
    patching_node_ready_ids_.reserve(kPatchingNodeVectorReserveSizeBig);
    patching_node_reload_ids_.reserve(kPatchingNodeVectorReserveSizeSmall);
    patching_node_remove_ids_.reserve(kPatchingNodeVectorReserveSizeSmall);
  }
  virtual ~PaintingContext() = default;

  PaintingCtxPlatformImpl* impl() { return platform_impl_.get(); }

  inline void GetAbsolutePosition(int id, float* position) {
    platform_impl_->getAbsolutePosition(id, position);
  }

  inline void CreatePaintingNode(
      int id, const std::string& tag,
      const std::shared_ptr<PropBundle>& painting_data, bool flatten,
      bool create_node_async, uint32_t node_index = 0) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreatePaintingNode", "tag", tag);
    platform_impl_->CreatePaintingNode(id, tag, painting_data, flatten,
                                       create_node_async, node_index);
  }

  void EnableUIOperationBatching() {
    platform_impl_->EnableUIOperationBatching();
  }

  void InsertPaintingNode(int parent, int child, int index);
  // The `is_move` flag indicates that this is a part of a move operation. For
  // move operations, we can skip the detach lifecycle and maintain the view
  // state without resetting it (such as the focus state).
  //
  // To move a painting node, you are required to promptly add the view back.
  // For example:
  //   RemovePaintingNode(parent, child, index, true);
  //   InsertPaintingNode(new_parent, child, new_index);
  void RemovePaintingNode(int parent, int child, int index, bool is_move);
  void DestroyPaintingNode(int parent, int child, int index);

  inline void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const std::shared_ptr<PropBundle>& painting_data) {
    platform_impl_->UpdatePaintingNode(id, tend_to_flatten, painting_data);
  }

  inline void UpdateLayout(int tag, float x, float y, float width, float height,
                           const float* paddings, const float* margins,
                           const float* borders, const float* bounds,
                           const float* sticky, float max_height,
                           uint32_t node_index = 0) {
    platform_impl_->UpdateLayout(tag, x, y, width, height, paddings, margins,
                                 borders, bounds, sticky, max_height,
                                 node_index);
  }

  inline void SetKeyframes(std::unique_ptr<PropBundle> keyframes_data) {
    platform_impl_->SetKeyframes(std::move(keyframes_data));
  }

  inline void FinishTasmOperation(const PipelineOptions& options) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FinishTasmOperation");
    platform_impl_->FinishTasmOperation(options);
  }

  void OnNodeReady(int tag);
  void OnNodeReload(int tag);
  void UpdateNodeReadyPatching();
  void UpdateNodeReloadPatching();

  inline void UpdateLayoutPatching() { platform_impl_->UpdateLayoutPatching(); }

  void UpdatePlatformExtraBundle(int32_t id, PlatformExtraBundle* bundle) {
    platform_impl_->UpdatePlatformExtraBundle(id, bundle);
  }

  inline void Flush() { platform_impl_->Flush(); }

  inline void FlushImmediately() { platform_impl_->FlushImmediately(); }

  inline void HandleValidate(int tag) { platform_impl_->HandleValidate(tag); }

  inline std::vector<float> getBoundingClientOrigin(int id) {
    return platform_impl_->getBoundingClientOrigin(id);
  }

  inline std::vector<float> getWindowSize(int id) {
    return platform_impl_->getWindowSize(id);
  }

  inline std::vector<float> GetRectToWindow(int id) {
    return platform_impl_->GetRectToWindow(id);
  }
  inline void ConsumeGesture(int64_t id, int32_t gesture_id,
                             const pub::Value& params) {
    platform_impl_->ConsumeGesture(id, gesture_id, params);
  }

  inline int32_t GetTagInfo(const std::string& tag_name) {
    return platform_impl_->GetTagInfo(tag_name);
  }

  inline bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) {
    return platform_impl_->IsFlatten(std::move(func));
  }

  inline std::vector<float> GetRectToLynxView(int64_t id) {
    return platform_impl_->GetRectToLynxView(id);
  }

  inline std::vector<float> ScrollBy(int64_t id, float width, float height) {
    return platform_impl_->ScrollBy(id, width, height);
  }

  inline void Invoke(
      int64_t id, const std::string& method, const pub::Value& params,
      const std::function<void(int32_t code, const pub::Value& data)>&
          callback) {
    return platform_impl_->Invoke(id, method, params, callback);
  }

  inline void OnFirstMeaningfulLayout() {
    platform_impl_->OnFirstMeaningfulLayout();
  }

  inline void SetEnableVsyncAlignedFlush(bool enabled) {
    platform_impl_->SetEnableVsyncAlignedFlush(enabled);
  }

  inline void InvokeUIMethod(int32_t view_id, const std::string& method,
                             std::unique_ptr<tasm::PropBundle> args,
                             int32_t callback_id) {
    platform_impl_->InvokeUIMethod(view_id, method, std::move(args),
                                   callback_id);
  }

  void OnFirstScreen() { has_first_screen_ = true; }

  // Pass the opions to the tasm thread through the tasm queue, and mount them
  // on the PaintingContext. The UI Flush stage reads the opions from the
  // PaintingContext for collecting timing, and clears the opions at the end.
  void AppendOptionsForTiming(const PipelineOptions& options) {
    options_for_timing_.emplace_back(options);
  }

  void ClearOptionsForTiming() { options_for_timing_.clear(); }

  inline bool NeedAnimationProps() {
    return platform_impl_->NeedAnimationProps();
  }

  inline bool DefaultOverflowAlwaysVisible() {
    return platform_impl_->DefaultOverflowAlwaysVisible();
  }

  void SetUIOperationQueue(
      const std::shared_ptr<shell::DynamicUIOperationQueue>& queue);

  void OnCollectExtraUpdates(int32_t id);

  void UpdateScrollInfo(int32_t container_id, bool smooth,
                        float estimated_offset, bool scrolling);

  void SetGestureDetectorState(int64_t id, int32_t gesture_id, int32_t state);

  void UpdateEventInfo(bool has_touch_pseudo);
  void UpdateFlattenStatus(int id, bool flatten);

  void ListReusePaintingNode(int id, const base::String& item_key);
  void ListCellWillAppear(int id, const base::String& item_key);
  void ListCellDisappear(int id, bool isExist, const base::String& item_key);
  void InsertListItemPaintingNode(int32_t list_id, int32_t child_id);
  void RemoveListItemPaintingNode(int32_t list_id, int32_t child_id);
  void UpdateContentOffsetForListContainer(int32_t container_id,
                                           float content_size, float delta_x,
                                           float delta_y,
                                           bool is_init_scroll_offset);

  void FinishLayoutOperation(const PipelineOptions& options);
  void SetTimingCollectorPlatform(
      const std::shared_ptr<shell::TimingCollectorPlatform>& timing);
  void SetNeedMarkDrawEndTiming(const tasm::PipelineID& pipeline_id);
  void MarkUIOperationQueueFlushTiming(tasm::TimingKey key,
                                       const tasm::PipelineID& pipeline_id);
  void MarkLayoutUIOperationQueueFlushStartIfNeed();
  void SetContextHasAttached();

 private:
  void Enqueue(shell::UIOperation op, bool high_priority = false);
  void EnqueueHighPriorityUIOperation(shell::UIOperation op) {
    Enqueue(std::move(op), true);
  }

  std::unique_ptr<PaintingCtxPlatformImpl> platform_impl_;

  PaintingContext(const PaintingContext&) = delete;
  PaintingContext& operator=(const PaintingContext&) = delete;

  std::shared_ptr<shell::TimingCollectorPlatform> timing_collector_platform_;

  std::shared_ptr<shell::DynamicUIOperationQueue> ui_operation_queue_;

  std::vector<int> patching_node_ready_ids_{};
  std::vector<int> patching_node_remove_ids_{};
  std::vector<int> patching_node_reload_ids_{};

  bool has_first_screen_ = false;
  // Pass the opions to the tasm thread through the tasm queue, and mount them
  // on the PaintingContext. The UI Flush stage reads the opions from the
  // PaintingContext for collecting timing, and clears the opions at the end.
  base::InlineVector<PipelineOptions, 1> options_for_timing_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_PAINTING_CONTEXT_H_
