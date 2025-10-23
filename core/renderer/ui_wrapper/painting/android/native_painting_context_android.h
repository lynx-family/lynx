// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/painting_context.h"

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_ANDROID_H_

#include <memory>
#include <string>
#include <vector>

#include "core/base/android/jni_helper.h"

namespace lynx::tasm {

class NativePaintingCtxAndroidRef : public PaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxAndroidRef() {}

  ~NativePaintingCtxAndroidRef() override = default;

  void InsertPaintingNode(int parent, int child, int index) override {
    PaintingCtxPlatformRef::InsertPaintingNode(parent, child, index);
  }

  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override {
    PaintingCtxPlatformRef::RemovePaintingNode(parent, child, index, is_move);
  }

  void DestroyPaintingNode(int parent, int child, int index) override {
    PaintingCtxPlatformRef::DestroyPaintingNode(parent, child, index);
  }

  void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                               std::vector<int32_t> remove_ids) override {
    PaintingCtxPlatformRef::UpdateNodeReadyPatching(ready_ids, remove_ids);
  }

  void UpdateFlattenStatus(int id, bool flatten) override {
    PaintingCtxPlatformRef::UpdateFlattenStatus(id, flatten);
  }
};

class PlatformRendererContext;

class NativePaintingCtxAndroid : public PaintingCtxPlatformImpl {
 public:
  NativePaintingCtxAndroid(JNIEnv *env, jobject text_layout,
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

  int32_t GetTagInfo(const std::string &tag_name) override;

  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;

  bool NeedAnimationProps() override;

 private:
  std::unique_ptr<PlatformRendererContext> view_manager_;
  std::unique_ptr<TextLayoutImpl> text_layout_impl_;
};

}  // namespace lynx::tasm
#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_ANDROID_H_
