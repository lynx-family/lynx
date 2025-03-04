// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_H_

#import <Foundation/Foundation.h>

#import <atomic>
#include <memory>
#include <string>
#include <vector>

#import "LynxUIOwner.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/shell/dynamic_ui_operation_queue.h"

#if ENABLE_TESTBENCH_REPLAY
#include "core/runtime/vm/lepus/json_parser.h"
#endif
namespace lynx {
namespace tasm {

class PaintingContextDarwinRef : public PaintingCtxPlatformRef {
 public:
  explicit PaintingContextDarwinRef(LynxUIOwner* owner) : uiOwner_(owner) {}
  ~PaintingContextDarwinRef() override = default;

  void InsertPaintingNode(int parent, int child, int index) override;
  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;

  void SetGestureDetectorState(int64_t idx, int32_t gesture_id,
                               int32_t state) override;
  void UpdateScrollInfo(int32_t container_id, bool smooth,
                        float estimated_offset, bool scrolling) override;

  void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                               std::vector<int32_t> remove_ids) override;
  void UpdateNodeReloadPatching(std::vector<int32_t> reload_ids) override;

  void UpdateEventInfo(bool has_touch_pseudo) override;

  void ListReusePaintingNode(int id, const std::string& item_key) override;
  void ListCellWillAppear(int sign, const std::string& item_key) override;
  void ListCellDisappear(int sign, bool isExist,
                         const std::string& item_key) override;
  void InsertListItemPaintingNode(int32_t list_id, int32_t child_id) override;
  void RemoveListItemPaintingNode(int32_t list_id, int32_t child_id) override;
  void UpdateContentOffsetForListContainer(int32_t container_id,
                                           float content_size, float delta_x,
                                           float delta_y,
                                           bool is_init_scroll_offset) override;
  void SetNeedMarkDrawEndTiming(
      std::weak_ptr<shell::TimingCollectorPlatform> weak_timing_collector,
      const tasm::PipelineID& pipeline_id) override;

 private:
  __weak LynxUIOwner* uiOwner_;
};

class PaintingContextDarwin : public PaintingCtxPlatformImpl {
 public:
  PaintingContextDarwin(LynxUIOwner* owner, bool enable_create_ui_async);
  ~PaintingContextDarwin() override;
  virtual void SetUIOperationQueue(
      const std::shared_ptr<shell::DynamicUIOperationQueue>& queue) override;
  void SetInstanceId(const int32_t instance_id) override;
  void CreatePaintingNode(int sign, const std::string& tag,
                          const std::shared_ptr<PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override;

  void SetKeyframes(std::unique_ptr<PropBundle> keyframes_data) override;
  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const std::shared_ptr<PropBundle>& painting_data) override;
  void UpdateLayout(int sign, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* flatten_bounds,
                    const float* sticky, float max_height,
                    uint32_t node_index = 0) override;
  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info) override;
  void UpdatePlatformExtraBundle(int32_t signature,
                                 PlatformExtraBundle* bundle) override;

  void Flush() override;
  void HandleValidate(int tag) override {
    // TODO(liujilong): Implement.
  }

  void ConsumeGesture(int64_t id, int32_t gesture_id,
                      const pub::Value& params) override;

  void FinishTasmOperation(const PipelineOptions& options) override;
  std::vector<float> getBoundingClientOrigin(int id) override;
  std::vector<float> getWindowSize(int id) override;
  std::vector<float> GetRectToWindow(int id) override;

  std::vector<float> GetRectToLynxView(int64_t id) override;
  std::vector<float> ScrollBy(int64_t id, float width, float height) override;
  void Invoke(int64_t id, const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback) override;
  int32_t GetTagInfo(const std::string& tag_name) override;
  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;

  // LayoutDidFinish is called only LayoutRecursively is actually executed
  // FinishLayoutOperation on the other hand, is always being called, and it is
  // called before LayoutDidFinish
  // TODO(heshan):merge to FinishLayoutOperation...
  void LayoutDidFinish();
  void FinishLayoutOperation(const PipelineOptions& options) override;

  void SetEnableFlush(bool enable_flush);
  void ForceFlush();
  bool IsLayoutFinish();
  void ResetLayoutStatus();

  void OnFirstMeaningfulLayout() override;

  bool NeedAnimationProps() override { return false; }

  static lepus::Value GetUITreeRecursive(LynxUI* ui);
  std::string GetUITree();

  bool EnableUIOperationQueue() override { return true; }

  shell::UIOperation ExecuteOperationSafely(shell::UIOperation op) override;

 private:
  __weak LynxUIOwner* uiOwner_;
  bool enable_create_ui_async_{false};

  std::shared_ptr<shell::DynamicUIOperationQueue> queue_;
  std::atomic<bool> is_layout_finish_ = {false};

  int32_t instance_id_ = 0;

  template <typename F>
  void Enqueue(F&& func);

  template <typename F>
  void EnqueueHighPriorityUIOperation(F&& func);

  PaintingContextDarwin(const PaintingContextDarwin&) = delete;
  PaintingContextDarwin& operator=(const PaintingContextDarwin&) = delete;
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_H_
