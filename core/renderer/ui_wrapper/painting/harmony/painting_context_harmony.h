// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PAINTING_CONTEXT_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PAINTING_CONTEXT_HARMONY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "core/public/painting_ctx_platform_impl.h"
#include "core/shell/lynx_ui_operation_queue.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {

class PaintingContextHarmonyRef : public PaintingCtxPlatformRef {
 public:
  explicit PaintingContextHarmonyRef(harmony::UIOwner* ui_owner)
      : ui_owner_(ui_owner) {}
  ~PaintingContextHarmonyRef() override = default;

  void InsertPaintingNode(int parent, int child, int index) override;
  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;
  void UpdateScrollInfo(int32_t container_id, bool smooth,
                        float estimated_offset, bool scrolling) override;

  void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                               std::vector<int32_t> remove_ids) override;
  void UpdateEventInfo(bool has_touch_pseudo) override;
  void InsertListItemPaintingNode(int list_sign, int child_sign) override;
  void RemoveListItemPaintingNode(int list_sign, int child_sign) override;
  void UpdateContentOffsetForListContainer(int32_t container_id,
                                           float content_size, float delta_x,
                                           float delta_y,
                                           bool is_init_scroll_offset,
                                           bool from_layout) override;

  void SetGestureDetectorState(int64_t id, int32_t gesture_id,
                               int32_t state) override;

  void SetNeedMarkPaintEndTiming(const tasm::PipelineID& pipeline_id) override;

 private:
  harmony::UIOwner* ui_owner_;
};

class PaintingContextHarmony : public PaintingCtxPlatformImpl {
 public:
  explicit PaintingContextHarmony(harmony::UIOwner* ui_owner)
      : ui_owner_{ui_owner} {
    platform_ref_ = std::make_shared<PaintingContextHarmonyRef>(ui_owner);
  }
  ~PaintingContextHarmony() override;
  void CreatePaintingNode(int id, const std::string& tag,
                          const fml::RefPtr<PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override;
  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override;
  void UpdateLayout(int tag, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height,
                    uint32_t node_index) override;
  void SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) override;
  void Flush() override;
  void HandleValidate(int tag) override;
  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions>& options) override;
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override;
  std::vector<float> getBoundingClientOrigin(int id) override;
  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info) override;
  std::vector<float> getWindowSize(int id) override;
  std::vector<float> GetRectToWindow(int id) override;
  /**
   * @brief get lynxView rect with id
   * @param id
   * @return result[0] - left  result[1] - top result[2] - width  result[3] -
   * height
   */
  std::vector<float> GetRectToLynxView(int64_t id) override;
  std::vector<float> ScrollBy(int64_t id, float width, float height) override;
  void ConsumeGesture(int64_t id, int32_t gesture_id,
                      const pub::Value& params) override;

  void Invoke(int64_t id, const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback) override;
  int32_t GetTagInfo(const std::string& tag_name) override;
  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;
  void UpdatePlatformExtraBundle(int32_t id,
                                 PlatformExtraBundle* bundle) override;
  bool NeedAnimationProps() override;

  void InvokeUIMethod(int32_t id, const std::string& method,
                      fml::RefPtr<tasm::PropBundle> value,
                      int32_t callback_id) override;
  void SetUIOperationQueue(
      const std::shared_ptr<shell::DynamicUIOperationQueue>& queue) override;

  bool DefaultOverflowAlwaysVisible() override { return true; }

  bool EnableUIOperationQueue() override { return true; }

 private:
  std::unique_ptr<harmony::UIOwner> ui_owner_;
  std::shared_ptr<shell::DynamicUIOperationQueue> queue_;
  void Enqueue(shell::UIOperation&& op);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PAINTING_CONTEXT_HARMONY_H_
