// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_PAINTING_CONTEXT_CLAY_H_
#define CLAY_LYNX_ADAPTOR_PAINTING_CONTEXT_CLAY_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/lynx_adaptor/perf_controller_clay.h"
#include "clay/ui/component/view_context.h"
#include "core/public/lynx_engine_proxy.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/public/painting_ctx_platform_impl.h"
#include "core/public/perf_controller_proxy.h"

namespace lynx {
namespace tasm {

class PaintingContextClayRef : public PaintingCtxPlatformRef {
 public:
  explicit PaintingContextClayRef(clay::ViewContext* view_context)
      : view_context_(view_context) {}
  ~PaintingContextClayRef() override = default;

  void SetUIOperationQueue(
      const std::shared_ptr<shell::UIOperationQueueInterface>& queue) {
    queue_ = queue;
  }

  void InsertPaintingNode(int parent, int child, int index) override;
  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;
  void UpdateScrollInfo(int32_t container_id, bool smooth,
                        float estimated_offset, bool scrolling) override;
  void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                               std::vector<int32_t> remove_ids) override;
  void InsertListItemPaintingNode(int list_sign, int child_sign) override;
  void RemoveListItemPaintingNode(int list_sign, int child_sign) override;
  void UpdateContentOffsetForListContainer(int32_t container_id,
                                           float content_size, float delta_x,
                                           float delta_y,
                                           bool is_init_scroll_offset,
                                           bool from_layout) override;
  void SetNeedMarkPaintEndTiming(const tasm::PipelineID& pipeline_id) override;
  void SetGestureDetectorState(int64_t id, int32_t gesture_id,
                               int32_t state) override;
  void SetPerfController(const std::shared_ptr<PerfControllerClay>& collector);

 private:
  clay::ViewContext* view_context_ = nullptr;
  std::weak_ptr<shell::UIOperationQueueInterface> queue_;
  std::shared_ptr<PerfControllerClay> perf_controller_;
};

class PaintingContextClay : public PaintingCtxPlatformImpl,
                            public clay::UIComponentDelegate {
 public:
  explicit PaintingContextClay(clay::ViewContext* view_context);
  ~PaintingContextClay() override;

  void SetUIOperationQueue(
      const std::shared_ptr<shell::UIOperationQueueInterface>& queue) override {
    ui_operation_queue_ref_ = queue;
    auto ref = std::static_pointer_cast<PaintingContextClayRef>(platform_ref_);
    if (ref) {
      ref->SetUIOperationQueue(queue);
    }
  }

  void SetEngineProxy(
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy) {
    engine_proxy_ = engine_proxy;
  }
  void SetRuntimeProxy(
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy) {
    runtime_proxy_ = runtime_proxy;
  }
  void SetInstanceId(const int32_t instance_id) override {
    instance_id_ = instance_id;
  }

  void Flush() override;
  void HandleValidate(int tag) override;

  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions>& options) override;
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override;

  std::vector<float> getBoundingClientOrigin(int id) override;
  void InvokeUIMethod(int32_t view_id, const std::string& method,
                      fml::RefPtr<tasm::PropBundle> args,
                      int32_t callback_id) override;
  void CreatePaintingNode(int id, const std::string& tag,
                          const fml::RefPtr<PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override;
  void SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) override;
  bool DefaultOverflowAlwaysVisible() override { return true; }
  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override;
  void UpdateLayout(int tag, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height,
                    uint32_t node_index) override;
  void Invoke(int64_t id, const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback) override;

  void getAbsolutePosition(int id, float* position) override;

  void OnFirstMeaningfulLayout() override;

  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;

  bool NeedAnimationProps() override { return true; }
  bool EnableParallelElement() override { return true; }
  bool EnableUIOperationQueue() override { return true; }

  // TODO(chenhyouhui): Use default implementations
  std::vector<float> getWindowSize(int id) override { return floats_; }
  std::vector<float> GetRectToWindow(int id) override { return floats_; }
  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info) override;
  void StopExposure(const pub::Value& options) override;
  void ResumeExposure() override;
  std::vector<float> GetRectToLynxView(int64_t id) override;
  std::vector<float> ScrollBy(int64_t id, float width, float height) override {
    return floats_;
  }
  int32_t GetTagInfo(const std::string& tag_name) override;
  void UpdatePlatformExtraBundle(int32_t id,
                                 PlatformExtraBundle* bundle) override;

  void ConsumeGesture(int64_t id, int32_t gesture_id,
                      const pub::Value& params) override;
  void Enqueue(base::closure&& op);

 private:
  std::shared_ptr<shell::UIOperationQueueInterface> ui_operation_queue_ref_;
  static void SetAttribute(clay::ViewContext* view_context, int sign,
                           PropBundle* attributes, bool init);

  // clay::UIComponentDelegate
  clay::LynxListData* OnListGetData(int view_id) override;
  int OnObtainChild(int view_id, int index, int operation_id) override;
  void OnRecycleChild(int view_id, int child_id) override;
  void OnCreateAddChild(int view_id, int index, int operation_id) override;
  void OnUpdateChild(int parent_id, int to_update_id, int target_index,
                     int64_t operation_id) override;
  void OnScrollByListContainer(int view_id, float offset_x, float offset_y,
                               float original_x, float original_y) override;
  void OnScrollToPosition(int view_id, int position, float offset, int align,
                          bool smooth) override;
  void OnScrollStopped(int view_id) override;
  bool OnEnableRasterAnimation() override;
  std::list<int32_t> GetAncestorElements(int32_t tag) override;

  clay::ViewContext* view_context_ = nullptr;
  std::shared_ptr<shell::LynxEngineProxy> engine_proxy_ = nullptr;
  std::shared_ptr<shell::LynxRuntimeProxy> runtime_proxy_ = nullptr;

  int32_t instance_id_ = 0;

  // Use to return an empty vector
  std::vector<float> floats_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_PAINTING_CONTEXT_CLAY_H_
