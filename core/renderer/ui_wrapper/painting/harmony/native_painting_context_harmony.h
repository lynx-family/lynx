// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_HARMONY_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"

namespace lynx::tasm {

// Harmony NativePaintingContext skeleton. Runtime wiring and platform
// operations are introduced in follow-up changes.
class NativePaintingCtxHarmony : public PaintingCtxPlatformImpl,
                                 public NativePaintingContext {
 public:
  NativePaintingCtxHarmony();
  ~NativePaintingCtxHarmony() override = default;

  NativePaintingCtxHarmony(const NativePaintingCtxHarmony&) = delete;
  NativePaintingCtxHarmony& operator=(const NativePaintingCtxHarmony&) = delete;

  void CreatePaintingNode(int id, const std::string& tag,
                          const fml::RefPtr<PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override {}
  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override {}
  void UpdateLayout(int tag, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height, uint32_t node_index,
                    bool display_none) override {}
  void SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) override {}
  void Flush() override {}
  void HandleValidate(int tag) override {}

  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info) override;
  void StopExposure(const pub::Value& options) override {}
  void ResumeExposure() override {}
  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions>& options) override {}
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override {}

  std::vector<float> getBoundingClientOrigin(int id) override;
  std::vector<float> getWindowSize(int id) override;
  std::vector<float> GetRectToWindow(int id) override;
  std::vector<float> GetRectToLynxView(int64_t id) override;
  std::vector<float> ScrollBy(int64_t id, float width, float height) override;

  void Invoke(
      int64_t id, const std::string& method, const pub::Value& params,
      const std::function<void(int32_t, const pub::Value&)>& callback) override;
  void EnqueueInvoke(
      int64_t id, const std::string& method, const pub::Value& params,
      const std::function<void(int32_t, const pub::Value&)>& callback) override;

  int32_t GetTagInfo(const std::string& tag_name) override;
  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;
  bool NeedAnimationProps() override;

  NativePaintingContext* CastToNativeCtx() override { return this; }

  void OnFirstScreen() override {}
  void CreatePlatformRenderer(int id, PlatformRendererType type,
                              const fml::RefPtr<PropBundle>& init_data,
                              const PlatformRendererInitConfig& init_config =
                                  PlatformRendererInitConfig()) override;
  void CreatePlatformExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig()) override;
  fml::RefPtr<PaintImage> CreateImage(
      int id, base::String src, const ImagePaintInfo& paint_info, float width,
      float height, int32_t event_mask = 0,
      bool disable_default_resize = false) override;
  void UpdateTextBundle(int id, intptr_t bundle) override {}
  void DestroyTextBundle(int id) override {}
  void UpdatePlatformEventBundle(int id, PlatformEventBundle bundle) override;

 protected:
  void EnqueueDisplayList(int id, DisplayList list) override;
  void EnqueueDisplayLists(DisplayListUpdateBatch batch) override {}
  void EnqueueReconstructEventTargetTreeRecursively() override {}
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_HARMONY_H_
