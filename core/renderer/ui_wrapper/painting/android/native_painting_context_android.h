// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_ANDROID_H_

#include <jni.h>

#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/public/painting_ctx_platform_impl.h"
#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"
#include "core/shell/dynamic_ui_operation_queue.h"

namespace lynx::tasm {

class PlatformRendererContext;

class NativePaintingCtxAndroid : public PaintingCtxPlatformImpl,
                                 public NativePaintingContext {
 public:
  NativePaintingCtxAndroid(JNIEnv *env, jobject text_layout, jlong textra,
                           PlatformRendererContext *view_manager);
  NativePaintingCtxAndroid(const NativePaintingCtxAndroid &) = delete;

  void SetUIOperationQueue(
      const std::shared_ptr<shell::UIOperationQueueInterface> &queue) override;

  void CreatePaintingNode(int id, const std::string &tag,
                          const fml::RefPtr<PropBundle> &painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override;

  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const fml::RefPtr<PropBundle> &painting_data) override;

  std::unique_ptr<pub::Value> GetTextInfo(const std::string &content,
                                          const pub::Value &info) override;

  void StopExposure(const pub::Value &options) override;
  void ResumeExposure() override;

  void UpdateLayout(int tag, float x, float y, float width, float height,
                    const float *paddings, const float *margins,
                    const float *borders, const float *bounds,
                    const float *sticky, float max_height,
                    uint32_t node_index) override;

  void UpdatePlatformExtraBundle(int32_t id,
                                 PlatformExtraBundle *bundle) override;

  void SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) override;

  void Flush() override;

  void HandleValidate(int tag) override;

  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions> &options) override;

  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions> &options) override;

  std::vector<float> getBoundingClientOrigin(int id) override;

  std::vector<float> getWindowSize(int id) override;

  std::vector<float> GetRectToWindow(int id) override;

  std::vector<float> GetRectToLynxView(int64_t id) override;

  std::vector<float> ScrollBy(int64_t id, float width, float height) override;

  void Invoke(int64_t id, const std::string &method, const pub::Value &params,
              const std::function<void(int32_t, const pub::Value &)> &callback)
      override;

  void EnqueueInvoke(int64_t id, const std::string &method,
                     const pub::Value &params,
                     const std::function<void(int32_t, const pub::Value &)>
                         &callback) override;

  int32_t GetTagInfo(const std::string &tag_name) override;

  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;

  bool NeedAnimationProps() override;

  NativePaintingContext *CastToNativeCtx() override {
    return static_cast<NativePaintingContext *>(this);
  }

  bool EnableUIOperationQueue() override { return true; }

  void OnFirstScreen() override;

#pragma region NativePaintingContext

  void CreatePlatformRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle> &init_data) override;
  void CreatePlatformExtendedRenderer(
      int id, const base::String &tag_name,
      const fml::RefPtr<PropBundle> &init_data) override;

  void UpdateDisplayList(int id, DisplayList display_list) override;

  void UpdatePlatformEventBundle(int32_t id,
                                 PlatformEventBundle bundle) override;

  void CreateImage(int id, base::String src, float width, float height,
                   int32_t event_mask = 0) override;

  void UpdateTextBundle(int id, intptr_t bundle) override;

  void DestroyTextBundle(int id) override;

  void InsertListItemPaintingNode(int32_t list_id, int32_t child_id) override;

  void RemoveListItemPaintingNode(int32_t list_id, int32_t child_id) override;

  void UpdateContentOffsetForListContainer(int32_t container_id,
                                           float content_size, float delta_x,
                                           float delta_y,
                                           bool is_init_scroll_offset,
                                           bool from_layout) override;

  void ReconstructEventTargetTreeRecursively() override;
#pragma endregion  // NativePaintingContext

 private:
  void Enqueue(shell::UIOperation op) {
    queue_->EnqueueUIOperation(std::move(op));
  }

  bool has_first_screen_ = false;
  std::shared_ptr<std::atomic_bool> event_target_tree_update_enqueued_ =
      std::make_shared<std::atomic_bool>(false);
  std::unique_ptr<PlatformRendererContext> view_manager_;
  std::shared_ptr<shell::DynamicUIOperationQueue> queue_;
};

}  // namespace lynx::tasm
#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_ANDROID_H_
